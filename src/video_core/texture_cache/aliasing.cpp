// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <optional>

#include "video_core/texture_cache/aliasing.h"
#include "video_core/texture_cache/host_compatibility.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/types.h"

namespace VideoCore {

std::optional<Extent3D> GetAliasCopyExtent(const ImageInfo& src, const ImageInfo& dst) {
    const bool same_layout =
        src.guest_address == dst.guest_address && src.num_bits == dst.num_bits &&
        src.num_samples == dst.num_samples && !src.props.is_depth && !dst.props.is_depth &&
        src.props.is_block == dst.props.is_block && src.props.is_pow2 == dst.props.is_pow2 &&
        src.type == dst.type && src.pitch == dst.pitch && src.tile_mode == dst.tile_mode &&
        src.array_mode == dst.array_mode && src.bank_swizzle == dst.bank_swizzle &&
        src.alt_tile == dst.alt_tile;
    if (!same_layout) {
        return std::nullopt;
    }

    if (src.pixel_format == dst.pixel_format && src.resources.levels == 1 &&
        dst.resources.levels == 1 && src.resources.layers >= dst.resources.layers &&
        src.size.width >= dst.size.width && src.size.height >= dst.size.height &&
        src.size.depth >= dst.size.depth) {
        return dst.size;
    }

    const bool can_copy_intersection = !src.props.is_block && src.resources.levels == 1 &&
                                       dst.resources.levels == 1 && src.resources.layers == 1 &&
                                       dst.resources.layers == 1 && src.size.depth == 1 &&
                                       dst.size.depth == 1 && src.size.width == dst.size.width &&
                                       IsVulkanFormatCompatible(src.pixel_format, dst.pixel_format);
    if (!can_copy_intersection) {
        return std::nullopt;
    }

    return Extent3D{
        .width = src.size.width,
        .height = std::min(src.size.height, dst.size.height),
        .depth = 1,
    };
}

} // namespace VideoCore
