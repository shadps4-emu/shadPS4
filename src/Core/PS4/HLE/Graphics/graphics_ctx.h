#pragma once
#include <types.h>
#include <vulkan/vulkan_core.h> 
#include <emulator.h>
#include "Lib/Threads.h"

namespace HLE::Libs::Graphics {

struct VulkanQueueInfo {
    Lib::Mutex* mutex = nullptr;
    u32 family = static_cast<u32>(-1);
    u32 index = static_cast<u32>(-1);
    VkQueue vk_queue = nullptr;
};

struct GraphicCtx {
    u32 screen_width = 0;
    u32 screen_height = 0;
    VkInstance m_instance = nullptr;
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice m_device = nullptr;
    VulkanQueueInfo queues[11]; //VULKAN_QUEUES_NUM
};
}  // namespace HLE::Libs::Graphics