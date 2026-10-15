// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <string>
#include <string_view>
#include <fmt/format.h>
#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/libs.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_util.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_signaling/np_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_helpers.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"
#include "core/libraries/np/signaling_handler.h"
#include "core/memory.h"

namespace Libraries::Np::NpSignaling::Helpers {

namespace {

constexpr s32 InferredNpAppType = 0;
constexpr uintptr_t kHostPageSize = 0x1000;
constexpr size_t kDumpRowSize = 32;

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

std::string HexDumpRow(const u8* bytes, size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 3);
    for (size_t i = 0; i < size; ++i) {
        if (i != 0) {
            result.push_back(' ');
        }
        result.push_back(kHex[bytes[i] >> 4]);
        result.push_back(kHex[bytes[i] & 0xf]);
    }
    return result;
}

std::string AsciiDumpRow(const u8* bytes, size_t size) {
    std::string result;
    result.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(std::isprint(bytes[i]) ? static_cast<char>(bytes[i]) : '.');
    }
    return result;
}

void LogMemoryWindow(std::string_view label, const void* focus, size_t focus_size, size_t before,
                     size_t after) {
    if (!focus) {
        LOG_INFO(Lib_NpSignaling, "Invalid activation dump {}: unavailable (null focus)", label);
        return;
    }

    const uintptr_t focus_addr = reinterpret_cast<uintptr_t>(focus);
    const uintptr_t page_start = focus_addr & ~(kHostPageSize - 1);
    const uintptr_t page_end = page_start + kHostPageSize;
    const uintptr_t start = std::max(page_start, focus_addr - std::min(before, focus_addr));
    const uintptr_t requested_end = focus_addr + focus_size + after;
    const uintptr_t end = std::min(page_end, requested_end);
    const size_t size = end > start ? end - start : 0;

    auto* memory = Core::Memory::Instance();
    if (size == 0 || !memory || !memory->IsValidMapping(start, size)) {
        LOG_INFO(Lib_NpSignaling,
                 "Invalid activation dump {}: unavailable start={:#x} size={:#x} focus={:#x}",
                 label, start, size, focus_addr);
        return;
    }

    LOG_INFO(Lib_NpSignaling,
             "Invalid activation dump {}: start={:#x} size={:#x} focus={:#x} focus_offset={:#x} "
             "focus_size={:#x}",
             label, start, size, focus_addr, focus_addr - start, focus_size);
    const auto* bytes = reinterpret_cast<const u8*>(start);
    for (size_t offset = 0; offset < size; offset += kDumpRowSize) {
        const size_t row_size = std::min(kDumpRowSize, size - offset);
        LOG_INFO(Lib_NpSignaling, "Invalid activation dump {} {:#x} +{:#05x}: {} |{}|", label,
                 start + offset, offset, HexDumpRow(bytes + offset, row_size),
                 AsciiDumpRow(bytes + offset, row_size));
    }
}

} // namespace

void LogInvalidActivationContext(s64 call_time, const void* peer_npid, const void* out_conn_id,
                                 const void* caller, const void* frame) {
    LOG_INFO(Lib_NpSignaling,
             "Invalid activation context: t={} caller={:p} frame={:p} peerNpId={:p} "
             "outConnId={:p} peer_to_out_delta={}",
             call_time, caller, frame, peer_npid, out_conn_id,
             reinterpret_cast<intptr_t>(out_conn_id) - reinterpret_cast<intptr_t>(peer_npid));

    // Keep each read within the focus address's host page so diagnostics cannot cross into an
    // unmapped guard page. The peer window is intentionally large enough to include adjacent room
    // member/scratch records and, for the observed title, the nearby output connection ID.
    LogMemoryWindow("peer", peer_npid, sizeof(OrbisNpId), 0x100, 0x200);
    LogMemoryWindow("stack", frame, sizeof(uintptr_t) * 2, 0x100, 0x100);
    LogMemoryWindow("caller_code", caller, 1, 0x40, 0x40);
}

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
    return ORBIS_OK;
}

s32 StartEchoRuntime(s32 thread_priority, s32 cpu_affinity_mask) {
    LOG_DEBUG(Lib_NpSignaling, "thread_priority={} cpu_affinity_mask={}", thread_priority,
              cpu_affinity_mask);

    // A DGRAM_P2P socket, which also starts the P2P transport.
    auto sock =
        Libraries::Net::sceNetSocket("SceNpSignalingIoctl", Libraries::Net::ORBIS_NET_AF_INET,
                                     Libraries::Net::ORBIS_NET_SOCK_DGRAM_P2P, 0);
    if (sock < 0) {
        // Only an emulator-side problem (the P2P UDP port is taken): signaling itself still
        // works without the echo probe, so carry on as the console would.
        LOG_WARNING(Lib_NpSignaling, "P2P socket for the echo probe failed: {:#x}",
                    static_cast<u32>(sock));
        return ORBIS_OK;
    }

    std::array<u8, 36> ioctl_data{};
    const s32 ioctl_rc = Libraries::Net::sceNetIoctl(sock, 0xc02450ca, ioctl_data.data());
    Libraries::Net::sceNetSocketClose(sock);

    g_echo_probe_word2 = 0;
    g_echo_probe_word3 = 0;
    if (ioctl_rc >= 0) {
        std::memcpy(&g_echo_probe_word2, ioctl_data.data() + 16, sizeof(g_echo_probe_word2));
        std::memcpy(&g_echo_probe_word3, ioctl_data.data() + 24, sizeof(g_echo_probe_word3));
    }
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

static void DispatchThreadMain() {
    auto& state = NpHandler::GetInstance().GetSignalingState();
    for (;;) {
        QueuedDispatch dispatch;
        {
            std::unique_lock<std::mutex> lock(state.dispatch_mutex);
            for (;;) {
                if (state.dispatch_stop) {
                    return;
                }
                if (state.dispatch_queue.empty()) {
                    state.dispatch_cv.wait_for(lock, std::chrono::seconds(1), [&state] {
                        return state.dispatch_stop || !state.dispatch_queue.empty();
                    });
                    break;
                }
                const auto it = state.dispatch_queue.begin();
                const auto now = std::chrono::steady_clock::now();
                if (it->first > now) {
                    state.dispatch_cv.wait_until(lock, it->first);
                    break;
                }
                dispatch = std::move(it->second);
                state.dispatch_queue.erase(it);
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
    auto& state = NpHandler::GetInstance().GetSignalingState();
    std::lock_guard<std::mutex> lock(state.dispatch_mutex);
    state.dispatch_stop = false;
    if (!state.dispatch_thread) {
        NpCommon::sceNpCreateThread(&state.dispatch_thread, DispatchThreadFunc, nullptr, priority,
                                    stack_size, affinity_mask, "SceNpSignalingMain");
    }
}

static void StopDispatchThread() {
    auto& state = NpHandler::GetInstance().GetSignalingState();
    {
        std::lock_guard<std::mutex> lock(state.dispatch_mutex);
        state.dispatch_stop = true;
        state.dispatch_queue.clear();
    }
    state.dispatch_cv.notify_all();
    if (state.dispatch_thread) {
        NpCommon::sceNpJoinThread(state.dispatch_thread, nullptr);
        state.dispatch_thread = {};
    }
}

void RegisterRuntimeHooks() {
    Helpers::SetRuntimeHooks({
        .start_dispatch = StartDispatchThread,
        .stop_dispatch = StopDispatchThread,
    });
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0UvTFeomAUM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnection);
    LIB_FUNCTION("ZPLavCKqAB0", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnectionA);
    LIB_FUNCTION("X1G4kkN2R-8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCancelPeerNetInfo);
    LIB_FUNCTION("5yYjEdd4t8Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContext);
    LIB_FUNCTION("dDLNFdY8dws", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContextA);
    LIB_FUNCTION("6UEembipgrM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeactivateConnection);
    LIB_FUNCTION("hx+LIg-1koI", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeleteContext);
    LIB_FUNCTION("GQ0hqmzj0F4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromNpId);
    LIB_FUNCTION("CkPxQjSm018", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddress);
    LIB_FUNCTION("B7cT9aVby7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddressA);
    LIB_FUNCTION("AN3h0EBSX7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfo);
    LIB_FUNCTION("rcylknsUDwg", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfoA);
    LIB_FUNCTION("C6ZNCDTj00Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatistics);
    LIB_FUNCTION("bD-JizUb3JM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatus);
    LIB_FUNCTION("npU5V56id34", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetContextOption);
    LIB_FUNCTION("U8AQMlOFBc8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetLocalNetInfo);
    LIB_FUNCTION("tOpqyDyMje4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetMemoryInfo);
    LIB_FUNCTION("zFgFHId7vAE", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfo);
    LIB_FUNCTION("Shr7bZq8QHY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoA);
    LIB_FUNCTION("2HajCEGgG4s", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoResult);
    LIB_FUNCTION("3KOuC4RmZZU", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingInitialize);
    LIB_FUNCTION("IHRDvZodPYY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingSetContextOption);
    LIB_FUNCTION("NPhw0UXaNrk", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminate);
    LIB_FUNCTION("b4qaXPzMJxo", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminateConnection);
}

} // namespace Libraries::Np::NpSignaling
