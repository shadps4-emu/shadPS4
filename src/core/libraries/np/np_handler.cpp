// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <httplib.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/network/net_upnp.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/system/systemservice.h"
#include "core/user_settings.h"
#include "imgui/invitation_prompt_layer.h"
#include "imgui/shadnet_notifications_layer.h"
#include "np_handler.h"
#include "shadnet.pb.h"
#include "shadnet/server_probe.h"

namespace Libraries::Np {

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
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

// Minimal JSON string escaper for the invitation-request body (npids are safe but the user message
// can contain quotes/backslashes/control chars).
std::string JsonEscape(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (const char c : in) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    return out;
}

bool NpHandler::SendSessionInvitation(s32 user_id, const std::string& session_id,
                                      const std::vector<std::string>& to,
                                      const std::string& message) {
    if (session_id.empty() || to.empty()) {
        LOG_ERROR(NpHandler, "SendSessionInvitation: empty session_id or recipient list");
        return false;
    }
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    if (base_url.empty()) {
        LOG_ERROR(NpHandler, "SendSessionInvitation: WebAPI server address is empty");
        return false;
    }
    const std::string token = GetBearerToken(user_id);
    if (token.empty()) {
        LOG_ERROR(NpHandler, "SendSessionInvitation: no bearer token for user_id={}", user_id);
        return false;
    }

    // invitation-request JSON: {"to":[...],"message":"..."}
    std::string json = "{\"to\":[";
    for (size_t i = 0; i < to.size(); ++i) {
        if (i != 0) {
            json += ',';
        }
        json += '"';
        json += JsonEscape(to[i]);
        json += '"';
    }
    json += ']';
    if (!message.empty()) {
        json += ",\"message\":\"";
        json += JsonEscape(message);
        json += '"';
    }
    json += '}';

    // multipart/mixed with a single JSON part; shadNet keys on the Content-Description.
    const std::string boundary = "shadps4invite" + std::to_string(user_id);
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "Content-Type: application/json; charset=utf-8\r\n";
    body += "Content-Description: invitation-request\r\n\r\n";
    body += json;
    body += "\r\n--" + boundary + "--\r\n";

    const std::string path = "/v1/sessions/" + session_id + "/invitations";
    const std::string content_type = "multipart/mixed; boundary=" + boundary;

    httplib::Client cli(base_url);
    cli.set_connection_timeout(5);
    cli.set_read_timeout(10);
    const httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    const auto res = cli.Post(path.c_str(), headers, body, content_type.c_str());
    if (!res) {
        LOG_ERROR(NpHandler, "SendSessionInvitation: POST {} failed (no response)", path);
        return false;
    }
    if (res->status != 200 && res->status != 204) {
        LOG_ERROR(NpHandler, "SendSessionInvitation: POST {} -> HTTP {}", path, res->status);
        return false;
    }
    LOG_INFO(NpHandler, "SendSessionInvitation: sent invite to {} recipient(s) for session '{}'",
             to.size(), session_id);
    return true;
}

void NpHandler::PostSessionInvitationEvent(const std::string& session_id,
                                           const std::string& invitation_id,
                                           const std::string& accepter_online_id) {
    using Libraries::InvitationDialog::ORBIS_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION;
    using Libraries::InvitationDialog::OrbisNpSessionInvitationEventParam;

    Libraries::SystemService::OrbisSystemServiceEvent event{};
    event.event_type = Libraries::SystemService::OrbisSystemServiceEventType::SessionInvitation;

    auto* param = reinterpret_cast<OrbisNpSessionInvitationEventParam*>(event.param);
    std::memset(param, 0, sizeof(*param));
    std::strncpy(param->sessionId.data, session_id.c_str(), sizeof(param->sessionId.data) - 1);
    if (!invitation_id.empty()) {
        std::strncpy(param->invitationId.data, invitation_id.c_str(),
                     sizeof(param->invitationId.data) - 1);
        param->flag = ORBIS_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION;
    } else {
        param->flag = 0; // join from session info (no invitation id in the push)
    }
    std::strncpy(param->onlineId.data, accepter_online_id.c_str(),
                 sizeof(param->onlineId.data) - 1);

    Libraries::SystemService::PushSystemServiceEvent(event);
    LOG_INFO(NpHandler, "Posted SESSION_INVITATION session='{}' flag={} onlineId='{}'", session_id,
             param->flag, accepter_online_id);
}

std::vector<NpHandler::PendingInvitation> NpHandler::GetPendingInvitations(s32 user_id) const {
    std::lock_guard lk(m_mutex_pending_invites);
    const auto it = m_pending_invites.find(user_id);
    if (it == m_pending_invites.end()) {
        return {};
    }
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    std::vector<PendingInvitation> out;
    out.reserve(it->second.size());
    for (const auto& p : it->second) {
        if (p.valid_until == 0 || now <= p.valid_until) {
            out.push_back(p);
        }
    }
    return out;
}

bool NpHandler::AcceptSessionInvitation(s32 user_id, const std::string& invitation_id) {
    // Pull the matching stashed invite (and drop it).
    PendingInvitation inv;
    bool found = false;
    {
        std::lock_guard lk(m_mutex_pending_invites);
        const auto mit = m_pending_invites.find(user_id);
        if (mit != m_pending_invites.end()) {
            auto& v = mit->second;
            for (auto it = v.begin(); it != v.end(); ++it) {
                if (it->invitation_id == invitation_id) {
                    inv = *it;
                    v.erase(it);
                    found = true;
                    break;
                }
            }
        }
    }
    // Whatever surface handled it (RECV dialog or the emulator prompt), retire the other one.
    ImGui::InvitationPrompt::Dismiss(invitation_id);
    if (!found) {
        LOG_ERROR(NpHandler, "AcceptSessionInvitation: no pending invite '{}' for user_id={}",
                  invitation_id, user_id);
        return false;
    }
    // Raise the join event now that the user has explicitly accepted (via the RECV dialog or the
    // emulator's system-UI equivalent).
    PostSessionInvitationEvent(inv.session_id, invitation_id, inv.to_npid);
    // Consume it server-side (PUT usedFlag=true).
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        LOG_ERROR(NpHandler, "AcceptSessionInvitation: no WebAPI server/token for user_id={}",
                  user_id);
        return false;
    }
    httplib::Client cli(base_url);
    cli.set_connection_timeout(5);
    cli.set_read_timeout(10);
    const httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    const std::string path = "/v1/users/me/invitations/" + invitation_id;
    const auto res = cli.Put(path.c_str(), headers, "{\"usedFlag\":true}", "application/json");
    if (!res || (res->status != 200 && res->status != 204)) {
        LOG_ERROR(NpHandler, "AcceptSessionInvitation: PUT {} failed ({})", path,
                  res ? res->status : 0);
        return false;
    }
    LOG_INFO(NpHandler, "AcceptSessionInvitation: consumed '{}' session='{}'", invitation_id,
             inv.session_id);
    return true;
}

void NpHandler::DeclineSessionInvitation(s32 user_id, const std::string& invitation_id) {
    ImGui::InvitationPrompt::Dismiss(invitation_id);
    std::lock_guard lock(m_mutex_pending_invites);
    auto uit = m_pending_invites.find(user_id);
    if (uit == m_pending_invites.end()) {
        return;
    }
    auto& v = uit->second;
    v.erase(std::remove_if(
                v.begin(), v.end(),
                [&](const PendingInvitation& p) { return p.invitation_id == invitation_id; }),
            v.end());
    LOG_INFO(NpHandler, "DeclineSessionInvitation: dismissed '{}' for user_id={}", invitation_id,
             user_id);
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

// friends calls
u32 NpHandler::GetNumFriends(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetNumFriends() : 0;
}

std::optional<std::string> NpHandler::GetFriendNpid(s32 user_id, u32 index) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetFriendNpid(index) : std::nullopt;
}

void NpHandler::OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n) {
    LOG_INFO(NpHandler, "user_id={} FriendQuery from '{}'", user_id, n.fromNpid);
    ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::FriendRequest,
                               "Friend request from " + n.fromNpid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    if (std::find(st.requests_received.begin(), st.requests_received.end(), n.fromNpid) ==
        st.requests_received.end()) {
        st.requests_received.push_back(n.fromNpid);
    }
}

void NpHandler::OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n) {
    LOG_INFO(NpHandler, "user_id={} FriendNew '{}' ({})", user_id, n.npid,
             n.online ? "online" : "offline");
    ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::FriendNew,
                               n.npid + " is now your friend");
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    auto it = std::find_if(st.friends.begin(), st.friends.end(),
                           [&](const FriendInfo& f) { return f.npid == n.npid; });
    if (it == st.friends.end()) {
        st.friends.push_back({n.npid, n.online});
    } else {
        it->online = n.online;
    }
    const auto drop = [&](std::vector<std::string>& v) {
        v.erase(std::remove(v.begin(), v.end(), n.npid), v.end());
    };
    drop(st.requests_received);
    drop(st.requests_sent);
}

void NpHandler::OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n) {
    LOG_INFO(NpHandler, "user_id={} FriendLost '{}'", user_id, n.npid);
    ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::FriendLost,
                               n.npid + " removed you as a friend");
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    st.friends.erase(std::remove_if(st.friends.begin(), st.friends.end(),
                                    [&](const FriendInfo& f) { return f.npid == n.npid; }),
                     st.friends.end());
}

void NpHandler::OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n) {
    LOG_DEBUG(NpHandler, "user_id={} FriendStatus '{}' is {}", user_id, n.npid,
              n.online ? "online" : "offline");
    if (n.online) {
        ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::Online, n.npid + " is online");
    }
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    auto it = std::find_if(st.friends.begin(), st.friends.end(),
                           [&](const FriendInfo& f) { return f.npid == n.npid; });
    if (it != st.friends.end()) {
        it->online = n.online;
    } else {
        st.friends.push_back({n.npid, n.online});
    }
}

void NpHandler::OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n) {
    LOG_INFO(NpHandler, "user_id={} WebApiPushEvent svc='{}' type='{}' bytes={}", user_id,
             n.npServiceName, n.dataType, n.data.size());
    // Forward verbatim to the libSceNpWebApi push-event dispatch.it queues the event
    // and delivers it on the game's thread during sceNpCheckCallback to any registered
    // (and filter-matching) push-event callback.
    NpWebApi::PushEventInput ev;
    ev.targetUserId = user_id;
    ev.npServiceName = n.npServiceName;
    ev.npServiceLabel = n.npServiceLabel;
    ev.dataType = n.dataType;
    ev.data = n.data;
    if (!n.fromNpid.empty()) {
        ev.hasFrom = true;
        SetNpOnlineId(ev.fromOnlineId, n.fromNpid);
    }
    if (!n.toNpid.empty()) {
        ev.hasTo = true;
        SetNpOnlineId(ev.toOnlineId, n.toNpid);
    }
    ev.extdData = n.extdData; // extended-data (key,value) pairs -> dispatched as pExtdData
    NpWebApi::EnqueuePushEvent(ev);

    // Also surface a SESSION_INVITATION system-service event for titles that watch it instead of
    // (or in addition to) the WebAPI push callback
    if (n.npServiceName == "sessionInvitation") {
        std::string session_id, invitation_id;
        int64_t valid_until = 0;
        for (const auto& kv : n.extdData) {
            if (kv.first == "sessionId") {
                session_id = kv.second;
            } else if (kv.first == "invitationId") {
                invitation_id = kv.second;
            } else if (kv.first == "validUntil") {
                valid_until = std::strtoll(kv.second.c_str(), nullptr, 10);
            }
        }
        if (!session_id.empty()) {
            {
                std::lock_guard lk(m_mutex_pending_invites);
                auto& v = m_pending_invites[user_id];
                v.erase(std::remove_if(v.begin(), v.end(),
                                       [&](const PendingInvitation& p) {
                                           return p.invitation_id == invitation_id;
                                       }),
                        v.end());
                v.push_back({session_id, invitation_id, n.fromNpid, n.toNpid, valid_until});
            }
            // ORBIS_SYSTEM_SERVICE_EVENT_SESSION_INVITATION is a *join* event: it
            // fires only after the user explicitly accepts, via the game-opened invitation dialog
            // (RECV) or the system software UI. Posting it here on arrival makes titles silently
            // auto-join. The event is raised from AcceptSessionInvitation instead. The WebAPI push
            // callback above still fires on arrival for titles that watch invites themselves.
            //
            // The emulator has no ShellUI, so surface the "system software UI" half here: an
            // overlay prompt whose Accept routes through AcceptSessionInvitation.
            ImGui::InvitationPrompt::Push(user_id, invitation_id, session_id, n.fromNpid);
        }
    }
}

void NpHandler::OnLoginResult(s32 user_id, const ShadNet::LoginResult& res) {
    if (res.error != ShadNet::ErrorType::NoError) {
        return;
    }
    FriendListSnapshot snap;
    snap.friends.reserve(res.friends.size());
    for (const auto& f : res.friends) {
        snap.friends.push_back({f.npid, f.online});
    }
    snap.requests_sent = res.requestsSent;
    snap.requests_received = res.requestsReceived;
    snap.blocked = res.blocked;
    {
        std::lock_guard lock(m_mutex_friend_state);
        m_friend_state[user_id] = std::move(snap);
    }
    LOG_INFO(NpHandler,
             "user_id={} friend state seeded: {} friends, {} received, {} sent, {} blocked",
             user_id, res.friends.size(), res.requestsReceived.size(), res.requestsSent.size(),
             res.blocked.size());

    // Surface friend requests that arrived while offline (no live notification fires for these).
    if (!res.requestsReceived.empty()) {
        ImGui::ShadNetNotify::Push(ImGui::ShadNetNotify::Kind::FriendRequest,
                                   std::to_string(res.requestsReceived.size()) +
                                       " pending friend request(s)");
    }
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

NpHandler::FriendListSnapshot NpHandler::GetFriendList(s32 user_id) const {
    std::lock_guard lock(m_mutex_friend_state);
    const auto it = m_friend_state.find(user_id);
    return it == m_friend_state.end() ? FriendListSnapshot{} : it->second;
}

s32 NpHandler::SendFriendRequest(s32 user_id, const std::string& npid) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        const auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    client->AddFriend(npid);
    LOG_INFO(NpHandler, "user_id={} AddFriend '{}'", user_id, npid);

    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    const bool accepting = std::find(st.requests_received.begin(), st.requests_received.end(),
                                     npid) != st.requests_received.end();
    if (!accepting && std::find(st.requests_sent.begin(), st.requests_sent.end(), npid) ==
                          st.requests_sent.end()) {
        st.requests_sent.push_back(npid);
    }
    return ORBIS_OK;
}

s32 NpHandler::RemoveFriend(s32 user_id, const std::string& npid) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        const auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    client->RemoveFriend(npid);
    LOG_INFO(NpHandler, "user_id={} RemoveFriend '{}'", user_id, npid);

    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    st.friends.erase(std::remove_if(st.friends.begin(), st.friends.end(),
                                    [&](const FriendInfo& f) { return f.npid == npid; }),
                     st.friends.end());
    const auto drop = [&](std::vector<std::string>& v) {
        v.erase(std::remove(v.begin(), v.end(), npid), v.end());
    };
    drop(st.requests_received);
    drop(st.requests_sent);
    return ORBIS_OK;
}

s32 NpHandler::BlockUser(s32 user_id, const std::string& npid) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        const auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    client->AddBlock(npid);
    LOG_INFO(NpHandler, "user_id={} AddBlock '{}'", user_id, npid);

    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    if (std::find(st.blocked.begin(), st.blocked.end(), npid) == st.blocked.end()) {
        st.blocked.push_back(npid);
    }
    st.friends.erase(std::remove_if(st.friends.begin(), st.friends.end(),
                                    [&](const FriendInfo& f) { return f.npid == npid; }),
                     st.friends.end());
    const auto drop = [&](std::vector<std::string>& v) {
        v.erase(std::remove(v.begin(), v.end(), npid), v.end());
    };
    drop(st.requests_received);
    drop(st.requests_sent);
    return ORBIS_OK;
}

s32 NpHandler::UnblockUser(s32 user_id, const std::string& npid) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        const auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    client->RemoveBlock(npid);
    LOG_INFO(NpHandler, "user_id={} RemoveBlock '{}'", user_id, npid);

    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state[user_id];
    st.blocked.erase(std::remove(st.blocked.begin(), st.blocked.end(), npid), st.blocked.end());
    return ORBIS_OK;
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

// Score

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
                        "GetNpCommId: service_label={} >= npbind entry count {} — falling back "
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

s32 NpHandler::RecordScore(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                           const char* comment, size_t commentLen, const u8* gameInfoData,
                           size_t gameInfoSize, std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    // Look up the user's shadNet session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "RecordScore: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "RecordScore: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    // Build RecordScoreRequest proto.
    shadnet::RecordScoreRequest proto;
    proto.set_boardid(boardId);
    proto.set_pcid(pcId);
    proto.set_score(score);
    if (comment && commentLen > 0) {
        proto.set_comment(std::string(comment, commentLen));
    }
    if (gameInfoData && gameInfoSize > 0) {
        proto.set_data(std::string(reinterpret_cast<const char*>(gameInfoData), gameInfoSize));
    }

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::RecordScore, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::RecordScore;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "RecordScore: user_id={} service_label={} board={} pcId={} score={} commentLen={} "
             "gameInfoSize={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, pcId, score, commentLen, gameInfoSize, pkt_id,
             com_id);
    return ORBIS_OK;
}

s32 NpHandler::RecordGameData(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                              const u8* data, size_t size,
                              std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "RecordGameData: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "RecordGameData: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::RecordScoreGameDataRequest proto;
    proto.set_boardid(boardId);
    proto.set_pcid(pcId);
    proto.set_score(score);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size() + size);
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());
    if (data != nullptr && size > 0) {
        payload.insert(payload.end(), data, data + size);
    }

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::RecordScoreData, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::RecordScoreData;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "RecordGameData: user_id={} service_label={} board={} pcId={} score={} dataSize={} "
             "pkt_id={} com_id='{}'",
             user_id, service_label, boardId, pcId, score, size, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetGameData(s32 user_id, s32 service_label, u32 boardId, const std::string& npId,
                           s32 pcId, void* dataOut, u64 recvSize, u64* totalSizeOut,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetGameData: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetGameData: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreGameDataRequest proto;
    proto.set_boardid(boardId);
    proto.set_npid(npId);
    proto.set_pcid(pcId);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreData, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreData;
        pending.dataOut = dataOut;
        pending.recvSize = recvSize;
        pending.totalSizeOut = totalSizeOut;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetGameData: user_id={} service_label={} board={} npId='{}' pcId={} recvSize={} "
             "pkt_id={} com_id='{}'",
             user_id, service_label, boardId, npId, pcId, recvSize, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetGameDataByAccountId(s32 user_id, s32 service_label, u32 boardId, u64 accountId,
                                      s32 pcId, void* dataOut, u64 recvSize, u64* totalSizeOut,
                                      std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetGameDataByAccountId: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetGameDataByAccountId: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreGameDataByAccountIdRequest proto;
    proto.set_boardid(boardId);
    proto.set_accountid(accountId);
    proto.set_pcid(pcId);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id =
        client->SubmitRequest(ShadNet::CommandType::GetScoreGameDataByAccId, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreGameDataByAccId;
        pending.dataOut = dataOut;
        pending.recvSize = recvSize;
        pending.totalSizeOut = totalSizeOut;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetGameDataByAccountId: user_id={} service_label={} board={} accountId={} pcId={} "
             "recvSize={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, accountId, pcId, recvSize, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetBoardInfo(s32 user_id, s32 service_label, u32 boardId,
                            NpScore::OrbisNpScoreBoardInfo* boardInfo,
                            std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetBoardInfo: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetBoardInfo: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    std::vector<u8> payload;
    payload.reserve(12 + 4);
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    payload.push_back(static_cast<u8>(boardId));
    payload.push_back(static_cast<u8>(boardId >> 8));
    payload.push_back(static_cast<u8>(boardId >> 16));
    payload.push_back(static_cast<u8>(boardId >> 24));

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetBoardInfos, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetBoardInfos;
        pending.boardInfo = boardInfo;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler, "GetBoardInfo: user_id={} service_label={} board={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByNpId(s32 user_id, s32 service_label, u32 boardId,
                                const std::vector<std::string>& npIds,
                                const std::vector<s32>& pcIds,
                                NpScore::OrbisNpScorePlayerRankData* rankArray,
                                NpScore::OrbisNpScoreComment* commentArray,
                                NpScore::OrbisNpScoreGameInfo* infoArray,
                                Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    // pcIds must either be empty (use 0 for everything) or match npIds in size.
    if (!pcIds.empty() && pcIds.size() != npIds.size()) {
        LOG_ERROR(NpHandler, "GetRankingByNpId: pcIds size {} != npIds size {}", pcIds.size(),
                  npIds.size());
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    // Look up the user's session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetRankingByNpId: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetRankingByNpId: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreNpIdRequest proto;
    proto.set_boardid(boardId);
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);
    for (size_t i = 0; i < npIds.size(); ++i) {
        auto* entry = proto.add_npids();
        entry->set_npid(npIds[i]);
        entry->set_pcid(pcIds.empty() ? 0 : pcIds[i]);
    }

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreNpid, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreNpid;
        pending.requestedNpIds = npIds;
        pending.rankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = npIds.size();
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetRankingByNpId: user_id={} service_label={} board={} npIdCount={} "
             "withPcId={} withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, npIds.size(), !pcIds.empty(), commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByRange(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                                 u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                                 NpScore::OrbisNpScoreComment* commentArray,
                                 NpScore::OrbisNpScoreGameInfo* infoArray,
                                 Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                 std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetRankingByRange: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetRankingByRange: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreRangeRequest proto;
    proto.set_boardid(boardId);
    proto.set_startrank(startSerialRank);
    proto.set_numranks(arrayNum);
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreRange, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreRange;
        pending.plainRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetRankingByRange: user_id={} service_label={} board={} startRank={} numRanks={} "
             "withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, startSerialRank, arrayNum, commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByRangeA(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                                  u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                                  NpScore::OrbisNpScoreComment* commentArray,
                                  NpScore::OrbisNpScoreGameInfo* infoArray,
                                  Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                  std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetRankingByRangeA: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetRankingByRangeA: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreRangeRequest proto;
    proto.set_boardid(boardId);
    proto.set_startrank(startSerialRank);
    proto.set_numranks(arrayNum);
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreRange, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreRange;
        // Note: aRankArray is set, plainRankArray remains null. The shared
        // GetScoreRange/GetScoreFriends branch in OnScoreReply dispatches
        // on which one is non-null to pick the right fill helper.
        pending.aRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetRankingByRangeA: user_id={} service_label={} board={} startRank={} numRanks={} "
             "withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, startSerialRank, arrayNum, commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByAccountId(s32 user_id, s32 service_label, u32 boardId,
                                     const std::vector<u64>& accountIds,
                                     const std::vector<s32>& pcIds,
                                     NpScore::OrbisNpScorePlayerRankDataA* rankArray,
                                     NpScore::OrbisNpScoreComment* commentArray,
                                     NpScore::OrbisNpScoreGameInfo* infoArray,
                                     Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                     std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetRankingByAccountId: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreAccountIdRequest proto;
    proto.set_boardid(boardId);
    for (size_t i = 0; i < accountIds.size(); ++i) {
        auto* entry = proto.add_ids();
        entry->set_accountid(static_cast<int64_t>(accountIds[i]));
        // If the caller provided per-entry pcIds, use them; otherwise all zero.
        entry->set_pcid(i < pcIds.size() ? pcIds[i] : 0);
    }
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreAccountId, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreAccountId;
        pending.aPlayerRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = accountIds.size();
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetRankingByAccountId: user_id={} service_label={} board={} accountIdCount={} "
             "withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, accountIds.size(), commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetFriendsRanking(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                                 u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                                 NpScore::OrbisNpScoreComment* commentArray,
                                 NpScore::OrbisNpScoreGameInfo* infoArray,
                                 Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                 std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetFriendsRanking: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetFriendsRanking: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreFriendsRequest proto;
    proto.set_boardid(boardId);
    proto.set_includeself(includeSelf);
    proto.set_max(arrayNum);
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreFriends, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreFriends;
        pending.plainRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetFriendsRanking: user_id={} service_label={} board={} includeSelf={} "
             "arrayNum={} withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, includeSelf, arrayNum, commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetFriendsRankingA(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                                  u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                                  NpScore::OrbisNpScoreComment* commentArray,
                                  NpScore::OrbisNpScoreGameInfo* infoArray,
                                  Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                                  std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "GetFriendsRankingA: user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "GetFriendsRankingA: user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreFriendsRequest proto;
    proto.set_boardid(boardId);
    proto.set_includeself(includeSelf);
    proto.set_max(arrayNum);
    proto.set_withcomment(commentArray != nullptr);
    proto.set_withgameinfo(infoArray != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "no valid NP Communication ID (npbind.dat missing or blank); "
                             "rejecting request");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreFriends, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreFriends;
        // Note: aRankArray is set, plainRankArray remains null. OnScoreReply
        // dispatches on which one is non-null.
        pending.aRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "GetFriendsRankingA: user_id={} service_label={} board={} includeSelf={} "
             "arrayNum={} withComment={} withGameInfo={} pkt_id={} com_id='{}'",
             user_id, service_label, boardId, includeSelf, arrayNum, commentArray != nullptr,
             infoArray != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

static u32 FillPlainRankArrayFromProto(const shadnet::GetScoreResponse& resp, u64 maxSlots,
                                       NpScore::OrbisNpScoreRankData* rankArray,
                                       NpScore::OrbisNpScoreComment* commentArray,
                                       NpScore::OrbisNpScoreGameInfo* infoArray) {
    const int n_resp = resp.rankarray_size();
    u32 out_i = 0;
    for (int i = 0; i < n_resp && out_i < maxSlots; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }
        auto& out = rankArray[out_i];
        // Copy OnlineId (the npId string) into the 16-byte data field.
        const std::string& npid = r.npid();
        const size_t cp = std::min<size_t>(npid.size(), sizeof(out.npId.handle.data));
        std::memcpy(out.npId.handle.data, npid.data(), cp);
        out.npId.handle.term = 0;
        LOG_INFO(NpHandler,
                 "FillPlainRankArrayFromProto: out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
                 "score={}",
                 out_i, i, npid, npid.size(), r.rank(), r.score());
        out.pcId = r.pcid();
        out.serialRank = r.rank();
        out.rank = r.rank();
        out.highestRank = r.rank();
        out.hasGameData = r.hasgamedata() ? 1 : 0;
        out.scoreValue = r.score();
        out.recordDate.tick = r.recorddate();

        if (commentArray != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const size_t ccp =
                std::min<size_t>(cmt.size(), sizeof(commentArray[out_i].utf8Comment) - 1);
            std::memcpy(commentArray[out_i].utf8Comment, cmt.data(), ccp);
        }
        if (infoArray != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const size_t gcp = std::min<size_t>(gi.size(), sizeof(infoArray[out_i].data));
            infoArray[out_i].infoSize = static_cast<u32>(gcp);
            std::memcpy(infoArray[out_i].data, gi.data(), gcp);
        }
        ++out_i;
    }
    return out_i;
}

static u32 FillPlainRankArrayAFromProto(const shadnet::GetScoreResponse& resp, u64 maxSlots,
                                        NpScore::OrbisNpScoreRankDataA* rankArray,
                                        NpScore::OrbisNpScoreComment* commentArray,
                                        NpScore::OrbisNpScoreGameInfo* infoArray) {
    const int n_resp = resp.rankarray_size();
    u32 out_i = 0;
    for (int i = 0; i < n_resp && out_i < maxSlots; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }
        auto& out = rankArray[out_i];
        // RankDataA stores OnlineId directly (not wrapped in NpId).
        const std::string& npid = r.npid();
        const size_t cp = std::min<size_t>(npid.size(), sizeof(out.onlineId.data));
        std::memcpy(out.onlineId.data, npid.data(), cp);
        out.onlineId.term = 0;
        LOG_INFO(NpHandler,
                 "FillPlainRankArrayAFromProto: out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
                 "score={} accountId={}",
                 out_i, i, npid, npid.size(), r.rank(), r.score(), r.accountid());
        out.pcId = r.pcid();
        out.serialRank = r.rank();
        out.rank = r.rank();
        out.highestRank = r.rank();
        out.hasGameData = r.hasgamedata() ? 1 : 0;
        out.scoreValue = r.score();
        out.recordDate.tick = r.recorddate();
        out.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(r.accountid());

        if (commentArray != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const size_t ccp =
                std::min<size_t>(cmt.size(), sizeof(commentArray[out_i].utf8Comment) - 1);
            std::memcpy(commentArray[out_i].utf8Comment, cmt.data(), ccp);
        }
        if (infoArray != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const size_t gcp = std::min<size_t>(gi.size(), sizeof(infoArray[out_i].data));
            infoArray[out_i].infoSize = static_cast<u32>(gcp);
            std::memcpy(infoArray[out_i].data, gi.data(), gcp);
        }
        ++out_i;
    }
    return out_i;
}

static u32 FillPlayerRankArrayAFromProto(const shadnet::GetScoreResponse& resp, u64 requestedCount,
                                         NpScore::OrbisNpScorePlayerRankDataA* rankArray,
                                         NpScore::OrbisNpScoreComment* commentArray,
                                         NpScore::OrbisNpScoreGameInfo* infoArray) {
    const int n_resp = resp.rankarray_size();
    const int n_req = static_cast<int>(requestedCount);
    if (n_resp != n_req) {
        LOG_WARNING(NpHandler,
                    "FillPlayerRankArrayAFromProto: response count {} != request count {} — "
                    "will fill up to min() and leave extras at hasData=0",
                    n_resp, n_req);
    }
    const int n = std::min(n_resp, n_req);
    u32 found = 0;
    for (int i = 0; i < n; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue; // no score on this board for this input accountId
        }
        auto& out = rankArray[i];
        out.hasData = 1;
        const std::string& npid = r.npid();
        const size_t cp = std::min<size_t>(npid.size(), sizeof(out.rankData.onlineId.data));
        std::memcpy(out.rankData.onlineId.data, npid.data(), cp);
        out.rankData.onlineId.term = 0;
        out.rankData.pcId = r.pcid();
        out.rankData.serialRank = r.rank();
        out.rankData.rank = r.rank();
        out.rankData.highestRank = r.rank();
        out.rankData.hasGameData = r.hasgamedata() ? 1 : 0;
        out.rankData.scoreValue = r.score();
        out.rankData.recordDate.tick = r.recorddate();
        out.rankData.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(r.accountid());

        if (commentArray != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const size_t ccp =
                std::min<size_t>(cmt.size(), sizeof(commentArray[i].utf8Comment) - 1);
            std::memcpy(commentArray[i].utf8Comment, cmt.data(), ccp);
        }
        if (infoArray != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const size_t gcp = std::min<size_t>(gi.size(), sizeof(infoArray[i].data));
            infoArray[i].infoSize = static_cast<u32>(gcp);
            std::memcpy(infoArray[i].data, gi.data(), gcp);
        }
        ++found;
    }
    return found;
}

static u32 FillRankArrayFromProto(const shadnet::GetScoreResponse& resp,
                                  const std::vector<std::string>& requestedNpIds,
                                  NpScore::OrbisNpScorePlayerRankData* rankArray,
                                  NpScore::OrbisNpScoreComment* commentArray,
                                  NpScore::OrbisNpScoreGameInfo* infoArray) {
    const int n_resp = resp.rankarray_size();
    const int n_req = static_cast<int>(requestedNpIds.size());
    if (n_resp != n_req) {
        LOG_WARNING(NpHandler,
                    "FillRankArrayFromProto: response count {} != request count {} — "
                    "will fill up to min() and leave extras at hasData=0",
                    n_resp, n_req);
    }
    const int n = std::min(n_resp, n_req);
    u32 found = 0;
    for (int i = 0; i < n; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue; // server signalled "no data for this slot"
        }

        auto& out = rankArray[i];
        out.hasData = 1;
        const std::string& npid = r.npid();
        const size_t cp = std::min<size_t>(npid.size(), sizeof(out.rankData.npId.handle.data));
        std::memcpy(out.rankData.npId.handle.data, npid.data(), cp);
        out.rankData.npId.handle.term = 0;
        out.rankData.pcId = r.pcid();
        out.rankData.serialRank = r.rank();
        out.rankData.rank = r.rank();
        out.rankData.highestRank = r.rank();
        out.rankData.hasGameData = r.hasgamedata() ? 1 : 0;
        out.rankData.scoreValue = r.score();
        out.rankData.recordDate.tick = r.recorddate();

        if (commentArray != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const size_t ccp =
                std::min<size_t>(cmt.size(), sizeof(commentArray[i].utf8Comment) - 1);
            std::memcpy(commentArray[i].utf8Comment, cmt.data(), ccp);
        }
        if (infoArray != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const size_t gcp = std::min<size_t>(gi.size(), sizeof(infoArray[i].data));
            infoArray[i].infoSize = static_cast<u32>(gcp);
            std::memcpy(infoArray[i].data, gi.data(), gcp);
        }
        ++found;
    }
    return found;
}

void NpHandler::OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    const auto cmd_val = static_cast<u16>(cmd);
    if (cmd_val >= 100 && cmd_val <= 200) {
        NpMatching2::OnMatchingReply(cmd, pkt_id, error, body);
    } else {
        OnScoreReply(user_id, cmd, pkt_id, error, body);
    }
}

void NpHandler::OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    PendingScoreRequest pending;
    {
        std::lock_guard lock(m_mutex_pending_score);
        auto it = m_pending_score.find(pkt_id);
        if (it == m_pending_score.end()) {
            LOG_WARNING(NpHandler, "OnScoreReply: no pending request for pkt_id={} cmd={}", pkt_id,
                        static_cast<int>(cmd));
            return;
        }
        pending = std::move(it->second);
        m_pending_score.erase(it);
    }
    auto& req = pending.req;

    // Server-side errors take precedence over success-path parsing.
    if (error != ShadNet::ErrorType::NoError) {
        s32 orbis_err = ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE;
        switch (error) {
        case ShadNet::ErrorType::ScoreNotBest:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE;
            break;
        case ShadNet::ErrorType::ScoreInvalid:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE;
            break;
        case ShadNet::ErrorType::NotFound:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND;
            break;
        case ShadNet::ErrorType::Unauthorized:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN;
            break;
        case ShadNet::ErrorType::DbFail:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR;
            break;
        case ShadNet::ErrorType::Malformed:
        case ShadNet::ErrorType::Invalid:
        case ShadNet::ErrorType::InvalidInput:
            orbis_err = ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE;
            break;
        default:
            break;
        }
        LOG_WARNING(NpHandler,
                    "OnScoreReply: user_id={} pkt_id={} cmd={} server_error={} → orbis_err={:#x}",
                    user_id, pkt_id, static_cast<int>(cmd), static_cast<int>(error), orbis_err);
        req->SetResult(orbis_err);
        return;
    }

    // Per-command success-path parsing.
    switch (cmd) {
    case ShadNet::CommandType::RecordScore: {
        // Reply body = u32 LE rank.
        if (req->tmpRankOut != nullptr && body.size() >= 4) {
            const u32 rank = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                             (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
            *req->tmpRankOut = rank;
            LOG_INFO(NpHandler, "OnScoreReply: RecordScore user_id={} pkt_id={} rank={}", user_id,
                     pkt_id, rank);
        }
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::RecordScoreData: {
        // Reply body is empty on success — error byte already consumed by
        // the caller. Server-side errors are mapped to ErrorType values
        // (NotFound / ScoreInvalid / ScoreHasData) which arrive as the
        // packet's error byte; if we got here, error == NoError.
        LOG_INFO(NpHandler, "OnScoreReply: RecordScoreData user_id={} pkt_id={} ok", user_id,
                 pkt_id);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetScoreData:
    case ShadNet::CommandType::GetScoreGameDataByAccId: {
        const char* cmd_name = (cmd == ShadNet::CommandType::GetScoreData)
                                   ? "GetScoreData"
                                   : "GetScoreGameDataByAccId";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "OnScoreReply: {} body too small ({})", cmd_name, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 stored_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                                (static_cast<u32>(body[2]) << 16) |
                                (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + stored_size > body.size()) {
            LOG_ERROR(NpHandler, "OnScoreReply: {} blob size {} exceeds body size {}", cmd_name,
                      stored_size, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }

        // Always populate *totalSizeOut with the actual stored size on
        // the server, even when truncating into a smaller dataOut buffer.
        // Lets the caller know if they undersized their buffer and need
        // to retry with a larger one.
        if (pending.totalSizeOut != nullptr) {
            *pending.totalSizeOut = stored_size;
        }
        if (pending.dataOut != nullptr && pending.recvSize > 0) {
            const u64 copy_n = std::min<u64>(static_cast<u64>(stored_size), pending.recvSize);
            std::memcpy(pending.dataOut, body.data() + 4, static_cast<size_t>(copy_n));
            if (copy_n < stored_size) {
                LOG_WARNING(NpHandler,
                            "OnScoreReply: {} truncated user_id={} pkt_id={} stored={} recv={}",
                            cmd_name, user_id, pkt_id, stored_size, pending.recvSize);
            }
        }
        LOG_INFO(NpHandler, "OnScoreReply: {} user_id={} pkt_id={} storedSize={} recvSize={}",
                 cmd_name, user_id, pkt_id, stored_size, pending.recvSize);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetBoardInfos: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetBoardInfos body too small ({})", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetBoardInfos proto size {} exceeds body size {}",
                      proto_size, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::BoardInfo bi;
        if (!bi.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetBoardInfos proto parse failed");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        if (pending.boardInfo != nullptr) {
            pending.boardInfo->rankLimit = bi.ranklimit();
            pending.boardInfo->updateMode = bi.updatemode();
            pending.boardInfo->sortMode = bi.sortmode();
            pending.boardInfo->uploadNumLimit = bi.uploadnumlimit();
            pending.boardInfo->uploadSizeLimit = bi.uploadsizelimit();
        }
        LOG_INFO(NpHandler,
                 "OnScoreReply: GetBoardInfos user_id={} pkt_id={} rankLimit={} updateMode={} "
                 "sortMode={} uploadNumLimit={} uploadSizeLimit={}",
                 user_id, pkt_id, bi.ranklimit(), bi.updatemode(), bi.sortmode(),
                 bi.uploadnumlimit(), bi.uploadsizelimit());
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetScoreNpid: {
        // Reply body = u32 LE proto size + GetScoreResponse proto bytes.
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetScoreNpid body too small ({})", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetScoreNpid proto size {} exceeds body size {}",
                      proto_size, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetScoreNpid proto parse failed");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 found = FillRankArrayFromProto(resp, pending.requestedNpIds, pending.rankArray,
                                                 pending.commentArray, pending.infoArray);
        if (pending.lastSortDate != nullptr) {
            pending.lastSortDate->tick = resp.lastsortdate();
        }
        if (pending.totalRecord != nullptr) {
            *pending.totalRecord = resp.totalrecord();
        }
        LOG_INFO(NpHandler,
                 "OnScoreReply: GetScoreNpid user_id={} pkt_id={} found={}/{} totalRecord={}",
                 user_id, pkt_id, found, pending.arrayNum, resp.totalrecord());
        req->SetResult(static_cast<s32>(found));
        break;
    }
    case ShadNet::CommandType::GetScoreAccountId: {
        // Reply body = u32 LE proto size + GetScoreResponse proto bytes.
        // Index-aligned with the request's accountIds[] (empty-npid sentinel
        // marks "no score on this board"), same contract as GetScoreNpid.
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetScoreAccountId body too small ({})",
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler,
                      "OnScoreReply: GetScoreAccountId proto size {} exceeds body size {}",
                      proto_size, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "OnScoreReply: GetScoreAccountId proto parse failed");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 found =
            FillPlayerRankArrayAFromProto(resp, pending.arrayNum, pending.aPlayerRankArray,
                                          pending.commentArray, pending.infoArray);
        if (pending.lastSortDate != nullptr) {
            pending.lastSortDate->tick = resp.lastsortdate();
        }
        if (pending.totalRecord != nullptr) {
            *pending.totalRecord = resp.totalrecord();
        }
        LOG_INFO(NpHandler,
                 "OnScoreReply: GetScoreAccountId user_id={} pkt_id={} found={}/{} totalRecord={}",
                 user_id, pkt_id, found, pending.arrayNum, resp.totalrecord());
        req->SetResult(static_cast<s32>(found));
        break;
    }

    case ShadNet::CommandType::GetScoreRange:
    case ShadNet::CommandType::GetScoreFriends: {
        const char* cmd_name =
            (cmd == ShadNet::CommandType::GetScoreRange) ? "GetScoreRange" : "GetScoreFriends";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "OnScoreReply: {} body too small ({})", cmd_name, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "OnScoreReply: {} proto size {} exceeds body size {}", cmd_name,
                      proto_size, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "OnScoreReply: {} proto parse failed", cmd_name);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        u32 found = 0;
        if (pending.aRankArray != nullptr) {
            found = FillPlainRankArrayAFromProto(resp, pending.arrayNum, pending.aRankArray,
                                                 pending.commentArray, pending.infoArray);
        } else {
            found = FillPlainRankArrayFromProto(resp, pending.arrayNum, pending.plainRankArray,
                                                pending.commentArray, pending.infoArray);
        }
        if (pending.lastSortDate != nullptr) {
            pending.lastSortDate->tick = resp.lastsortdate();
        }
        if (pending.totalRecord != nullptr) {
            *pending.totalRecord = resp.totalrecord();
        }
        LOG_INFO(NpHandler, "OnScoreReply: {} user_id={} pkt_id={} found={}/{} totalRecord={}{}",
                 cmd_name, user_id, pkt_id, found, pending.arrayNum, resp.totalrecord(),
                 pending.aRankArray != nullptr ? " (A-variant)" : "");
        req->SetResult(static_cast<s32>(found));
        break;
    }

    default:
        LOG_WARNING(NpHandler, "OnScoreReply: unexpected cmd={} pkt_id={}", static_cast<int>(cmd),
                    pkt_id);
        req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
        break;
    }
}

} // namespace Libraries::Np
