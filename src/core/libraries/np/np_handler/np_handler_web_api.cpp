// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "imgui/invitation_prompt_layer.h"

namespace Libraries::Np {

void NpHandler::OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n) {
    LOG_INFO(NpHandler, "user_id={} WebApiPushEvent svc='{}' type='{}' bytes={}", user_id,
             n.npServiceName, n.dataType, n.data.size());
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
    // Account ids for pFrom/pTo.
    ev.toAccountId = n.toAccountId != 0 ? n.toAccountId : GetAccountId(user_id);
    ev.fromAccountId = n.fromAccountId;
    if (ev.fromAccountId == 0) {
        for (const auto& kv : n.extdData) {
            if (kv.first == "fromAccountId") {
                ev.fromAccountId = std::strtoull(kv.second.c_str(), nullptr, 10);
                break;
            }
        }
    }
    if (ev.toAccountId != 0) {
        ev.hasTo = true;
        if (n.toNpid.empty()) {
            // e.g. friendlist events carry no toNpid; the recipient is still us.
            std::lock_guard lock(m_mutex_clients);
            if (const auto it = m_np_ids.find(user_id); it != m_np_ids.end()) {
                std::memcpy(ev.toOnlineId.data, it->second.handle.data, sizeof(ev.toOnlineId.data));
            }
        }
    }
    NpWebApi::EnqueuePushEvent(ev);
    NpWebApi2::EnqueuePushEvent(ev);

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
                PendingInvitation inv{session_id, invitation_id, n.fromNpid, n.toNpid, valid_until};
                inv.from_account_id = ev.fromAccountId;
                v.push_back(std::move(inv));
            }
            ImGui::InvitationPrompt::Push(user_id, invitation_id, session_id, n.fromNpid);
        }
    }
}

} // namespace Libraries::Np
