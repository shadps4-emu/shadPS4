// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <fmt/format.h>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "composite.h"
#include "composite_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/libs.h"
#include "video_core/amdgpu/pm4_cmds.h"

namespace Libraries::Composite {

namespace {

constexpr u32 MaxRegionSize = 0x20000000;
constexpr u8 MaxProcessOrder = 2;
constexpr u8 MaxCanvasIndex = 7;
constexpr u8 MaxPostEventIndex = 0xF;
constexpr u8 MaxCursorIndex = 1;
constexpr s32 MaxMorpheusState = 2;
constexpr u32 IndexCount = 16;
constexpr u64 RegionAlignment = 0x4000;
constexpr u32 RingCapacity = 0xFF;
constexpr u16 ClientId = 0;
constexpr u16 RenderTargetWidth = 1920;
constexpr u16 RenderTargetHeight = 1080;
constexpr u64 DummyAddress = 0xFF2000000;
constexpr u64 DummySize = 0x400;
constexpr std::chrono::seconds PostEventTimeout{10};
constexpr std::chrono::microseconds FencePollInterval{200};
constexpr u64 SyncRegionSize = 0x4000;
constexpr u64 SyncPacketOffset = 0x100;
constexpr u32 SyncPacketSize = 32;
constexpr u32 SyncPacketCount = (SyncRegionSize - SyncPacketOffset) / SyncPacketSize;

struct Region {
    void* addr{};
    u64 size{};
};

struct PendingDcb {
    const u32* addr;
    u32 size;
};

struct State {
    std::mutex mutex;
    std::condition_variable post_event_cv;
    bool connected{};
    Region system;
    Region video;
    Region sync;
    std::vector<PendingDcb> pending_dcbs;
    std::vector<u8> pending_post_events;
    std::array<u32, MaxPostEventIndex + 1> post_serials{};
    u32 last_serial{};
    u32 post_events{};
    bool event_pending{};
    bool end_of_frame_pending{};
};

struct CommandLock {
    std::mutex mutex;
    std::condition_variable cv;
    std::thread::id owner;
    u32 depth{};
};

State g_state;
CommandLock g_command_lock;
std::mutex g_flush_mutex;
std::mutex g_index_mutex;
u32 g_index_bitmap{};

bool MapRegion(Region& region, u64 size, const char* name) {
    region = {};
    if (size == 0) {
        return true;
    }
    const u64 aligned_size = Common::AlignUp(size, RegionAlignment);
    void* addr = nullptr;
    const s32 rc =
        Libraries::Kernel::sceKernelMapNamedFlexibleMemory(&addr, aligned_size, 3, 0, name);
    if (rc != ORBIS_OK) {
        LOG_ERROR(Lib_Composite, "Failed to map {} ({:#x} bytes), rc = {:#x}", name, aligned_size,
                  static_cast<u32>(rc));
        return false;
    }
    region = {addr, aligned_size};
    return true;
}

void UnmapRegion(Region& region) {
    if (region.addr != nullptr) {
        Libraries::Kernel::sceKernelReleaseFlexibleMemory(region.addr, region.size);
    }
    region = {};
}

u32 ReadFence(const Region& sync) {
    return std::atomic_ref<u32>{*static_cast<u32*>(sync.addr)}.load(std::memory_order_acquire);
}

bool FenceReached(const Region& sync, u32 serial) {
    return serial == 0 || sync.addr == nullptr || static_cast<s32>(ReadFence(sync) - serial) >= 0;
}

u32 BuildFencePacket(const Region& sync, u32 serial) {
    const u32 slot = serial % SyncPacketCount;
    const u64 fence_addr = reinterpret_cast<u64>(sync.addr);
    auto* packet = reinterpret_cast<AmdGpu::PM4CmdEventWriteEop*>(
        static_cast<u8*>(sync.addr) + SyncPacketOffset + slot * SyncPacketSize);
    packet->header = AmdGpu::PM4Type3Header{AmdGpu::PM4ItOpcode::EventWriteEop, 4};
    packet->event_control = static_cast<u32>(AmdGpu::EventType::BottomOfPipeTs) | (5u << 8);
    packet->address_lo = static_cast<u32>(fence_addr);
    packet->data_control = static_cast<u32>(fence_addr >> 32) & 0xFFFFu;
    packet->data_control |= static_cast<u32>(AmdGpu::DataSelect::Data32Low) << 29;
    packet->data_lo = serial;
    packet->data_hi = 0;
    return slot;
}

bool IsConnected() {
    std::scoped_lock lk{g_state.mutex};
    return g_state.connected;
}

s32 CheckConnected() {
    return IsConnected() ? ORBIS_OK : ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
}

bool IsInsideVideoRegion(const void* addr, u64 size) {
    std::scoped_lock lk{g_state.mutex};
    const u64 begin = reinterpret_cast<u64>(g_state.video.addr);
    const u64 value = reinterpret_cast<u64>(addr);
    return value >= begin && value + size <= begin + g_state.video.size;
}

} // namespace

s32 PS4_SYSV_ABI sceCompositorInitWithProcessOrder(u32 system_size, u32 video_shared_size,
                                                   u32 video_private_size, u32 process_order) {
    LOG_INFO(Lib_Composite,
             "system_size = {:#x}, video_shared_size = {:#x}, video_private_size = {:#x}, "
             "process_order = {}",
             system_size, video_shared_size, video_private_size, process_order);
    if (static_cast<u8>(process_order) > MaxProcessOrder) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    if (system_size > MaxRegionSize || video_shared_size > MaxRegionSize ||
        video_private_size > MaxRegionSize) {
        return ORBIS_COMPOSITE_ERROR_INVALID_MEMORY;
    }

    std::scoped_lock lk{g_state.mutex};
    if (g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_ALREADY_INITIALIZED;
    }
    if (!MapRegion(g_state.system, system_size, "SceCompositorSystemShared")) {
        return ORBIS_COMPOSITE_ERROR_INVALID_MEMORY;
    }
    const u64 video_size = static_cast<u64>(video_shared_size) + video_private_size;
    if (!MapRegion(g_state.video, video_size, "SceCompositorVideoShared")) {
        UnmapRegion(g_state.system);
        return ORBIS_COMPOSITE_ERROR_INVALID_MEMORY;
    }
    if (!MapRegion(g_state.sync, SyncRegionSize, "SceCompositorSync")) {
        LOG_WARNING(Lib_Composite, "Post events will not wait for the GPU");
    }
    g_state.pending_dcbs.clear();
    g_state.pending_post_events.clear();
    g_state.post_serials.fill(0);
    g_state.last_serial = 0;
    g_state.post_events = 0;
    g_state.event_pending = false;
    g_state.end_of_frame_pending = false;
    g_state.connected = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorInit(u32 system_size, u32 video_shared_size, u32 video_private_size) {
    return sceCompositorInitWithProcessOrder(system_size, video_shared_size, video_private_size, 0);
}

s32 PS4_SYSV_ABI sceCompositorQuit() {
    LOG_INFO(Lib_Composite, "called");
    std::scoped_lock flush_lock{g_flush_mutex};
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_OK;
    }
    const auto deadline = std::chrono::steady_clock::now() + PostEventTimeout;
    while (!FenceReached(g_state.sync, g_state.last_serial) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(FencePollInterval);
    }
    UnmapRegion(g_state.system);
    UnmapRegion(g_state.video);
    UnmapRegion(g_state.sync);
    g_state.pending_dcbs.clear();
    g_state.pending_post_events.clear();
    g_state.connected = false;
    g_state.post_events = 0;
    g_state.event_pending = false;
    g_state.end_of_frame_pending = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorAllocateIndex() {
    std::scoped_lock lk{g_index_mutex};
    for (u32 i = 0; i < IndexCount; i++) {
        if ((g_index_bitmap & (1u << i)) == 0) {
            g_index_bitmap |= 1u << i;
            return static_cast<s32>(i);
        }
    }
    return -1;
}

void PS4_SYSV_ABI sceCompositorReleaseIndex(u32 index) {
    if (index >= IndexCount) {
        return;
    }
    std::scoped_lock lk{g_index_mutex};
    g_index_bitmap &= ~(1u << index);
}

s32 PS4_SYSV_ABI sceCompositorLockCommandBuffer() {
    std::unique_lock lk{g_command_lock.mutex};
    const auto self = std::this_thread::get_id();
    if (g_command_lock.depth != 0 && g_command_lock.owner == self) {
        g_command_lock.depth++;
        return ORBIS_OK;
    }
    g_command_lock.cv.wait(lk, [] { return g_command_lock.depth == 0; });
    g_command_lock.owner = self;
    g_command_lock.depth = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorReleaseCommandBuffer() {
    std::scoped_lock lk{g_command_lock.mutex};
    if (g_command_lock.depth == 0 || g_command_lock.owner != std::this_thread::get_id()) {
        return ORBIS_OK;
    }
    if (--g_command_lock.depth == 0) {
        g_command_lock.owner = {};
        g_command_lock.cv.notify_one();
    }
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceCompositorGetSystemAddress() {
    std::scoped_lock lk{g_state.mutex};
    return g_state.system.addr;
}

u64 PS4_SYSV_ABI sceCompositorGetSystemSize() {
    std::scoped_lock lk{g_state.mutex};
    return g_state.system.size;
}

void* PS4_SYSV_ABI sceCompositorGetVideoAddress() {
    std::scoped_lock lk{g_state.mutex};
    return g_state.video.addr;
}

u64 PS4_SYSV_ABI sceCompositorGetVideoSize() {
    std::scoped_lock lk{g_state.mutex};
    return g_state.video.size;
}

u64 PS4_SYSV_ABI sceCompositorGetAnotherProcessSystemAddress(u16 process_id) {
    return 0;
}

u64 PS4_SYSV_ABI sceCompositorGetAnotherProcessSystemSize(u16 process_id) {
    return 0;
}

u64 PS4_SYSV_ABI sceCompositorGetAnotherProcessVideoAddress(u16 process_id) {
    return 0;
}

u64 PS4_SYSV_ABI sceCompositorGetAnotherProcessVideoSize(u16 process_id) {
    return 0;
}

s32 PS4_SYSV_ABI sceCompositorMapAnotherProcess(u16 process_id) {
    LOG_ERROR(Lib_Composite, "(STUBBED) called, process_id = {}", process_id);
    return ORBIS_COMPOSITE_ERROR_INVALID_MEMORY;
}

u64 PS4_SYSV_ABI sceCompositorGetDummyAddress() {
    return DummyAddress;
}

u64 PS4_SYSV_ABI sceCompositorGetDummySize() {
    return DummySize;
}

u64 PS4_SYSV_ABI sceCompositorGetDmemOffset() {
    return 0;
}

u32 PS4_SYSV_ABI sceCompositorIsReady4K() {
    return 0;
}

s32 PS4_SYSV_ABI sceCompositorIsDebugCaptureEnabled() {
    return 0;
}

s32 PS4_SYSV_ABI sceCompositorInsertThreadTraceMarker() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorCheckCrash(u32 index) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorGetCanvasHandle(u8 index, u32* handle) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NO_CONNECTION;
    }
    if (handle == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    *handle = (static_cast<u32>(index) << 16) | ClientId;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorMakeCanvasHandle(u16 client_id, u32 index, u32* handle) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NO_CONNECTION;
    }
    if (handle == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    *handle = (index << 16) | client_id;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorGetRenderTargetResolution(u16* width, u16* height) {
    if (width == nullptr || height == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    *width = RenderTargetWidth;
    *height = RenderTargetHeight;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompsoitorGetRenderingTime(u32* time) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NO_CONNECTION;
    }
    if (time == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    *time = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompsoitorGetProcessRenderingTime(u32* time) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NO_CONNECTION;
    }
    if (time == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    *time = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompsoitorGetGpuClock(u64* clock) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NO_CONNECTION;
    }
    if (clock == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    *clock = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorGetRemainedCommandNum() {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    return RingCapacity;
}

s32 PS4_SYSV_ABI sceCompositorReserveCommandBuffer(u32 count, u32 unk1, u32 unk2) {
    return CheckConnected();
}

static s32 FlushCommands() {
    std::scoped_lock flush_lock{g_flush_mutex};
    std::vector<PendingDcb> dcbs;
    std::vector<u8> post_events;
    u32 serial = 0;
    const u32* fence_packet = nullptr;
    {
        std::scoped_lock lk{g_state.mutex};
        if (!g_state.connected) {
            return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
        }
        dcbs.swap(g_state.pending_dcbs);
        post_events.swap(g_state.pending_post_events);
        if (!post_events.empty() && g_state.sync.addr != nullptr) {
            serial = ++g_state.last_serial;
            const u32 slot = BuildFencePacket(g_state.sync, serial);
            fence_packet = reinterpret_cast<const u32*>(static_cast<u8*>(g_state.sync.addr) +
                                                        SyncPacketOffset + slot * SyncPacketSize);
        }
    }

    for (const auto& dcb : dcbs) {
        const u32* addr = dcb.addr;
        u32 size = dcb.size;
        const s32 rc =
            Libraries::GnmDriver::sceGnmSubmitCommandBuffers(1, &addr, &size, nullptr, nullptr);
        if (rc != ORBIS_OK) {
            LOG_ERROR(Lib_Composite, "Failed to submit DCB {} ({:#x} bytes), rc = {:#x}",
                      fmt::ptr(addr), size, static_cast<u32>(rc));
        }
    }
    if (fence_packet != nullptr) {
        const u32* addr = fence_packet;
        u32 size = sizeof(AmdGpu::PM4CmdEventWriteEop);
        const s32 rc =
            Libraries::GnmDriver::sceGnmSubmitCommandBuffers(1, &addr, &size, nullptr, nullptr);
        if (rc != ORBIS_OK) {
            LOG_ERROR(Lib_Composite, "Failed to submit fence, rc = {:#x}", static_cast<u32>(rc));
            serial = 0;
        }
    }

    if (!post_events.empty()) {
        std::scoped_lock lk{g_state.mutex};
        for (const u8 index : post_events) {
            g_state.post_serials[index] = serial;
            g_state.post_events |= 1u << index;
        }
        g_state.post_event_cv.notify_all();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorFlush() {
    return FlushCommands();
}

s32 PS4_SYSV_ABI sceCompositorFlushWithRepeat() {
    return FlushCommands();
}

s32 PS4_SYSV_ABI sceCompositorSetGnmContextCommand(const void* dcb, u32 dcb_size, const void* ccb,
                                                   u32 ccb_size) {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (ccb != nullptr || ccb_size != 0) {
        LOG_ERROR(Lib_Composite, "Constant command buffers are not supported");
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    LOG_TRACE(Lib_Composite, "dcb = {}, dcb_size = {:#x}", fmt::ptr(dcb), dcb_size);
    g_state.pending_dcbs.push_back({static_cast<const u32*>(dcb), dcb_size});
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetSeparateContextCommand() {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetCompositeCanvasCommandInC(u8 canvas, const void* params) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (canvas > MaxCanvasIndex || params == nullptr) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetInvisibleCanvasCommand(u8 canvas) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetEndOfFrameCommand() {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    g_state.end_of_frame_pending = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetEventCommand() {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    g_state.event_pending = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetPostEventCommand(u8 index) {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (index > MaxPostEventIndex) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    g_state.pending_post_events.push_back(index);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorWaitPostEvent(u8 index) {
    std::unique_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (index > MaxPostEventIndex) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    const u32 bit = 1u << index;
    const auto deadline = std::chrono::steady_clock::now() + PostEventTimeout;
    if (!g_state.post_event_cv.wait_until(lk, deadline,
                                          [&] { return (g_state.post_events & bit) != 0; })) {
        return ORBIS_COMPOSITE_ERROR_WAIT_FAILED;
    }
    const u32 serial = g_state.post_serials[index];
    while (g_state.connected && !FenceReached(g_state.sync, serial)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return ORBIS_COMPOSITE_ERROR_WAIT_FAILED;
        }
        lk.unlock();
        std::this_thread::sleep_for(FencePollInterval);
        lk.lock();
    }
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    g_state.post_events &= ~bit;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorWaitEvent() {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    g_state.event_pending = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorWaitEndOfRendering() {
    std::scoped_lock lk{g_state.mutex};
    if (!g_state.connected) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    g_state.end_of_frame_pending = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetLocalStallCommand() {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorReleaseLocalStall(u32 slot) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetDebugPositionCommand(u8 id, u16 x, u16 y, u16 width, u16 height) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetPatchCommand(void* addr, u16 value) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetMemoryCommand(u32 value) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetRepeatCommand() {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetCursorImageAddress(u8 cursor, const void* addr) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (!IsInsideVideoRegion(addr, 4) || cursor > MaxCursorIndex) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetMorpheusState(s32 state) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_CONNECTED;
    }
    if (state > MaxMorpheusState) {
        return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetIndirectCompositionCommand(void* addr, u16 value) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetIndirectRenderTargetConfigCommand(u8 target, u16 width,
                                                                   u16 height) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetIndirectCanvasCommand(u8 canvas, u8 target) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorSetKeepDisplayBufferCommand() {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorCommandGpuPerfBegin(const void* data, void* addr) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorCommandGpuPerfEnd(const void* data, void* addr) {
    return CheckConnected();
}

s32 PS4_SYSV_ABI sceCompositorCreateIndirectRenderTarget(u16 width, u16 height) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_INITIALIZED;
    }
    LOG_ERROR(Lib_Composite, "(STUBBED) called, width = {}, height = {}", width, height);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorDeleteIndirectRenderTarget(u16 target) {
    if (!IsConnected()) {
        return ORBIS_COMPOSITE_ERROR_NOT_INITIALIZED;
    }
    LOG_ERROR(Lib_Composite, "(STUBBED) called, target = {}", target);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetFlipCommand() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetZoomCommand() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorResetZoomCommand() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetResolutionCommand() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetVideoOutMode() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSetGameSufaceControlCommand() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompositorSystemConfig() {
    return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
}

s32 PS4_SYSV_ABI sceCompositorSetSystemConfigWithMask() {
    return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
}

s32 PS4_SYSV_ABI sceCompositorGetClientList() {
    return ORBIS_COMPOSITE_ERROR_INVALID_ARGUMENT;
}

void PS4_SYSV_ABI sceCompositorHandleProcessEvents() {}

s32 PS4_SYSV_ABI Func_A5E7CB11092CC77E() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_2A9B92DC221C724C() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5D9CF2417E13E6DB() {
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("DhtKelVAIaA", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetGnmContextCommand);
    LIB_FUNCTION("5Z0YF3QKlhk", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetSeparateContextCommand);
    LIB_FUNCTION("gVoOE32ATg0", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetCompositeCanvasCommandInC);
    LIB_FUNCTION("9sdvKfLR+mY", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetInvisibleCanvasCommand);
    LIB_FUNCTION("m4cofzsxUWY", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetEndOfFrameCommand);
    LIB_FUNCTION("3Q85e5cS3e0", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetPostEventCommand);
    LIB_FUNCTION("qZNF03+ghLI", "libSceComposite", 1, "libSceComposite", sceCompositorFlush);
    LIB_FUNCTION("yW5gfrDFAoc", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetLocalStallCommand);
    LIB_FUNCTION("9zmcX1uKDGg", "libSceComposite", 1, "libSceComposite",
                 sceCompositorReleaseLocalStall);
    LIB_FUNCTION("5fbNa7tkGoI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetEventCommand);
    LIB_FUNCTION("7NxubT1-VV4", "libSceComposite", 1, "libSceComposite",
                 sceCompositorReserveCommandBuffer);
    LIB_FUNCTION("dHK+qu5D2HM", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetDebugPositionCommand);
    LIB_FUNCTION("YzI2BOoDw+I", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetPatchCommand);
    LIB_FUNCTION("n2XkxixrY2k", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetGameSufaceControlCommand);
    LIB_FUNCTION("aroIog+umzo", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetZoomCommand);
    LIB_FUNCTION("1PX0NFNL1BQ", "libSceComposite", 1, "libSceComposite",
                 sceCompositorResetZoomCommand);
    LIB_FUNCTION("eLU8pDi9KN0", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetResolutionCommand);
    LIB_FUNCTION("kn5tI0VFTVY", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetMemoryCommand);
    LIB_FUNCTION("fEQcEYs1yuU", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetRepeatCommand);
    LIB_FUNCTION("9bnaeOBBD1Q", "libSceComposite", 1, "libSceComposite",
                 sceCompositorFlushWithRepeat);
    LIB_FUNCTION("OSqOny+kPrA", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetCursorImageAddress);
    LIB_FUNCTION("M6wp6+JUSRI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetIndirectCompositionCommand);
    LIB_FUNCTION("kZ7Xre07w70", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetIndirectRenderTargetConfigCommand);
    LIB_FUNCTION("-zpfGY1n49E", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetIndirectCanvasCommand);
    LIB_FUNCTION("1oTrw-ivVpA", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetFlipCommand);
    LIB_FUNCTION("H4EXZ9L3p2M", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetMorpheusState);
    LIB_FUNCTION("r0QtEP0p4Gs", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetVideoOutMode);
    LIB_FUNCTION("2xhRHbvhwfU", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetRemainedCommandNum);
    LIB_FUNCTION("6bz4VVSSFyg", "libSceComposite", 1, "libSceComposite",
                 sceCompositorCommandGpuPerfBegin);
    LIB_FUNCTION("fH2IStnGK4M", "libSceComposite", 1, "libSceComposite",
                 sceCompositorCommandGpuPerfEnd);
    LIB_FUNCTION("dIA+v-Lh1+o", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetKeepDisplayBufferCommand);
    LIB_FUNCTION("N7PrM+lPMW0", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetCanvasHandle);
    LIB_FUNCTION("R00q9P3BKAc", "libSceComposite", 1, "libSceComposite",
                 sceCompositorWaitEndOfRendering);
    LIB_FUNCTION("deKovf3qViA", "libSceComposite", 1, "libSceComposite",
                 sceCompositorWaitPostEvent);
    LIB_FUNCTION("0wFl-CwJ37Y", "libSceComposite", 1, "libSceComposite", sceCompositorWaitEvent);
    LIB_FUNCTION("T6CVkdCDO7o", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetSystemAddress);
    LIB_FUNCTION("N6ID0KNnzY8", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetSystemSize);
    LIB_FUNCTION("bxt+muwit0w", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetVideoAddress);
    LIB_FUNCTION("FTQCTDU0b4g", "libSceComposite", 1, "libSceComposite", sceCompositorGetVideoSize);
    LIB_FUNCTION("s-6xbR71jQI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetAnotherProcessVideoAddress);
    LIB_FUNCTION("gbjddWAc6-o", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetAnotherProcessVideoSize);
    LIB_FUNCTION("ayhAQXdGNqU", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetAnotherProcessSystemAddress);
    LIB_FUNCTION("8bAq9t5kF9k", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetAnotherProcessSystemSize);
    LIB_FUNCTION("9M7HkbwUs-E", "libSceComposite", 1, "libSceComposite", sceCompositorCheckCrash);
    LIB_FUNCTION("twGXom56jw0", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetRenderTargetResolution);
    LIB_FUNCTION("q+Qw1ESxCj8", "libSceComposite", 1, "libSceComposite",
                 sceCompositorIsDebugCaptureEnabled);
    LIB_FUNCTION("-wIAG588mvg", "libSceComposite", 1, "libSceComposite",
                 sceCompositorInsertThreadTraceMarker);
    LIB_FUNCTION("MjH7MOV6LVk", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetDmemOffset);
    LIB_FUNCTION("4PTzGjEmkXI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorMakeCanvasHandle);
    LIB_FUNCTION("rIH7UTXfho4", "libSceComposite", 1, "libSceComposite",
                 sceCompsoitorGetRenderingTime);
    LIB_FUNCTION("EcTHemmz6n4", "libSceComposite", 1, "libSceComposite",
                 sceCompsoitorGetProcessRenderingTime);
    LIB_FUNCTION("4yWqjTZtvs4", "libSceComposite", 1, "libSceComposite", sceCompsoitorGetGpuClock);
    LIB_FUNCTION("fcDt18r-ibY", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetDummyAddress);
    LIB_FUNCTION("F9gmz-sU-Z4", "libSceComposite", 1, "libSceComposite", sceCompositorGetDummySize);
    LIB_FUNCTION("KpuS3CIcckw", "libSceComposite", 1, "libSceComposite", Func_2A9B92DC221C724C);
    LIB_FUNCTION("XZzyQX4T5ts", "libSceComposite", 1, "libSceComposite", Func_5D9CF2417E13E6DB);
    LIB_FUNCTION("pefLEQksx34", "libSceComposite", 1, "libSceComposite", Func_A5E7CB11092CC77E);
    LIB_FUNCTION("RSEX1eS1tCI", "libSceComposite", 1, "libSceComposite", sceCompositorIsReady4K);
    LIB_FUNCTION("xIkcEpdMxrw", "libSceComposite", 1, "libSceComposite", sceCompositorInit);
    LIB_FUNCTION("IUlpGnuoR1c", "libSceComposite", 1, "libSceComposite",
                 sceCompositorInitWithProcessOrder);
    LIB_FUNCTION("y5IJdPxdBrE", "libSceComposite", 1, "libSceComposite", sceCompositorQuit);
    LIB_FUNCTION("GgOrwi+9vcA", "libSceComposite", 1, "libSceComposite",
                 sceCompositorLockCommandBuffer);
    LIB_FUNCTION("1OXbuWLRxqI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorReleaseCommandBuffer);
    LIB_FUNCTION("WkdkfQun1gc", "libSceComposite", 1, "libSceComposite", sceCompositorSystemConfig);
    LIB_FUNCTION("dN3ooEQkn90", "libSceComposite", 1, "libSceComposite",
                 sceCompositorSetSystemConfigWithMask);
    LIB_FUNCTION("meXSAmyPBUE", "libSceComposite", 1, "libSceComposite",
                 sceCompositorCreateIndirectRenderTarget);
    LIB_FUNCTION("wCqghwW7vQw", "libSceComposite", 1, "libSceComposite",
                 sceCompositorDeleteIndirectRenderTarget);
    LIB_FUNCTION("9vHGtqLkmTQ", "libSceComposite", 1, "libSceComposite",
                 sceCompositorGetClientList);
    LIB_FUNCTION("mKEAx7QnPeg", "libSceComposite", 1, "libSceComposite",
                 sceCompositorMapAnotherProcess);
    LIB_FUNCTION("f9X0imA90fI", "libSceComposite", 1, "libSceComposite",
                 sceCompositorHandleProcessEvents);
    LIB_FUNCTION("G4Q8KNkb5XE", "libSceComposite", 1, "libSceComposite",
                 sceCompositorAllocateIndex);
    LIB_FUNCTION("ZwsBB3s8qMk", "libSceComposite", 1, "libSceComposite", sceCompositorReleaseIndex);
}

} // namespace Libraries::Composite
