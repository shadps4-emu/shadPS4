// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::JpegDec {

enum class OrbisJpegDecPixelFormat : u16 {
    R8G8B8A8 = 0,
    B8G8R8A8 = 1,
    Y8U8Y8V8 = 10,
    Y8 = 11,
};

enum class OrbisJpegDecDecodeMode : u16 {
    Normal = 0,
    Mjpeg = 1,
};

enum class OrbisJpegDecColorSpace : u16 {
    Ycc = 0,
    Grayscale = 1,
};

enum class OrbisJpegDecDownscaleRatio : u16 {
    Ratio1 = 1,
    Ratio2 = 2,
    Ratio4 = 4,
    Ratio8 = 8,
};

struct OrbisJpegDecHandleInternal {
    OrbisJpegDecHandleInternal* handle;
    u32 handle_size;
    u32 decode_mode;
    u32 max_width;
    u32 stride_or_aux;
};
static_assert(sizeof(OrbisJpegDecHandleInternal) == 0x18);

using OrbisJpegDecHandle = OrbisJpegDecHandleInternal*;

struct OrbisJpegDecCreateParam {
    u32 size;
    u32 decode_mode;
    u32 max_width;
};
static_assert(sizeof(OrbisJpegDecCreateParam) == 0xc);

struct OrbisJpegDecParseParam {
    const void* jpeg_addr;
    u32 jpeg_size;
    OrbisJpegDecDecodeMode decode_mode;
    OrbisJpegDecDownscaleRatio downscale_ratio;
};
static_assert(sizeof(OrbisJpegDecParseParam) == 0x10);

struct OrbisJpegDecImageInfo {
    u32 image_width;
    u32 image_height;
    OrbisJpegDecColorSpace color_space;
    u16 num_components;
    u8 component_id[4];
    u8 sampling_factor[4];
    u32 decode_mode;
    u32 total_samples;
    u32 output_width;
    u32 output_height;
};
static_assert(sizeof(OrbisJpegDecImageInfo) == 0x24);

struct OrbisJpegDecDecodeParam {
    const void* jpeg_addr;
    void* image_addr;
    void* aux_addr;
    u32 jpeg_size;
    u32 image_size;
    u32 aux_size;
    OrbisJpegDecDecodeMode decode_mode;
    OrbisJpegDecDownscaleRatio downscale_ratio;
    OrbisJpegDecPixelFormat pixel_format;
    u16 alpha_value;
    u32 image_pitch;
};
static_assert(sizeof(OrbisJpegDecDecodeParam) == 0x30);

s32 PS4_SYSV_ABI sceJpegDecQueryMemorySize(const OrbisJpegDecCreateParam* param);
s32 PS4_SYSV_ABI sceJpegDecCreate(const OrbisJpegDecCreateParam* param, void* memory,
                                  u32 memory_size, OrbisJpegDecHandle* handle);
s32 PS4_SYSV_ABI sceJpegDecDelete(OrbisJpegDecHandle handle);
s32 PS4_SYSV_ABI sceJpegDecParseHeader(const OrbisJpegDecParseParam* param,
                                       OrbisJpegDecImageInfo* image_info);
s32 PS4_SYSV_ABI sceJpegDecDecode(OrbisJpegDecHandle handle, const OrbisJpegDecDecodeParam* param,
                                  OrbisJpegDecImageInfo* image_info);
s32 PS4_SYSV_ABI sceJpegDecDecodeWithInputControl(OrbisJpegDecHandle handle,
                                                  const OrbisJpegDecDecodeParam* param,
                                                  OrbisJpegDecImageInfo* image_info);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::JpegDec
