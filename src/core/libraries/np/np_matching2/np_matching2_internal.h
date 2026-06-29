// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/types.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_types.h"
#include "core/libraries/np/np_types.h"

namespace shadnet {
class CreateJoinRoomResponse;
class GetWorldInfoListReply;
class SearchRoomReply;
} // namespace shadnet

namespace Libraries::Np::NpMatching2 {

struct PeerInfo {
    u32 addr = 0;
    u16 port = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    s32 conn_id = 0;
    s32 status = 0;
    Libraries::Np::OrbisNpOnlineId online_id{};
};

struct MemberBinCache {
    OrbisNpMatching2AttributeId id = 0;
    u64 update_date = 0;
    std::vector<u8> data;
};

struct MemberCache {
    Libraries::Np::OrbisNpId np_id{};
    u64 join_date = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    OrbisNpMatching2TeamId team_id = 0;
    OrbisNpMatching2RoomGroupId group_id = 0;
    OrbisNpMatching2NatType nat_type = 0;
    OrbisNpMatching2Flags flag_attr = 0;
    u32 addr = 0;
    u16 port = 0;
    std::map<OrbisNpMatching2AttributeId, MemberBinCache> bins;
};

struct RoomCache {
    u32 num_slots = 0;
    u64 mask_password = 0;
    std::map<OrbisNpMatching2RoomGroupId, OrbisNpMatching2RoomGroup> groups;
    std::map<OrbisNpMatching2RoomMemberId, MemberCache> members;
    bool owner = false;
};

struct CallbackPayload {
    std::unique_ptr<OrbisNpMatching2RoomDataInternal> room_data;
    std::unique_ptr<OrbisNpMatching2CreateJoinRoomResponse> create_join_response;
    std::unique_ptr<OrbisNpMatching2SearchRoomResponse> search_room_response;
    std::unique_ptr<OrbisNpMatching2GetWorldInfoListResponse> world_info_response;
    std::vector<OrbisNpMatching2World> world_list;
    std::vector<OrbisNpMatching2RoomMemberDataInternal> member_data;
    std::vector<OrbisNpMatching2RoomGroup> room_groups;
    std::vector<OrbisNpMatching2RoomBinAttrInternal> room_bin_attrs;
    std::vector<OrbisNpMatching2RoomMemberBinAttrInternal> member_bin_attrs;
    std::vector<OrbisNpMatching2RoomDataExternal> room_data_external;
    std::vector<std::vector<u8>> bin_buffers;
    std::deque<OrbisNpMatching2IntAttr> ext_int_attrs;
    std::deque<OrbisNpMatching2BinAttr> ext_bin_attrs;
    std::deque<OrbisNpMatching2RoomGroupInfo> ext_room_groups;
    std::deque<Libraries::Np::OrbisNpId> ext_owner_npids;
    void* request_data = nullptr;

    std::unique_ptr<OrbisNpMatching2RoomMemberUpdate> room_member_update;
    std::unique_ptr<OrbisNpMatching2RoomUpdate> room_update;
    std::unique_ptr<OrbisNpMatching2RoomMemberDataInternal> event_member;
    void* room_event_data = nullptr;

    void Reset() {
        *this = CallbackPayload{};
    }
};

struct ContextObject {
    OrbisNpMatching2ContextId ctx_id = 0;
    bool started = false;

    OrbisNpMatching2ServerId server_id = 1;
    OrbisNpServiceLabel service_label = 0;
    Libraries::Np::OrbisNpId owner_np_id{};
    Libraries::Np::OrbisNpOnlineId online_id{};

    std::string signaling_addr;
    u16 signaling_port = 0;

    u64 handler_registration_generation = 1;

    OrbisNpMatching2WorldId world_id = 0;
    OrbisNpMatching2LobbyId lobby_id = 0;
    OrbisNpMatching2RoomId room_id = 0;
    OrbisNpMatching2RoomMemberId my_member_id = 0;
    bool is_room_owner = false;
    u32 max_slot = 5;
    OrbisNpMatching2Flags flag_attr = 0;

    std::map<OrbisNpMatching2RoomMemberId, PeerInfo> peers;

    std::map<OrbisNpMatching2RoomId, RoomCache> room_cache;

    CallbackPayload request_payload;
    CallbackPayload room_event_payload;

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

class ContextManager {
public:
    static constexpr u32 kMaxContexts = 255;

    static ContextManager& Instance();

    s32 CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                      OrbisNpMatching2ContextId* out_ctx_id);

    bool Check(OrbisNpMatching2ContextId ctx_id);
    ContextObject* Get(OrbisNpMatching2ContextId ctx_id);
    bool Destroy(OrbisNpMatching2ContextId ctx_id);

    s32 Start(OrbisNpMatching2ContextId ctx_id);
    s32 Stop(OrbisNpMatching2ContextId ctx_id);

    void ApplyContextCallback(OrbisNpMatching2ContextCallback callback, void* arg);

    void Reset();

private:
    ContextManager() = default;
    ContextObject* GetLocked(OrbisNpMatching2ContextId ctx_id);

    std::mutex m_mutex;
    std::array<ContextObject, kMaxContexts + 1> m_contexts{};
    std::array<bool, kMaxContexts + 1> m_used{};
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
    std::chrono::steady_clock::time_point fire_at;

    OrbisNpMatching2Event ctx_event{};
    OrbisNpMatching2EventCause ctx_event_cause{};
    s32 error_code = 0;

    OrbisNpMatching2RequestId req_id = 0;
    OrbisNpMatching2Event req_event{};
    OrbisNpMatching2RequestCallback request_cb = nullptr;
    void* request_cb_arg = nullptr;
    void* request_data = nullptr;

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

extern NpMatching2State g_state;

OrbisNpMatching2RequestId AllocRequestId();

void* BuildCreateJoinRoomPayload(ContextObject& ctx, const shadnet::CreateJoinRoomResponse& resp);
void* BuildGetWorldInfoListPayload(ContextObject& ctx, const shadnet::GetWorldInfoListReply& resp);
void* BuildSearchRoomPayload(ContextObject& ctx, const shadnet::SearchRoomReply& resp);

void InitEventDispatcher();
void TermEventDispatcher();

void ScheduleEvent(PendingEvent ev);

PS4_SYSV_ABI void* EventDispatcherThreadMain(void* arg);

} // namespace Libraries::Np::NpMatching2
