#pragma once
#include "emulator.h"
#include <SDL.h>
#include <vector>
#include <src/video/khronos/vulkan/vulkan_core.h>

namespace Graphics::Vulkan {

struct VulkanExt {
    bool enable_validation_layers = false;

    std::vector<const char*> required_extensions;
    std::vector<VkExtensionProperties> available_extensions;
    std::vector<const char*> required_layers;
    std::vector<VkLayerProperties> available_layers;
};

void vulkanCreate(Emulator::WindowCtx *ctx);
void vulkanGetExtensions(VulkanExt* ext);

};