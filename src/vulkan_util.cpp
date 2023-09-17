#include "vulkan_util.h"
#include <Util/log.h>
#include <debug.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

constexpr bool log_file_vulkanutil = true;  // disable it to disable logging

void Graphics::Vulkan::vulkanCreate(Emulator::WindowCtx* ctx) {
    VulkanExt ext;
    vulkanGetExtensions(&ext);

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "shadps4";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "shadps4";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_2;
}

void Graphics::Vulkan::vulkanGetExtensions(VulkanExt* ext) {
    u32 required_extensions_count = 0;
    u32 available_extensions_count = 0;
    u32 available_layers_count = 0;
    auto result = SDL_Vulkan_GetInstanceExtensions(&required_extensions_count, nullptr);

    ext->required_extensions = std::vector<const char*>(required_extensions_count);

    result = SDL_Vulkan_GetInstanceExtensions(&required_extensions_count, ext->required_extensions.data());

    vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);

    ext->available_extensions = std::vector<VkExtensionProperties>(available_extensions_count);

    vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, ext->available_extensions.data());

    vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);
    ext->available_layers = std::vector<VkLayerProperties>(available_layers_count);
    vkEnumerateInstanceLayerProperties(&available_layers_count, ext->available_layers.data());

    for (const char* ext : ext->required_extensions) {
        LOG_INFO_IF(log_file_vulkanutil, "Vulkan required extension  = {}\n", ext);
    }

    for (const auto& ext : ext->available_extensions) {
        LOG_INFO_IF(log_file_vulkanutil, "Vulkan available extension: {}, version = {}\n", ext.extensionName, ext.specVersion);
    }

    for (const auto& l : ext->available_layers) {
        LOG_INFO_IF(log_file_vulkanutil, "Vulkan available layer: {}, specVersion = {}, implVersion = {}, {}\n", l.layerName, l.specVersion,l.implementationVersion,l.description);
    }
}
