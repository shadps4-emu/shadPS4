// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/videoout/driver.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/videoout/videoout_error.h"
#include "core/platform.h"
#include "video_core/renderer_vulkan/vk_presenter.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

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
    event.event.ident = static_cast<u64>(OrbisVideoOutInternalEventId::Flip);
    event.event.filter = Kernel::SceKernelEvent::Filter::VideoOut;
    event.event.flags = Kernel::SceKernelEvent::Flags::Add;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    event.data = port;
    eq->AddEvent(event);

    port->flip_events.push_back(eq);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutDeleteFlipEvent(Kernel::SceKernelEqueue eq, s32 handle) {
    auto* port = driver->GetPort(handle);
    if (port == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (eq == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }
    eq->RemoveEvent(handle, Kernel::SceKernelEvent::Filter::VideoOut);
    port->flip_events.erase(find(port->flip_events.begin(), port->flip_events.end(), eq));
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
    event.event.ident = static_cast<u64>(OrbisVideoOutInternalEventId::Vblank);
    event.event.filter = Kernel::SceKernelEvent::Filter::VideoOut;
    event.event.flags = Kernel::SceKernelEvent::Flags::Add;
    event.event.udata = udata;
    event.event.fflags = 0;
    event.event.data = 0;
    event.data = port;
    eq->AddEvent(event);

    port->vblank_events.push_back(eq);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutDeleteVblankEvent(Kernel::SceKernelEqueue eq, s32 handle) {
    auto* port = driver->GetPort(handle);
    if (port == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (eq == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
    }
    eq->RemoveEvent(handle, Kernel::SceKernelEvent::Filter::VideoOut);
    port->vblank_events.erase(find(port->vblank_events.begin(), port->vblank_events.end(), eq));
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
    s32 pending = port->flip_status.flip_pending_num;
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

s32 PS4_SYSV_ABI sceVideoOutGetEventId(const Kernel::SceKernelEvent* ev) {
    if (ev == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    if (ev->filter != Kernel::SceKernelEvent::Filter::VideoOut) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT;
    }

    OrbisVideoOutInternalEventId internal_event_id =
        static_cast<OrbisVideoOutInternalEventId>(ev->ident);
    switch (internal_event_id) {
    case OrbisVideoOutInternalEventId::Flip:
        return static_cast<s32>(OrbisVideoOutEventId::Flip);
    case OrbisVideoOutInternalEventId::Vblank:
    case OrbisVideoOutInternalEventId::SysVblank:
        return static_cast<s32>(OrbisVideoOutEventId::Vblank);
    case OrbisVideoOutInternalEventId::PreVblankStart:
        return static_cast<s32>(OrbisVideoOutEventId::PreVblankStart);
    case OrbisVideoOutInternalEventId::SetMode:
        return static_cast<s32>(OrbisVideoOutEventId::SetMode);
    case OrbisVideoOutInternalEventId::Position:
        return static_cast<s32>(OrbisVideoOutEventId::Position);
    default: {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT;
    }
    }
}

s32 PS4_SYSV_ABI sceVideoOutGetEventData(const Kernel::SceKernelEvent* ev, s64* data) {
    if (ev == nullptr || data == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    if (ev->filter != Kernel::SceKernelEvent::Filter::VideoOut) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT;
    }

    auto event_data = ev->data >> 0x10;
    if (ev->ident != static_cast<s32>(OrbisVideoOutInternalEventId::Flip) || ev->data == 0) {
        *data = event_data;
    } else {
        *data = event_data | 0xffff000000000000;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetEventCount(const Kernel::SceKernelEvent* ev) {
    if (ev == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    if (ev->filter != Kernel::SceKernelEvent::Filter::VideoOut) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_EVENT;
    }

    auto event_data = static_cast<OrbisVideoOutEventData>(ev->data);
    return event_data.count;
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
              status->count, status->process_time, status->tsc, status->submit_tsc,
              status->flip_arg, status->gc_queue_num, status->flip_pending_num,
              status->current_buffer);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(int handle, SceVideoOutVblankStatus* status) {
    if (status == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
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
    auto* port = driver->GetPort(handle);
    if (!port || !port->is_open) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    *status = port->resolution;
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

s32 PS4_SYSV_ABI sceVideoOutGetBufferLabelAddress(s32 handle, uintptr_t* label_addr) {
    if (label_addr == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }
    auto* port = driver->GetPort(handle);
    if (!port) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    *label_addr = reinterpret_cast<uintptr_t>(port->buffer_labels.data());
    return 16;
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
    if (presenter->IsHDRSupported()) {
        auto& game_info = Common::ElfInfo::Instance();
        if (game_info.GetPSFAttributes().support_hdr) {
            pDeviceCapabilityInfo->capability |= ORBIS_VIDEO_OUT_DEVICE_CAPABILITY_BT2020_PQ;
        }
    }
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

s32 PS4_SYSV_ABI sceVideoOutColorSettingsSetGamma(SceVideoOutColorSettings* settings, float gamma) {
    if (gamma < 0.1f || gamma > 2.0f) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }
    settings->gamma = gamma;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutAdjustColor(s32 handle, const SceVideoOutColorSettings* settings) {
    if (settings == nullptr) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ADDRESS;
    }

    auto* port = driver->GetPort(handle);
    if (!port) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    presenter->GetPPSettingsRef().gamma = settings->gamma;
    return ORBIS_OK;
}

struct Mode {
    u32 size;
    u8 encoding;
    u8 range;
    u8 colorimetry;
    u8 depth;
    u64 refresh_rate;
    u64 resolution;
    u8 reserved[8];
};

void PS4_SYSV_ABI sceVideoOutModeSetAny_(Mode* mode, u32 size) {
    std::memset(mode, 0xff, size);
    mode->size = size;
}

s32 PS4_SYSV_ABI sceVideoOutConfigureOutputMode_(s32 handle, u32 reserved, const Mode* mode,
                                                 const void* options, u32 size_mode,
                                                 u32 size_options) {
    auto* port = driver->GetPort(handle);
    if (!port) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }

    if (reserved != 0) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }

    switch (mode->colorimetry) {
    case OrbisVideoOutColorimetry::Any:
        port->is_hdr = false;
        break;
    case OrbisVideoOutColorimetry::Bt2020PQ:
        if (Common::ElfInfo::Instance().GetPSFAttributes().support_hdr) {
            port->is_hdr = true;
        }
        break;
    default:
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideoOutSetWindowModeMargins(s32 handle, s32 top, s32 bottom) {
    LOG_ERROR(Lib_VideoOut, "(STUBBED) called top = {}, bottom = {}", top, bottom);
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    driver = std::make_unique<VideoOutDriver>(Config::getInternalScreenWidth(),
                                              Config::getInternalScreenHeight());

    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutAddFlipEvent);
    LIB_FUNCTION("Xru92wHJRmg", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutAddVblankEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutIsFlipPending);
    LIB_FUNCTION("N5KDtkIjjJ4", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutUnregisterBuffers);
    LIB_FUNCTION("OcQybQejHEY", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutGetBufferLabelAddress);
    LIB_FUNCTION("uquVH4-Du78", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutClose);
    LIB_FUNCTION("1FZBKy8HeNU", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutGetVblankStatus);
    LIB_FUNCTION("kGVLc3htQE8", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutGetDeviceCapabilityInfo);
    LIB_FUNCTION("j6RaAUlaLv0", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutWaitVblank);
    LIB_FUNCTION("U2JJtSqNKZI", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutGetEventId);
    LIB_FUNCTION("rWUTcKdkUzQ", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutGetEventData);
    LIB_FUNCTION("Mt4QHHkxkOc", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutGetEventCount);
    LIB_FUNCTION("DYhhWbJSeRg", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutColorSettingsSetGamma);
    LIB_FUNCTION("pv9CI5VC+R0", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutAdjustColor);
    LIB_FUNCTION("-Ozn0F1AFRg", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutDeleteFlipEvent);
    LIB_FUNCTION("oNOQn3knW6s", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutDeleteVblankEvent);
    LIB_FUNCTION("pjkDsgxli6c", "libSceVideoOut", 1, "libSceVideoOut", sceVideoOutModeSetAny_);
    LIB_FUNCTION("N1bEoJ4SRw4", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutConfigureOutputMode_);
    LIB_FUNCTION("MTxxrOCeSig", "libSceVideoOut", 1, "libSceVideoOut",
                 sceVideoOutSetWindowModeMargins);
}

} // namespace Libraries::VideoOut
