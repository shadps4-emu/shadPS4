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

        ConnectUser(u->user_id, host, port, u->shadnet_npid, u->shadnet_password, u->shadnet_token);
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

bool NpHandler::IsPsnSignedIn(s32 user_id) const {
    return false;
}

bool NpHandler::IsAnySignedIn() const {
    return false;
}

std::string NpHandler::GetOnlineName(s32 user_id) const {
    return {};
}

std::string NpHandler::GetAvatarUrl(s32 user_id) const {
    return {};
}

OrbisNpAccountId NpHandler::GetAccountId(s32 user_id) const {
    return 0;
}

u32 NpHandler::GetLocalIpAddr(s32 user_id) const {
    return u32();
}

s32 NpHandler::GetUserIdByAccountId(u64 account_id) const {
    return -1;
}

u32 NpHandler::GetNumFriends(s32 user_id) const {
    return u32();
}

std::optional<std::string> NpHandler::GetFriendNpid(s32 user_id, u32 index) const {
    return std::nullopt;
}

s32 NpHandler::RegisterStateCallback(StateCallback cb, void* userdata) {
    return -1;
}

void NpHandler::UnregisterStateCallback(s32 handle) {}

void NpHandler::ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                            const std::string& password, const std::string& token) {}

void NpHandler::DisconnectUser(s32 user_id) {}

void NpHandler::WorkerThread() {}

void NpHandler::FireStateCallback(s32 user_id, NpManager::OrbisNpState state) {}

void NpHandler::OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n) {}

void NpHandler::OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n) {}

void NpHandler::OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n) {}

void OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n) {}

} // namespace Libraries::Np
