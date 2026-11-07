// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <utility>
#include "common/logging/log.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/videoout/videocore_interface.h"
#include "core/libraries/videoout/videocore_interface_error.h"
#include "core/libraries/videoout/videoout_error.h"

namespace Libraries::VideoCoreInterface {

namespace {

constexpr s32 MainVideoOutHandle = 1;
constexpr s32 BlankBufferIndex = -1;
constexpr s32 FlipModeVsync = 1;
constexpr s32 MaxBufferStartIndex = 15;
constexpr s32 MaxBufferCount = 16;
constexpr u32 MaxCanvasId = 7;
constexpr u32 FlipPollIntervalUs = 10000;

struct VideoOutContext {
    std::array<u8, 0x78> reserved{};
};

class SimpleVideoOut {
public:
    s32 Open(s32* out_handle) {
        std::scoped_lock lock{state_mutex};
        const s32 video_out =
            VideoOut::sceVideoOutOpen(UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM,
                                      VideoOut::SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, nullptr);
        if (video_out < 0) {
            LOG_ERROR(Lib_VideoCoreInterface, "Failed to open video out: {:#x}", video_out);
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_OPEN_FAILED;
        }
        Kernel::sceKernelCreateEqueue(&equeue, "queue for SimpleVideoOut");
        VideoOut::sceVideoOutAddFlipEvent(equeue, video_out, nullptr);
        handle = video_out;

        {
            std::scoped_lock init_lock{init_mutex};
            thread_started = false;
        }
        flip_thread.Run([this](std::stop_token stop) { FlipLoop(stop); });
        if (!flip_thread.Joinable()) {
            ReleaseVideoOut();
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_OPEN_FAILED;
        }

        std::unique_lock init_lock{init_mutex};
        init_cv.wait(init_lock, [this] { return thread_started; });
        *out_handle = video_out;
        return ORBIS_OK;
    }

    void Close() {
        {
            std::scoped_lock lock{state_mutex};
            flip_thread.Stop();
            ReleaseVideoOut();
        }
        std::scoped_lock lock{draw_mutex};
        buffers = nullptr;
        start_index = 0;
        buffer_count = 0;
    }

    s32 SetBuffers(s32 index, void* const* addresses, s32 count) {
        std::scoped_lock lock{draw_mutex};
        if (flush_callback || finished_count != flipped_count) {
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_NOT_READY;
        }
        start_index = index;
        buffers = addresses;
        buffer_count = static_cast<u32>(count);
        render_count = 1;
        finished_count = 0;
        flipped_count = 0;
        return ORBIS_OK;
    }

    s32 Flush(OrbisVideoCoreInterfaceFlushCallback callback) {
        std::scoped_lock lock{draw_mutex};
        if (flush_callback) {
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_NOT_READY;
        }
        flush_callback = callback;
        return ORBIS_OK;
    }

    s32 GetRenderFrameBuffer(OrbisVideoCoreInterfaceFrameBuffer* frame_buffer) {
        std::scoped_lock lock{draw_mutex};
        if (!buffers || render_count > MaxRenderCount ||
            render_count - finished_count > buffer_count ||
            render_count - flipped_count > buffer_count) {
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_NOT_READY;
        }
        void* address = buffers[(render_count - 1) % buffer_count];
        frame_buffer->address[0] = address;
        frame_buffer->address[1] = address;
        ++render_count;
        return ORBIS_OK;
    }

    s32 FinishRendering() {
        std::scoped_lock lock{draw_mutex};
        if (finished_count + 1 >= render_count) {
            return ORBIS_VIDEO_CORE_INTERFACE_ERROR_NOT_READY;
        }
        ++finished_count;
        return ORBIS_OK;
    }

private:
    static constexpr u32 MaxRenderCount = 0x1ffffffe;

    void ReleaseVideoOut() {
        if (handle == ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE) {
            return;
        }
        Kernel::sceKernelDeleteEqueue(equeue);
        VideoOut::sceVideoOutClose(handle);
        handle = ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    std::optional<s32> NextBufferIndex() {
        std::scoped_lock lock{draw_mutex};
        if (!buffers || flipped_count >= finished_count) {
            return std::nullopt;
        }
        return static_cast<s32>(flipped_count % buffer_count) + start_index;
    }

    void WaitForFlip(s32 video_out, s64 flip_arg, std::stop_token stop) {
        VideoOut::FlipStatus status{};
        VideoOut::sceVideoOutGetFlipStatus(video_out, &status);
        while (status.flip_arg < flip_arg && !stop.stop_requested()) {
            WaitForFlipEvent();
            VideoOut::sceVideoOutGetFlipStatus(video_out, &status);
        }
    }

    void WaitForNoPendingFlip(s32 video_out) {
        while (VideoOut::sceVideoOutIsFlipPending(video_out) != 0) {
            WaitForFlipEvent();
        }
    }

    void WaitForFlipEvent() {
        Kernel::OrbisKernelEvent event{};
        s32 count = 0;
        Kernel::sceKernelWaitEqueue(equeue, &event, 1, &count, nullptr);
    }

    void FlushPendingCallback(s32 video_out) {
        OrbisVideoCoreInterfaceFlushCallback callback{};
        {
            std::scoped_lock lock{draw_mutex};
            callback = std::exchange(flush_callback, nullptr);
            if (callback) {
                VideoOut::sceVideoOutSubmitFlip(video_out, BlankBufferIndex, FlipModeVsync, 0);
            }
        }
        if (callback) {
            callback(ORBIS_OK);
        }
    }

    void FlipLoop(std::stop_token stop) {
        const s32 video_out = handle;
        {
            std::scoped_lock lock{init_mutex};
            thread_started = true;
        }
        init_cv.notify_all();

        s64 flip_arg = 0;
        while (!stop.stop_requested()) {
            const auto index = NextBufferIndex();
            if (!index) {
                FlushPendingCallback(video_out);
                Kernel::sceKernelUsleep(FlipPollIntervalUs);
                continue;
            }
            if (VideoOut::sceVideoOutSubmitFlip(video_out, *index, FlipModeVsync, flip_arg) >= 0) {
                WaitForFlip(video_out, flip_arg, stop);
            }
            {
                std::scoped_lock lock{draw_mutex};
                ++flipped_count;
            }
            ++flip_arg;
        }

        VideoOut::sceVideoOutSubmitFlip(video_out, BlankBufferIndex, FlipModeVsync, 0);
        WaitForNoPendingFlip(video_out);
    }

    std::mutex state_mutex;
    std::mutex draw_mutex;
    std::mutex init_mutex;
    std::condition_variable init_cv;
    bool thread_started{};
    Kernel::Thread flip_thread;
    Kernel::OrbisKernelEqueue equeue{};
    s32 handle{ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE};
    void* const* buffers{};
    s32 start_index{};
    u32 buffer_count{};
    u32 render_count{};
    u32 finished_count{};
    u32 flipped_count{};
    OrbisVideoCoreInterfaceFlushCallback flush_callback{};
};

struct InterfaceState {
    std::mutex mutex;
    std::atomic<bool> initialized{};
    SimpleVideoOut simple_video_out;
    std::unique_ptr<VideoOutContext> video_out_context;
};

InterfaceState& State() {
    static InterfaceState state;
    return state;
}

s32 PS4_SYSV_ABI UnsupportedFunction() {
    LOG_ERROR(Lib_VideoCoreInterface, "Unsupported function called");
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_UNSUPPORTED;
}

} // namespace

s32 PS4_SYSV_ABI sceVideoCoreInterfaceInitializeInterface(void* param) {
    if (!param) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    State().initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetFunctionTable() {
    LOG_INFO(Lib_VideoCoreInterface, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetFunctionTableExt() {
    LOG_INFO(Lib_VideoCoreInterface, "called");
    if (!State().initialized) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceLoadModule() {
    LOG_INFO(Lib_VideoCoreInterface, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceUnloadModule() {
    LOG_INFO(Lib_VideoCoreInterface, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceMainLoop(const char* name) {
    if (!name) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_VideoCoreInterface, "name = {}", name);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceCreateFrameBufferContext(void* param, const void* info,
                                                               void** frame_buffer_context) {
    if (!param || !info || !frame_buffer_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_VideoCoreInterface, "Direct video out has not been initialized");
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_NOT_READY;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceDestroyFrameBufferContext(void* frame_buffer_context) {
    if (!frame_buffer_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceCreateCanvasContext(const void* info, void** canvas_context) {
    if (!info || !canvas_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_VideoCoreInterface, "Shared canvas resources are not available");
    *canvas_context = nullptr;
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_FAILED;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceDestroyCanvasContext(void* canvas_context) {
    if (!canvas_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetCanvasId(void* canvas_context, u32 canvas_id) {
    if (!canvas_context || canvas_id > MaxCanvasId) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceOpenCanvas(u32 canvas_id) {
    LOG_ERROR(Lib_VideoCoreInterface, "Compositor is not available, canvas_id = {}", canvas_id);
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_FAILED;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceCloseCanvas() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetCanvasVideoBufferAddress(u64* address) {
    if (!address) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    *address = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetCanvasVideoBufferSize(u64* size) {
    if (!size) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    *size = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceSubmitCanvases(const u32* canvas_ids, const u64* commands,
                                                     s32 count) {
    if (count == 0) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    if (canvas_ids && commands) {
        for (s32 i = 0; i < count; ++i) {
            if (canvas_ids[i] > MaxCanvasId) {
                return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    LOG_ERROR(Lib_VideoCoreInterface, "Compositor is not available");
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_FAILED;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceWaitCanvasFlip() {
    LOG_ERROR(Lib_VideoCoreInterface, "Compositor is not available");
    return ORBIS_VIDEO_CORE_INTERFACE_ERROR_FAILED;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceAddVblankEvent(Kernel::OrbisKernelEqueue eq, void* udata) {
    const s32 result = VideoOut::sceVideoOutAddVblankEvent(eq, MainVideoOutHandle, udata);
    return result < 0 ? ORBIS_VIDEO_CORE_INTERFACE_ERROR_FAILED : ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceOpenVideoOut(void** video_out_context, s32* handle) {
    if (!video_out_context || !handle) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    s32 video_out_handle{};
    if (const s32 result = state.simple_video_out.Open(&video_out_handle); result != ORBIS_OK) {
        return result;
    }
    state.video_out_context = std::make_unique<VideoOutContext>();
    *video_out_context = state.video_out_context.get();
    *handle = video_out_handle;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceCloseVideoOut(void* video_out_context) {
    if (!video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (!state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    state.simple_video_out.Close();
    state.video_out_context.reset();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceSetVideoOutBufferInfo(void* video_out_context,
                                                            s32 start_index, void* const* addresses,
                                                            s32 count) {
    if (!video_out_context || start_index < 0 || start_index > MaxBufferStartIndex || !addresses ||
        count <= 0 || count > MaxBufferCount) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (!state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    return state.simple_video_out.SetBuffers(start_index, addresses, count);
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceFlushVideoOutBuffer(
    void* video_out_context, OrbisVideoCoreInterfaceFlushCallback callback) {
    if (!video_out_context || !callback) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (!state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    return state.simple_video_out.Flush(callback);
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceGetRenderFrameBuffer(
    void* context, OrbisVideoCoreInterfaceFrameBuffer* frame_buffer) {
    if (!context || !frame_buffer) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (!state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    *frame_buffer = {};
    if (state.simple_video_out.GetRenderFrameBuffer(frame_buffer) != ORBIS_OK) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoCoreInterfaceFinishRendering(void* context) {
    if (!context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_ARGUMENT;
    }
    auto& state = State();
    std::scoped_lock lock{state.mutex};
    if (!state.video_out_context) {
        return ORBIS_VIDEO_CORE_INTERFACE_ERROR_INVALID_STATE;
    }
    return state.simple_video_out.FinishRendering();
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("WNIV7+VKQaA", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceInitializeInterface);
    LIB_FUNCTION("uGnFTTL5rzg", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceSetFunctionTable);
    LIB_FUNCTION("H9VH6VZBsSY", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceSetFunctionTableExt);
    LIB_FUNCTION("C9E1TPtMAjg", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceLoadModule);
    LIB_FUNCTION("CZRLO-gAmpk", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceUnloadModule);
    LIB_FUNCTION("Aj2Ds5L9tmU", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceMainLoop);
    LIB_FUNCTION("FyvlwTMC2TI", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceCreateFrameBufferContext);
    LIB_FUNCTION("+2Ed65+55ZI", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceDestroyFrameBufferContext);
    LIB_FUNCTION("DdJvHNduw2M", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceCreateCanvasContext);
    LIB_FUNCTION("LvLdnjxdEVQ", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceDestroyCanvasContext);
    LIB_FUNCTION("SEU9eYrQFsA", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceSetCanvasId);
    LIB_FUNCTION("GV6hye4Lsjc", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceOpenCanvas);
    LIB_FUNCTION("izssDkN7IAU", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceCloseCanvas);
    LIB_FUNCTION("AJ3xhNOXs-A", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceGetCanvasVideoBufferAddress);
    LIB_FUNCTION("Sgi+fOhrWUQ", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceGetCanvasVideoBufferSize);
    LIB_FUNCTION("fUDokJvA7l0", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceSubmitCanvases);
    LIB_FUNCTION("PfoKAY2M0QE", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceWaitCanvasFlip);
    LIB_FUNCTION("Ik5TMINZINo", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceAddVblankEvent);
    LIB_FUNCTION("4g5vWZV5goU", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceOpenVideoOut);
    LIB_FUNCTION("4IsRgjK5hsU", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceCloseVideoOut);
    LIB_FUNCTION("FdbMnFUqVN0", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceSetVideoOutBufferInfo);
    LIB_FUNCTION("cBl8O2bQwos", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceFlushVideoOutBuffer);
    LIB_FUNCTION("IHhl7z1-uFI", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceGetRenderFrameBuffer);
    LIB_FUNCTION("-yexw2HJTr4", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 sceVideoCoreInterfaceFinishRendering);
    LIB_FUNCTION("xBEMTDFT+T8", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
    LIB_FUNCTION("m2qUOTiZRiI", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
    LIB_FUNCTION("A4Fc6G+tFfQ", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
    LIB_FUNCTION("mqGUZXETSQc", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
    LIB_FUNCTION("-4I1PulG6a4", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
    LIB_FUNCTION("RFRVneI4Pd4", "libSceVideoCoreInterface", 1, "libSceVideoCoreInterface",
                 UnsupportedFunction);
}

} // namespace Libraries::VideoCoreInterface
