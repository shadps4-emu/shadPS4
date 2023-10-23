#pragma once
#include <Core/PS4/HLE/Graphics/graphics_render.h>
#include <SDL.h>
#include <src/video/khronos/vulkan/vulkan_core.h>

#include <vector>

#include "emulator.h"

namespace Graphics::Vulkan {

constexpr int VULKAN_QUEUES_NUM = 11;  // Total of the above
constexpr int VULKAN_QUEUE_GRAPHICS_NUM = 1;
constexpr int VULKAN_QUEUE_TRANSFER_NUM = 1;
constexpr int VULKAN_QUEUE_PRESENT_NUM = 1;
constexpr int VULKAN_QUEUE_COMPUTE_NUM = 8;

constexpr int VULKAN_QUEUE_GFX = 8;
constexpr int VULKAN_QUEUE_UTIL = 9;
constexpr int VULKAN_QUEUE_PRESENT = 10;

template <typename T>
const T& clamp(const T& x, const T& min, const T& max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

void vulkanCreate(Emu::WindowCtx* ctx);
void vulkanGetInstanceExtensions(Emu::VulkanExt* ext);
void vulkanFindCompatiblePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions,
                                        Emu::VulkanSurfaceCapabilities* out_capabilities, VkPhysicalDevice* out_device,
                                        Emu::VulkanQueues* out_queues);
VkDevice vulkanCreateDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const Emu::VulkanExt* r,
                            const Emu::VulkanQueues& queues, const std::vector<const char*>& device_extensions);
Emu::VulkanQueues vulkanFindQueues(VkPhysicalDevice device, VkSurfaceKHR surface);
void vulkanGetSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface, Emu::VulkanSurfaceCapabilities* surfaceCap);
void vulkanCreateQueues(HLE::Libs::Graphics::GraphicCtx* ctx, const Emu::VulkanQueues& queues);
Emu::VulkanSwapchain* vulkanCreateSwapchain(HLE::Libs::Graphics::GraphicCtx* ctx, u32 image_count);
void vulkanBlitImage(GPU::CommandBuffer* buffer, HLE::Libs::Graphics::VulkanImage* src_image, Emu::VulkanSwapchain* dst_swapchain);
void vulkanFillImage(HLE::Libs::Graphics::GraphicCtx* ctx, HLE::Libs::Graphics::VulkanImage* dst_image, const void* src_data, u64 size, u32 src_pitch,
                     u64 dst_layout);
void vulkanBufferToImage(GPU::CommandBuffer* buffer, HLE::Libs::Graphics::VulkanBuffer* src_buffer, u32 src_pitch,
                         HLE::Libs::Graphics::VulkanImage* dst_image, u64 dst_layout);
void vulkanCreateBuffer(HLE::Libs::Graphics::GraphicCtx* ctx, u64 size, HLE::Libs::Graphics::VulkanBuffer* buffer);
void vulkanDeleteBuffer(HLE::Libs::Graphics::GraphicCtx* ctx, HLE::Libs::Graphics::VulkanBuffer* buffer);
};  // namespace Graphics::Vulkan