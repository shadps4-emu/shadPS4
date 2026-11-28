// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/types.h"
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

inline CompSwizzle Channel(u32 channel) {
    return CompSwizzle(u32(CompSwizzle::Red) + channel);
}

constexpr bool IsChannel(CompSwizzle swizzle) {
    return swizzle >= CompSwizzle::Red;
}

inline u32 ChannelIndex(CompSwizzle swizzle) {
    return u32(swizzle) - u32(CompSwizzle::Red);
}

// clang-format off
inline constexpr std::array<std::array<CompMapping, 4>, 4> COMP_SWAP_LUT{{
    // Standard
    std::array<CompMapping, 4>{{
        {.r = CompSwizzle::Red, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Alpha},
    }},
    // Alternate
    std::array<CompMapping, 4>{{
        {.r = CompSwizzle::Green, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Red, .g = CompSwizzle::Alpha, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Alpha, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Alpha},
    }},
    // StandardReverse
    std::array<CompMapping, 4>{{
        {.r = CompSwizzle::Blue, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Green, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Alpha, .g = CompSwizzle::Blue, .b = CompSwizzle::Green, .a = CompSwizzle::Red},
    }},
    // AlternateReverse
    std::array<CompMapping, 4>{{
        {.r = CompSwizzle::Alpha, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Alpha, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
        {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Green, .a = CompSwizzle::Blue},
    }},
}};
// clang-format on

enum class CompSwap : u32 {
    Standard = 0,
    Alternate = 1,
    StandardReverse = 2,
    AlternateReverse = 3,
};

inline CompMapping CompSwapMapping(CompSwap swap, u32 num_comps) {
    num_comps = num_comps ? num_comps : 1;
    return COMP_SWAP_LUT[u32(swap)][num_comps - 1];
}

/// Normalizes a raw dst_sel for formatted loads.
inline CompMapping DstSelLoad(CompMapping dst_sel, u32 num_comps) {
    num_comps = num_comps ? num_comps : 1;
    CompMapping mapping;
    for (u32 k = 0; k < 4; ++k) {
        CompSwizzle out = dst_sel.Get(k);
        if (IsChannel(out)) {
            out = Channel(ChannelIndex(out) % num_comps);
        } else if (out == CompSwizzle::Reserved3) {
            out = num_comps == 1 ? CompSwizzle::Red : CompSwizzle::Zero;
        }
        mapping.Set(k, out);
    }
    return mapping;
}

/// COMP_SWAP mode that a formatted store derives from dst_sel.
inline CompSwap GetCompSwap(CompMapping dst_sel, u32 num_components) {
    const bool x_is_x = dst_sel.r == CompSwizzle::Red;
    const bool y_used = IsChannel(dst_sel.g);
    const bool z_used = IsChannel(dst_sel.b);
    switch (num_components) {
    case 1:
        return x_is_x ? CompSwap::Standard : CompSwap::AlternateReverse;
    case 2:
        if (y_used && !z_used) {
            return x_is_x ? CompSwap::Standard : CompSwap::StandardReverse;
        }
        return x_is_x || !z_used ? CompSwap::Alternate : CompSwap::AlternateReverse;
    case 3:
        return CompSwap::Standard;
    default:
        return dst_sel.g == CompSwizzle::Green ? CompSwap::Standard : CompSwap::StandardReverse;
    }
}

/// Normalizes a raw dst_sel for formatted stores.
inline CompMapping DstSelStore(CompMapping dst_sel, u32 num_comps, bool is_format_x_op) {
    num_comps = num_comps ? num_comps : 1;
    CompMapping swizzle{};
    if (num_comps == 1 && is_format_x_op) {
        const u32 s = u32(dst_sel.r);
        swizzle.r =
            IsChannel(dst_sel.r) ? CompSwizzle::Red : (s == 3 ? CompSwizzle::Zero : CompSwizzle(s));
        return swizzle;
    }
    const CompSwap swap = GetCompSwap(dst_sel, num_comps);
    const CompMapping& slot = COMP_SWAP_LUT[u32(swap)][num_comps - 1];
    if (num_comps <= 2) {
        return slot;
    }
    const CompMapping& comp = COMP_SWAP_LUT[u32(swap)][3];
    for (u32 c = 0; c < num_comps; ++c) {
        const auto s = dst_sel.Get(ChannelIndex(slot.Get(c)));
        CompSwizzle out = CompSwizzle::Zero;
        if (IsChannel(s)) {
            out = comp.Get(ChannelIndex(s));
        } else if (s == CompSwizzle::One || s == CompSwizzle::RawOne) {
            out = CompSwizzle(s);
        } else if (s == CompSwizzle::Reserved3 && swap == CompSwap::Standard) {
            out = CompSwizzle::Alpha;
        }
        swizzle.Set(c, out);
    }
    return swizzle;
}

inline std::array<u8, 4> HostChannelOrder(DataFormat format) {
    switch (format) {
    case DataFormat::Format5_6_5:    // R5G6B5: X is B
    case DataFormat::Format1_5_5_5:  // A1R5G5B5: X is B
    case DataFormat::Format11_11_10: // Emulated as 10_11_11 with X/Z exchanged
        return {2, 1, 0, 3};
    case DataFormat::Format10_10_10_2: // Emulated as 2_10_10_10 with channels reversed
    case DataFormat::Format5_5_5_1:    // Emulated as 1_5_5_5 with channels reversed
        return {3, 2, 1, 0};
    case DataFormat::Format4_4_4_4: // B4G4R4A4: X is A, Y is R, Z is G, W is B
        return {3, 0, 1, 2};
    default:
        return {0, 1, 2, 3};
    }
}

inline CompMapping RemapLoadSwizzle(DataFormat format, CompMapping mapping) {
    const auto order = HostChannelOrder(format);
    CompMapping result = mapping;
    for (u32 k = 0; k < 4; ++k) {
        const CompSwizzle s = mapping.Get(k);
        if (IsChannel(s)) {
            result.Set(k, Channel(order[ChannelIndex(s)]));
        }
    }
    return result;
}

inline CompMapping RemapStoreSwizzle(DataFormat format, CompMapping mapping) {
    const auto order = HostChannelOrder(format);
    CompMapping result = mapping;
    for (u32 c = 0; c < 4; ++c) {
        result.Set(order[c], mapping.Get(c));
    }
    return result;
}

/// Replaces RawOne for use as an image view component mapping, which has no equivalent for it.
inline CompMapping ImageViewMapping(CompMapping m, bool is_integer) {
    for (u32 k = 0; k < 4; ++k) {
        if (m.Get(k) == CompSwizzle::RawOne) {
            m.Set(k, is_integer ? CompSwizzle::One : CompSwizzle::Zero);
        }
    }
    return m;
}

} // namespace AmdGpu
