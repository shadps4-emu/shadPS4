// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/event_queues.h"
#include "core/libraries/videoout/buffer.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VideoOut {

using SceUserServiceUserId = s32; // TODO move it to proper place

// SceVideoOutBusType
constexpr int SCE_VIDEO_OUT_BUS_TYPE_MAIN = 0;                    // Main output
constexpr int SCE_VIDEO_OUT_BUS_TYPE_AUX_SOCIAL_SCREEN = 5;       // Aux output for social
constexpr int SCE_VIDEO_OUT_BUS_TYPE_AUX_GAME_LIVE_STREAMING = 6; // Aux output for screaming

// SceVideoOutRefreshRate
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_UNKNOWN = 0;
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_23_98HZ = 1;
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_50HZ = 2;
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ = 3;
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_119_88HZ = 13;
constexpr int SCE_VIDEO_OUT_REFRESH_RATE_89_91HZ = 35;
constexpr s64 SCE_VIDEO_OUT_REFRESH_RATE_ANY = 0xFFFFFFFFFFFFFFFFUL;

constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB = 0x80000000;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB = 0x80002200;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10 = 0x88060000;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_SRGB = 0x88000000;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A2R10G10B10_BT2020_PQ = 0x88740000;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_A16R16G16B16_FLOAT = 0xC1060000;
constexpr int SCE_VIDEO_OUT_PIXEL_FORMAT_YCBCR420_BT709 = 0x08322200;

constexpr int SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_NONE = 0;
constexpr int SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_VR = 7;
constexpr int SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_OPTION_STRICT_COLORIMETRY = 8;

enum SceVideoOutEventId : s16 {
    SCE_VIDEO_OUT_EVENT_FLIP = 0,
    SCE_VIDEO_OUT_EVENT_VBLANK = 1,
    SCE_VIDEO_OUT_EVENT_PRE_VBLANK_START = 2
};

enum AspectRatioMode : s32 {
    SCE_VIDEO_OUT_ASPECT_RATIO_16_9 = 0,
};

struct FlipStatus {
    u64 count = 0;
    u64 processTime = 0;
    u64 tsc = 0;
    s64 flipArg = -1;
    u64 submitTsc = 0;
    u64 reserved0 = 0;
    s32 gcQueueNum = 0;
    s32 flipPendingNum = 0;
    s32 currentBuffer = -1;
    u32 reserved1 = 0;
};

struct SceVideoOutResolutionStatus {
    s32 fullWidth = 1280;
    s32 fullHeight = 720;
    s32 paneWidth = 1280;
    s32 paneHeight = 720;
    u64 refreshRate = SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ;
    float screenSizeInInch = 50;
    u16 flags = 0;
    u16 reserved0 = 0;
    u32 reserved1[3] = {0};
};

struct SceVideoOutVblankStatus {
    u64 count = 0;
    u64 processTime = 0;
    u64 tsc = 0;
    u64 reserved[1] = {0};
    u8 flags = 0;
    u8 pad1[7] = {};
};

struct SceVideoOutDeviceCapabilityInfo {
    u64 capability;
};

struct SceVideoOutColorSettings {
    float gamma;
    u32 reserved[3];
};

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(BufferAttribute* attribute, PixelFormat pixelFormat,
                                                u32 tilingMode, u32 aspectRatio, u32 width,
                                                u32 height, u32 pitchInPixel);
s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(Kernel::SceKernelEqueue eq, s32 handle, void* udata);
s32 PS4_SYSV_ABI sceVideoOutAddVblankEvent(Kernel::SceKernelEqueue eq, s32 handle, void* udata);
s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex, void* const* addresses,
                                            s32 bufferNum, const BufferAttribute* attribute);
s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate);
s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle);
s32 PS4_SYSV_ABI sceVideoOutWaitVblank(s32 handle);
s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex, s32 flipMode, s64 flipArg);
s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, FlipStatus* status);
s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(s32 handle, SceVideoOutResolutionStatus* status);
s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index,
                                 const void* param);
s32 PS4_SYSV_ABI sceVideoOutClose(s32 handle);
int PS4_SYSV_ABI sceVideoOutGetEventId(const Kernel::SceKernelEvent* ev);
int PS4_SYSV_ABI sceVideoOutGetEventData(const Kernel::SceKernelEvent* ev, int64_t* data);
s32 PS4_SYSV_ABI sceVideoOutColorSettingsSetGamma(SceVideoOutColorSettings* settings, float gamma);
s32 PS4_SYSV_ABI sceVideoOutAdjustColor(s32 handle, const SceVideoOutColorSettings* settings);

// Internal system functions
void sceVideoOutGetBufferLabelAddress(s32 handle, uintptr_t* label_addr);
s32 sceVideoOutSubmitEopFlip(s32 handle, u32 buf_id, u32 mode, u32 arg, void** unk);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::VideoOut
