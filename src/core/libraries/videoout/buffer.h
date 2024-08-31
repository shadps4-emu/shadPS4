// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

#include "common/assert.h"
#include "common/types.h"

namespace Libraries::VideoOut {

constexpr std::size_t MaxDisplayBuffers = 16;
constexpr std::size_t MaxDisplayBufferGroups = 4;

enum class PixelFormat : u32 {
    Unknown,
    A8R8G8B8Srgb = 0x80000000,
    A8B8G8R8Srgb = 0x80002200,
    A2R10G10B10 = 0x88060000,
    A2R10G10B10Srgb = 0x88000000,
    A2R10G10B10Bt2020Pq = 0x88740000,
    A16R16G16B16Float = 0xC1060000,
};

enum class TilingMode : s32 {
    Tile = 0,
    Linear = 1,
};

constexpr std::string_view GetPixelFormatString(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8R8G8B8Srgb:
        return "A8R8G8B8Srgb";
    case PixelFormat::A8B8G8R8Srgb:
        return "A8B8G8R8Srgb";
    case PixelFormat::A2R10G10B10:
        return "A2R10G10B10";
    case PixelFormat::A2R10G10B10Srgb:
        return "A2R10G10B10Srgb";
    case PixelFormat::A2R10G10B10Bt2020Pq:
        return "A2R10G10B10Bt2020Pq";
    case PixelFormat::A16R16G16B16Float:
        return "A16R16G16B16Float";
    default:
        UNREACHABLE_MSG("Unknown pixel format {}", static_cast<u32>(format));
        return "";
    }
}

struct BufferAttribute {
    PixelFormat pixel_format;
    TilingMode tiling_mode;
    s32 aspect_ratio;
    u32 width;
    u32 height;
    u32 pitch_in_pixel;
    u32 option;
    u32 reserved0;
    u64 reserved1;
};

struct BufferAttributeGroup {
    bool is_occupied;
    BufferAttribute attrib;
};

struct VideoOutBuffer {
    s32 group_index{-1};
    uintptr_t address_left;
    uintptr_t address_right;
};

} // namespace Libraries::VideoOut
