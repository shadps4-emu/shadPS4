// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/videoout/driver.h"
#include "core/libraries/videoout/video_out.h"
#include "core/loader/symbols_resolver.h"
#include "core/platform.h"

namespace Libraries::VideoOut {

static std::unique_ptr<VideoOutDriver> driver;

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(BufferAttribute* attribute, PixelFormat pixelFormat,
                                                u32 tilingMode, u32 aspectRatio, u32 width,
                                                u32 height, u32 pitchInPixel) {
    LOG_INFO(Lib_VideoOut,
             "pixelFormat = {}, tilingMode = {}, aspectRatio = {}, width = {}, height = {}, "
             "pitchInPixel = {}",
             GetPixelFormatString(pixelFormat), tilingMode, aspectRatio, width, height,
             pitchInPixel);

    std::memset(attribute, 0, sizeof(BufferAttribute));
    attribute->pixel_format = static_cast<PixelFormat>(pixelFormat);
    attribute->tiling_mode = static_cast<TilingMode>(tilingMode);
    attribute->aspect_ratio = aspectRatio;
    attribute->width = width;
    attribute->height = height;
    attribute->pitch_in_pixel = pitchInPixel;
    attribute->option = SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_NONE;
}

s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(Kernel::SceKernelEqueue eq, s32 handle, void* udata) {
    LOG_INFO(Lib_VideoOut, "handle = {}", handle);

    auto* port = driver->GetPort(handle);
    if (port == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (eq == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }

    Kernel::EqueueEvent event{};
    event.event.ident = SCE_VIDEO_OUT_EVENT_FLIP;
    event.event.filter = Kernel::SceKernelEvent::Filter::VideoOut;
    // The library only sets EV_ADD but kernel driver forces EV_CLEAR
    event.event.flags = Kernel::SceKernelEvent::Flags::Clear;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    event.data = port;
    eq->AddEvent(event);

    port->flip_events.push_back(eq);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutAddVblankEvent(Kernel::SceKernelEqueue eq, s32 handle, void* udata) {
    LOG_INFO(Lib_VideoOut, "handle = {}", handle);

    auto* port = driver->GetPort(handle);
    if (port == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (eq == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }

    Kernel::EqueueEvent event{};
    event.event.ident = SCE_VIDEO_OUT_EVENT_VBLANK;
    event.event.filter = Kernel::SceKernelEvent::Filter::VideoOut;
    // The library only sets EV_ADD but kernel driver forces EV_CLEAR
    event.event.flags = Kernel::SceKernelEvent::Flags::Clear;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    event.data = port;
    eq->AddEvent(event);

    port->vblank_events.push_back(eq);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex, void* const* addresses,
                                            s32 bufferNum, const BufferAttribute* attribute) {
    if (!addresses || !attribute) {
        LOG_ERROR(Lib_VideoOut, "Addresses are null");
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    auto* port = driver->GetPort(handle);
    if (!port || !port->is_open) {
        LOG_ERROR(Lib_VideoOut, "Invalid handle = {}", handle);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    return driver->RegisterBuffers(port, startIndex, addresses, bufferNum, attribute);
}

s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate) {
    LOG_TRACE(Lib_VideoOut, "called");
    driver->GetPort(handle)->flip_rate = rate;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle) {
    LOG_TRACE(Lib_VideoOut, "called");
    auto* port = driver->GetPort(handle);
    std::unique_lock lock{port->port_mutex};
    s32 pending = port->flip_status.flipPendingNum;
    return pending;
}

s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex, s32 flipMode, s64 flipArg) {
    auto* port = driver->GetPort(handle);
    if (!port) {
        LOG_ERROR(Lib_VideoOut, "Invalid handle = {}", handle);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (flipMode != 1) {
        LOG_WARNING(Lib_VideoOut, "flipmode = {}", flipMode);
    }

    if (bufferIndex < -1 || bufferIndex > 15) {
        LOG_ERROR(Lib_VideoOut, "Invalid bufferIndex = {}", bufferIndex);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_INDEX;
    }

    if (bufferIndex != -1 && port->buffer_slots[bufferIndex].group_index < 0) {
        LOG_ERROR(Lib_VideoOut, "Slot in bufferIndex = {} is not registered", bufferIndex);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_INDEX;
    }

    LOG_DEBUG(Lib_VideoOut, "bufferIndex = {}, flipMode = {}, flipArg = {}", bufferIndex, flipMode,
              flipArg);

    if (!driver->SubmitFlip(port, bufferIndex, flipArg)) {
        LOG_ERROR(Lib_VideoOut, "Flip queue is full");
        return ORBIS_VIDEO_OUT_ERROR_FLIP_QUEUE_FULL;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideoOutGetEventId(const Kernel::SceKernelEvent* ev) {
    if (ev == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    if (ev->filter != Kernel::SceKernelEvent::Filter::VideoOut) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }
    return ev->ident;
}

int PS4_SYSV_ABI sceVideoOutGetEventData(const Kernel::SceKernelEvent* ev, int64_t* data) {
    if (ev == nullptr || data == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    if (ev->filter != Kernel::SceKernelEvent::Filter::VideoOut) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }

    *data = ev->data;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, FlipStatus* status) {
    if (!status) {
        LOG_ERROR(Lib_VideoOut, "Flip status is null");
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    auto* port = driver->GetPort(handle);
    if (!port) {
        LOG_ERROR(Lib_VideoOut, "Invalid port handle = {}", handle);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    {
        std::unique_lock lock{port->port_mutex};
        *status = port->flip_status;
    }

    LOG_TRACE(Lib_VideoOut,
              "count = {}, processTime = {}, tsc = {}, submitTsc = {}, flipArg = {}, gcQueueNum = "
              "{}, flipPendingNum = {}, currentBuffer = {}",
              status->count, status->processTime, status->tsc, status->submitTsc, status->flipArg,
              status->gcQueueNum, status->flipPendingNum, status->currentBuffer);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(int handle, SceVideoOutVblankStatus* status) {
    if (status == nullptr) {
        return SCE_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    auto* port = driver->GetPort(handle);
    if (!port) {
        LOG_ERROR(Lib_VideoOut, "Invalid port handle = {}", handle);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    std::unique_lock lock{port->vo_mutex};
    *status = port->vblank_status;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(s32 handle, SceVideoOutResolutionStatus* status) {
    LOG_INFO(Lib_VideoOut, "called");
    *status = driver->GetPort(handle)->resolution;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index,
                                 const void* param) {
    LOG_INFO(Lib_VideoOut, "called");
    ASSERT(busType == SCE_VIDEO_OUT_BUS_TYPE_MAIN);

    if (index != 0) {
        LOG_ERROR(Lib_VideoOut, "Index != 0");
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }

    auto* params = reinterpret_cast<const ServiceThreadParams*>(param);
    int handle = driver->Open(params);

    if (handle < 0) {
        LOG_ERROR(Lib_VideoOut, "All available handles are open");
        return ORBIS_VIDEO_OUT_ERROR_RESOURCE_BUSY;
    }

    return handle;
}

s32 PS4_SYSV_ABI sceVideoOutClose(s32 handle) {
    driver->Close(handle);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutUnregisterBuffers(s32 handle, s32 attributeIndex) {
    auto* port = driver->GetPort(handle);
    if (!port || !port->is_open) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    return driver->UnregisterBuffers(port, attributeIndex);
}

void sceVideoOutGetBufferLabelAddress(s32 handle, uintptr_t* label_addr) {
    auto* port = driver->GetPort(handle);
    ASSERT(port);
    *label_addr = reinterpret_cast<uintptr_t>(port->buffer_labels.data());
}

s32 sceVideoOutSubmitEopFlip(s32 handle, u32 buf_id, u32 mode, u32 arg, void** unk) {
    auto* port = driver->GetPort(handle);
    if (!port) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    Platform::IrqC::Instance()->RegisterOnce(
        Platform::InterruptId::GfxFlip, [=](Platform::InterruptId irq) {
            ASSERT_MSG(irq == Platform::InterruptId::GfxFlip, "An unexpected IRQ occured");
            ASSERT_MSG(port->buffer_labels[buf_id] == 1, "Out of order flip IRQ");
            const auto result = driver->SubmitFlip(port, buf_id, arg, true);
            ASSERT_MSG(result, "EOP flip submission failed");
        });

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetDeviceCapabilityInfo(
    s32 handle, SceVideoOutDeviceCapabilityInfo* pDeviceCapabilityInfo) {
    pDeviceCapabilityInfo->capability = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutWaitVblank(s32 handle) {
    auto* port = driver->GetPort(handle);
    if (!port) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    std::unique_lock lock{port->vo_mutex};
    const auto prev_counter = port->vblank_status.count;
    port->vblank_cv.wait(lock, [&]() { return prev_counter != port->vblank_status.count; });
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    driver = std::make_unique<VideoOutDriver>(Config::getScreenWidth(), Config::getScreenHeight());

    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutAddFlipEvent);
    LIB_FUNCTION("Xru92wHJRmg", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutAddVblankEvent);
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
    LIB_FUNCTION("kGVLc3htQE8", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetDeviceCapabilityInfo);
    LIB_FUNCTION("j6RaAUlaLv0", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutWaitVblank);
    LIB_FUNCTION("U2JJtSqNKZI", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetEventId);
    LIB_FUNCTION("rWUTcKdkUzQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0,
                 sceVideoOutGetEventData);

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
    LIB_FUNCTION("kGVLc3htQE8", "libSceVideoOut", 1, "libSceVideoOut", 1, 1,
                 sceVideoOutGetDeviceCapabilityInfo);
}

} // namespace Libraries::VideoOut
