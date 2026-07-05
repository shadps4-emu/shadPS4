// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/net_util.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_signaling/np_signaling_helpers.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_stubs.h"

namespace Libraries::Np::NpSignaling::Helpers {

namespace {

constexpr s32 InferredNpAppType = 0;

bool g_signaling_heap_initialized = false;
void* g_signaling_heap_base = nullptr;
s64 g_signaling_heap_size = 0;
SignalingRuntimeHooks g_runtime_hooks{};
bool g_runtime_hooks_registered = false;
u64 g_echo_probe_word2 = 0;
u64 g_echo_probe_word3 = 0;
u64 g_echo_thread_handle = static_cast<u64>(-1);
s32 g_echo_thread_status = -1;

s32 GetInferredAppType(s32* app_type) {
    if (app_type == nullptr) {
        return ORBIS_NP_INT_ERROR_INVALID_ARGUMENT;
    }

    *app_type = InferredNpAppType;
    return ORBIS_OK;
}

s16 GetAppTypeStateGate() {
    return 0;
}

void SetAppTypeMarker(u16 marker) {
    LOG_DEBUG(Lib_NpSignaling, "marker={:#x}", marker);
}
} // namespace

void SetRuntimeHooks(const SignalingRuntimeHooks& hooks) {
    g_runtime_hooks = hooks;
    g_runtime_hooks_registered = true;
}

s32 CheckInitializeAppType(u32* is_app_type_4) {
    s32 app_type = -1;
    const s32 rc = GetInferredAppType(&app_type);
    if (rc < 0) {
        return rc;
    }

    if (is_app_type_4 != nullptr) {
        *is_app_type_4 = app_type == 4 ? 1u : 0u;
    }

    return ORBIS_OK;
}

s32 InitSignalingHeap(s64 pool_size) {
    LOG_DEBUG(Lib_NpSignaling, "pool_size={}", pool_size);

    if (pool_size == 0 || g_signaling_heap_initialized) {
        return ORBIS_NP_SIGNALING_INTERNAL_ERROR_ALLOCATOR;
    }

    const u64 aligned_size = Common::AlignUp(static_cast<u64>(pool_size), 0x4000);

    void* heap_base = nullptr;
    const s32 rc = Libraries::Kernel::sceKernelMapNamedFlexibleMemory(&heap_base, aligned_size, 3,
                                                                      0, "SceNpSignaling");
    if (rc < 0) {
        return rc;
    }

    g_signaling_heap_initialized = true;
    g_signaling_heap_base = heap_base;
    g_signaling_heap_size = static_cast<s64>(aligned_size);
    return ORBIS_OK;
}

void ShutdownSignalingHeap() {
    if (!g_signaling_heap_initialized) {
        return;
    }

    g_signaling_heap_initialized = false;

    if (g_signaling_heap_base != nullptr) {
        Libraries::Kernel::sceKernelMunmap(g_signaling_heap_base,
                                           static_cast<u64>(g_signaling_heap_size));
        g_signaling_heap_base = nullptr;
        g_signaling_heap_size = 0;
    }
}

s32 CheckAppType() {
    s32 app_type = -1;
    const s32 rc = GetInferredAppType(&app_type);
    if (rc < 0) {
        return rc;
    }

    if (app_type == 5) {
        const s16 gate = GetAppTypeStateGate();
        if (gate != 0) {
            return ORBIS_OK;
        }
    }

    SetAppTypeMarker(0x245a);
    return ORBIS_OK;
}

s32 StartMainRuntime(s32 thread_priority, s32 cpu_affinity_mask, s64 thread_stack_size) {
    LOG_DEBUG(Lib_NpSignaling,
              "thread_priority={} cpu_affinity_mask={} "
              "thread_stack_size={}",
              thread_priority, cpu_affinity_mask, thread_stack_size);

    if (!g_runtime_hooks_registered) {
        return ORBIS_NP_SIGNALING_INTERNAL_ERROR_NOT_INITIALIZED;
    }

    const s32 priority = thread_priority;
    const u64 affinity = static_cast<u64>(static_cast<u32>(cpu_affinity_mask));
    const u64 stack_size = static_cast<u64>(thread_stack_size);
    if (g_runtime_hooks.start_dispatch != nullptr) {
        g_runtime_hooks.start_dispatch(priority, affinity, stack_size);
    }
    if (g_runtime_hooks.start_receive != nullptr) {
        g_runtime_hooks.start_receive(priority, affinity, stack_size);
    }
    if (g_runtime_hooks.start_ping != nullptr) {
        g_runtime_hooks.start_ping(priority, affinity, stack_size);
    }

    return ORBIS_OK;
}

s32 StartEchoRuntime(s32 thread_priority, s32 cpu_affinity_mask) {
    LOG_DEBUG(Lib_NpSignaling, "thread_priority={} cpu_affinity_mask={}", thread_priority,
              cpu_affinity_mask);

    auto sock = Libraries::Net::sceNetSocket("SceNpSignalingIoctl", 2, 6, 0);
    if (sock < 0) {
        return sock;
    }

    const s32 ioctl_rc = Libraries::Net::sceNetIoctl();
    Libraries::Net::sceNetSocketClose(sock);

    g_echo_probe_word2 = 0;
    g_echo_probe_word3 = 0;
    g_echo_thread_handle = static_cast<u64>(-1);
    g_echo_thread_status = -1;
    return ioctl_rc;
}

void ShutdownRuntime() {
    if (!g_runtime_hooks_registered) {
        g_echo_probe_word2 = 0;
        g_echo_probe_word3 = 0;
        g_echo_thread_handle = static_cast<u64>(-1);
        g_echo_thread_status = -1;
        return;
    }

    if (g_runtime_hooks.stop_ping != nullptr) {
        g_runtime_hooks.stop_ping();
    }
    if (g_runtime_hooks.stop_receive != nullptr) {
        g_runtime_hooks.stop_receive();
    }
    if (g_runtime_hooks.stop_dispatch != nullptr) {
        g_runtime_hooks.stop_dispatch();
    }

    g_echo_probe_word2 = 0;
    g_echo_probe_word3 = 0;
    g_echo_thread_handle = static_cast<u64>(-1);
    g_echo_thread_status = -1;
}

} // namespace Libraries::Np::NpSignaling::Helpers

namespace Libraries::Np::NpSignaling {

using Libraries::Net::sceNetNtohs;

static Kernel::PthreadT g_dispatch_thread{};
static Kernel::PthreadT g_receive_thread{};
static bool g_receive_stop = false;
static Kernel::PthreadT g_ping_thread{};
static bool g_ping_stop = false;

static void HandleStunEcho(s32 ctx_id, const StunEcho& echo) {
    SignalingMutexGuard lock;
    const auto it = g_contexts.find(ctx_id);
    if (it == g_contexts.end() || !it->second.active) {
        return;
    }
    NpSignalingContext& ctx = it->second;
    ctx.ext_addr.store(echo.ext_ip);
    ctx.ext_port.store(echo.ext_port);

    LOG_DEBUG(Lib_NpSignaling, "STUN echo: ctxId={} ext_addr={:#x} ext_port={}", ctx_id,
              echo.ext_ip, sceNetNtohs(echo.ext_port));

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    netinfo->SetExternalIp(echo.ext_ip);

    ctx.stun_cv.notify_all();
}

static bool HasSignalingMagic(const u8* buf, size_t nbytes) {
    return nbytes >= 5 && std::memcmp(buf, kSignalingMagic, sizeof(kSignalingMagic)) == 0;
}

static void DrainControlPackets() {
    static constexpr u32 kBufSize = 256;
    u8 buf[kBufSize];
    for (;;) {
        u32 from_addr = 0;
        u16 from_port = 0;
        const int rc = Stubs::ControlRecvFrom(buf, kBufSize, &from_addr, &from_port);
        if (rc <= 0) {
            break;
        }
        const auto nbytes = static_cast<size_t>(rc);
        if (nbytes == sizeof(SignalingControl) && HasSignalingMagic(buf, nbytes) &&
            buf[4] == static_cast<u8>(SignalingPacketType::Control)) {
            SignalingControl ctrl{};
            std::memcpy(&ctrl, buf, sizeof(ctrl));
            HandleControlPacket(from_addr, from_port, ctrl);
        } else {
            LOG_WARNING(Lib_NpSignaling, "ReceiveThread: bad control packet (size={})", nbytes);
        }
    }
}

static void ReceiveThreadMain() {
    static constexpr u32 kBufSize = 256;
    u8 buf[kBufSize];

    while (!g_receive_stop) {
        DrainControlPackets();

        u32 from_addr = 0;
        u16 from_port = 0;

        const int rc = Stubs::SignalingRecvFrom(buf, kBufSize, &from_addr, &from_port);
        if (rc <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const auto nbytes = static_cast<size_t>(rc);

        if (nbytes != sizeof(StunEcho)) {
            LOG_DEBUG(
                Lib_NpSignaling,
                "ReceiveThread DATA from {:#x}:{} size={} b0-4={:02x},{:02x},{:02x},{:02x},{:02x}",
                from_addr, sceNetNtohs(from_port), nbytes, buf[0], buf[1], buf[2], buf[3],
                nbytes >= 5 ? buf[4] : 0);
        }

        if (nbytes == sizeof(StunEcho)) {
            s32 ctx_id = 0;
            {
                SignalingMutexGuard lock;
                for (const auto& [cid, ctx] : g_contexts) {
                    if (ctx.active) {
                        ctx_id = cid;
                        break;
                    }
                }
            }
            if (ctx_id == 0) {
                continue;
            }
            StunEcho echo{};
            std::memcpy(&echo, buf, sizeof(echo));
            HandleStunEcho(ctx_id, echo);
            continue;
        }

        if (!HasSignalingMagic(buf, nbytes)) {
            LOG_WARNING(Lib_NpSignaling, "ReceiveThread: dropping non-SHAD packet (size={})",
                        nbytes);
            continue;
        }
        const u8 type = buf[4];

        if (nbytes == sizeof(SignalingEchoPing) &&
            type == static_cast<u8>(SignalingPacketType::EchoPing)) {
            SignalingEchoPing ping{};
            std::memcpy(&ping, buf, sizeof(ping));
            SignalingEchoPong pong{};
            pong.conn_id = ping.conn_id;
            pong.orig_ts_us = ping.send_ts_us;
            Stubs::SignalingSendTo(&pong, sizeof(pong), from_addr, from_port);
            continue;
        }
        if (nbytes == sizeof(SignalingEchoPong) &&
            type == static_cast<u8>(SignalingPacketType::EchoPong)) {
            SignalingEchoPong pong{};
            std::memcpy(&pong, buf, sizeof(pong));
            const s64 now = NowUs();
            s32 rtt_us = static_cast<s32>(now - static_cast<s64>(pong.orig_ts_us));
            if (rtt_us < 0) {
                rtt_us = 0;
            }
            {
                SignalingMutexGuard lock;
                RecordRttSampleLocked(static_cast<s32>(pong.conn_id), rtt_us);
            }
            continue;
        }
        if (nbytes == sizeof(SignalingHandshake) &&
            type == static_cast<u8>(SignalingPacketType::Handshake)) {
            SignalingHandshake hs{};
            std::memcpy(&hs, buf, sizeof(hs));
            HandleHandshakePacket(from_addr, from_port, hs);
            continue;
        }

        LOG_WARNING(Lib_NpSignaling, "ReceiveThread: unexpected SHAD packet type={} size={}", type,
                    nbytes);
    }
}

static PS4_SYSV_ABI void* ReceiveThreadFunc(void*) {
    ReceiveThreadMain();
    return nullptr;
}

static void StartReceiveThread(s32 priority, u64 affinity_mask, u64 stack_size) {
    g_receive_stop = false;
    if (!g_receive_thread) {
        NpCommon::sceNpCreateThread(&g_receive_thread, ReceiveThreadFunc, nullptr, priority,
                                    stack_size, affinity_mask, "SceNpSignalingRecv");
    }
}

static void StopReceiveThread() {
    {
        SignalingMutexGuard lock;
        g_receive_stop = true;
    }
    if (g_receive_thread) {
        NpCommon::sceNpJoinThread(g_receive_thread, nullptr);
        g_receive_thread = {};
    }
}

static void PingThreadMain() {
    while (!g_ping_stop) {
        if (!Stubs::EnsureTransport()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kSigRetryMs));
            continue;
        }

        struct CtxSnapshot {
            s32 ctx_id;
            OrbisNpOnlineId online_id{};
            bool resolved;
        };
        std::vector<CtxSnapshot> contexts;
        {
            SignalingMutexGuard lock;
            for (const auto& [ctx_id, ctx] : g_contexts) {
                if (ctx.active) {
                    contexts.push_back({ctx_id, ctx.owner_online_id, ctx.ext_addr.load() != 0});
                }
            }
        }

        const u32 server_addr = Stubs::MmServerAddr();
        const u16 server_port = Stubs::MmServerUdpPort();

        for (const auto& cs : contexts) {
            if (server_addr == 0 || server_port == 0) {
                break;
            }
            StunPing ping{};
            ping.cmd = 0x01;
            std::memcpy(ping.online_id, cs.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
            ping.local_ip = Stubs::AdvertisedAddr();

            Stubs::SignalingSendTo(&ping, sizeof(ping), server_addr, server_port);
        }

        SendEchoPings();

        const bool any_unresolved = std::any_of(contexts.begin(), contexts.end(),
                                                [](const auto& cs) { return !cs.resolved; });
        const u32 sleep_ms = any_unresolved ? kSigRetryMs : kSigPingMs;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
}

static PS4_SYSV_ABI void* PingThreadFunc(void*) {
    PingThreadMain();
    return nullptr;
}

static void StartPingThread(s32 priority, u64 affinity_mask, u64 stack_size) {
    g_ping_stop = false;
    if (!g_ping_thread) {
        NpCommon::sceNpCreateThread(&g_ping_thread, PingThreadFunc, nullptr, priority, stack_size,
                                    affinity_mask, "SceNpSignalingPing");
    }
}

static void StopPingThread() {
    g_ping_stop = true;
    if (g_ping_thread) {
        NpCommon::sceNpJoinThread(g_ping_thread, nullptr);
        g_ping_thread = {};
    }
}

static void DispatchThreadMain() {
    for (;;) {
        ProcessPendingActivations();

        QueuedDispatch dispatch;
        {
            std::unique_lock<std::mutex> lock(g_dispatch_mutex);
            for (;;) {
                if (g_dispatch_stop) {
                    return;
                }
                if (g_dispatch_queue.empty()) {
                    g_dispatch_cv.wait_for(lock, std::chrono::seconds(1), [] {
                        return g_dispatch_stop || !g_dispatch_queue.empty();
                    });
                    break;
                }
                const auto it = g_dispatch_queue.begin();
                const auto now = std::chrono::steady_clock::now();
                if (it->first > now) {
                    g_dispatch_cv.wait_until(lock, it->first);
                    break;
                }
                dispatch = std::move(it->second);
                g_dispatch_queue.erase(it);
                break;
            }
        }

        if (dispatch.callback) {
            LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} event={}({}) delay={}ms", NowMs(),
                     dispatch.ctx_id, dispatch.conn_id, dispatch.event_type,
                     SignalingEventName(dispatch.event_type), dispatch.delay_ms);
            LOG_DEBUG(Lib_NpSignaling,
                      "INVOKE signaling_cb={} ctxId={} connId={} event={}({}) errorCode={} arg={}",
                      fmt::ptr(reinterpret_cast<void*>(dispatch.callback)), dispatch.ctx_id,
                      dispatch.conn_id, dispatch.event_type,
                      SignalingEventName(dispatch.event_type), dispatch.error_code,
                      fmt::ptr(dispatch.callback_arg));
            dispatch.callback(static_cast<u32>(dispatch.ctx_id), static_cast<u32>(dispatch.conn_id),
                              dispatch.event_type, dispatch.error_code, dispatch.callback_arg);
        }
    }
}

static PS4_SYSV_ABI void* DispatchThreadFunc(void*) {
    DispatchThreadMain();
    return nullptr;
}

static void StartDispatchThread(s32 priority, u64 affinity_mask, u64 stack_size) {
    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    g_dispatch_stop = false;
    if (!g_dispatch_thread) {
        NpCommon::sceNpCreateThread(&g_dispatch_thread, DispatchThreadFunc, nullptr, priority,
                                    stack_size, affinity_mask, "SceNpSignalingMain");
    }
}

static void StopDispatchThread() {
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        g_dispatch_stop = true;
        g_dispatch_queue.clear();
    }
    g_dispatch_cv.notify_all();
    if (g_dispatch_thread) {
        NpCommon::sceNpJoinThread(g_dispatch_thread, nullptr);
        g_dispatch_thread = {};
    }
}

void RegisterRuntimeHooks() {
    Helpers::SetRuntimeHooks({
        .start_dispatch = StartDispatchThread,
        .start_receive = StartReceiveThread,
        .start_ping = StartPingThread,
        .stop_ping = StopPingThread,
        .stop_receive = StopReceiveThread,
        .stop_dispatch = StopDispatchThread,
    });
}

} // namespace Libraries::Np::NpSignaling
