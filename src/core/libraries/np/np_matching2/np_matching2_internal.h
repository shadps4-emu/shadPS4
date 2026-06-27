// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "common/types.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_types.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpMatching2 {

struct PeerInfo {
    u32 addr = 0;
    u16 port = 0;
    OrbisNpMatching2RoomMemberId member_id = 0;
    s32 conn_id = 0;
    s32 status = 0;
    std::string online_id;
};

struct ContextObject {
    OrbisNpMatching2ContextId ctx_id = 0;
    bool started = false;

    OrbisNpMatching2ServerId server_id = 1;
    OrbisNpServiceLabel service_label = 0;
    Libraries::Np::OrbisNpId owner_np_id{};
    std::string online_id;

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

    void Reset();

private:
    ContextManager() = default;
    ContextObject* GetLocked(OrbisNpMatching2ContextId ctx_id);

    std::mutex m_mutex;
    std::array<ContextObject, kMaxContexts + 1> m_contexts{};
    std::array<bool, kMaxContexts + 1> m_used{};
    OrbisNpMatching2ContextId m_next_id = 1;
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

void InitEventDispatcher();
void TermEventDispatcher();

void ScheduleEvent(PendingEvent ev);

PS4_SYSV_ABI void* EventDispatcherThreadMain(void* arg);

} // namespace Libraries::Np::NpMatching2
