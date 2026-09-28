// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/kernel/time.h"
#include "qt_gui/screenshot.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"
#include "video_core/texture_cache/types.h"

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include "common/singleton.h"
#include "common/stb.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace VideoCore {

static std::filesystem::path screenshot_dir;

bool CaptureScreenshot(Vulkan::Rasterizer& rasterizer, const std::filesystem::path& output_dir,
                       const std::string& filename) {
    try {
        auto& texture_cache = rasterizer.GetTextureCache();
        auto& scheduler = rasterizer.GetScheduler();
        auto& runtime = rasterizer.GetRuntime();
        const auto& instance = rasterizer.GetInstance();
        const VmaAllocator allocator = instance.GetAllocator();

        const auto image_id = rasterizer.GetCurrentColorBuffer(0);
        if (!image_id) {
            LOG_ERROR(Render, "Screenshot failed: No active color buffer found.");
            return false;
        }

        auto& image = texture_cache.GetImage(image_id);
        const u32 width = image.info.size.width;
        const u32 height = image.info.size.height;

        if (width == 0 || height == 0) {
            LOG_ERROR(Render, "Screenshot failed: Invalid image dimensions.");
            return false;
        }

        std::filesystem::path save_dir = output_dir.empty() ? screenshot_dir : output_dir;
        if (save_dir.empty()) {
            save_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "screenshots";
        }
        std::filesystem::create_directories(save_dir);

        std::string screenshot_filename = filename;
        if (screenshot_filename.empty()) {
            auto now = std::chrono::system_clock::now();
            screenshot_filename = fmt::format("{:%Y-%m-%d_%H-%M-%S}", now);
        }
        if (!screenshot_filename.ends_with(".png")) {
            screenshot_filename += ".png";
        }
        const auto full_path = save_dir / screenshot_filename;

        const size_t image_size = width * height * 4;
        const vk::BufferCreateInfo buffer_info{
            .size = image_size,
            .usage = vk::BufferUsageFlagBits::eTransferDst,
        };

        VmaAllocationCreateInfo alloc_info{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            .requiredFlags =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        vk::Buffer staging_buffer;
        VmaAllocation allocation;
        VkBufferCreateInfo buffer_ci = static_cast<VkBufferCreateInfo>(buffer_info);

        VkResult result =
            vmaCreateBuffer(allocator, &buffer_ci, &alloc_info,
                            reinterpret_cast<VkBuffer*>(&staging_buffer), &allocation, nullptr);

        if (result != VK_SUCCESS) {
            LOG_ERROR(Render, "Failed to create staging buffer for screenshot");
            return false;
        }
        scheduler.EndRendering();
        auto cmdbuf = scheduler.CommandBuffer();

        const VideoCore::SubresourceRange range{
            .base = {.level = 0, .layer = 0},
            .extent = image.info.resources,
        };

        runtime.Transit(&image, vk::ImageLayout::eTransferSrcOptimal,
                        vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead,
                        range);
        runtime.FlushBarriers();

        const vk::BufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        };

        cmdbuf.copyImageToBuffer(image.GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                                 staging_buffer, region);

        rasterizer.Flush();
        rasterizer.Finish();

        void* mapped_data = nullptr;
        result = vmaMapMemory(allocator, allocation, &mapped_data);

        if (result == VK_SUCCESS) {
            std::vector<u8> pixel_data(image_size);
            std::memcpy(pixel_data.data(), mapped_data, image_size);
            vmaUnmapMemory(allocator, allocation);
            for (size_t i = 0; i < pixel_data.size(); i += 4) {
                std::swap(pixel_data[i], pixel_data[i + 2]);
            }

            LOG_INFO(Render, "Saving screenshot to: {}", full_path.string());
            stbi_write_png(full_path.string().c_str(), width, height, 4, pixel_data.data(),
                           width * 4);
        }

        vmaDestroyBuffer(allocator, staging_buffer, allocation);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(Render, "Exception during screenshot capture: {}", e.what());
        return false;
    }
}
void SetScreenshotDir(const std::filesystem::path& path) {
    screenshot_dir = path;
    std::filesystem::create_directories(screenshot_dir);
    LOG_INFO(Render, "Screenshot directory set to: {}", screenshot_dir.string());
}

} // namespace VideoCore
