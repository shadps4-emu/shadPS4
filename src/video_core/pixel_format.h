// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <climits>
#include <utility>
#include "common/types.h"

namespace VideoCore {

// Based on Table 8.13 Data and Image Formats in Sea Islands Series Instruction Set Architecture
enum class PixelFormat : u32 {
    Invalid,
    R32G32B32A32_Float,
    B32G32R32A32_Float,
    R32G32B32X32_Float,
    B32G32R32X32_Float,
    R32G32B32A32_Uint,
    R32G32B32A32_Sint,
    R32G32B32_Float,
    R32G32B32_Uint,
    R32G32B32_Sint,
    R16G16B16A16_Float,
    R16G16B16X16_Float,
    B16G16R16X16_Float,
    R16G16B16A16_Uint,
    R16G16B16A16_Sint,
    R16G16B16A16_Unorm,
    B16G16R16A16_Unorm,
    R16G16B16X16_Unorm,
    B16G16R16X16_Unorm,
    R16G16B16A16_Snorm,
    L32A32_Float,
    R32G32_Float,
    R32G32_Uint,
    R32G32_Sint,
    R11G11B10_Float,
    R8G8B8A8_Unorm,
    R8G8B8X8_Unorm,
    R8G8B8A8_UnormSrgb,
    R8G8B8X8_UnormSrgb,
    R8G8B8A8_Uint,
    R8G8B8A8_Snorm,
    R8G8B8A8_Sint,
    L16A16_Float,
    R16G16_Float,
    L16A16_Unorm,
    R16G16_Unorm,
    R16G16_Uint,
    R16G16_Snorm,
    R16G16_Sint,
    R32_Float,
    L32_Float,
    A32_Float,
    R32_Uint,
    R32_Sint,
    R8G8_Unorm,
    R8G8_Uint,
    R8G8_Snorm,
    R8G8_Sint,
    L8A8_Unorm,
    L8A8_UnormSrgb,
    R16_Float,
    L16_Float,
    A16_Float,
    R16_Unorm,
    L16_Unorm,
    A16_Unorm,
    R16_Uint,
    R16_Snorm,
    R16_Sint,
    R8_Unorm,
    L8_Unorm,
    L8_UnormSrgb,
    R8_Uint,
    R8_Snorm,
    R8_Sint,
    A8_Unorm,
};

constexpr bool IsDepthStencilFormat(PixelFormat format) {
    return false;
}

} // namespace VideoCore
