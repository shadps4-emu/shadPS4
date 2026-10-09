// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_score/np_score.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

s32 NpHandler::RecordScore(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                           const char* comment, size_t commentLen, const u8* gameInfoData,
                           size_t gameInfoSize, std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    // Look up the user's shadNet session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} pcId={} score={} commentLen={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} pcId={} score={} dataSize={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} npId='{}' pcId={} recvSize={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} accountId={} pcId={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler, "user_id={} service_label={} board={} pkt_id={} com_id='{}'", user_id,
             service_label, boardId, pkt_id, com_id);
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
        LOG_ERROR(NpHandler, "pcIds size {} != npIds size {}", pcIds.size(), npIds.size());
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    // Look up the user's session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} npIdCount={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} startRank={} numRanks={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.aRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} startRank={} numRanks={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} accountIdCount={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} includeSelf={} "
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
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
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
        pending.aRankArray = rankArray;
        pending.commentArray = commentArray;
        pending.infoArray = infoArray;
        pending.lastSortDate = lastSortDate;
        pending.totalRecord = totalRecord;
        pending.arrayNum = arrayNum;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} includeSelf={} "
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
                 "out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
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
                 "out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
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
                    "response count {} != request count {} — "
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
                    "response count {} != request count {} — "
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

void NpHandler::OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    PendingScoreRequest pending;
    {
        std::lock_guard lock(m_mutex_pending_score);
        auto it = m_pending_score.find(pkt_id);
        if (it == m_pending_score.end()) {
            LOG_WARNING(NpHandler, "no pending request for pkt_id={} cmd={}", pkt_id,
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
        LOG_WARNING(NpHandler, "user_id={} pkt_id={} cmd={} server_error={} → orbis_err={:#x}",
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
            LOG_INFO(NpHandler, "RecordScore user_id={} pkt_id={} rank={}", user_id, pkt_id, rank);
        }
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::RecordScoreData: {
        LOG_INFO(NpHandler, "RecordScoreData user_id={} pkt_id={} ok", user_id, pkt_id);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetScoreData:
    case ShadNet::CommandType::GetScoreGameDataByAccId: {
        const char* cmd_name = (cmd == ShadNet::CommandType::GetScoreData)
                                   ? "GetScoreData"
                                   : "GetScoreGameDataByAccId";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "{} body too small ({})", cmd_name, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 stored_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                                (static_cast<u32>(body[2]) << 16) |
                                (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + stored_size > body.size()) {
            LOG_ERROR(NpHandler, "{} blob size {} exceeds body size {}", cmd_name, stored_size,
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        if (pending.totalSizeOut != nullptr) {
            *pending.totalSizeOut = stored_size;
        }
        if (pending.dataOut != nullptr && pending.recvSize > 0) {
            const u64 copy_n = std::min<u64>(static_cast<u64>(stored_size), pending.recvSize);
            std::memcpy(pending.dataOut, body.data() + 4, static_cast<size_t>(copy_n));
            if (copy_n < stored_size) {
                LOG_WARNING(NpHandler, "{} truncated user_id={} pkt_id={} stored={} recv={}",
                            cmd_name, user_id, pkt_id, stored_size, pending.recvSize);
            }
        }
        LOG_INFO(NpHandler, "{} user_id={} pkt_id={} storedSize={} recvSize={}", cmd_name, user_id,
                 pkt_id, stored_size, pending.recvSize);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetBoardInfos: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "GetBoardInfos body too small ({})", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "GetBoardInfos proto size {} exceeds body size {}", proto_size,
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::BoardInfo bi;
        if (!bi.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "GetBoardInfos proto parse failed");
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
                 "GetBoardInfos user_id={} pkt_id={} rankLimit={} updateMode={} "
                 "sortMode={} uploadNumLimit={} uploadSizeLimit={}",
                 user_id, pkt_id, bi.ranklimit(), bi.updatemode(), bi.sortmode(),
                 bi.uploadnumlimit(), bi.uploadsizelimit());
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetScoreNpid: {
        // Reply body = u32 LE proto size + GetScoreResponse proto bytes.
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "GetScoreNpid body too small ({})", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "GetScoreNpid proto size {} exceeds body size {}", proto_size,
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "GetScoreNpid proto parse failed");
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
        LOG_INFO(NpHandler, "GetScoreNpid user_id={} pkt_id={} found={}/{} totalRecord={}", user_id,
                 pkt_id, found, pending.arrayNum, resp.totalrecord());
        req->SetResult(static_cast<s32>(found));
        break;
    }
    case ShadNet::CommandType::GetScoreAccountId: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "GetScoreAccountId body too small ({})", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "GetScoreAccountId proto size {} exceeds body size {}", proto_size,
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "GetScoreAccountId proto parse failed");
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
        LOG_INFO(NpHandler, "GetScoreAccountId user_id={} pkt_id={} found={}/{} totalRecord={}",
                 user_id, pkt_id, found, pending.arrayNum, resp.totalrecord());
        req->SetResult(static_cast<s32>(found));
        break;
    }

    case ShadNet::CommandType::GetScoreRange:
    case ShadNet::CommandType::GetScoreFriends: {
        const char* cmd_name =
            (cmd == ShadNet::CommandType::GetScoreRange) ? "GetScoreRange" : "GetScoreFriends";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "{} body too small ({})", cmd_name, body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<size_t>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "{} proto size {} exceeds body size {}", cmd_name, proto_size,
                      body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<int>(proto_size))) {
            LOG_ERROR(NpHandler, "{} proto parse failed", cmd_name);
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
        LOG_INFO(NpHandler, "{} user_id={} pkt_id={} found={}/{} totalRecord={}{}", cmd_name,
                 user_id, pkt_id, found, pending.arrayNum, resp.totalrecord(),
                 pending.aRankArray != nullptr ? " (A-variant)" : "");
        req->SetResult(static_cast<s32>(found));
        break;
    }

    default:
        LOG_WARNING(NpHandler, "unexpected cmd={} pkt_id={}", static_cast<int>(cmd), pkt_id);
        req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
        break;
    }
}

} // namespace Libraries::Np
