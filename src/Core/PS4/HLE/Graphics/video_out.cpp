#include "video_out.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>
#include <Core/PS4/HLE/UserManagement/UsrMngCodes.h>
#include <Util/log.h>
#include <debug.h>
#include <stdio.h>

#include <magic_enum.hpp>
#include <string>

#include "Objects/video_out_ctx.h"
#include "Util/Singleton.h"

namespace HLE::Libs::Graphics::VideoOut {

constexpr bool log_file_videoout = true;  // disable it to disable logging

void videoOutInit(u32 width, u32 height) {
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    videoOut->Init(width, height);
}
std::string getPixelFormatString(s32 format) {
    switch (format) {
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB: return "PIXEL_FORMAT_A8R8G8B8_SRGB";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB: return "PIXEL_FORMAT_A8B8G8R8_SRGB";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10: return "PIXEL_FORMAT_A2R10G10B10";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_SRGB: return "PIXEL_FORMAT_A2R10G10B10_SRGB";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_BT2020_PQ: return "PIXEL_FORMAT_A2R10G10B10_BT2020_PQ";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_A16R16G16B16_FLOAT: return "PIXEL_FORMAT_A16R16G16B16_FLOAT";
        case SCE_VIDEO_OUT_PIXEL_FORMAT_YCBCR420_BT709: return "PIXEL_FORMAT_YCBCR420_BT709";
        default: return "Unknown pixel format";
    }
}

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(SceVideoOutBufferAttribute* attribute, u32 pixelFormat, u32 tilingMode, u32 aspectRatio, u32 width,
                                                u32 height, u32 pitchInPixel) {
    PRINT_FUNCTION_NAME();

    auto tileMode = magic_enum::enum_cast<SceVideoOutTilingMode>(tilingMode);
    auto aspectR = magic_enum::enum_cast<AspectRatioMode>(aspectRatio);

    LOG_INFO_IF(log_file_videoout, "pixelFormat  = {}\n", getPixelFormatString(pixelFormat));
    LOG_INFO_IF(log_file_videoout, "tilingMode   = {}\n", magic_enum::enum_name(tileMode.value()));
    LOG_INFO_IF(log_file_videoout, "aspectRatio  = {}\n", magic_enum::enum_name(aspectR.value()));
    LOG_INFO_IF(log_file_videoout, "width        = {}\n", width);
    LOG_INFO_IF(log_file_videoout, "height       = {}\n", height);
    LOG_INFO_IF(log_file_videoout, "pitchInPixel = {}\n", pitchInPixel);

    memset(attribute, 0, sizeof(SceVideoOutBufferAttribute));

    attribute->pixelFormat = pixelFormat;
    attribute->tilingMode = tilingMode;
    attribute->aspectRatio = aspectRatio;
    attribute->width = width;
    attribute->height = height;
    attribute->pitchInPixel = pitchInPixel;
    attribute->option = SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_NONE;
}

static void flip_reset_event_func(HLE::Kernel::Objects::EqueueEvent* event) {
    event->isTriggered = false;
    event->event.fflags = 0;
    event->event.data = 0;
}
s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(LibKernel::EventQueues::SceKernelEqueue eq, s32 handle, void* udata) {
    PRINT_FUNCTION_NAME();
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();

    auto* ctx = videoOut->getCtx(handle);

    if (ctx == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    Lib::LockMutexGuard lock(ctx->m_mutex);

    if (eq == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }

    HLE::Kernel::Objects::EqueueEvent event;
    event.isTriggered = false;
    event.event.ident = SCE_VIDEO_OUT_EVENT_FLIP;
    event.event.filter = HLE::Kernel::Objects::EVFILT_VIDEO_OUT;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    // event.filter.delete_event_func = flip_event_delete_func;//called in sceKernelDeleteEvent //TODO
    event.filter.reset_event_func = flip_reset_event_func;
    // event.filter.trigger_event_func = flip_event_trigger_func;//called in sceKernelTriggerEvent //TODO
    event.filter.data = ctx;

    int result = eq->addEvent(event);

    ctx->m_flip_evtEq.push_back(eq);

    return result;
}

s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex, void* const* addresses, s32 bufferNum,
                                            const SceVideoOutBufferAttribute* attribute) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate) {
    PRINT_FUNCTION_NAME();
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    videoOut->getCtx(handle)->m_flip_rate = rate;
    return SCE_OK;
}
s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex, s32 flipMode, s64 flipArg) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, SceVideoOutFlipStatus* status) {
    PRINT_FUNCTION_NAME();
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    auto* ctx = videoOut->getCtx(handle);
    videoOut->getFlipQueue().getFlipStatus(ctx, status);

    LOG_INFO_IF(log_file_videoout, "count = {}\n", status->count);
    LOG_INFO_IF(log_file_videoout, "processTime = {}\n", status->processTime);
    LOG_INFO_IF(log_file_videoout, "tsc = {}\n", status->tsc);
    LOG_INFO_IF(log_file_videoout, "submitTsc = {}\n", status->submitTsc);
    LOG_INFO_IF(log_file_videoout, "flipArg = {}\n", status->flipArg);
    LOG_INFO_IF(log_file_videoout, "gcQueueNum = {}\n", status->gcQueueNum);
    LOG_INFO_IF(log_file_videoout, "flipPendingNum = {}\n", status->flipPendingNum);
    LOG_INFO_IF(log_file_videoout, "currentBuffer = {}\n", status->currentBuffer);
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(s32 handle, SceVideoOutResolutionStatus* status) {
    PRINT_FUNCTION_NAME();
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    *status = videoOut->getCtx(handle)->m_resolution;
    return SCE_OK;
}
s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index, const void* param) {
    PRINT_FUNCTION_NAME();
    if (userId != SCE_USER_SERVICE_USER_ID_SYSTEM) {
        BREAKPOINT();
    }
    if (busType != SCE_VIDEO_OUT_BUS_TYPE_MAIN) {
        BREAKPOINT();
    }
    if (index != 0) {
        LOG_TRACE_IF(log_file_videoout, "sceVideoOutOpen index!=0\n");
        return SCE_VIDEO_OUT_ERROR_INVALID_VALUE;
    }
    if (param != nullptr) {
        BREAKPOINT();
    }
    auto* videoOut = Singleton<HLE::Graphics::Objects::VideoOutCtx>::Instance();
    int handle = videoOut->Open();

    if (handle < 0) {
        LOG_TRACE_IF(log_file_videoout, "sceVideoOutOpen all available handles are open\n");
        return SCE_VIDEO_OUT_ERROR_RESOURCE_BUSY;  // it is alreadyOpened
    }

    return handle;
}

void videoOutRegisterLib(SymbolsResolver* sym) {
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutAddFlipEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutIsFlipPending);
}
}  // namespace HLE::Libs::Graphics::VideoOut