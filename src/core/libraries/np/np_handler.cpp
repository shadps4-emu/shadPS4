// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "np_handler.h"

namespace Libraries::Np {

void NpHandler::Initialize() {}

void NpHandler::Shutdown() {}

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
