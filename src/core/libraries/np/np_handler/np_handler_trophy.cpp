// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

void NpHandler::ReportTrophyUnlock(s32 user_id, s32 service_label, s32 trophy_id, u64 timestamp) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_DEBUG(NpHandler, "no shadNet session for user_id={}; skipping", user_id);
            return;
        }
        client = it->second;
    }
    if (!client) {
        LOG_DEBUG(NpHandler, "null client for user_id={}; skipping", user_id);
        return;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "not authenticated; trophy {} not mirrored", trophy_id);
        return;
    }
    if (!client->IsTrophiesEnabled()) {
        LOG_WARNING(NpHandler,
                    "server has not advertised trophy support; trophy {} "
                    "not mirrored",
                    trophy_id);
        return;
    }

    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_WARNING(NpHandler,
                    "no valid NP Communication ID for service_label={}; "
                    "skipping trophy {}",
                    service_label, trophy_id);
        return;
    }

    const u64 pkt_id = client->UnlockTrophy(com_id, trophy_id, timestamp);
    LOG_INFO(NpHandler, "user_id={} trophy={} com_id='{}' pkt_id={}", user_id, trophy_id, com_id,
             pkt_id);
}

void NpHandler::SyncTrophies(
    s32 user_id, s32 service_label, const std::vector<std::pair<s32, u64>>& local_trophies,
    std::function<void(const std::vector<std::pair<s32, u64>>&)> on_merged) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_DEBUG(NpHandler, "no shadNet session for user_id={}; skipping", user_id);
            return;
        }
        client = it->second;
    }
    if (!client) {
        LOG_DEBUG(NpHandler, "null client for user_id={}; skipping", user_id);
        return;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "not authenticated; skipping sync");
        return;
    }
    if (!client->IsTrophiesEnabled()) {
        LOG_WARNING(NpHandler, "server has not advertised trophy support; "
                               "skipping sync");
        return;
    }

    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_WARNING(NpHandler, "no valid NP Communication ID for service_label={}; skipping",
                    service_label);
        return;
    }

    const u64 pkt_id = client->SyncTrophies(com_id, local_trophies);
    if (on_merged) {
        std::lock_guard lock(m_mutex_pending_trophy);
        m_pending_trophy.emplace(pkt_id, std::move(on_merged));
    }
    LOG_INFO(NpHandler, "user_id={} uploading {} com_id='{}' pkt_id={}", user_id,
             local_trophies.size(), com_id, pkt_id);
}

void NpHandler::OnTrophyReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                              ShadNet::ErrorType error, const std::vector<u8>& body) {
    if (cmd == ShadNet::CommandType::UnlockTrophy) {
        if (error != ShadNet::ErrorType::NoError) {
            LOG_WARNING(NpHandler, "UnlockTrophy rejected by server: error={}",
                        static_cast<int>(error));
        }
        return;
    }
    if (cmd != ShadNet::CommandType::SyncTrophies)
        return;

    std::function<void(const std::vector<std::pair<s32, u64>>&)> on_merged;
    {
        std::lock_guard lock(m_mutex_pending_trophy);
        auto it = m_pending_trophy.find(pkt_id);
        if (it == m_pending_trophy.end())
            return;
        on_merged = std::move(it->second);
        m_pending_trophy.erase(it);
    }

    if (error != ShadNet::ErrorType::NoError) {
        LOG_WARNING(NpHandler, "SyncTrophies failed: error={}", static_cast<int>(error));
        return; // leave local trophies exactly as they were
    }

    // Reply body is u32 LE blob size followed by the proto.
    if (body.size() < 4) {
        LOG_WARNING(NpHandler, "SyncTrophies: truncated reply");
        return;
    }
    const u32 blob_sz = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                        (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
    if (body.size() < 4 + blob_sz) {
        LOG_WARNING(NpHandler, "SyncTrophies: reply shorter than declared blob");
        return;
    }

    shadnet::SyncTrophiesReply pb;
    if (!pb.ParseFromArray(body.data() + 4, static_cast<int>(blob_sz))) {
        LOG_WARNING(NpHandler, "could not parse reply");
        return;
    }

    std::vector<std::pair<s32, u64>> merged;
    merged.reserve(pb.trophies_size());
    for (const auto& t : pb.trophies())
        merged.emplace_back(t.trophyid(), t.timestamp());

    LOG_INFO(NpHandler, "server holds {} trophies for user_id={}", merged.size(), user_id);
    on_merged(merged);
}

} // namespace Libraries::Np
