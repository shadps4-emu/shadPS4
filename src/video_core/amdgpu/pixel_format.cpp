// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include "common/assert.h"
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

std::string_view NameOf(DataFormat fmt) {
    switch (fmt) {
    case DataFormat::FormatInvalid:
        return "FormatInvalid";
    case DataFormat::Format8:
        return "Format8";
    case DataFormat::Format16:
        return "Format16";
    case DataFormat::Format8_8:
        return "Format8_8";
    case DataFormat::Format32:
        return "Format32";
    case DataFormat::Format16_16:
        return "Format16_16";
    case DataFormat::Format10_11_11:
        return "Format10_11_11";
    case DataFormat::Format11_11_10:
        return "Format11_11_10";
    case DataFormat::Format10_10_10_2:
        return "Format10_10_10_2";
    case DataFormat::Format2_10_10_10:
        return "Format2_10_10_10";
    case DataFormat::Format8_8_8_8:
        return "Format8_8_8_8";
    case DataFormat::Format32_32:
        return "Format32_32";
    case DataFormat::Format16_16_16_16:
        return "Format16_16_16_16";
    case DataFormat::Format32_32_32:
        return "Format32_32_32";
    case DataFormat::Format32_32_32_32:
        return "Format32_32_32_32";
    case DataFormat::Format5_6_5:
        return "Format5_6_5";
    case DataFormat::Format1_5_5_5:
        return "Format1_5_5_5";
    case DataFormat::Format5_5_5_1:
        return "Format5_5_5_1";
    case DataFormat::Format4_4_4_4:
        return "Format4_4_4_4";
    case DataFormat::Format8_24:
        return "Format8_24";
    case DataFormat::Format24_8:
        return "Format24_8";
    case DataFormat::FormatX24_8_32:
        return "FormatX24_8_32";
    case DataFormat::FormatGB_GR:
        return "FormatGB_GR";
    case DataFormat::FormatBG_RG:
        return "FormatBG_RG";
    case DataFormat::Format5_9_9_9:
        return "Format5_9_9_9";
    case DataFormat::FormatBc1:
        return "FormatBc1";
    case DataFormat::FormatBc2:
        return "FormatBc2";
    case DataFormat::FormatBc3:
        return "FormatBc3";
    case DataFormat::FormatBc4:
        return "FormatBc4";
    case DataFormat::FormatBc5:
        return "FormatBc5";
    case DataFormat::FormatBc6:
        return "FormatBc6";
    case DataFormat::FormatBc7:
        return "FormatBc7";
    default:
        UNREACHABLE();
    }
}

std::string_view NameOf(NumberFormat fmt) {
    switch (fmt) {
    case NumberFormat::Unorm:
        return "Unorm";
    case NumberFormat::Snorm:
        return "Snorm";
    case NumberFormat::Uscaled:
        return "Uscaled";
    case NumberFormat::Sscaled:
        return "Sscaled";
    case NumberFormat::Uint:
        return "Uint";
    case NumberFormat::Sint:
        return "Sint";
    case NumberFormat::SnormNz:
        return "SnormNz";
    case NumberFormat::Float:
        return "Float";
    case NumberFormat::Srgb:
        return "Srgb";
    case NumberFormat::Ubnorm:
        return "Ubnorm";
    case NumberFormat::UbnormNz:
        return "UbnormNz";
    case NumberFormat::Ubint:
        return "Ubint";
    case NumberFormat::Ubscaled:
        return "Unscaled";
    default:
        UNREACHABLE();
    }
}

static constexpr std::array NUM_COMPONENTS = {
    0, //  0 FormatInvalid
    1, //  1 Format8
    1, //  2 Format16
    2, //  3 Format8_8
    1, //  4 Format32
    2, //  5 Format16_16
    3, //  6 Format10_11_11
    3, //  7 Format11_11_10
    4, //  8 Format10_10_10_2
    4, //  9 Format2_10_10_10
    4, // 10 Format8_8_8_8
    2, // 11 Format32_32
    4, // 12 Format16_16_16_16
    3, // 13 Format32_32_32
    4, // 14 Format32_32_32_32
    0, // 15
    3, // 16 Format5_6_5
    4, // 17 Format1_5_5_5
    4, // 18 Format5_5_5_1
    4, // 19 Format4_4_4_4
    2, // 20 Format8_24
    2, // 21 Format24_8
    2, // 22 FormatX24_8_32
    0, // 23
    0, // 24
    0, // 25
    0, // 26
    0, // 27
    0, // 28
    0, // 29
    0, // 30
    0, // 31
    3, // 32 FormatGB_GR
    3, // 33 FormatBG_RG
    4, // 34 Format5_9_9_9
    4, // 35 FormatBc1
    4, // 36 FormatBc2
    4, // 37 FormatBc3
    1, // 38 FormatBc4
    2, // 39 FormatBc5
    3, // 40 FormatBc6
    4, // 41 FormatBc7
};

u32 NumComponents(DataFormat format) {
    const u32 index = static_cast<u32>(format);
    ASSERT_MSG(index < NUM_COMPONENTS.size(), "Invalid data format = {}", format);
    return NUM_COMPONENTS[index];
}

static constexpr std::array BITS_PER_BLOCK = {
    0,   //  0 FormatInvalid
    8,   //  1 Format8
    16,  //  2 Format16
    16,  //  3 Format8_8
    32,  //  4 Format32
    32,  //  5 Format16_16
    32,  //  6 Format10_11_11
    32,  //  7 Format11_11_10
    32,  //  8 Format10_10_10_2
    32,  //  9 Format2_10_10_10
    32,  // 10 Format8_8_8_8
    64,  // 11 Format32_32
    64,  // 12 Format16_16_16_16
    96,  // 13 Format32_32_32
    128, // 14 Format32_32_32_32
    -1,  // 15
    16,  // 16 Format5_6_5
    16,  // 17 Format1_5_5_5
    16,  // 18 Format5_5_5_1
    16,  // 19 Format4_4_4_4
    32,  // 20 Format8_24
    32,  // 21 Format24_8
    64,  // 22 FormatX24_8_32
    -1,  // 23
    -1,  // 24
    -1,  // 25
    -1,  // 26
    -1,  // 27
    -1,  // 28
    -1,  // 29
    -1,  // 30
    -1,  // 31
    16,  // 32 FormatGB_GR
    16,  // 33 FormatBG_RG
    32,  // 34 Format5_9_9_9
    64,  // 35 FormatBc1
    128, // 36 FormatBc2
    128, // 37 FormatBc3
    64,  // 38 FormatBc4
    128, // 39 FormatBc5
    128, // 40 FormatBc6
    128, // 41 FormatBc7
};

u32 NumBitsPerBlock(DataFormat format) {
    const u32 index = static_cast<u32>(format);
    ASSERT_MSG(index < BITS_PER_BLOCK.size(), "Invalid data format = {}", format);
    return BITS_PER_BLOCK[index];
}

static constexpr std::array BITS_PER_ELEMENT = {
    0,   //  0 FormatInvalid
    8,   //  1 Format8
    16,  //  2 Format16
    16,  //  3 Format8_8
    32,  //  4 Format32
    32,  //  5 Format16_16
    32,  //  6 Format10_11_11
    32,  //  7 Format11_11_10
    32,  //  8 Format10_10_10_2
    32,  //  9 Format2_10_10_10
    32,  // 10 Format8_8_8_8
    64,  // 11 Format32_32
    64,  // 12 Format16_16_16_16
    96,  // 13 Format32_32_32
    128, // 14 Format32_32_32_32
    -1,  // 15
    16,  // 16 Format5_6_5
    16,  // 17 Format1_5_5_5
    16,  // 18 Format5_5_5_1
    16,  // 19 Format4_4_4_4
    32,  // 20 Format8_24
    32,  // 21 Format24_8
    64,  // 22 FormatX24_8_32
    -1,  // 23
    -1,  // 24
    -1,  // 25
    -1,  // 26
    -1,  // 27
    -1,  // 28
    -1,  // 29
    -1,  // 30
    -1,  // 31
    16,  // 32 FormatGB_GR
    16,  // 33 FormatBG_RG
    32,  // 34 Format5_9_9_9
    4,   // 35 FormatBc1
    8,   // 36 FormatBc2
    8,   // 37 FormatBc3
    4,   // 38 FormatBc4
    8,   // 39 FormatBc5
    8,   // 40 FormatBc6
    8,   // 41 FormatBc7
};

u32 NumBitsPerElement(DataFormat format) {
    const u32 index = static_cast<u32>(format);
    ASSERT_MSG(index < BITS_PER_ELEMENT.size(), "Invalid data format = {}", format);
    return BITS_PER_ELEMENT[index];
}

} // namespace AmdGpu
