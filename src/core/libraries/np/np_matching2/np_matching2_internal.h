// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/types.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_cache.h"
#include "core/libraries/np/np_types.h"

namespace shadnet {
class CreateJoinRoomResponse;
class GetWorldInfoListReply;
class GetRoomDataExternalListReply;
class GetRoomMemberDataExternalListReply;
class GetUserInfoListReply;
class LeaveRoomReply;
class SearchRoomReply;
} // namespace shadnet

namespace Libraries::Np {
class NpHandler;
}

namespace Libraries::Np::NpMatching2 {

struct CallbackPayload {
    std::unique_ptr<OrbisNpMatching2RoomDataInternal> room_data;
    std::unique_ptr<OrbisNpMatching2CreateJoinRoomResponse> create_join_response;
    std::unique_ptr<OrbisNpMatching2CreateJoinRoomResponseA> create_join_response_a;
    std::unique_ptr<OrbisNpMatching2LeaveRoomResponse> leave_room_response;
    std::unique_ptr<OrbisNpMatching2SearchRoomResponse> search_room_response;
    std::unique_ptr<OrbisNpMatching2SearchRoomResponseA> search_room_response_a;
    std::unique_ptr<OrbisNpMatching2GetRoomDataExternalListResponse>
        room_data_external_list_response;
    std::unique_ptr<OrbisNpMatching2GetRoomDataExternalListResponseA>
        room_data_external_list_response_a;
    std::unique_ptr<OrbisNpMatching2GetRoomMemberDataExternalListResponse>
        room_member_data_external_list_response;
    std::unique_ptr<OrbisNpMatching2GetRoomMemberDataExternalListResponseA>
        room_member_data_external_list_response_a;
    std::unique_ptr<OrbisNpMatching2GetRoomMemberDataInternalResponse>
        room_member_data_internal_response;
    std::unique_ptr<OrbisNpMatching2GetRoomMemberDataInternalResponseA>
        room_member_data_internal_response_a;
    std::unique_ptr<OrbisNpMatching2GetUserInfoListResponse> user_info_list_response;
    std::unique_ptr<OrbisNpMatching2GetUserInfoListResponseA> user_info_list_response_a;
    std::unique_ptr<OrbisNpMatching2GetWorldInfoListResponse> world_info_response;
    std::unique_ptr<OrbisNpMatching2SignalingGetPingInfoResponse> ping_info_response;
    std::vector<OrbisNpMatching2World> world_list;
    std::vector<OrbisNpMatching2RoomMemberDataInternal> member_data;
    std::vector<OrbisNpMatching2RoomMemberDataInternalA> member_data_a;
    std::vector<OrbisNpMatching2RoomMemberDataExternal> member_data_external;
    std::vector<OrbisNpMatching2RoomMemberDataExternalA> member_data_external_a;
    std::vector<OrbisNpMatching2RoomGroup> room_groups;
    std::vector<OrbisNpMatching2RoomGroup*> room_group_ptrs;
    std::vector<OrbisNpMatching2RoomBinAttrInternal> room_bin_attrs;
    std::vector<OrbisNpMatching2RoomBinAttrInternal*> room_bin_attr_ptrs;
    std::vector<OrbisNpMatching2RoomMemberBinAttrInternal> member_bin_attrs;
    std::vector<OrbisNpMatching2RoomMemberBinAttrInternal*> member_bin_attr_ptrs;
    std::vector<OrbisNpMatching2RoomDataExternal> room_data_external;
    std::vector<OrbisNpMatching2RoomDataExternalA> room_data_external_a;
    std::vector<OrbisNpMatching2UserInfo> user_info;
    std::vector<OrbisNpMatching2UserInfoA> user_info_a;
    std::vector<OrbisNpMatching2BinAttr> user_bin_attrs;
    std::vector<std::vector<u8>> bin_buffers;
    std::vector<OrbisNpMatching2IntAttr> ext_int_attrs;
    std::vector<OrbisNpMatching2BinAttr> ext_bin_attrs;
    std::vector<OrbisNpMatching2RoomGroupInfo> ext_room_groups;
    std::vector<Libraries::Np::OrbisNpId> ext_owner_npids;
    void* request_data = nullptr;

    std::unique_ptr<OrbisNpMatching2RoomMessageInfo> room_message_info;
    std::unique_ptr<OrbisNpMatching2RoomMessageInfoA> room_message_info_a;
    std::unique_ptr<OrbisNpMatching2RoomMessageDestination> room_message_dst;
    std::unique_ptr<Libraries::Np::OrbisNpId> room_message_src_npid;
    std::vector<OrbisNpMatching2RoomMemberId> room_message_multicast_members;
    std::vector<u8> room_message_data;
    void* room_message_callback_data = nullptr;

    std::unique_ptr<OrbisNpMatching2RoomMemberUpdate> room_member_update;
    std::unique_ptr<OrbisNpMatching2RoomMemberUpdateA> room_member_update_a;
    std::unique_ptr<OrbisNpMatching2RoomMemberDataInternalUpdate> room_member_data_internal_update;
    std::unique_ptr<OrbisNpMatching2RoomMemberDataInternalUpdateA>
        room_member_data_internal_update_a;
    std::unique_ptr<OrbisNpMatching2RoomUpdate> room_update;
    std::unique_ptr<OrbisNpMatching2RoomDataInternalUpdate> room_data_internal_update;
    std::unique_ptr<OrbisNpMatching2FlagAttr> event_chg_flag_attr;
    std::unique_ptr<OrbisNpMatching2FlagAttr> event_prev_flag_attr;
    std::unique_ptr<OrbisNpMatching2TeamId> event_chg_team_id;
    std::unique_ptr<OrbisNpMatching2RoomPasswordSlotMask> event_chg_passwd_slot_mask;
    std::unique_ptr<OrbisNpMatching2RoomPasswordSlotMask> event_prev_passwd_slot_mask;
    std::unique_ptr<OrbisNpMatching2RoomMemberDataInternal> event_member;
    std::unique_ptr<OrbisNpMatching2RoomMemberDataInternalA> event_member_a;
    void* room_event_data = nullptr;

    CallbackPayload() = default;
    CallbackPayload(const CallbackPayload&) = delete;
    CallbackPayload& operator=(const CallbackPayload&) = delete;
    CallbackPayload(CallbackPayload&&) = delete;
    CallbackPayload& operator=(CallbackPayload&&) = delete;

    void Reset() {
        room_data.reset();
        create_join_response.reset();
        create_join_response_a.reset();
        leave_room_response.reset();
        search_room_response.reset();
        search_room_response_a.reset();
        room_data_external_list_response.reset();
        room_data_external_list_response_a.reset();
        room_member_data_external_list_response.reset();
        room_member_data_external_list_response_a.reset();
        room_member_data_internal_response.reset();
        room_member_data_internal_response_a.reset();
        user_info_list_response.reset();
        user_info_list_response_a.reset();
        world_info_response.reset();
        ping_info_response.reset();
        world_list.clear();
        member_data.clear();
        member_data_a.clear();
        member_data_external.clear();
        member_data_external_a.clear();
        room_groups.clear();
        room_group_ptrs.clear();
        room_bin_attrs.clear();
        room_bin_attr_ptrs.clear();
        member_bin_attrs.clear();
        member_bin_attr_ptrs.clear();
        room_data_external.clear();
        room_data_external_a.clear();
        user_info.clear();
        user_info_a.clear();
        user_bin_attrs.clear();
        bin_buffers.clear();
        ext_int_attrs.clear();
        ext_bin_attrs.clear();
        ext_room_groups.clear();
        ext_owner_npids.clear();
        request_data = nullptr;
        room_message_info.reset();
        room_message_info_a.reset();
        room_message_dst.reset();
        room_message_src_npid.reset();
        room_message_multicast_members.clear();
        room_message_data.clear();
        room_message_callback_data = nullptr;
        room_member_update.reset();
        room_member_update_a.reset();
        room_member_data_internal_update.reset();
        room_member_data_internal_update_a.reset();
        room_update.reset();
        room_data_internal_update.reset();
        event_chg_flag_attr.reset();
        event_prev_flag_attr.reset();
        event_chg_team_id.reset();
        event_chg_passwd_slot_mask.reset();
        event_prev_passwd_slot_mask.reset();
        event_member.reset();
        event_member_a.reset();
        room_event_data = nullptr;
    }
};

struct ContextObject : std::enable_shared_from_this<ContextObject> {
    OrbisNpMatching2ContextId ctx_id = 0;
    bool started = false;
    bool stop_pending = false;
    bool destroy_pending = false;
    bool a_variant = false;

    OrbisNpMatching2ServerId server_id = 1;
    OrbisNpServiceLabel service_label = 0;
    Libraries::Np::OrbisNpId owner_np_id{};
    Libraries::Np::OrbisNpOnlineId online_id{};

    OrbisNpMatching2WorldId world_id = 0;
    OrbisNpMatching2LobbyId lobby_id = 0;
    OrbisNpMatching2RoomId room_id = 0;
    OrbisNpMatching2RoomMemberId my_member_id = 0;
    bool is_room_owner = false;
    u32 max_slot = 5;
    OrbisNpMatching2Flags flag_attr = 0;

    OrbisNpMatching2ContextCallback context_callback = nullptr;
    void* context_callback_arg = nullptr;
    OrbisNpMatching2RequestCallback default_request_callback = nullptr;
    void* default_request_callback_arg = nullptr;
    OrbisNpMatching2RequestCallback per_request_callback = nullptr;
    void* per_request_callback_arg = nullptr;
    OrbisNpMatching2RoomEventCallback room_event_callback = nullptr;
    void* room_event_callback_arg = nullptr;
    OrbisNpMatching2RoomMessageCallback room_message_callback = nullptr;
    void* room_message_callback_arg = nullptr;
    OrbisNpMatching2LobbyEventCallback lobby_event_callback = nullptr;
    void* lobby_event_callback_arg = nullptr;
    OrbisNpMatching2LobbyMessageCallback lobby_message_callback = nullptr;
    void* lobby_message_callback_arg = nullptr;
    OrbisNpMatching2SignalingCallback signaling_callback = nullptr;
    void* signaling_callback_arg = nullptr;
};

struct RequestCallbackInfo {
    OrbisNpMatching2RequestCallback callback = nullptr;
    void* arg = nullptr;
};

class ContextManager {
public:
    static constexpr u32 kMaxContexts = 10;

    s32 CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                      OrbisNpMatching2ContextId* out_ctx_id, bool a_variant = false);

    std::shared_ptr<ContextObject> Get(OrbisNpMatching2ContextId ctx_id);
    bool Destroy(OrbisNpMatching2ContextId ctx_id);
    void CompleteStop(OrbisNpMatching2ContextId ctx_id,
                      const std::shared_ptr<ContextObject>& expected_context);

    s32 Start(OrbisNpMatching2ContextId ctx_id);
    s32 Stop(OrbisNpMatching2ContextId ctx_id);

    void ApplyContextCallback(OrbisNpMatching2ContextCallback callback, void* arg);

    void Reset();

private:
    friend class Libraries::Np::NpHandler;

    ContextManager() = default;
    std::shared_ptr<ContextObject> GetLocked(OrbisNpMatching2ContextId ctx_id);

    std::mutex m_mutex;
    std::array<std::shared_ptr<ContextObject>, kMaxContexts + 1> m_contexts{};
    OrbisNpMatching2ContextId m_next_id = 1;

    OrbisNpMatching2ContextCallback m_pending_context_callback = nullptr;
    void* m_pending_context_callback_arg = nullptr;
};

struct PendingEvent {
    enum Type {
        CONTEXT_CB,
        REQUEST_CB,
        SIGNALING_CB,
        ROOM_EVENT_CB,
        LOBBY_EVENT_CB,
        LOBBY_MESSAGE_CB,
        ROOM_MESSAGE_CB,
    };
    Type type = CONTEXT_CB;
    OrbisNpMatching2ContextId ctx_id = 0;
    std::shared_ptr<ContextObject> context_owner;
    std::chrono::steady_clock::time_point fire_at;

    OrbisNpMatching2Event ctx_event{};
    OrbisNpMatching2EventCause ctx_event_cause{};
    s32 error_code = 0;

    OrbisNpMatching2RequestId req_id = 0;
    OrbisNpMatching2Event req_event{};
    OrbisNpMatching2RequestCallback request_cb = nullptr;
    void* request_cb_arg = nullptr;
    void* request_data = nullptr;
    std::shared_ptr<CallbackPayload> payload_owner;

    OrbisNpMatching2RoomId room_id = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    OrbisNpMatching2Event sig_event{};
    u32 conn_id = 0;

    OrbisNpMatching2Event room_event{};
    void* room_event_data = nullptr;

    OrbisNpMatching2LobbyId lobby_id = 0;
    OrbisNpMatching2Event lobby_event{};
    void* lobby_event_data = nullptr;

    OrbisNpMatching2RoomMemberId src_member_id = 0;
    OrbisNpMatching2Event msg_event{};
    void* message_data = nullptr;
};

struct NpMatching2State {
    std::atomic<bool> initialized = false;

    OrbisNpMatching2RequestId next_request_id = 1;
    std::mutex mutex;

    Kernel::PthreadT dispatch_thread = nullptr;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::vector<PendingEvent> pending_events;
    std::atomic<bool> dispatch_running{false};
};

OrbisNpMatching2RequestId AllocRequestId();

bool IsInitialized();
void SetInitialized(bool initialized);
void StoreRequestCallback(const std::shared_ptr<ContextObject>& ctx,
                          const OrbisNpMatching2RequestOptParam* requestOpt);
RequestCallbackInfo ConsumeRequestCallback(const std::shared_ptr<ContextObject>& ctx);

void* BuildCreateJoinRoomPayload(ContextObject& ctx, CallbackPayload& payload,
                                 const shadnet::CreateJoinRoomResponse& resp);
void* BuildCreateJoinRoomPayloadA(ContextObject& ctx, CallbackPayload& payload,
                                  const shadnet::CreateJoinRoomResponse& resp);
void* BuildLeaveRoomPayload(ContextObject& ctx, CallbackPayload& payload,
                            const shadnet::LeaveRoomReply& resp);
void* BuildGetWorldInfoListPayload(ContextObject& ctx, CallbackPayload& payload,
                                   const shadnet::GetWorldInfoListReply& resp);
void* BuildSearchRoomPayload(ContextObject& ctx, CallbackPayload& payload,
                             const shadnet::SearchRoomReply& resp);
void* BuildSearchRoomPayloadA(ContextObject& ctx, CallbackPayload& payload,
                              const shadnet::SearchRoomReply& resp);
void* BuildGetRoomDataExternalListPayload(ContextObject& ctx, CallbackPayload& payload,
                                          const shadnet::GetRoomDataExternalListReply& resp);
void* BuildGetRoomDataExternalListPayloadA(ContextObject& ctx, CallbackPayload& payload,
                                           const shadnet::GetRoomDataExternalListReply& resp);
void* BuildGetRoomMemberDataExternalListPayload(
    ContextObject& ctx, CallbackPayload& payload,
    const shadnet::GetRoomMemberDataExternalListReply& resp);
void* BuildGetRoomMemberDataExternalListPayloadA(
    ContextObject& ctx, CallbackPayload& payload,
    const shadnet::GetRoomMemberDataExternalListReply& resp);
void* BuildGetUserInfoListPayload(ContextObject& ctx, CallbackPayload& payload,
                                  const shadnet::GetUserInfoListReply& resp);
void* BuildGetUserInfoListPayloadA(ContextObject& ctx, CallbackPayload& payload,
                                   const shadnet::GetUserInfoListReply& resp);
void* BuildGetRoomDataInternalPayload(ContextObject& ctx, CallbackPayload& payload,
                                      OrbisNpMatching2RoomId room_id);
void* BuildGetRoomMemberDataInternalPayload(
    ContextObject& ctx, CallbackPayload& payload,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest& request);
void* BuildRoomMessagePayload(CallbackPayload& p, bool a_variant, OrbisNpMatching2CastType castType,
                              const std::vector<OrbisNpMatching2RoomMemberId>& dstMembers,
                              const MemberCache* srcMember, const std::vector<u8>& msg);

void InitEventDispatcher();
void TermEventDispatcher();

void ScheduleEvent(PendingEvent ev);

PS4_SYSV_ABI void* EventDispatcherThreadMain(void* arg);

} // namespace Libraries::Np::NpMatching2
