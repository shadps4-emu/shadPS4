// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <iostream>
#include <string_view>
#include "video_core/renderer_vulkan/vk_image_transfer.h"

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            std::cerr << "Failed: " #expression << '\n';                                           \
            return 1;                                                                              \
        }                                                                                          \
    } while (false)

int main(int argc, char** argv) {
    using namespace Vulkan::ImageTransfer;
    if (argc != 2) {
        return 2;
    }
    const std::string_view mode{argv[1]};
    if (mode == "formats") {
        CHECK(NeedsDepthBufferCopy(vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint));
        CHECK(NeedsDepthBufferCopy(vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat));
        for (const auto format :
             {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD16Unorm,
              vk::Format::eR32Sfloat, vk::Format::eR8G8B8A8Unorm}) {
            CHECK(!NeedsDepthBufferCopy(format, format));
            CHECK(!NeedsDepthBufferCopy(format, vk::Format::eR32Sfloat));
        }
    } else if (mode == "regions") {
        vk::ImageCopy region{
            .srcSubresource = {vk::ImageAspectFlagBits::eDepth, 2, 1, 3},
            .srcOffset = {4, 8, 0},
            .dstSubresource = {vk::ImageAspectFlagBits::eDepth, 3, 2, 3},
            .dstOffset = {12, 16, 0},
            .extent = {1717, 965, 1},
        };
        const auto copy = MakeDepthBufferCopy(region, 256);
        CHECK(copy.source.imageSubresource == region.srcSubresource);
        CHECK(copy.destination.imageSubresource == region.dstSubresource);
        CHECK(copy.source.imageOffset == region.srcOffset);
        CHECK(copy.destination.imageOffset == region.dstOffset);
        CHECK(copy.source.imageExtent == region.extent);
        CHECK(copy.destination.imageExtent == region.extent);
        CHECK(copy.source.bufferOffset == 256 && copy.destination.bufferOffset == 256);
        CHECK(copy.source.bufferRowLength == 0 && copy.source.bufferImageHeight == 0);
        CHECK(copy.size == 19'882'860);
        CHECK(!(copy.destination.imageSubresource.aspectMask & vk::ImageAspectFlagBits::eStencil));
        region.extent = vk::Extent3D{16384, 16384, 1};
        region.srcSubresource.layerCount = region.dstSubresource.layerCount = 5;
        CHECK(MakeDepthBufferCopy(region, 0).size == 5'368'709'120ULL);
    } else if (mode == "transitions") {
        const vk::Image depth{reinterpret_cast<VkImage>(uintptr_t{1})};
        const vk::Image other{reinterpret_cast<VkImage>(uintptr_t{2})};
        const std::array barriers{vk::ImageMemoryBarrier2{
            .srcStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests,
            .srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
            .oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            .image = depth,
        }};
        // The sampled-depth transition must be emitted before another transition uses its
        // new layout/access state. Independent images can stay in the same barrier batch.
        CHECK(HasPendingTransition(barriers, depth));
        CHECK(!HasPendingTransition(barriers, other));
        CHECK(!HasPendingTransition({}, depth));
    } else {
        return 2;
    }
    return 0;
}
