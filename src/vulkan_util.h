#pragma once
#include <SDL.h>
#include <src/video/khronos/vulkan/vulkan_core.h>

#include <vector>

#include "emulator.h"

namespace Graphics::Vulkan {

void vulkanCreate(Emulator::WindowCtx* ctx);
void vulkanGetInstanceExtensions(Emulator::VulkanExt* ext);
void vulkanFindCompatiblePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions,
                                       Emulator::VulkanSurfaceCapabilities* out_capabilities, VkPhysicalDevice* out_device, Emulator::VulkanQueues* out_queues);
Emulator::VulkanQueues vulkanFindQueues(VkPhysicalDevice device, VkSurfaceKHR surface);
};  // namespace Graphics::Vulkan