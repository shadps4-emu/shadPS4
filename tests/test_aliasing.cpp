// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

#include "video_core/amdgpu/resource.h"
#include "video_core/texture_cache/aliasing.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/types.h"

namespace VideoCore {
namespace {

ImageInfo MakeImageInfo(Extent3D size, vk::Format format = vk::Format::eR8G8B8A8Unorm) {
    ImageInfo info{};
    info.guest_address = 0x100000;
    info.num_bits = 32;
    info.pixel_format = format;
    info.type = AmdGpu::ImageType::Color2D;
    info.pitch = size.width;
    info.size = size;
    return info;
}

TEST(AliasingTest, CopiesCoveredImageWithIdenticalFormat) {
    const ImageInfo src = MakeImageInfo({1920, 1088, 1});
    const ImageInfo dst = MakeImageInfo({1920, 1080, 1});

    EXPECT_EQ(GetAliasCopyExtent(src, dst), dst.size);
    EXPECT_EQ(GetAliasCopyExtent(dst, src), dst.size);
}

TEST(AliasingTest, CopiesIntersectionBetweenCompatibleViews) {
    const ImageInfo src = MakeImageInfo({1920, 1088, 1}, vk::Format::eR8G8B8A8Uint);
    const ImageInfo dst = MakeImageInfo({1920, 1080, 1}, vk::Format::eR8G8B8A8Unorm);

    const Extent3D intersection{1920, 1080, 1};
    EXPECT_EQ(GetAliasCopyExtent(src, dst), intersection);
    EXPECT_EQ(GetAliasCopyExtent(dst, src), intersection);
}

TEST(AliasingTest, RejectsDifferentMemoryLayouts) {
    const ImageInfo src = MakeImageInfo({1920, 1088, 1});
    ImageInfo dst = MakeImageInfo({1920, 1080, 1});

    dst.guest_address += 0x1000;
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());

    dst = MakeImageInfo({1920, 1080, 1});
    ++dst.pitch;
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());

    dst = MakeImageInfo({1920, 1080, 1});
    dst.props.is_depth = true;
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());
}

TEST(AliasingTest, RejectsIncompatibleIntersectionCopies) {
    const ImageInfo src = MakeImageInfo({1920, 1088, 1}, vk::Format::eR8G8B8A8Uint);
    ImageInfo dst = MakeImageInfo({1280, 1080, 1}, vk::Format::eR8G8B8A8Unorm);
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());

    dst = MakeImageInfo({1920, 1080, 1}, vk::Format::eR16G16B16A16Sfloat);
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());

    dst = MakeImageInfo({1920, 1080, 1}, vk::Format::eR8G8B8A8Unorm);
    dst.resources.levels = 2;
    EXPECT_FALSE(GetAliasCopyExtent(src, dst).has_value());
}

} // namespace
} // namespace VideoCore
