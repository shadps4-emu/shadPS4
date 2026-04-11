// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/np_manager.h"
#include "core/user_settings.h"
#include "np_handler.h"

namespace Libraries::Np {

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
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

    // Parse server address from GeneralSettings
    const std::string server_str = EmulatorSettings.GetShadNetServer();
    std::string host = server_str;
    u16 port = 31313; // default port
    const auto colon = server_str.rfind(':');
    if (colon != std::string::npos) {
        host = server_str.substr(0, colon);
        try {
            port = static_cast<u16>(std::stoi(server_str.substr(colon + 1)));
        } catch (...) {
        }
    }

    const auto logged_in = UserManagement.GetLoggedInUsers(); // get all login users
    int connected_count = 0;

    for (int i = 0; i < Libraries::UserService::ORBIS_USER_SERVICE_MAX_LOGIN_USERS; ++i) {
        const User* u = logged_in[i];
        if (!u)
            continue;
        // skip users that has shadnet disabled
        if (!u->shadnet_enabled) {
            LOG_DEBUG(NpHandler, "user_id={} ('{}') shadNet disabled,skipping", u->user_id,
                      u->user_name);
            continue;
        }
        // skip also users that doesn't have npid or password empty
        if (u->shadnet_npid.empty() || u->shadnet_password.empty()) {
            LOG_WARNING(NpHandler,
                        "user_id={} ('{}') shadNet enabled but credentials missing,skipping",
                        u->user_id, u->user_name);
            continue;
        }

        if (ConnectUser(u->user_id, host, port, u->shadnet_npid, u->shadnet_password,
                        u->shadnet_token))
            ++connected_count;
    }

    if (connected_count == 0) {
        LOG_WARNING(NpHandler, "no users connected to shadNet");
        return;
    }

    // Start the health-monitor worker
    m_worker_running = true;
    m_worker_thread = std::thread(&NpHandler::WorkerThread, this);
}

void NpHandler::Shutdown() {
    if (!m_initialized.exchange(false))
        return;

    m_worker_running = false;

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
    LOG_INFO(NpHandler, "Connecting user_id={} npid='{}' to {}:{}", user_id, npid, host, port);

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

    client->Start(host, port, npid, password, token);

    const ShadNet::ShadNetState conn_state = client->WaitForConnection();
    if (conn_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "user_id={} connection failed (state={})", user_id,
                  static_cast<int>(conn_state));
        client->Stop();
        return false;
    }

    const ShadNet::ShadNetState auth_state = client->WaitForAuthenticated();
    if (auth_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "user_id={} authentication failed (state={})", user_id,
                  static_cast<int>(auth_state));
        client->Stop();
        return false;
    }

    LOG_INFO(NpHandler, "user_id={} signed in onlineName='{}' accountId={}", user_id,
             client->GetOnlineName(), client->GetUserId());

    // Write server-authoritative onlineName and avatarUrl back into the User
    // record so they survive restarts and stay in sync with the server.
    // The local shadnet_online_name/shadnet_avatar_url are what the client sent at
    // account creation; the server may normalise them, so we update from the
    // reply and persist immediately.
    // p.s i am not sure if we need to do this TODO check if it is needed
    if (User* u = UserManagement.GetUserByID(user_id)) {
        bool dirty = false;
        const std::string server_name = client->GetOnlineName();
        const std::string server_avatar = client->GetAvatarUrl();
        if (!server_name.empty() && u->shadnet_online_name != server_name) {
            u->shadnet_online_name = server_name;
            dirty = true;
        }
        if (!server_avatar.empty() && u->shadnet_avatar_url != server_avatar) {
            u->shadnet_avatar_url = server_avatar;
            dirty = true;
        }
        if (dirty)
            UserSettings.Save();
    }

    {
        std::lock_guard lock(m_mutex_clients);
        m_clients[user_id] = std::move(client);
    }

    FireStateCallback(user_id, NpManager::OrbisNpState::SignedIn);
    return true;
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
    client->Stop();
    FireStateCallback(user_id, NpManager::OrbisNpState::SignedOut);
    LOG_INFO(NpHandler, "user_id={} disconnected", user_id);
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
            LOG_WARNING(NpHandler, "NpHandler: user_id={} connection dropped", uid);
            DisconnectUser(uid);
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

std::string NpHandler::GetOnlineName(s32 user_id) const {
    // Returns the display name set at account creation and returned by the
    // server in the Login reply.
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetOnlineName() : std::string{};
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
    LOG_INFO(NpHandler, "NpHandler: user_id={} FriendQuery from '{}'", user_id, n.fromNpid);
    // TODO: enqueue for sceNpCheckCallback dispatch
}

void NpHandler::OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n) {
    LOG_INFO(NpHandler, "NpHandler: user_id={} FriendNew '{}' ({})", user_id, n.npid,
             n.online ? "online" : "offline");
}

void NpHandler::OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n) {
    LOG_INFO(NpHandler, "NpHandler: user_id={} FriendLost '{}'", user_id, n.npid);
}

void NpHandler::OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n) {
    LOG_DEBUG(NpHandler, "NpHandler: user_id={} FriendStatus '{}' is {}", user_id, n.npid,
              n.online ? "online" : "offline");
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

} // namespace Libraries::Np
