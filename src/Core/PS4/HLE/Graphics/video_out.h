#pragma once
#include <types.h>

#include <string>

namespace HLE::Libs::Graphics::VideoOut {

struct SceVideoOutBufferAttribute {
    s32 pixelFormat;
    s32 tilingMode;
    s32 aspectRatio;
    u32 width;
    u32 height;
    u32 pitchInPixel;
    u32 option;
    u32 reserved0;
    u64 reserved1;
};

constexpr int PIXEL_FORMAT_A8R8G8B8_SRGB = 0x80000000;
constexpr int PIXEL_FORMAT_A8B8G8R8_SRGB = 0x80002200;
constexpr int PIXEL_FORMAT_A2R10G10B10 = 0x88060000;
constexpr int PIXEL_FORMAT_A2R10G10B10_SRGB = 0x88000000;
constexpr int PIXEL_FORMAT_A2R10G10B10_BT2020_PQ = 0x88740000;
constexpr int PIXEL_FORMAT_A16R16G16B16_FLOAT = 0xC1060000;
constexpr int PIXEL_FORMAT_YCBCR420_BT709 = 0x08322200;

constexpr int BUFFER_ATTRIBUTE_OPTION_NONE = 0;
constexpr int BUFFER_ATTRIBUTE_OPTION_VR = 7;
constexpr int BUFFER_ATTRIBUTE_OPTION_STRICT_COLORIMETRY = 8;

enum SceVideoOutTilingMode : s32 { TILING_MODE_TILE = 0, TILING_MODE_LINEAR = 1 };

enum AspectRatioMode : s32 { ASPECT_RATIO_16_9 = 0 };

std::string getPixelFormatString(s32 format);

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(SceVideoOutBufferAttribute* attribute, u32 pixelFormat, u32 tilingMode, u32 aspectRatio, u32 width,
                                                u32 height, u32 pitchInPixel);
}  // namespace HLE::Libs::Graphics::VideoOut