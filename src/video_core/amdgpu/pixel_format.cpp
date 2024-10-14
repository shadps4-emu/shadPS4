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
    case NumberFormat::UbnromNz:
        return "UbnormNz";
    case NumberFormat::Ubint:
        return "Ubint";
    case NumberFormat::Ubscaled:
        return "Unscaled";
    default:
        UNREACHABLE();
    }
}

int NumComponents(DataFormat format) {
    constexpr std::array num_components_per_element = {
        0,  1,  1,  2,  1,  2,  3,  3,  4,  4,  4,  2, 4, 3, 4, -1, 3, 4, 4, 4, 2,
        2,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 3, 3, 4, 4,  4, 1, 2, 3, 4,
        -1, -1, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2,  3, 1, 1};

    const u32 index = static_cast<u32>(format);
    if (index >= num_components_per_element.size()) {
        return 0;
    }
    return num_components_per_element[index];
}

int NumBits(DataFormat format) {
    const std::array num_bits_per_element = {
        0,  8,  16, 16, 32, 32, 32, 32, 32, 32, 32, 64, 64, 96, 128, -1, 16, 16, 16, 16, 32,
        32, 64, -1, -1, -1, -1, -1, -1, -1, -1, -1, 16, 16, 32, 4,   8,  8,  4,  8,  8,  8,
        -1, -1, 8,  8,  8,  8,  8,  8,  16, 16, 32, 32, 32, 64, 64,  8,  16, 1,  1};

    const u32 index = static_cast<u32>(format);
    if (index >= num_bits_per_element.size()) {
        return 0;
    }
    return num_bits_per_element[index];
}

static constexpr std::array component_bits = {
    std::array{0, 0, 0, 0},     //  0 FormatInvalid
    std::array{8, 0, 0, 0},     //  1 Format8
    std::array{16, 0, 0, 0},    //  2 Format16
    std::array{8, 8, 0, 0},     //  3 Format8_8
    std::array{32, 0, 0, 0},    //  4 Format32
    std::array{16, 16, 0, 0},   //  5 Format16_16
    std::array{11, 11, 10, 0},  //  6 Format10_11_11
    std::array{10, 11, 11, 0},  //  7 Format11_11_10
    std::array{2, 10, 10, 10},  //  8 Format10_10_10_2
    std::array{10, 10, 10, 2},  //  9 Format2_10_10_10
    std::array{8, 8, 8, 8},     // 10 Format8_8_8_8
    std::array{32, 32, 0, 0},   // 11 Format32_32
    std::array{16, 16, 16, 16}, // 12 Format16_16_16_16
    std::array{32, 32, 32, 0},  // 13 Format32_32_32
    std::array{32, 32, 32, 32}, // 14 Format32_32_32_32
    std::array{0, 0, 0, 0},     // 15
    std::array{5, 6, 5, 0},     // 16 Format5_6_5
    std::array{5, 5, 5, 1},     // 17 Format1_5_5_5
    std::array{1, 5, 5, 5},     // 18 Format5_5_5_1
    std::array{4, 4, 4, 4},     // 19 Format4_4_4_4
    std::array{24, 8, 0, 0},    // 20 Format8_24
    std::array{8, 24, 0, 0},    // 21 Format24_8
    std::array{8, 24, 0, 0},    // 22 FormatX24_8_32
    std::array{0, 0, 0, 0},     // 23
    std::array{0, 0, 0, 0},     // 24
    std::array{0, 0, 0, 0},     // 25
    std::array{0, 0, 0, 0},     // 26
    std::array{0, 0, 0, 0},     // 27
    std::array{0, 0, 0, 0},     // 28
    std::array{0, 0, 0, 0},     // 29
    std::array{0, 0, 0, 0},     // 30
    std::array{0, 0, 0, 0},     // 31
    std::array{0, 0, 0, 0},     // 32 FormatGB_GR
    std::array{0, 0, 0, 0},     // 33 FormatBG_RG
    std::array{0, 0, 0, 0},     // 34 Format5_9_9_9
    std::array{0, 0, 0, 0},     // 35 FormatBc1
    std::array{0, 0, 0, 0},     // 36 FormatBc2
    std::array{0, 0, 0, 0},     // 37 FormatBc3
    std::array{0, 0, 0, 0},     // 38 FormatBc4
    std::array{0, 0, 0, 0},     // 39 FormatBc5
    std::array{0, 0, 0, 0},     // 40 FormatBc6
    std::array{0, 0, 0, 0},     // 41 FormatBc7
};

u32 ComponentBits(DataFormat format, u32 comp) {
    const u32 index = static_cast<u32>(format);
    if (index >= component_bits.size() || comp >= 4) {
        return 0;
    }
    return component_bits[index][comp];
}

static constexpr std::array component_offset = {
    std::array{-1, -1, -1, -1}, //  0 FormatInvalid
    std::array{0, -1, -1, -1},  //  1 Format8
    std::array{0, -1, -1, -1},  //  2 Format16
    std::array{0, 8, -1, -1},   //  3 Format8_8
    std::array{0, -1, -1, -1},  //  4 Format32
    std::array{0, 16, -1, -1},  //  5 Format16_16
    std::array{0, 11, 22, -1},  //  6 Format10_11_11
    std::array{0, 10, 21, -1},  //  7 Format11_11_10
    std::array{0, 2, 12, 22},   //  8 Format10_10_10_2
    std::array{0, 10, 20, 30},  //  9 Format2_10_10_10
    std::array{0, 8, 16, 24},   // 10 Format8_8_8_8
    std::array{0, 32, -1, -1},  // 11 Format32_32
    std::array{0, 16, 32, 48},  // 12 Format16_16_16_16
    std::array{0, 32, 64, -1},  // 13 Format32_32_32
    std::array{0, 32, 64, 96},  // 14 Format32_32_32_32
    std::array{-1, -1, -1, -1}, // 15
    std::array{0, 5, 11, -1},   // 16 Format5_6_5
    std::array{0, 5, 10, 15},   // 17 Format1_5_5_5
    std::array{0, 1, 6, 11},    // 18 Format5_5_5_1
    std::array{0, 4, 8, 12},    // 19 Format4_4_4_4
    std::array{0, 24, -1, -1},  // 20 Format8_24
    std::array{0, 8, -1, -1},   // 21 Format24_8
    std::array{0, 8, -1, -1},   // 22 FormatX24_8_32
    std::array{-1, -1, -1, -1}, // 23
    std::array{-1, -1, -1, -1}, // 24
    std::array{-1, -1, -1, -1}, // 25
    std::array{-1, -1, -1, -1}, // 26
    std::array{-1, -1, -1, -1}, // 27
    std::array{-1, -1, -1, -1}, // 28
    std::array{-1, -1, -1, -1}, // 29
    std::array{-1, -1, -1, -1}, // 30
    std::array{-1, -1, -1, -1}, // 31
    std::array{-1, -1, -1, -1}, // 32 FormatGB_GR
    std::array{-1, -1, -1, -1}, // 33 FormatBG_RG
    std::array{-1, -1, -1, -1}, // 34 Format5_9_9_9
    std::array{-1, -1, -1, -1}, // 35 FormatBc1
    std::array{-1, -1, -1, -1}, // 36 FormatBc2
    std::array{-1, -1, -1, -1}, // 37 FormatBc3
    std::array{-1, -1, -1, -1}, // 38 FormatBc4
    std::array{-1, -1, -1, -1}, // 39 FormatBc5
    std::array{-1, -1, -1, -1}, // 40 FormatBc6
    std::array{-1, -1, -1, -1}, // 41 FormatBc7
};

s32 ComponentOffset(DataFormat format, u32 comp) {
    const u32 index = static_cast<u32>(format);
    if (index >= component_offset.size() || comp >= 4) {
        return -1;
    }
    return component_offset[index][comp];
}

} // namespace AmdGpu
