// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>

#include "imgui/layer/video_info.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Frontend {
class WindowSDL;
}

namespace AmdGpu {
struct Liverpool;
}

namespace Vulkan {

struct Frame {
    u32 width;
    u32 height;
    VmaAllocation allocation;
    vk::Image image;
    vk::ImageView image_view;
    vk::Fence present_done;
    vk::Semaphore ready_semaphore;
    u64 ready_tick;
};

enum SchedulerType {
    Draw,
    Present,
    CpuFlip,
};

class Rasterizer;

class RendererVulkan {
public:
    explicit RendererVulkan(Frontend::WindowSDL& window, AmdGpu::Liverpool* liverpool);
    ~RendererVulkan();

    Frame* PrepareFrame(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                        VAddr cpu_address, bool is_eop) {
        const auto info = VideoCore::ImageInfo{attribute, cpu_address};
        const auto image_id = texture_cache.FindImage(info);
        texture_cache.UpdateImage(image_id, is_eop ? nullptr : &flip_scheduler);
        auto& image = texture_cache.GetImage(image_id);
        return PrepareFrameInternal(image, is_eop);
    }

    Frame* PrepareBlankFrame(bool is_eop) {
        auto& image = texture_cache.GetImage(VideoCore::NULL_IMAGE_ID);
        return PrepareFrameInternal(image, is_eop);
    }

    VideoCore::Image& RegisterVideoOutSurface(
        const Libraries::VideoOut::BufferAttributeGroup& attribute, VAddr cpu_address) {
        vo_buffers_addr.emplace_back(cpu_address);
        const auto info = VideoCore::ImageInfo{attribute, cpu_address};
        const auto image_id = texture_cache.FindImage(info);
        return texture_cache.GetImage(image_id);
    }

    bool IsVideoOutSurface(const AmdGpu::Liverpool::ColorBuffer& color_buffer) {
        return std::ranges::find_if(vo_buffers_addr, [&](VAddr vo_buffer) {
                   return vo_buffer == color_buffer.Address();
               }) != vo_buffers_addr.end();
    }

    bool ShowSplash(Frame* frame = nullptr);
    void Present(Frame* frame);
    void RecreateFrame(Frame* frame, u32 width, u32 height);

    void FlushDraw() {
        SubmitInfo info{};
        draw_scheduler.Flush(info);
    }

private:
    Frame* PrepareFrameInternal(VideoCore::Image& image, bool is_eop = true);
    Frame* GetRenderFrame();

private:
    Frontend::WindowSDL& window;
    AmdGpu::Liverpool* liverpool;
    Instance instance;
    Scheduler draw_scheduler;
    Scheduler present_scheduler;
    Scheduler flip_scheduler;
    Swapchain swapchain;
    std::unique_ptr<Rasterizer> rasterizer;
    VideoCore::TextureCache& texture_cache;
    vk::UniqueCommandPool command_pool;
    std::vector<Frame> present_frames;
    std::queue<Frame*> free_queue;
    std::mutex free_mutex;
    std::condition_variable free_cv;
    std::condition_variable_any frame_cv;
    std::optional<VideoCore::Image> splash_img;
    std::vector<VAddr> vo_buffers_addr;

    ImGui::Layers::VideoInfo video_info_ui;
};

} // namespace Vulkan
