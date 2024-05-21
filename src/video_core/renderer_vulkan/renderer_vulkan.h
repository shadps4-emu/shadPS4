// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
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
    vk::Semaphore render_ready;
    vk::Fence present_done;
    vk::CommandBuffer cmdbuf;
};

class Rasterizer;

class RendererVulkan {
public:
    explicit RendererVulkan(Frontend::WindowSDL& window, AmdGpu::Liverpool* liverpool);
    ~RendererVulkan();

    Frame* PrepareFrame(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                        VAddr cpu_address);

    bool ShowSplash(Frame* frame = nullptr);
    void Present(Frame* frame);
    void RecreateFrame(Frame* frame, u32 width, u32 height);

private:
    Frame* PrepareFrameInternal(VideoCore::Image& image);
    Frame* GetRenderFrame();

private:
    Frontend::WindowSDL& window;
    Instance instance;
    Scheduler scheduler;
    Swapchain swapchain;
    std::unique_ptr<Rasterizer> rasterizer;
    VideoCore::TextureCache texture_cache;
    vk::UniqueCommandPool command_pool;
    std::vector<Frame> present_frames;
    std::queue<Frame*> free_queue;
    std::mutex free_mutex;
    std::condition_variable free_cv;
    std::condition_variable_any frame_cv;
    std::optional<VideoCore::Image> splash_img;
};

} // namespace Vulkan
