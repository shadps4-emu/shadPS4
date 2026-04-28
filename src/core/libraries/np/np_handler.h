// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_score/np_score_ctx.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/system/userservice.h"
#include "shadnet/client.h"

namespace Libraries::Np {

class NpHandler {
public:
    static NpHandler& GetInstance();

    NpHandler(const NpHandler&) = delete;
    NpHandler& operator=(const NpHandler&) = delete;

    // Connect every currently-logged-in user that has shadNet credentials.
    void Initialize();

    // Disconnect all clients, stop worker threads, fire SignedOut for each.
    void Shutdown();

    // True if this specific user is authenticated to the shadNet server.
    bool IsPsnSignedIn(s32 user_id) const;

    /// True if any user is currently signed in
    bool IsAnySignedIn() const;

    /// Full NP ID for this user, built once from shadnet_npid after login.
    const OrbisNpId& GetNpId(s32 user_id) const;

    /// The Online ID embedded in the NP ID (npid.handle).
    const OrbisNpOnlineId& GetOnlineId(s32 user_id) const;

    // Avatar URL returned by the server.
    std::string GetAvatarUrl(s32 user_id) const;

    // 64-bit account ID assigned by the server.
    OrbisNpAccountId GetAccountId(s32 user_id) const;

    // Local IP address (network byte order) as seen at connect time.
    u32 GetLocalIpAddr(s32 user_id) const;

    // Reverse lookup: server account_id to local user_id.
    // Returns -1 if no connected user owns that account_id.
    s32 GetUserIdByAccountId(u64 account_id) const;

    // Reverse lookup: OrbisNpOnlineId to local user_id.
    // Scans m_np_ids for a matching handle.data string.
    // Returns -1 if no connected user has that Online ID.
    s32 GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const;

    // Friend list
    u32 GetNumFriends(s32 user_id) const;
    std::optional<std::string> GetFriendNpid(s32 user_id, u32 index) const;

    // Submit a RecordScore request to the shadNet server.
    s32 RecordScore(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                    const char* comment, size_t commentLen, const u8* gameInfoData,
                    size_t gameInfoSize, std::shared_ptr<NpScore::ScoreRequestCtx> req);

    s32 RecordGameData(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                       const u8* data, size_t size, std::shared_ptr<NpScore::ScoreRequestCtx> req);

    s32 GetGameData(s32 user_id, s32 service_label, u32 boardId, const std::string& npId, s32 pcId,
                    void* dataOut, u64 recvSize, u64* totalSizeOut,
                    std::shared_ptr<NpScore::ScoreRequestCtx> req);

    s32 GetGameDataByAccountId(s32 user_id, s32 service_label, u32 boardId, u64 accountId, s32 pcId,
                               void* dataOut, u64 recvSize, u64* totalSizeOut,
                               std::shared_ptr<NpScore::ScoreRequestCtx> req);

    s32 GetBoardInfo(s32 user_id, s32 service_label, u32 boardId,
                     NpScore::OrbisNpScoreBoardInfo* boardInfo,
                     std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // Submit a GetRankingByNpId request to the shadNet server.
    s32 GetRankingByNpId(s32 user_id, s32 service_label, u32 boardId,
                         const std::vector<std::string>& npIds, const std::vector<s32>& pcIds,
                         NpScore::OrbisNpScorePlayerRankData* rankArray,
                         NpScore::OrbisNpScoreComment* commentArray,
                         NpScore::OrbisNpScoreGameInfo* infoArray,
                         Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                         std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // Submit a GetRankingByRange request to the shadNet server.
    s32 GetRankingByRange(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                          u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                          NpScore::OrbisNpScoreComment* commentArray,
                          NpScore::OrbisNpScoreGameInfo* infoArray,
                          Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // A-variant of GetRankingByRange.
    s32 GetRankingByRangeA(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                           u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                           NpScore::OrbisNpScoreComment* commentArray,
                           NpScore::OrbisNpScoreGameInfo* infoArray,
                           Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);

    s32 GetRankingByAccountId(s32 user_id, s32 service_label, u32 boardId,
                              const std::vector<u64>& accountIds, const std::vector<s32>& pcIds,
                              NpScore::OrbisNpScorePlayerRankDataA* rankArray,
                              NpScore::OrbisNpScoreComment* commentArray,
                              NpScore::OrbisNpScoreGameInfo* infoArray,
                              Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                              std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // Submit a GetFriendsRanking request to the shadNet server.
    s32 GetFriendsRanking(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                          u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                          NpScore::OrbisNpScoreComment* commentArray,
                          NpScore::OrbisNpScoreGameInfo* infoArray,
                          Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // A-variant of GetFriendsRanking
    s32 GetFriendsRankingA(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                           u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                           NpScore::OrbisNpScoreComment* commentArray,
                           NpScore::OrbisNpScoreGameInfo* infoArray,
                           Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // State callbacks
    using StateCallback = std::function<void(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             NpManager::OrbisNpState state)>;

    s32 RegisterStateCallback(StateCallback cb, void* userdata);
    void UnregisterStateCallback(s32 handle);

private:
    NpHandler() = default;
    ~NpHandler() = default;

    /// Connect one user.  Blocks until connected+authenticated or failed.
    bool ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                     const std::string& password, const std::string& token);

    // Disconnect and remove one user's client.
    void DisconnectUser(s32 user_id);

    void WorkerThread();
    void FireStateCallback(s32 user_id, NpManager::OrbisNpState state);

    // Notification forwarders wired into each client
    void OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n);
    void OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n);
    void OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n);
    void OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n);

    // Async reply dispatch for score commands. Called from the per-user
    // ShadNetClient on the reader thread.
    void OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body);

    // 12-byte NP Communication ID
    std::string GetNpCommId(s32 service_label) const;

    // Per-user client map
    mutable std::mutex m_mutex_clients;
    std::map<s32, std::shared_ptr<ShadNet::ShadNetClient>> m_clients;
    // Per-user NP ID built once from shadnet_npid after login.
    std::map<s32, OrbisNpId> m_np_ids;
    // Returned by GetNpId/GetOnlineId when user_id is not connected.
    static const OrbisNpId s_empty_np_id;

    // Score requests awaiting a reply, keyed by the submit packet id.
    struct PendingScoreRequest {
        std::shared_ptr<NpScore::ScoreRequestCtx> req;
        ShadNet::CommandType cmd;
        std::vector<std::string> requestedNpIds;
        NpScore::OrbisNpScorePlayerRankData* rankArray = nullptr;
        NpScore::OrbisNpScoreRankData* plainRankArray = nullptr;
        NpScore::OrbisNpScoreRankDataA* aRankArray = nullptr;
        NpScore::OrbisNpScorePlayerRankDataA* aPlayerRankArray = nullptr;
        NpScore::OrbisNpScoreComment* commentArray = nullptr;
        NpScore::OrbisNpScoreGameInfo* infoArray = nullptr;
        NpScore::OrbisNpScoreBoardInfo* boardInfo = nullptr;
        // GetGameData / GetGameDataByAccountId output buffers.
        void* dataOut = nullptr;
        u64 recvSize = 0;
        u64* totalSizeOut = nullptr;
        Libraries::Rtc::OrbisRtcTick* lastSortDate = nullptr;
        u32* totalRecord = nullptr;
        u64 arrayNum = 0;
    };
    mutable std::mutex m_mutex_pending_score;
    std::map<u64, PendingScoreRequest> m_pending_score;

    // Worker thread
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_worker_running{false};
    std::thread m_worker_thread;

    // State callbacks
    struct CbEntry {
        s32 handle;
        StateCallback cb;
        void* userdata;
    };
    mutable std::mutex m_mutex_cbs;
    std::vector<CbEntry> m_state_cbs;
    s32 m_next_handle{1};
};

} // namespace Libraries::Np