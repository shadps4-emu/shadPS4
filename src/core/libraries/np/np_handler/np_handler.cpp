// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fmt/format.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/net/net_p2p.h"
#include "core/libraries/net/net_upnp.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/user_settings.h"
#include "imgui/shadnet_notifications_layer.h"
#include "shadnet.pb.h"
#include "shadnet/server_probe.h"

namespace Libraries::Np {

NpHandler::NpHandler()
    : m_matching2_contexts(
          std::unique_ptr<NpMatching2::ContextManager>(new NpMatching2::ContextManager())),
      m_matching2_state(std::make_unique<NpMatching2::NpMatching2State>()) {}

NpHandler::~NpHandler() = default;

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
}

NpMatching2::ContextManager& NpHandler::GetMatching2ContextManager() {
    return *m_matching2_contexts;
}

NpMatching2::NpMatching2State& NpHandler::GetMatching2State() {
    return *m_matching2_state;
}

NpHandler::Matching2CacheGuard NpHandler::LockMatching2Cache(
    NpMatching2::OrbisNpMatching2ContextId ctx_id) {
    std::shared_ptr<NpMatching2::Matching2ContextCache> cache;
    {
        std::lock_guard lock(m_mutex_matching2_cache);
        auto& entry = m_matching2_cache[ctx_id];
        if (!entry) {
            entry = std::make_shared<NpMatching2::Matching2ContextCache>();
        }
        cache = entry;
    }
    return Matching2CacheGuard(std::move(cache));
}

void NpHandler::ResetMatching2Cache(NpMatching2::OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex_matching2_cache);
    m_matching2_cache.erase(ctx_id);
}

void NpHandler::ResetMatching2Caches() {
    std::lock_guard lock(m_mutex_matching2_cache);
    m_matching2_cache.clear();
}

std::pair<std::string, u16> NpHandler::ParseServerAddress() const {
    const std::string server_str = EmulatorSettings.GetShadNetServer();
    u16 port = 31313; // default port
    if (server_str.empty()) {
        LOG_ERROR(NpHandler,
                  "shadNet server address is empty,set the shadNet server field in settings. "
                  "Connection will fail.");
        return {std::string{}, port};
    }
    std::string host = server_str;
    const auto colon = server_str.rfind(':');
    if (colon != std::string::npos) {
        host = server_str.substr(0, colon);
        const std::string port_str = server_str.substr(colon + 1);
        try {
            port = static_cast<u16>(std::stoi(port_str));
        } catch (const std::exception&) {
            LOG_WARNING(NpHandler,
                        "shadNet server port '{}' is not a valid number; using default {}",
                        port_str, port);
        }
    }
    return {host, port};
}

bool NpHandler::ConnectUserById(s32 user_id) {
    if (!EmulatorSettings.IsShadNetEnabled())
        return false;

    {
        std::lock_guard lock(m_mutex_clients);
        if (m_clients.find(user_id) != m_clients.end())
            return false; // already connected
    }

    const User* u = UserManagement.GetUserByID(user_id);
    if (!u)
        return false;
    if (!u->shadnet_enabled) {
        LOG_DEBUG(NpHandler, "user_id={} ('{}') shadNet disabled,skipping", u->user_id,
                  u->user_name);
        return false;
    }
    if (u->shadnet_npid.empty() || u->shadnet_password.empty()) {
        LOG_WARNING(NpHandler, "user_id={} ('{}') shadNet enabled but credentials missing,skipping",
                    u->user_id, u->user_name);
        return false;
    }

    const auto [host, port] = ParseServerAddress();
    return ConnectUser(u->user_id, host, port, u->shadnet_npid, u->shadnet_password,
                       u->shadnet_token);
}

void NpHandler::StartWorker() {
    bool expected = false;
    if (m_worker_running.compare_exchange_strong(expected, true)) {
        m_worker_thread = std::thread(&NpHandler::WorkerThread, this);
    }
}

void NpHandler::Initialize() {
    if (m_initialized.exchange(true)) {
        LOG_WARNING(NpHandler, "Initialize called more than once");
        return;
    }

    if (!EmulatorSettings.IsShadNetEnabled()) {
        LOG_INFO(NpHandler, "shadNet disabled globally we are in offline mode");
        return;
    }

    {
        const auto [host, port] = ParseServerAddress();
        const ShadNet::ProbeInfo probe = ShadNet::ProbeServer(host, port);
        if (probe.result != ShadNet::ProbeResult::Ok) {
            EmulatorSettings.SetShadNetSessionDisabled(true);
            switch (probe.result) {
            case ShadNet::ProbeResult::VersionMismatch:
                LOG_WARNING(NpHandler,
                            "shadNet server {}:{} protocol version mismatch (server v{}, emulator "
                            "v{}); disabling shadNet for this run (saved setting unchanged)",
                            host, port, probe.server_version, ShadNet::SHAD_PROTOCOL_VERSION);
                ImGui::ShadNetNotify::Push(
                    ImGui::ShadNetNotify::Kind::Info,
                    fmt::format("shadNet protocol version mismatch (server v{}, emulator v{}). "
                                "Please update shadPS4. Online features are disabled for this "
                                "session.",
                                probe.server_version, ShadNet::SHAD_PROTOCOL_VERSION));
                break;
            case ShadNet::ProbeResult::ProtocolError:
                LOG_WARNING(NpHandler,
                            "shadNet server {}:{} sent an invalid ServerInfo handshake; disabling "
                            "shadNet for this run (saved setting unchanged)",
                            host, port);
                ImGui::ShadNetNotify::Push(
                    ImGui::ShadNetNotify::Kind::Info,
                    fmt::format("shadNet server ({}:{}) uses an incompatible protocol. Online "
                                "features are disabled for this session.",
                                host, port));
                break;
            default: // Unreachable
                LOG_WARNING(NpHandler,
                            "shadNet server {}:{} is offline/unreachable; disabling shadNet for "
                            "this run (saved setting unchanged)",
                            host, port);
                ImGui::ShadNetNotify::Push(
                    ImGui::ShadNetNotify::Kind::Info,
                    fmt::format("shadNet server ({}:{}) is offline. Online features are disabled "
                                "for this session.",
                                host, port));
                break;
            }
            return;
        }
    }

    const auto logged_in = UserManagement.GetLoggedInUsers(); // get all login users
    int connected_count = 0;
    for (int i = 0; i < Libraries::UserService::ORBIS_USER_SERVICE_MAX_LOGIN_USERS; ++i) {
        const User* u = logged_in[i];
        if (!u)
            continue;
        if (ConnectUserById(u->user_id))
            ++connected_count;
    }

    if (connected_count == 0) {
        LOG_WARNING(NpHandler, "no users connected to shadNet");
        return;
    }

    StartWorker();
}

void NpHandler::Shutdown() {
    if (!m_initialized.exchange(false))
        return;

    m_worker_running = false;

    // Stop any pending reconnect retries.
    {
        std::lock_guard lock(m_mutex_clients);
        m_reconnect.clear();
    }

    // Collect user IDs to disconnect (avoid holding m_mutex_clients during Stop)
    std::vector<s32> ids;
    {
        std::lock_guard lock(m_mutex_clients);
        for (auto& [uid, _] : m_clients)
            ids.push_back(uid);
    }
    for (s32 uid : ids)
        DisconnectUser(uid);

    if (m_worker_thread.joinable())
        m_worker_thread.join();

    // P2P goes with NP: closes the UDP port and removes its UPnP forwarding.
    Net::StopP2P();

    LOG_INFO(NpHandler, "Shutdown complete");
}

bool NpHandler::ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                            const std::string& password, const std::string& token) {
    LOG_INFO(NpHandler, "Connecting user_id={} npid='{}' to {}:{} (timeout {}s)", user_id, npid,
             host, port, ShadNet::SHAD_CONNECT_TIMEOUT_MS / 1000);

    auto client = std::make_shared<ShadNet::ShadNetClient>();

    // Wire per-user notification callbacks
    client->onFriendQuery = [this, user_id](const ShadNet::NotifyFriendQuery& n) {
        OnFriendQuery(user_id, n);
    };
    client->onFriendNew = [this, user_id](const ShadNet::NotifyFriendNew& n) {
        OnFriendNew(user_id, n);
    };
    client->onFriendLost = [this, user_id](const ShadNet::NotifyFriendLost& n) {
        OnFriendLost(user_id, n);
    };
    client->onFriendStatus = [this, user_id](const ShadNet::NotifyFriendStatus& n) {
        OnFriendStatus(user_id, n);
    };
    client->onWebApiPushEvent = [this, user_id](const ShadNet::NotifyWebApiPushEvent& n) {
        OnWebApiPushEvent(user_id, n);
    };
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnAsyncReply(user_id, cmd, pkt_id, err, body);
    };
    client->onLoginResult = [this, user_id](const ShadNet::LoginResult& res) {
        OnLoginResult(user_id, res);
    };

    // Seed the current Appear-Offline preference so the login packet carries it (the send
    // is suppressed pre-auth; it just caches on the client).
    client->SetAppearOffline(m_appear_offline.load());
    client->Start(host, port, npid, password, token);

    // Shared handling for an incompatible-protocol failure (version mismatch or
    // corrupt/unparseable stream): tell the user and disable shadNet for this run,
    // since reconnect attempts against an incompatible server are pointless.
    const auto handle_protocol_mismatch = [&client](ShadNet::ShadNetState st) {
        if (st != ShadNet::ShadNetState::FailureProtocol)
            return;
        const u32 server_ver = client->GetServerProtocolVersion();
        EmulatorSettings.SetShadNetSessionDisabled(true);
        if (server_ver != 0) {
            LOG_ERROR(NpHandler,
                      "shadNet protocol version mismatch (server v{}, emulator v{}); disabling "
                      "shadNet for this run (saved setting unchanged)",
                      server_ver, ShadNet::SHAD_PROTOCOL_VERSION);
            ImGui::ShadNetNotify::Push(
                ImGui::ShadNetNotify::Kind::Info,
                fmt::format("shadNet protocol version mismatch (server v{}, emulator v{}). "
                            "Please update shadPS4. Online features are disabled for this "
                            "session.",
                            server_ver, ShadNet::SHAD_PROTOCOL_VERSION));
        } else {
            LOG_ERROR(NpHandler, "shadNet protocol error during handshake; disabling shadNet for "
                                 "this run (saved setting unchanged)");
            ImGui::ShadNetNotify::Push(
                ImGui::ShadNetNotify::Kind::Info,
                "shadNet server uses an incompatible protocol. Online features are disabled "
                "for this session.");
        }
    };

    const ShadNet::ShadNetState conn_state = client->WaitForConnection();
    if (conn_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "user_id={} connection failed (state={})", user_id,
                  static_cast<int>(conn_state));
        handle_protocol_mismatch(conn_state);
        client->Stop();
        return false;
    }

    const ShadNet::ShadNetState auth_state = client->WaitForAuthenticated();
    if (auth_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "user_id={} authentication failed (state={})", user_id,
                  static_cast<int>(auth_state));
        handle_protocol_mismatch(auth_state);
        client->Stop();
        return false;
    }

    LOG_INFO(NpHandler, "user_id={} signed in npid='{}' accountId={}", user_id, npid,
             client->GetUserId());

    Net::UPnPClient::Instance().SetP2PFeaturesEnabled(client->IsMatching2Enabled());
    if (client->IsMatching2Enabled() && EmulatorSettings.IsUPnPEnabled()) {
        Net::UPnPClient::Instance().Start();
    }

    NpMatching2::SetMmShadNetClient(client, host, port);

    // Build OrbisNpId
    {
        OrbisNpId np_id{};
        SetNpId(np_id, npid);
        std::lock_guard lock(m_mutex_clients);
        m_np_ids[user_id] = np_id;
        m_clients[user_id] = std::move(client);
    }

    FireStateCallback(user_id, NpManager::OrbisNpState::SignedIn);
    return true;
}

void NpHandler::SetAppearOffline(bool enable) {
    m_appear_offline = enable;
    std::lock_guard lock(m_mutex_clients);
    for (auto& [uid, client] : m_clients) {
        if (client)
            client->SetAppearOffline(enable); // sends the toggle to connected sessions
    }
}

void NpHandler::FailPendingRequests(s32 user_id, s32 error_code) {
    size_t failed = 0;
    {
        std::lock_guard lock(m_mutex_pending_tus);
        for (auto it = m_pending_tus.begin(); it != m_pending_tus.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.req) {
                    it->second.req->SetResult(error_code);
                }
                it = m_pending_tus.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    {
        std::lock_guard lock(m_mutex_pending_score);
        for (auto it = m_pending_score.begin(); it != m_pending_score.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.req) {
                    it->second.req->SetResult(error_code);
                }
                it = m_pending_score.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    {
        std::lock_guard lock(m_mutex_pending_lookup);
        for (auto it = m_pending_lookup.begin(); it != m_pending_lookup.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.on_result) {
                    it->second.on_result(error_code, 0, {});
                }
                it = m_pending_lookup.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    if (failed > 0) {
        LOG_WARNING(NpHandler, "user_id={} failed {} pending request(s) with {:#x}", user_id,
                    failed, static_cast<u32>(error_code));
    }
}

void NpHandler::DisconnectUser(s32 user_id) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end())
            return;
        client = std::move(it->second);
        m_clients.erase(it);
    }
    {
        std::lock_guard lock(m_mutex_friend_state);
        m_friend_state.erase(user_id);
    }
    client->Stop();
    // The reader thread is joined, so no reply can race us: complete every
    // in-flight request now, or PollAsync spins and WaitAsync blocks forever.
    FailPendingRequests(user_id, ORBIS_NP_ERROR_SIGNED_OUT);
    FireStateCallback(user_id, NpManager::OrbisNpState::SignedOut);
    LOG_INFO(NpHandler, "user_id={} disconnected", user_id);
}

void NpHandler::OnUserLoggedIn(s32 user_id) {
    if (ConnectUserById(user_id))
        StartWorker();
}

void NpHandler::OnUserLoggedOut(s32 user_id) {
    {
        std::lock_guard lock(m_mutex_clients);
        m_reconnect.erase(user_id); // do not auto-reconnect
    }
    DisconnectUser(user_id);
}

void NpHandler::WorkerThread() {
    constexpr auto INTERVAL = std::chrono::milliseconds(500);

    while (m_worker_running) {
        std::this_thread::sleep_for(INTERVAL);

        // Collect ids of dropped clients
        std::vector<s32> dropped;
        {
            std::lock_guard lock(m_mutex_clients);
            for (auto& [uid, client] : m_clients) {
                if (!client->IsConnected())
                    dropped.push_back(uid);
            }
        }

        for (s32 uid : dropped) {
            LOG_WARNING(NpHandler, "user_id={} connection dropped (network); will retry", uid);
            DisconnectUser(uid);   // reports SignedOut + stops/removes the dead client
            MarkForReconnect(uid); // schedule transparent reconnect (network drop, not logout)
        }

        TryReconnect();
    }
}

void NpHandler::MarkForReconnect(s32 user_id) {
    if (!EmulatorSettings.IsShadNetEnabled())
        return; // offline mode: nothing to reconnect to
    constexpr auto kInitialBackoff = std::chrono::milliseconds(2000);
    std::lock_guard lock(m_mutex_clients);
    auto& st = m_reconnect[user_id];
    st.backoff = kInitialBackoff;
    st.next_attempt = std::chrono::steady_clock::now() + st.backoff;
}

void NpHandler::TryReconnect() {
    constexpr auto kMaxBackoff = std::chrono::milliseconds(30000);
    const auto now = std::chrono::steady_clock::now();

    std::vector<s32> due;
    {
        std::lock_guard lock(m_mutex_clients);
        for (auto& [uid, st] : m_reconnect) {
            if (m_clients.count(uid) || now >= st.next_attempt)
                due.push_back(uid);
        }
    }

    for (s32 uid : due) {
        if (!m_worker_running)
            return;
        if (!EmulatorSettings.IsShadNetEnabled()) {
            std::lock_guard lock(m_mutex_clients);
            m_reconnect.erase(uid);
            continue;
        }
        // ConnectUserById locks m_mutex_clients internally; call it unlocked.
        const bool ok = ConnectUserById(uid);
        std::lock_guard lock(m_mutex_clients);
        if (ok || m_clients.count(uid)) {
            m_reconnect.erase(uid);
            LOG_INFO(NpHandler, "user_id={} reconnected to shadNet", uid);
        } else if (auto it = m_reconnect.find(uid); it != m_reconnect.end()) {
            it->second.backoff = std::min(it->second.backoff * 2, kMaxBackoff);
            it->second.next_attempt = std::chrono::steady_clock::now() + it->second.backoff;
            LOG_DEBUG(NpHandler, "user_id={} reconnect failed; next attempt in {}ms", uid,
                      it->second.backoff.count());
        }
    }
}

bool NpHandler::IsPsnSignedIn(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() && it->second->IsAuthenticated();
}

bool NpHandler::IsAnySignedIn() const {
    std::lock_guard lock(m_mutex_clients);
    for (auto& [_, client] : m_clients)
        if (client->IsAuthenticated())
            return true;
    return false;
}

OrbisNpId NpHandler::GetNpId(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_np_ids.find(user_id);
    return it != m_np_ids.end() ? it->second : OrbisNpId{};
}

OrbisNpOnlineId NpHandler::GetOnlineId(s32 user_id) const {
    return GetNpId(user_id).handle;
}

std::string NpHandler::GetAvatarUrl(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetAvatarUrl() : std::string{};
}

OrbisNpAccountId NpHandler::GetAccountId(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? static_cast<OrbisNpAccountId>(it->second->GetUserId()) : 0;
}

std::string NpHandler::GetBearerToken(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetBearerToken() : std::string{};
}

u32 NpHandler::GetLocalIpAddr(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetAddrLocal() : 0;
}

s32 NpHandler::GetUserIdByAccountId(u64 account_id) const {
    std::lock_guard lock(m_mutex_clients);
    for (auto& [uid, client] : m_clients) {
        if (static_cast<u64>(client->GetUserId()) == account_id)
            return uid;
    }
    return -1;
}

s32 NpHandler::GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const {
    std::lock_guard lock(m_mutex_clients);
    for (auto& [uid, np_id] : m_np_ids) {
        if (strncmp(np_id.handle.data, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH) == 0)
            return uid;
    }
    return -1;
}

std::vector<s32> NpHandler::GetConnectedUsers() const {
    std::vector<s32> out;
    std::lock_guard lock(m_mutex_clients);
    out.reserve(m_clients.size());
    for (const auto& [uid, c] : m_clients) {
        out.push_back(uid);
    }
    return out;
}

// state callbacks
s32 NpHandler::RegisterStateCallback(StateCallback cb, void* userdata) {
    std::lock_guard lock(m_mutex_cbs);
    const s32 h = m_next_handle++;
    m_state_cbs.push_back({h, std::move(cb), userdata});
    return h;
}

void NpHandler::UnregisterStateCallback(s32 handle) {
    std::lock_guard lock(m_mutex_cbs);
    m_state_cbs.erase(std::remove_if(m_state_cbs.begin(), m_state_cbs.end(),
                                     [handle](const CbEntry& e) { return e.handle == handle; }),
                      m_state_cbs.end());
}

void NpHandler::FireStateCallback(s32 user_id, NpManager::OrbisNpState state) {
    std::lock_guard lock(m_mutex_cbs);
    for (const auto& e : m_state_cbs) {
        if (e.cb)
            e.cb(user_id, state);
    }
}

// A usable NP Communication ID is the 12-char "NPWRxxxxx_00" form. An empty or
// all-NUL buffer means npbind.dat was missing or unparsed,treat it as invalid
// so callers reject the request instead of sending a blank comm id on the wire.
bool IsValidNpCommId(const std::string& id) {
    return !id.empty() && std::any_of(id.begin(), id.end(), [](char c) { return c != '\0'; });
}

std::string NpHandler::GetNpCommId(s32 service_label) const {
    // TODO complete guess of how commid is mapping to service_label.
    constexpr size_t COM_ID_LEN = 12;
    const auto& ids = Common::ElfInfo::Instance().GetNpCommIds();
    std::string com_id;
    if (!ids.empty()) {
        const size_t idx = (service_label >= 0 && static_cast<size_t>(service_label) < ids.size())
                               ? static_cast<size_t>(service_label)
                               : 0;
        com_id = ids[idx];
        if (static_cast<size_t>(service_label) >= ids.size()) {
            LOG_WARNING(NpHandler,
                        "service_label={} >= npbind entry count {} — falling back "
                        "to index 0",
                        service_label, ids.size());
        }
    }
    if (com_id.size() < COM_ID_LEN) {
        com_id.resize(COM_ID_LEN, '\0');
    } else if (com_id.size() > COM_ID_LEN) {
        com_id.resize(COM_ID_LEN);
    }
    return com_id;
}

void NpHandler::OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    const auto cmd_val = static_cast<u16>(cmd);
    if (cmd_val >= 100 && cmd_val <= 200) {
        NpMatching2::OnMatchingReply(cmd, pkt_id, error, body);
    } else if (cmd_val >= 201 && cmd_val <= 300) {
        OnTusReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd_val >= 301 && cmd_val <= 400) {
        OnTrophyReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd == ShadNet::CommandType::LookupOnlineId) {
        OnLookupReply(user_id, cmd, pkt_id, error, body);
    } else {
        OnScoreReply(user_id, cmd, pkt_id, error, body);
    }
}

void NpHandler::ResolveOnlineId(
    s32 user_id, const std::string& online_id,
    std::function<void(s32 result, u64 account_id, const std::string& canonical_online_id)>
        on_result) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "No shadNet session for user_id={}", user_id);
            if (on_result) {
                on_result(ORBIS_NP_ERROR_SIGNED_OUT, 0, {});
            }
            return;
        }
        client = it->second;
    }
    if (!client || !client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        if (on_result) {
            on_result(ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN, 0, {});
        }
        return;
    }
    if (online_id.empty()) {
        if (on_result) {
            on_result(ORBIS_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID, 0, {});
        }
        return;
    }

    const u64 pkt_id = client->LookupOnlineId(online_id);
    if (on_result) {
        std::lock_guard lock(m_mutex_pending_lookup);
        PendingLookupRequest pending;
        pending.user_id = user_id;
        pending.on_result = std::move(on_result);
        m_pending_lookup.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler, "user_id={} onlineId='{}' pkt_id={}", user_id, online_id, pkt_id);
}

void NpHandler::OnLookupReply(s32 user_id, ShadNet::CommandType /*cmd*/, u64 pkt_id,
                              ShadNet::ErrorType error, const std::vector<u8>& body) {
    std::function<void(s32, u64, const std::string&)> on_result;
    {
        std::lock_guard lock(m_mutex_pending_lookup);
        auto it = m_pending_lookup.find(pkt_id);
        if (it == m_pending_lookup.end()) {
            return; // no waiter (already flushed by a disconnect, or a stray reply)
        }
        on_result = std::move(it->second.on_result);
        m_pending_lookup.erase(it);
    }
    if (!on_result) {
        return;
    }

    if (error == ShadNet::ErrorType::NotFound) {
        LOG_INFO(NpHandler, "LookupOnlineId: user_id={} pkt_id={} -> not found", user_id, pkt_id);
        on_result(ORBIS_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID, 0, {});
        return;
    }
    if (error == ShadNet::ErrorType::InvalidInput) {
        on_result(ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT, 0, {});
        return;
    }
    if (error != ShadNet::ErrorType::NoError) {
        LOG_WARNING(NpHandler, "LookupOnlineId failed: error={}", static_cast<int>(error));
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    // Reply body is u32 LE blob size followed by the proto.
    if (body.size() < 4) {
        LOG_WARNING(NpHandler, "LookupOnlineId: truncated reply");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }
    const u32 blob_sz = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                        (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
    if (body.size() < 4 + blob_sz) {
        LOG_WARNING(NpHandler, "LookupOnlineId: reply shorter than declared blob");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    shadnet::LookupOnlineIdReply pb;
    if (!pb.ParseFromArray(body.data() + 4, static_cast<int>(blob_sz))) {
        LOG_WARNING(NpHandler, "LookupOnlineId: could not parse reply");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    LOG_INFO(NpHandler, "LookupOnlineId: user_id={} pkt_id={} -> accountId={} npid='{}'", user_id,
             pkt_id, pb.account_id(), pb.npid());
    on_result(ORBIS_OK, pb.account_id(), pb.npid());
}

} // namespace Libraries::Np
