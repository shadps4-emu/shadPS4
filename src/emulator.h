// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <SDL.h>
#include <core/PS4/HLE/Graphics/graphics_ctx.h>

#include <condition_variable>
#include <mutex>
#include <vector>

namespace Emu {

struct VulkanExt {
    bool enable_validation_layers = false;

    char const* const* required_extensions;
    u32 required_extensions_count;
    std::vector<VkExtensionProperties> available_extensions;
    std::vector<const char*> required_layers;
    std::vector<VkLayerProperties> available_layers;
};

struct VulkanSurfaceCapabilities {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    bool is_format_srgb_bgra32 = false;
    bool is_format_unorm_bgra32 = false;
};

struct VulkanQueueInfo {
    u32 family = 0;
    u32 index = 0;
    bool is_graphics = false;
    bool is_compute = false;
    bool is_transfer = false;
    bool is_present = false;
};

struct VulkanQueues {
    u32 family_count = 0;
    std::vector<VulkanQueueInfo> available;
    std::vector<VulkanQueueInfo> graphics;
    std::vector<VulkanQueueInfo> compute;
    std::vector<VulkanQueueInfo> transfer;
    std::vector<VulkanQueueInfo> present;
    std::vector<u32> family_used;
};

struct VulkanSwapchain {
    VkSwapchainKHR swapchain = nullptr;
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent = {};
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    u32 swapchain_images_count = 0;
    VkSemaphore present_complete_semaphore = nullptr;
    VkFence present_complete_fence = nullptr;
    u32 current_index = 0;
};

struct WindowCtx {
    HLE::Libs::Graphics::GraphicCtx m_graphic_ctx;
    std::mutex m_mutex;
    bool m_is_graphic_initialized = false;
    std::condition_variable m_graphic_initialized_cond;
    SDL_Window* m_window = nullptr;
    bool is_window_hidden = true;
    VkSurfaceKHR m_surface = nullptr;
    VulkanSurfaceCapabilities m_surface_capabilities;
    VulkanSwapchain swapchain;
};

struct EmuPrivate {
    EmuPrivate() = default;
    std::mutex m_mutex;
    HLE::Libs::Graphics::GraphicCtx* m_graphic_ctx = nullptr;
    void* data1 = nullptr;
    void* data2 = nullptr;
    u32 m_screen_width = {0};
    u32 m_screen_height = {0};
};

void emuInit(u32 width, u32 height);
void emuRun();
void checkAndWaitForGraphicsInit();
HLE::Libs::Graphics::GraphicCtx* getGraphicCtx();
void DrawBuffer(HLE::Libs::Graphics::VideoOutVulkanImage* image);
void keyboardEvent(SDL_Event* event);
} // namespace Emu
