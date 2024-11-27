// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::JpegEnc {

struct OrbisJpegEncHandleInternal {
    OrbisJpegEncHandleInternal* handle;
    u32 handle_size;
};
static_assert(sizeof(OrbisJpegEncHandleInternal) == 0x10);

typedef OrbisJpegEncHandleInternal* OrbisJpegEncHandle;

struct OrbisJpegEncCreateParam {
    u32 size;
    u32 attr;
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
    u16 pixel_format;
    u16 encode_mode;
    u16 color_space;
    u8 sampling_type;
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

void RegisterlibSceJpegEnc(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::JpegEnc
