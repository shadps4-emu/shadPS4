#pragma once
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

void vulkanCreate(Emulator::WindowCtx* ctx);
void vulkanGetInstanceExtensions(Emulator::VulkanExt* ext);
void vulkanFindCompatiblePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions,
                                        Emulator::VulkanSurfaceCapabilities* out_capabilities, VkPhysicalDevice* out_device,
                                        Emulator::VulkanQueues* out_queues);
VkDevice vulkanCreateDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const Emulator::VulkanExt* r,
                            const Emulator::VulkanQueues& queues, const std::vector<const char*>& device_extensions);
Emulator::VulkanQueues vulkanFindQueues(VkPhysicalDevice device, VkSurfaceKHR surface);
void vulkanGetSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface, Emulator::VulkanSurfaceCapabilities* surfaceCap);
void vulkanCreateQueues(HLE::Libs::Graphics::GraphicCtx* ctx, const Emulator::VulkanQueues& queues);
Emulator::VulkanSwapchain* vulkanCreateSwapchain(HLE::Libs::Graphics::GraphicCtx* ctx, u32 image_count);
};  // namespace Graphics::Vulkan