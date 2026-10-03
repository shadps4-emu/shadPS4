// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define ASSERT(expression)                                                                         \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            throw std::runtime_error(#expression);                                                 \
        }                                                                                          \
    } while (false)

using u32 = std::uint32_t;
using ImageId = u32;
constexpr bool False(u32 value) {
    return value == 0;
}

enum class BindingType { Texture, Storage, RenderTarget, DepthTarget };
enum ImageFlagBits : u32 { Dirty = 7, GpuModified = 8 };

struct SubresourceExtent {
    u32 levels = 1;
    u32 layers = 1;
    auto operator<=>(const SubresourceExtent&) const = default;
};

struct ImageInfo {
    struct {
        bool is_depth = false;
        bool has_stencil = false;
    } props;
    struct {
        u32 htile_clear_mask = ~0u;
    } meta_info;
    SubresourceExtent resources;
    u32 num_bits = 32;
    u32 num_samples = 1;
    u32 pixel_format = 100;
    std::uint64_t guest_address = 0x10000;
    void UpdateSize() {}
};

struct Image {
    ImageInfo info;
    u32 usage = 0;
    u32 flags = Dirty;
    bool freed = false;
    std::vector<std::vector<int>> pixels;

    explicit Image(const ImageInfo& info_) : info{info_} {
        pixels.resize(info.resources.levels, std::vector<int>(info.resources.layers, -1));
    }
};

struct Runtime {
    u32 copies = 0;
    void CopyColorAndDepth(Image* src, Image* dst) {
        ++copies;
        for (u32 mip = 0; mip < std::min(src->info.resources.levels, dst->info.resources.levels);
             ++mip) {
            for (u32 layer = 0;
                 layer < std::min(src->info.resources.layers, dst->info.resources.layers);
                 ++layer) {
                dst->pixels[mip][layer] = src->pixels[mip][layer];
            }
        }
        dst->flags |= src->flags & GpuModified;
        dst->flags &= ~Dirty;
    }
};

struct ImageSlots {
    std::deque<Image> images;
    ImageId Insert(int, Runtime&, int, const ImageInfo& info) {
        images.emplace_back(info);
        return static_cast<ImageId>(images.size());
    }
    Image& operator[](ImageId id) {
        return images.at(id - 1);
    }
};

struct TextureCache {
    ImageSlots slot_images;
    Runtime runtime;
    int instance = 0;
    int slot_image_views = 0;
    u32 uploads = 0;

    void RegisterImage(ImageId) {}
    void FreeImage(ImageId id) {
        slot_images[id].freed = true;
    }
    void RefreshImage(Image& image) {
        if (!(image.flags & Dirty) || image.info.num_samples > 1) {
            return;
        }
        ++uploads;
        for (u32 mip = 0; mip < image.info.resources.levels; ++mip) {
            for (u32 layer = 0; layer < image.info.resources.layers; ++layer) {
                image.pixels[mip][layer] = 100 + 10 * mip + layer;
            }
        }
        image.flags &= ~Dirty;
    }
    ImageId ResolveDepthOverlap(const ImageInfo&, BindingType, ImageId);
};

#include "depth_overlap_body.inc"

void CheckGrowth(const std::string& name, SubresourceExtent old_extent,
                 SubresourceExtent new_extent, bool source_depth) {
    TextureCache cache;
    ImageInfo old_info;
    old_info.props.is_depth = source_depth;
    old_info.pixel_format = source_depth ? 126 : 100;
    old_info.resources = old_extent;
    const auto old_id = cache.slot_images.Insert(0, cache.runtime, 0, old_info);
    auto& old = cache.slot_images[old_id];
    old.usage = 42;
    old.flags = GpuModified;
    for (u32 mip = 0; mip < old_extent.levels; ++mip) {
        for (u32 layer = 0; layer < old_extent.layers; ++layer) {
            old.pixels[mip][layer] = 1000 + 10 * mip + layer;
        }
    }
    ImageInfo requested = old_info;
    requested.props.is_depth = true;
    requested.pixel_format = 126;
    requested.resources = new_extent;
    const auto id = cache.ResolveDepthOverlap(requested, BindingType::DepthTarget, old_id);
    ASSERT(id != old_id);
    const auto& result = cache.slot_images[id];
    for (u32 mip = 0; mip < new_extent.levels; ++mip) {
        for (u32 layer = 0; layer < new_extent.layers; ++layer) {
            const int expected =
                (mip < old_extent.levels && layer < old_extent.layers ? 1000 : 100) + 10 * mip +
                layer;
            if (result.pixels[mip][layer] != expected) {
                throw std::runtime_error(name + ": mip=" + std::to_string(mip) +
                                         " layer=" + std::to_string(layer) +
                                         " expected=" + std::to_string(expected) +
                                         " actual=" + std::to_string(result.pixels[mip][layer]));
            }
        }
    }
    ASSERT(result.usage == old.usage);
    ASSERT(result.flags & GpuModified);
    ASSERT(!(result.flags & Dirty));
    ASSERT(result.info.meta_info.htile_clear_mask == 0);
    ASSERT(old.freed);
    ASSERT(cache.runtime.copies == 1);
    ASSERT(cache.uploads == (new_extent == old_extent ? 0u : 1u));
    std::cout << "PASS " << name << '\n';
}

int main() {
    try {
        CheckGrowth("logged R32 five layers to D32 six layers", {1, 5}, {1, 6}, false);
        CheckGrowth("depth layer expansion", {1, 2}, {1, 6}, true);
        CheckGrowth("depth mip expansion", {1, 6}, {2, 6}, true);
        CheckGrowth("depth mip and layer expansion", {1, 2}, {2, 6}, true);
        CheckGrowth("same-size format conversion preserves GPU data without upload", {1, 6}, {1, 6},
                    false);
        TextureCache cache;
        ImageInfo info;
        const auto id = cache.slot_images.Insert(0, cache.runtime, 0, info);
        ASSERT(cache.ResolveDepthOverlap(info, BindingType::Texture, id) == 0);
        cache.slot_images[id].info.props.is_depth = info.props.is_depth = true;
        ASSERT(cache.ResolveDepthOverlap(info, BindingType::DepthTarget, id) == id);
        ASSERT(cache.uploads == 0 && cache.runtime.copies == 0);
        ASSERT(!cache.slot_images[id].freed);
        std::cout << "PASS unchanged cache entries\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
    return 0;
}
