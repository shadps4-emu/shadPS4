// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/logging/log.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"

namespace Libraries::Np::NpMatching2 {

NpMatching2State g_state;

ContextManager& ContextManager::Instance() {
    static ContextManager instance;
    return instance;
}

ContextObject* ContextManager::GetLocked(OrbisNpMatching2ContextId ctx_id) {
    if (ctx_id == 0 || ctx_id > kMaxContexts || !m_used[ctx_id]) {
        return nullptr;
    }
    return &m_contexts[ctx_id];
}

s32 ContextManager::CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                                  OrbisNpMatching2ContextId* out_ctx_id) {
    if (!out_ctx_id) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard lock(m_mutex);

    OrbisNpMatching2ContextId id = 0;
    for (u32 i = 0; i < kMaxContexts; ++i) {
        const OrbisNpMatching2ContextId candidate =
            static_cast<OrbisNpMatching2ContextId>(((m_next_id - 1 + i) % kMaxContexts) + 1);
        if (!m_used[candidate]) {
            id = candidate;
            break;
        }
    }
    if (id == 0) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_MAX;
    }
    m_next_id = static_cast<OrbisNpMatching2ContextId>((id % kMaxContexts) + 1);

    ContextObject& ctx = m_contexts[id];
    ctx = ContextObject{};
    ctx.ctx_id = id;
    ctx.service_label = service_label;
    if (owner_np_id) {
        ctx.owner_np_id = *owner_np_id;
        ctx.online_id =
            std::string(owner_np_id->handle.data,
                        strnlen(owner_np_id->handle.data, sizeof(owner_np_id->handle.data)));
    }
    if (ctx.online_id.empty()) {
        ctx.online_id = "shadPS4_player";
    }
    m_used[id] = true;

    *out_ctx_id = id;
    LOG_INFO(Lib_NpMatching2, "context created: id={} online_id={} serviceLabel={:#x}", id,
             ctx.online_id, service_label);
    return ORBIS_OK;
}

bool ContextManager::Check(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id) != nullptr;
}

ContextObject* ContextManager::Get(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id);
}

bool ContextManager::Destroy(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    if (!GetLocked(ctx_id)) {
        return false;
    }
    m_contexts[ctx_id] = ContextObject{};
    m_used[ctx_id] = false;
    LOG_INFO(Lib_NpMatching2, "context destroyed: id={}", ctx_id);
    return true;
}

s32 ContextManager::Start(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    ContextObject* ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }
    ctx->started = true;
    LOG_INFO(Lib_NpMatching2, "context started: id={}", ctx_id);
    return ORBIS_OK;
}

s32 ContextManager::Stop(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    ContextObject* ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (!ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
    }
    ctx->started = false;
    LOG_INFO(Lib_NpMatching2, "context stopped: id={}", ctx_id);
    return ORBIS_OK;
}

void ContextManager::Reset() {
    std::lock_guard lock(m_mutex);
    m_contexts = {};
    m_used = {};
    m_next_id = 1;
}

OrbisNpMatching2RequestId AllocRequestId() {
    std::lock_guard lock(g_state.mutex);
    const OrbisNpMatching2RequestId id = g_state.next_request_id++;
    if (g_state.next_request_id == 0) {
        g_state.next_request_id = 1;
    }
    return id;
}

namespace {

void FireEvent(const PendingEvent& ev) {
    ContextObject* ctx = ContextManager::Instance().Get(ev.ctx_id);
    if (!ctx) {
        return;
    }
    switch (ev.type) {
    case PendingEvent::CONTEXT_CB:
        if (ctx->context_callback) {
            ctx->context_callback(ev.ctx_id, ev.ctx_event, ev.ctx_event_cause, ev.error_code,
                                  ctx->context_callback_arg);
        }
        break;
    case PendingEvent::REQUEST_CB:
        if (ev.request_cb) {
            ev.request_cb(ev.ctx_id, ev.req_id, ev.req_event, ev.error_code, ev.request_data,
                          ev.request_cb_arg);
        }
        break;
    case PendingEvent::SIGNALING_CB:
        if (ctx->signaling_callback) {
            ctx->signaling_callback(ev.ctx_id, ev.room_id, ev.member_id, ev.sig_event,
                                    ev.error_code, ctx->signaling_callback_arg);
        }
        break;
    case PendingEvent::ROOM_EVENT_CB:
        if (ctx->room_event_callback) {
            ctx->room_event_callback(ev.ctx_id, ev.room_id, ev.room_event, ev.room_event_data,
                                     ctx->room_event_callback_arg);
        }
        break;
    case PendingEvent::LOBBY_EVENT_CB:
        if (ctx->lobby_event_callback) {
            ctx->lobby_event_callback(ev.ctx_id, ev.lobby_id, ev.lobby_event, ev.lobby_event_data,
                                      ctx->lobby_event_callback_arg);
        }
        break;
    case PendingEvent::LOBBY_MESSAGE_CB:
        if (ctx->lobby_message_callback) {
            ctx->lobby_message_callback(ev.ctx_id, ev.lobby_id, ev.src_member_id, ev.msg_event,
                                        ev.message_data, ctx->lobby_message_callback_arg);
        }
        break;
    case PendingEvent::ROOM_MESSAGE_CB:
        if (ctx->room_message_callback) {
            ctx->room_message_callback(ev.ctx_id, ev.room_id, ev.src_member_id, ev.msg_event,
                                       ev.message_data, ctx->room_message_callback_arg);
        }
        break;
    }
}

} // namespace

PS4_SYSV_ABI void* EventDispatcherThreadMain(void* /*arg*/) {
    std::unique_lock lock(g_state.queue_mutex);
    while (g_state.dispatch_running.load()) {
        const auto now = std::chrono::steady_clock::now();

        std::vector<PendingEvent> ready;
        auto nearest = std::chrono::steady_clock::time_point::max();
        for (auto it = g_state.pending_events.begin(); it != g_state.pending_events.end();) {
            if (it->fire_at <= now) {
                ready.push_back(std::move(*it));
                it = g_state.pending_events.erase(it);
            } else {
                nearest = std::min(nearest, it->fire_at);
                ++it;
            }
        }

        if (!ready.empty()) {
            lock.unlock();
            for (const PendingEvent& ev : ready) {
                FireEvent(ev);
            }
            lock.lock();
            continue;
        }

        if (nearest == std::chrono::steady_clock::time_point::max()) {
            g_state.queue_cv.wait(lock);
        } else {
            g_state.queue_cv.wait_until(lock, nearest);
        }
    }
    return nullptr;
}

void InitEventDispatcher() {
    if (g_state.dispatch_running.exchange(true)) {
        return;
    }
    const s32 rc = NpCommon::sceNpCreateThread(
        &g_state.dispatch_thread, EventDispatcherThreadMain, nullptr,
        Libraries::Kernel::ORBIS_KERNEL_PRIO_FIFO_DEFAULT,
        ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT, 0, "SceNpMatching2Ex");
    if (rc < 0) {
        LOG_ERROR(Lib_NpMatching2, "failed to create event dispatcher thread: {:#x}", rc);
        g_state.dispatch_running.store(false);
        g_state.dispatch_thread = nullptr;
        return;
    }
    LOG_INFO(Lib_NpMatching2, "event dispatcher thread created");
}

void TermEventDispatcher() {
    if (!g_state.dispatch_running.exchange(false)) {
        return;
    }
    g_state.queue_cv.notify_all();
    if (g_state.dispatch_thread) {
        NpCommon::sceNpJoinThread(g_state.dispatch_thread, nullptr);
        g_state.dispatch_thread = nullptr;
    }
    std::lock_guard lock(g_state.queue_mutex);
    g_state.pending_events.clear();
}

void ScheduleEvent(PendingEvent ev) {
    {
        std::lock_guard lock(g_state.queue_mutex);
        g_state.pending_events.push_back(std::move(ev));
    }
    g_state.queue_cv.notify_all();
}

} // namespace Libraries::Np::NpMatching2
