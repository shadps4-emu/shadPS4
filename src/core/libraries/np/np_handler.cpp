// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/user_settings.h"
#include "np_handler.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

// Static empty NpId returned when user_id is not connected.
const OrbisNpId NpHandler::s_empty_np_id{};

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
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnScoreReply(user_id, cmd, pkt_id, err, body);
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

    LOG_INFO(NpHandler, "user_id={} signed in npid='{}' accountId={}", user_id, npid,
             client->GetUserId());

    // Build OrbisNpId
    {
        OrbisNpId np_id{};
        strncpy(np_id.handle.data, npid.c_str(), sizeof(np_id.handle.data) - 1);
        std::lock_guard lock(m_mutex_clients);
        m_np_ids[user_id] = np_id;
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

const OrbisNpId& NpHandler::GetNpId(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_np_ids.find(user_id);
    return it != m_np_ids.end() ? it->second : s_empty_np_id;
}

const OrbisNpOnlineId& NpHandler::GetOnlineId(s32 user_id) const {
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

// Score

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
            orbis_err = ORBIS_OK;
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
        const u32 found =
            FillPlainRankArrayFromProto(resp, pending.arrayNum, pending.plainRankArray,
                                        pending.commentArray, pending.infoArray);
        if (pending.lastSortDate != nullptr) {
            pending.lastSortDate->tick = resp.lastsortdate();
        }
        if (pending.totalRecord != nullptr) {
            *pending.totalRecord = resp.totalrecord();
        }
        LOG_INFO(NpHandler, "OnScoreReply: {} user_id={} pkt_id={} found={}/{} totalRecord={}",
                 cmd_name, user_id, pkt_id, found, pending.arrayNum, resp.totalrecord());
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
