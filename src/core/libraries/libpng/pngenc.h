// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::PngEnc {

enum class OrbisPngEncAttribute { None = 0 };

enum class OrbisPngEncColorSpace : u16 { RGB = 3, RGBA = 19 };

enum class OrbisPngEncPixelFormat : u16 { R8G8B8A8 = 0, B8G8R8A8 };

enum class OrbisPngEncFilterType : u16 {
    None = 0,
    Sub = 1,
    Up = 2,
    Average = 4,
    Paeth = 8,
    All = 15
};

struct OrbisPngEncCreateParam {
    u32 this_size;
    u32 attribute;
    u32 max_image_width;
    u32 max_filter_number;
};

struct OrbisPngEncEncodeParam {
    const u8* image_mem_addr;
    u8* png_mem_addr;
    u32 image_mem_size;
    u32 png_mem_size;
    u32 image_width;
    u32 image_height;
    u32 image_pitch;
    OrbisPngEncPixelFormat pixel_format;
    OrbisPngEncColorSpace color_space;
    u16 bit_depth;
    u16 clut_number;
    u16 filter_type;
    u16 compression_level;
};

struct OrbisPngEncOutputInfo {
    u32 data_size;
    u32 processed_height;
};

using OrbisPngEncHandle = void*;

s32 PS4_SYSV_ABI scePngEncCreate(const OrbisPngEncCreateParam* param, void* memoryAddress,
                                 u32 memorySize, OrbisPngEncHandle* handle);
s32 PS4_SYSV_ABI scePngEncDelete(OrbisPngEncHandle handle);
s32 PS4_SYSV_ABI scePngEncEncode(OrbisPngEncHandle, const OrbisPngEncEncodeParam* param,
                                 OrbisPngEncOutputInfo* outputInfo);
s32 PS4_SYSV_ABI scePngEncQueryMemorySize(const OrbisPngEncCreateParam* param);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::PngEnc