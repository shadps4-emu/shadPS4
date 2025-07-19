// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::JpegEnc {

enum OrbisJpegEncCreateParamAttributes : u32 { ORBIS_JPEG_ENC_ATTRIBUTE_NONE = 0 };

enum OrbisJpegEncEncodeParamPixelFormat : u16 {
    ORBIS_JPEG_ENC_PIXEL_FORMAT_R8G8B8A8 = 0,
    ORBIS_JPEG_ENC_PIXEL_FORMAT_B8G8R8A8 = 1,
    ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8U8Y8V8 = 10,
    ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8 = 11
};

enum OrbisJpengEncEncodeParamEncodeMode : u16 {
    ORBIS_JPEG_ENC_ENCODE_MODE_NORMAL = 0,
    ORBIS_JPEG_ENC_ENCODE_MODE_MJPEG = 1
};

enum OrbisJpengEncEncodeParamColorSpace : u16 {
    ORBIS_JPEG_ENC_COLOR_SPACE_YCC = 1,
    ORBIS_JPEG_ENC_COLOR_SPACE_GRAYSCALE = 2
};

enum OrbisJpengEncEncodeParamSamplingType : u8 {
    ORBIS_JPEG_ENC_SAMPLING_TYPE_FULL = 0,
    ORBIS_JPEG_ENC_SAMPLING_TYPE_422 = 1,
    ORBIS_JPEG_ENC_SAMPLING_TYPE_420 = 2
};

struct OrbisJpegEncHandleInternal {
    OrbisJpegEncHandleInternal* handle;
    u32 handle_size;
};
static_assert(sizeof(OrbisJpegEncHandleInternal) == 0x10);

typedef OrbisJpegEncHandleInternal* OrbisJpegEncHandle;

struct OrbisJpegEncCreateParam {
    u32 size;
    OrbisJpegEncCreateParamAttributes attr;
};
static_assert(sizeof(OrbisJpegEncCreateParam) == 0x8);

struct OrbisJpegEncEncodeParam {
    void* image;
    void* jpeg;
    u32 image_size;
    u32 jpeg_size;
    u32 image_width;
    u32 image_height;
    u32 image_pitch;
    OrbisJpegEncEncodeParamPixelFormat pixel_format;
    OrbisJpengEncEncodeParamEncodeMode encode_mode;
    OrbisJpengEncEncodeParamColorSpace color_space;
    OrbisJpengEncEncodeParamSamplingType sampling_type;
    u8 compression_ratio;
    s32 restart_interval;
};
static_assert(sizeof(OrbisJpegEncEncodeParam) == 0x30);

struct OrbisJpegEncOutputInfo {
    u32 size;
    u32 height;
};
static_assert(sizeof(OrbisJpegEncOutputInfo) == 0x8);

s32 PS4_SYSV_ABI sceJpegEncCreate(const OrbisJpegEncCreateParam* param, void* memory,
                                  u32 memory_size, OrbisJpegEncHandle* handle);
s32 PS4_SYSV_ABI sceJpegEncDelete(OrbisJpegEncHandle handle);
s32 PS4_SYSV_ABI sceJpegEncEncode(OrbisJpegEncHandle handle, const OrbisJpegEncEncodeParam* param,
                                  OrbisJpegEncOutputInfo* output_info);
s32 PS4_SYSV_ABI sceJpegEncQueryMemorySize(const OrbisJpegEncCreateParam* param);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::JpegEnc
