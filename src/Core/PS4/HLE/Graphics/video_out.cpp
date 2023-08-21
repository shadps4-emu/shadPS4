#include "video_out.h"

#include <Core/PS4/HLE/Libs.h>
#include <Util/log.h>
#include <stdio.h>

#include <magic_enum.hpp>
#include <string>

namespace HLE::Libs::Graphics::VideoOut {

constexpr bool log_file_videoout = true;  // disable it to disable logging

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

s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(LibKernel::EventQueues::SceKernelEqueue eq, s32 handle, void* udata) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}

s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex, void* const* addresses, s32 bufferNum,
    const SceVideoOutBufferAttribute* attribute) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex, s32 flipMode, s64 flipArg)
{
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, SceVideoOutFlipStatus* status) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(s32 handle, SceVideoOutResolutionStatus* status) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
}  // namespace HLE::Libs::Graphics::VideoOut