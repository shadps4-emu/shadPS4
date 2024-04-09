// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <string>
#include "Objects/video_out_ctx.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/PS4/GPU/gpu_memory.h"
#include "core/PS4/GPU/video_out_buffer.h"
#include "core/PS4/HLE/Graphics/graphics_render.h"
#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "core/loader/symbols_resolver.h"
#include "emulator.h"
#include "src/core/libraries/libsceuserservice.h"

namespace HLE::Libs::Graphics::VideoOut {

constexpr bool log_file_videoout = true; // disable it to disable logging

void videoOutInit(u32 width, u32 height) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    videoOut->Init(width, height);
}

bool videoOutFlip(u32 micros) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    return videoOut->getFlipQueue().flip(micros);
}
void VideoOutVblank() {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    return videoOut->Vblank();
}

std::string getPixelFormatString(s32 format) {
    switch (format) {
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB:
        return "PIXEL_FORMAT_A8R8G8B8_SRGB";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB:
        return "PIXEL_FORMAT_A8B8G8R8_SRGB";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10:
        return "PIXEL_FORMAT_A2R10G10B10";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_SRGB:
        return "PIXEL_FORMAT_A2R10G10B10_SRGB";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_BT2020_PQ:
        return "PIXEL_FORMAT_A2R10G10B10_BT2020_PQ";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_A16R16G16B16_FLOAT:
        return "PIXEL_FORMAT_A16R16G16B16_FLOAT";
    case SCE_VIDEO_OUT_PIXEL_FORMAT_YCBCR420_BT709:
        return "PIXEL_FORMAT_YCBCR420_BT709";
    default:
        return "Unknown pixel format";
    }
}

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(SceVideoOutBufferAttribute* attribute,
                                                u32 pixelFormat, u32 tilingMode, u32 aspectRatio,
                                                u32 width, u32 height, u32 pitchInPixel) {
    LOG_INFO(Lib_VideoOut,
             "pixelFormat = {}, tilingMode = {}, aspectRatio = {}, width = {}, height = {}, "
             "pitchInPixel = {}",
             getPixelFormatString(pixelFormat), tilingMode, aspectRatio, width, height,
             pitchInPixel);

    std::memset(attribute, 0, sizeof(SceVideoOutBufferAttribute));

    attribute->pixelFormat = pixelFormat;
    attribute->tilingMode = tilingMode;
    attribute->aspectRatio = aspectRatio;
    attribute->width = width;
    attribute->height = height;
    attribute->pitchInPixel = pitchInPixel;
    attribute->option = SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_NONE;
}

static void flip_reset_event_func(Core::Kernel::EqueueEvent* event) {
    event->isTriggered = false;
    event->event.fflags = 0;
    event->event.data = 0;
}

static void flip_trigger_event_func(Core::Kernel::EqueueEvent* event, void* trigger_data) {
    event->isTriggered = true;
    event->event.fflags++;
    event->event.data = reinterpret_cast<intptr_t>(trigger_data);
}

static void flip_delete_event_func(Core::Kernel::SceKernelEqueue eq,
                                   Core::Kernel::EqueueEvent* event) {
    BREAKPOINT(); // TODO
}

s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(Core::Kernel::SceKernelEqueue eq, s32 handle,
                                         void* udata) {
    LOG_INFO(Lib_VideoOut, "handle = {}", handle);

    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();

    auto* ctx = videoOut->getCtx(handle);

    if (ctx == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    std::scoped_lock lock(ctx->m_mutex);

    if (eq == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }

    Core::Kernel::EqueueEvent event{};
    event.isTriggered = false;
    event.event.ident = SCE_VIDEO_OUT_EVENT_FLIP;
    event.event.filter = Core::Kernel::EVFILT_VIDEO_OUT;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    event.filter.delete_event_func = flip_delete_event_func;
    event.filter.reset_event_func = flip_reset_event_func;
    event.filter.trigger_event_func = flip_trigger_event_func;
    event.filter.data = ctx;

    int result = eq->addEvent(event);

    ctx->m_flip_evtEq.push_back(eq);

    return result;
}

s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(int handle, SceVideoOutVblankStatus* status) {
    if (status == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    auto* ctx = videoOut->getCtx(handle);

    ctx->m_mutex.lock();
    *status = ctx->m_vblank_status;
    ctx->m_mutex.unlock();
    return SCE_OK;
}

s32 sceVideoOutRegisterBuffers(s32 handle, s32 startIndex, void* const* addresses, s32 bufferNum,
                               const SceVideoOutBufferAttribute* attribute) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    auto* ctx = videoOut->getCtx(handle);

    if (handle == 1) { // main port
        if (startIndex < 0 || startIndex > 15) {
            LOG_ERROR(Lib_VideoOut, "Invalid startIndex = {}", startIndex);
            return SCE_VIDEO_OUT_ERROR_INVALID_VALUE;
        }
        if (bufferNum < 1 || bufferNum > 16) {
            LOG_ERROR(Lib_VideoOut, "Invalid bufferNum = {}", bufferNum);
            return SCE_VIDEO_OUT_ERROR_INVALID_VALUE;
        }
    }
    if (addresses == nullptr) {
        LOG_ERROR(Lib_VideoOut, "Addresses are null");
        return SCE_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    if (attribute == nullptr) {
        LOG_ERROR(Lib_VideoOut, "Attribute is null");
        return SCE_VIDEO_OUT_ERROR_INVALID_OPTION;
    }
    if (attribute->aspectRatio != 0) {
        LOG_ERROR(Lib_VideoOut, "Invalid aspect ratio = {}", attribute->aspectRatio);
        return SCE_VIDEO_OUT_ERROR_INVALID_ASPECT_RATIO;
    }
    if (attribute->tilingMode < 0 || attribute->tilingMode > 1) {
        LOG_ERROR(Lib_VideoOut, "Invalid tilingMode = {}", attribute->tilingMode);
        return SCE_VIDEO_OUT_ERROR_INVALID_TILING_MODE;
    }

    LOG_INFO(Lib_VideoOut,
             "handle = {}, startIndex = {}, bufferNum = {}, pixelFormat = {:#x}, aspectRatio = {}, "
             "tilingMode = {}, width = {}, height = {}, pitchInPixel = {}, option = {:#x}",
             handle, startIndex, bufferNum, attribute->pixelFormat, attribute->aspectRatio,
             attribute->tilingMode, attribute->width, attribute->height, attribute->pitchInPixel,
             attribute->option);

    int registration_index = ctx->buffers_registration_index++;

    Emu::checkAndWaitForGraphicsInit();
    GPU::renderCreateCtx();

    // try to calculate buffer size
    u64 buffer_size = 0; // still calculation is probably partial or wrong :D
    if (attribute->tilingMode == 0) {
        buffer_size = 1920 * 1088 * 4;
    } else {
        buffer_size = 1920 * 1080 * 4;
    }
    u64 buffer_pitch = attribute->pitchInPixel;

    VideoOutBufferSetInternal buf{};

    buf.start_index = startIndex;
    buf.num = bufferNum;
    buf.set_id = registration_index;
    buf.attr = *attribute;

    ctx->buffers_sets.push_back(buf);

    GPU::VideoOutBufferFormat format = GPU::VideoOutBufferFormat::Unknown;

    if (attribute->pixelFormat == 0x80000000) {
        format = GPU::VideoOutBufferFormat::B8G8R8A8Srgb;
    } else if (attribute->pixelFormat == 0x80002200) {
        format = GPU::VideoOutBufferFormat::R8G8B8A8Srgb;
    }

    GPU::VideoOutBufferObj buffer_info(format, attribute->width, attribute->height,
                                       attribute->tilingMode == 0, Config::isNeoMode(),
                                       buffer_pitch);

    for (int i = 0; i < bufferNum; i++) {
        if (ctx->buffers[i + startIndex].buffer != nullptr) {
            LOG_ERROR(Lib_VideoOut, "Buffer slot {} is occupied!", i + startIndex);
            return SCE_VIDEO_OUT_ERROR_SLOT_OCCUPIED;
        }

        ctx->buffers[i + startIndex].set_id = registration_index;
        ctx->buffers[i + startIndex].buffer = addresses[i];
        ctx->buffers[i + startIndex].buffer_size = buffer_size;
        ctx->buffers[i + startIndex].buffer_pitch = buffer_pitch;
        ctx->buffers[i + startIndex].buffer_render =
            static_cast<Graphics::VideoOutVulkanImage*>(GPU::memoryCreateObj(
                0, videoOut->getGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(addresses[i]),
                buffer_size, buffer_info));

        LOG_INFO(Lib_VideoOut, "buffers[{}] = {:#x}", i + startIndex,
                 reinterpret_cast<u64>(addresses[i]));
    }

    return registration_index;
}

s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate) {
    LOG_INFO(Lib_VideoOut, "called");
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    videoOut->getCtx(handle)->m_flip_rate = rate;
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle) {
    LOG_INFO(Lib_VideoOut, "called");
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    s32 pending = videoOut->getCtx(handle)->m_flip_status.flipPendingNum;
    return pending;
}

s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex, s32 flipMode, s64 flipArg) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    auto* ctx = videoOut->getCtx(handle);

    if (flipMode != 1) {
        // BREAKPOINT();  // only flipmode==1 is supported
        LOG_WARNING(Lib_VideoOut, "flipmode = {}",
                    flipMode); // openBOR needs 2 but seems to work
    }
    if (bufferIndex == -1) {
        BREAKPOINT(); // blank output not supported
    }
    if (bufferIndex < -1 || bufferIndex > 15) {
        LOG_ERROR(Lib_VideoOut, "Invalid bufferIndex = {}", bufferIndex);
        return SCE_VIDEO_OUT_ERROR_INVALID_INDEX;
    }

    LOG_INFO(Lib_VideoOut, "bufferIndex = {}, flipMode = {}, flipArg = {}", bufferIndex, flipMode,
             flipArg);

    if (!videoOut->getFlipQueue().submitFlip(ctx, bufferIndex, flipArg)) {
        LOG_ERROR(Lib_VideoOut, "Flip queue is full");
        return SCE_VIDEO_OUT_ERROR_FLIP_QUEUE_FULL;
    }
    Core::Libraries::LibSceGnmDriver::sceGnmFlushGarlic(); // hackish should be done that neccesary
                                                           // for niko's homebrew
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, SceVideoOutFlipStatus* status) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    auto* ctx = videoOut->getCtx(handle);
    videoOut->getFlipQueue().getFlipStatus(ctx, status);

    LOG_INFO(Lib_VideoOut,
             "count = {}, processTime = {}, tsc = {}, submitTsc = {}, flipArg = {}, gcQueueNum = "
             "{}, flipPendingNum = {}, currentBuffer = {}",
             status->count, status->processTime, status->tsc, status->submitTsc, status->flipArg,
             status->gcQueueNum, status->flipPendingNum, status->currentBuffer);
    return 0;
}

s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(s32 handle, SceVideoOutResolutionStatus* status) {
    LOG_INFO(Lib_VideoOut, "called");
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    *status = videoOut->getCtx(handle)->m_resolution;
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index,
                                 const void* param) {
    LOG_INFO(Lib_VideoOut, "called");
    if (userId != Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM && userId != 0) {
        BREAKPOINT();
    }
    if (busType != SCE_VIDEO_OUT_BUS_TYPE_MAIN) {
        BREAKPOINT();
    }
    if (index != 0) {
        LOG_ERROR(Lib_VideoOut, "Index != 0");
        return SCE_VIDEO_OUT_ERROR_INVALID_VALUE;
    }
    if (param != nullptr) {
        BREAKPOINT();
    }
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    int handle = videoOut->Open();

    if (handle < 0) {
        LOG_ERROR(Lib_VideoOut, "All available handles are open");
        return SCE_VIDEO_OUT_ERROR_RESOURCE_BUSY; // it is alreadyOpened
    }

    return handle;
}

s32 PS4_SYSV_ABI sceVideoOutClose(s32 handle) {
    auto* videoOut = Common::Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    videoOut->Close(handle);
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceVideoOutUnregisterBuffers(s32 handle, s32 attributeIndex) {
    BREAKPOINT();
    return 0;
}

void videoOutRegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutAddFlipEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutIsFlipPending);
    LIB_FUNCTION("N5KDtkIjjJ4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutUnregisterBuffers);
    LIB_FUNCTION("uquVH4-Du78", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutClose);
    LIB_FUNCTION("1FZBKy8HeNU", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetVblankStatus);

    // openOrbis appears to have libSceVideoOut_v1 module libSceVideoOut_v1.1
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 1, 1, sceVideoOutOpen);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutSetFlipRate);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutAddFlipEvent);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutRegisterBuffers);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 1, 1, sceVideoOutSubmitFlip);
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutGetFlipStatus);
}
} // namespace HLE::Libs::Graphics::VideoOut
