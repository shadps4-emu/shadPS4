// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "imgui/shadnet_notifications_layer.h"

namespace Libraries::Np {

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

} // namespace Libraries::Np
