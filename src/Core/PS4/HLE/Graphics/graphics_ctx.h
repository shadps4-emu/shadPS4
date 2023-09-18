#pragma once
#include <types.h>
#include <vulkan/vulkan_core.h> 

namespace HLE::Libs::Graphics {
struct GraphicCtx {
    u32 screen_width = 0;
    u32 screen_height = 0;
    VkInstance m_instance = nullptr;
    VkPhysicalDevice physical_device = nullptr;
};
}  // namespace HLE::Libs::Graphics