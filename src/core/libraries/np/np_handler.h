// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_score/np_score_ctx.h"
#include "core/libraries/np/np_tus/np_tus.h"
#include "core/libraries/np/np_tus/np_tus_ctx.h"
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

    // Connect/disconnect a single user in response to a user service login/logout event.
    void OnUserLoggedIn(s32 user_id);
    void OnUserLoggedOut(s32 user_id);

    // True if this specific user is authenticated to the shadNet server.
    bool IsPsnSignedIn(s32 user_id) const;

    /// True if any user is currently signed in
    bool IsAnySignedIn() const;

    // Set the Appear-Offline preference for all signed-in users (and future logins). While
    // enabled, shadNet handles the user as offline for everyone else. Call from the UI/config.
    void SetAppearOffline(bool enable);
    bool IsAppearOffline() const {
        return m_appear_offline.load();
    }

    /// Full NP ID for this user, built once from shadnet_npid after login.
    OrbisNpId GetNpId(s32 user_id) const;

    /// The Online ID embedded in the NP ID (npid.handle).
    OrbisNpOnlineId GetOnlineId(s32 user_id) const;

    // Avatar URL returned by the server.
    std::string GetAvatarUrl(s32 user_id) const;

    // 64-bit account ID assigned by the server.
    OrbisNpAccountId GetAccountId(s32 user_id) const;

    // WebAPI bearer token
    std::string GetBearerToken(s32 user_id) const;

    // Sends a session invitation via the WebAPI (POST /v1/sessions/<id>/invitations). Each 'to'
    // entry is an online ID (npid) or a decimal account ID
    bool SendSessionInvitation(s32 user_id, const std::string& session_id,
                               const std::vector<std::string>& to, const std::string& message);

    // Raises ORBIS_SYSTEM_SERVICE_EVENT_SESSION_INVITATION (0x10000002) so titles that watch the
    // system-service event learn about the invite.
    void PostSessionInvitationEvent(const std::string& session_id, const std::string& invitation_id,
                                    const std::string& accepter_online_id);

    // A session invitation surfaced from a shadNet push, stashed until the user acts on it.
    struct PendingInvitation {
        std::string session_id;
        std::string invitation_id;
        std::string from_npid;   // sender (for display)
        std::string to_npid;     // local recipient / accepter
        int64_t valid_until = 0; // ms since epoch; 0 = never expires
    };
    // Pending invitations stashed for a local user (newest last), populated on arrival.
    std::vector<PendingInvitation> GetPendingInvitations(s32 user_id) const;
    // Consumes an invitation server-side (PUT usedFlag=true) and drops it from the stash. Returns
    // true if the invite was found and the consume succeeded.
    bool AcceptSessionInvitation(s32 user_id, const std::string& invitation_id);
    // Dismisses an invitation locally by dropping it from the stash. No server call (a declined
    // invite is left unconsumed and simply ages out server-side).
    void DeclineSessionInvitation(s32 user_id, const std::string& invitation_id);

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

    // ---- Friend list / requests / blocked (shadNet) ----
    struct FriendInfo {
        std::string npid;
        bool online = false;
    };
    struct FriendListSnapshot {
        std::vector<FriendInfo> friends;
        std::vector<std::string> requests_received;
        std::vector<std::string> requests_sent;
        std::vector<std::string> blocked;
    };

    // Local users that currently have an authenticated shadNet session.
    std::vector<s32> GetConnectedUsers() const;

    // user's friend state for UI display.
    FriendListSnapshot GetFriendList(s32 user_id) const;

    // Friend/block actions
    s32 SendFriendRequest(s32 user_id, const std::string& npid);
    s32 RemoveFriend(s32 user_id, const std::string& npid);
    s32 BlockUser(s32 user_id, const std::string& npid);
    s32 UnblockUser(s32 user_id, const std::string& npid);

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

    // Title User Storage (TUS)
    s32 TusGetMultiSlotVariable(s32 user_id, s32 service_label, const std::string& ownerNpId,
                                const std::string& virtualUser, const std::vector<s32>& slotIds,
                                NpTus::OrbisNpTusVariable* variablesOut, u64 arrayNum,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                NpTus::OrbisNpTusVariableA* variablesAOut = nullptr,
                                s64 ownerAccountId = 0,
                                NpTus::OrbisNpTusVariableForCrossSave* variablesCSOut = nullptr);
    s32 TusSetMultiSlotVariable(s32 user_id, s32 service_label, const std::string& ownerNpId,
                                const std::string& virtualUser, const std::vector<s32>& slotIds,
                                const std::vector<s64>& values,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx, s64 ownerAccountId = 0);

    // TUS blob data.
    s32 TusGetData(s32 user_id, s32 service_label, const std::string& ownerNpId,
                   const std::string& virtualUser, s64 ownerAccountId, s32 slotId,
                   NpTus::OrbisNpTusDataStatusA* statusAOut, u64 statusCap, void* dataOut,
                   u64 dataCap, u64 dataOffset, std::shared_ptr<NpTus::TusRequestCtx> ctx,
                   NpTus::OrbisNpTusDataStatus* statusOut = nullptr,
                   NpTus::OrbisNpTusDataStatusForCrossSave* statusCSOut = nullptr);
    s32 TusDeleteMultiSlotData(s32 user_id, s32 service_label, const std::string& ownerNpId,
                               const std::string& virtualUser, s64 ownerAccountId,
                               const std::vector<s32>& slotIds,
                               std::shared_ptr<NpTus::TusRequestCtx> ctx);
    s32 TusDeleteMultiSlotVariable(s32 user_id, s32 service_label, const std::string& ownerNpId,
                                   const std::string& virtualUser, s64 ownerAccountId,
                                   const std::vector<s32>& slotIds,
                                   std::shared_ptr<NpTus::TusRequestCtx> ctx);
    s32 TusGetMultiUserDataStatus(s32 user_id, s32 service_label, s32 slotId,
                                  const std::vector<std::string>& ownerNpIds,
                                  const std::vector<std::string>& virtualUsers,
                                  const std::vector<s64>& ownerAccountIds,
                                  NpTus::OrbisNpTusDataStatus* statusOut,
                                  NpTus::OrbisNpTusDataStatusA* statusAOut, u64 arrayNum,
                                  std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                  NpTus::OrbisNpTusDataStatusForCrossSave* statusCSOut = nullptr);
    s32 TusGetMultiUserVariable(s32 user_id, s32 service_label, s32 slotId,
                                const std::vector<std::string>& ownerNpIds,
                                const std::vector<std::string>& virtualUsers,
                                const std::vector<s64>& ownerAccountIds,
                                NpTus::OrbisNpTusVariable* variablesOut,
                                NpTus::OrbisNpTusVariableA* variablesAOut, u64 arrayNum,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                NpTus::OrbisNpTusVariableForCrossSave* variablesCSOut = nullptr);
    s32 TusTryAndSetVariable(s32 user_id, s32 service_label, const std::string& ownerNpId,
                             const std::string& virtualUser, s64 ownerAccountId, s32 slotId,
                             s32 opeType, s64 value, bool hasCompare, s64 compareValue,
                             bool hasAuthorCheck, s64 isLastChangedAuthor,
                             const std::string& isLastChangedAuthorNpId, bool hasDateCheck,
                             u64 isLastChangedDate, NpTus::OrbisNpTusVariable* variableOut,
                             NpTus::OrbisNpTusVariableA* variableAOut,
                             std::shared_ptr<NpTus::TusRequestCtx> ctx,
                             NpTus::OrbisNpTusVariableForCrossSave* variableCSOut = nullptr);
    s32 TusAddAndGetVariable(s32 user_id, s32 service_label, const std::string& ownerNpId,
                             const std::string& virtualUser, s64 ownerAccountId, s32 slotId,
                             s64 inVariable, bool hasAuthorCheck, s64 isLastChangedAuthor,
                             const std::string& isLastChangedAuthorNpId, bool hasDateCheck,
                             u64 isLastChangedDate, NpTus::OrbisNpTusVariable* variableOut,
                             NpTus::OrbisNpTusVariableA* variableAOut,
                             std::shared_ptr<NpTus::TusRequestCtx> ctx,
                             NpTus::OrbisNpTusVariableForCrossSave* variableCSOut = nullptr);
    s32 TusGetFriendsDataStatus(s32 user_id, s32 service_label, s32 slotId, bool includeSelf,
                                s32 sortType, u32 max, NpTus::OrbisNpTusDataStatus* statusOut,
                                NpTus::OrbisNpTusDataStatusA* statusAOut, u64 arrayNum,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 startOffset = 0,
                                u32* hitsOut = nullptr,
                                NpTus::OrbisNpTusDataStatusForCrossSave* statusCSOut = nullptr);
    s32 TusGetFriendsVariable(s32 user_id, s32 service_label, s32 slotId, bool includeSelf,
                              s32 sortType, u32 max, NpTus::OrbisNpTusVariable* variablesOut,
                              NpTus::OrbisNpTusVariableA* variablesAOut, u64 arrayNum,
                              std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 startOffset = 0,
                              u32* hitsOut = nullptr,
                              NpTus::OrbisNpTusVariableForCrossSave* variablesCSOut = nullptr);
    s32 TusGetMultiSlotDataStatus(s32 user_id, s32 service_label, const std::string& ownerNpId,
                                  const std::string& virtualUser, s64 ownerAccountId,
                                  const std::vector<s32>& slotIds,
                                  NpTus::OrbisNpTusDataStatus* statusOut,
                                  NpTus::OrbisNpTusDataStatusA* statusAOut, u64 arrayNum,
                                  std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                  NpTus::OrbisNpTusDataStatusForCrossSave* statusCSOut = nullptr);
    s32 TusSetData(s32 user_id, s32 service_label, const std::string& ownerNpId,
                   const std::string& virtualUser, s64 ownerAccountId, s32 slotId,
                   const std::vector<u8>& blob, const std::vector<u8>& info, bool hasAuthorCheck,
                   s64 isLastChangedAuthor, const std::string& isLastChangedAuthorNpId,
                   bool hasDateCheck, u64 isLastChangedDate,
                   std::shared_ptr<NpTus::TusRequestCtx> ctx);
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

    // Connect a single logged-in user by id (looks up credentials, parses server).
    bool ConnectUserById(s32 user_id);

    // Parse the configured shadNet server "host:port" (default port 31313).
    std::pair<std::string, u16> ParseServerAddress() const;

    // Start the health-monitor worker thread if not already running (idempotent).
    void StartWorker();

    // Disconnect and remove one user's client.
    void DisconnectUser(s32 user_id);
    // Fail every pending score/TUS request submitted by this user so pollers
    // and blocked WaitAsync callers wake up instead of hanging forever.
    void FailPendingRequests(s32 user_id, s32 error_code);

    void WorkerThread();
    // Transparent reconnect after a network drop
    void MarkForReconnect(s32 user_id);
    void TryReconnect();
    void FireStateCallback(s32 user_id, NpManager::OrbisNpState state);

    // Notification forwarders wired into each client
    void OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n);
    void OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n);
    void OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n);
    void OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n);
    void OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n);
    void OnLoginResult(s32 user_id, const ShadNet::LoginResult& res);

    // General async reply dispatch. Routes a reply to the owning subsystem by
    // command. Called from the per-user ShadNetClient on the reader thread.
    void OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body);

    void OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body);

    // Async reply dispatch for TUS commands.
    void OnTusReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                    const std::vector<u8>& body);

    // 12-byte NP Communication ID
    std::string GetNpCommId(s32 service_label) const;

    // Appear-Offline preference, applied to every client at login and on change.
    std::atomic<bool> m_appear_offline{false};

    // Per-user client map
    mutable std::mutex m_mutex_clients;
    std::map<s32, std::shared_ptr<ShadNet::ShadNetClient>> m_clients;
    // Per-user NP ID built once from shadnet_npid after login.
    std::map<s32, OrbisNpId> m_np_ids;

    // Score requests awaiting a reply, keyed by the submit packet id.
    struct PendingScoreRequest {
        std::shared_ptr<NpScore::ScoreRequestCtx> req;
        ShadNet::CommandType cmd;
        s32 user_id = -1; // submitting user, for flushing on disconnect
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

    // TUS requests awaiting a reply, keyed by the submit packet id.
    struct PendingTusRequest {
        std::shared_ptr<NpTus::TusRequestCtx> req;
        ShadNet::CommandType cmd;
        s32 user_id = -1; // submitting user, for flushing on disconnect
        // Only the fields relevant to `cmd` are set.
        NpTus::OrbisNpTusVariable* variableArray = nullptr;   // variable gets / add
        NpTus::OrbisNpTusVariableA* variableArrayA = nullptr; // account-variant variable gets
        NpTus::OrbisNpTusVariableForCrossSave* variableArrayCS = nullptr; // cross-save variant
        NpTus::OrbisNpTusDataStatus* statusArray = nullptr;   // data get / status families
        NpTus::OrbisNpTusDataStatusA* statusArrayA = nullptr; // account-variant data status
        NpTus::OrbisNpTusDataStatusForCrossSave* statusArrayCS = nullptr; // cross-save data status
        void* dataOut = nullptr;                                          // GetData payload
        u64 dataCap = 0;                                                  // GetData buffer capacity
        u64 dataOffset = 0;      // GetData installment offset into the blob
        u64 statusCap = 0;       // GetData dataStatus buffer size (0 = struct size)
        u32* totalOut = nullptr; // friends total
        u64 arrayNum = 0;        // expected entry count
    };
    mutable std::mutex m_mutex_pending_tus;
    std::map<u64, PendingTusRequest> m_pending_tus;

    // Worker thread
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_worker_running{false};
    std::thread m_worker_thread;

    // Users dropped by a network error and awaiting transparent reconnect
    struct ReconnectState {
        std::chrono::steady_clock::time_point next_attempt{};
        std::chrono::milliseconds backoff{0};
    };
    std::unordered_map<s32, ReconnectState> m_reconnect;

    mutable std::mutex m_mutex_pending_invites;
    std::unordered_map<s32, std::vector<PendingInvitation>> m_pending_invites;

    // State callbacks
    struct CbEntry {
        s32 handle;
        StateCallback cb;
        void* userdata;
    };
    mutable std::mutex m_mutex_cbs;
    std::vector<CbEntry> m_state_cbs;
    s32 m_next_handle{1};

    // Friend state per user, seeded from LoginReply and updated by notifications/actions.
    mutable std::mutex m_mutex_friend_state;
    std::map<s32, FriendListSnapshot> m_friend_state;
};

} // namespace Libraries::Np