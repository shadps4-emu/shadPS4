// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <boost/container/static_vector.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "common/assert.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"

#include <vk_mem_alloc.h>

namespace Vulkan {

namespace {

std::vector<std::string> GetSupportedExtensions(vk::PhysicalDevice physical) {
    const std::vector extensions = physical.enumerateDeviceExtensionProperties();
    std::vector<std::string> supported_extensions;
    supported_extensions.reserve(extensions.size());
    for (const auto& extension : extensions) {
        supported_extensions.emplace_back(extension.extensionName.data());
    }
    return supported_extensions;
}

std::string GetReadableVersion(u32 version) {
    return fmt::format("{}.{}.{}", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version),
                       VK_VERSION_PATCH(version));
}

} // Anonymous namespace

Instance::Instance(bool enable_validation, bool dump_command_buffers)
    : instance{CreateInstance(dl, Frontend::WindowSystemType::Headless, enable_validation,
                              dump_command_buffers)},
      physical_devices{instance->enumeratePhysicalDevices()} {}

Instance::Instance(Frontend::WindowSDL& window, u32 physical_device_index)
    : instance{CreateInstance(dl, window.getWindowInfo().type, true, false)},
      debug_callback{CreateDebugCallback(*instance)}, physical_devices{
                                                          instance->enumeratePhysicalDevices()} {
    const std::size_t num_physical_devices = static_cast<u16>(physical_devices.size());
    ASSERT_MSG(physical_device_index < num_physical_devices,
               "Invalid physical device index {} provided when only {} devices exist",
               physical_device_index, num_physical_devices);

    physical_device = physical_devices[physical_device_index];
    available_extensions = GetSupportedExtensions(physical_device);
    properties = physical_device.getProperties();
    if (properties.apiVersion < TargetVulkanApiVersion) {
        throw std::runtime_error(fmt::format(
            "Vulkan {}.{} is required, but only {}.{} is supported by device!",
            VK_VERSION_MAJOR(TargetVulkanApiVersion), VK_VERSION_MINOR(TargetVulkanApiVersion),
            VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion)));
    }

    CollectDeviceParameters();
    CreateDevice();
    CollectToolingInfo();
}

Instance::~Instance() {
    vmaDestroyAllocator(allocator);
}

std::string Instance::GetDriverVersionName() {
    // Extracted from
    // https://github.com/SaschaWillems/vulkan.gpuinfo.org/blob/5dddea46ea1120b0df14eef8f15ff8e318e35462/functions.php#L308-L314
    const u32 version = properties.driverVersion;
    if (driver_id == vk::DriverId::eNvidiaProprietary) {
        const u32 major = (version >> 22) & 0x3ff;
        const u32 minor = (version >> 14) & 0x0ff;
        const u32 secondary = (version >> 6) & 0x0ff;
        const u32 tertiary = version & 0x003f;
        return fmt::format("{}.{}.{}.{}", major, minor, secondary, tertiary);
    }
    if (driver_id == vk::DriverId::eIntelProprietaryWindows) {
        const u32 major = version >> 14;
        const u32 minor = version & 0x3fff;
        return fmt::format("{}.{}", major, minor);
    }
    return GetReadableVersion(version);
}

bool Instance::CreateDevice() {
    const vk::StructureChain feature_chain = physical_device.getFeatures2<
        vk::PhysicalDeviceFeatures2, vk::PhysicalDevicePortabilitySubsetFeaturesKHR,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT,
        vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR,
        vk::PhysicalDeviceCustomBorderColorFeaturesEXT, vk::PhysicalDeviceIndexTypeUint8FeaturesEXT,
        vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT,
        vk::PhysicalDevicePipelineCreationCacheControlFeaturesEXT,
        vk::PhysicalDeviceFragmentShaderBarycentricFeaturesKHR>();
    const vk::StructureChain properties_chain =
        physical_device.getProperties2<vk::PhysicalDeviceProperties2,
                                       vk::PhysicalDevicePortabilitySubsetPropertiesKHR,
                                       vk::PhysicalDeviceExternalMemoryHostPropertiesEXT>();

    features = feature_chain.get().features;
    if (available_extensions.empty()) {
        LOG_CRITICAL(Render_Vulkan, "No extensions supported by device.");
        return false;
    }

    boost::container::static_vector<const char*, 13> enabled_extensions;
    const auto add_extension = [&](std::string_view extension) -> bool {
        const auto result =
            std::find_if(available_extensions.begin(), available_extensions.end(),
                         [&](const std::string& name) { return name == extension; });

        if (result != available_extensions.end()) {
            LOG_INFO(Render_Vulkan, "Enabling extension: {}", extension);
            enabled_extensions.push_back(extension.data());
            return true;
        }

        LOG_WARNING(Render_Vulkan, "Extension {} unavailable.", extension);
        return false;
    };

    add_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    image_format_list = add_extension(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
    shader_stencil_export = add_extension(VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME);
    external_memory_host = add_extension(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
    tooling_info = add_extension(VK_EXT_TOOLING_INFO_EXTENSION_NAME);
    custom_border_color = add_extension(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
    index_type_uint8 = add_extension(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME);

    const auto family_properties = physical_device.getQueueFamilyProperties();
    if (family_properties.empty()) {
        LOG_CRITICAL(Render_Vulkan, "Physical device reported no queues.");
        return false;
    }

    bool graphics_queue_found = false;
    for (std::size_t i = 0; i < family_properties.size(); i++) {
        const u32 index = static_cast<u32>(i);
        if (family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            queue_family_index = index;
            graphics_queue_found = true;
        }
    }

    if (!graphics_queue_found) {
        LOG_CRITICAL(Render_Vulkan, "Unable to find graphics and/or present queues.");
        return false;
    }

    static constexpr std::array<f32, 1> queue_priorities = {1.0f};

    const vk::DeviceQueueCreateInfo queue_info = {
        .queueFamilyIndex = queue_family_index,
        .queueCount = static_cast<u32>(queue_priorities.size()),
        .pQueuePriorities = queue_priorities.data(),
    };

    vk::StructureChain device_chain = {
        vk::DeviceCreateInfo{
            .queueCreateInfoCount = 1u,
            .pQueueCreateInfos = &queue_info,
            .enabledExtensionCount = static_cast<u32>(enabled_extensions.size()),
            .ppEnabledExtensionNames = enabled_extensions.data(),
        },
        vk::PhysicalDeviceFeatures2{
            .features{
                .robustBufferAccess = features.robustBufferAccess,
                .geometryShader = features.geometryShader,
                .logicOp = features.logicOp,
                .samplerAnisotropy = features.samplerAnisotropy,
                .fragmentStoresAndAtomics = features.fragmentStoresAndAtomics,
                .shaderClipDistance = features.shaderClipDistance,
            },
        },
        vk::PhysicalDeviceVulkan12Features{
            .timelineSemaphore = true,
        },
        vk::PhysicalDeviceCustomBorderColorFeaturesEXT{
            .customBorderColors = true,
            .customBorderColorWithoutFormat = true,
        },
        vk::PhysicalDeviceIndexTypeUint8FeaturesEXT{
            .indexTypeUint8 = true,
        },
    };

    if (!index_type_uint8) {
        device_chain.unlink<vk::PhysicalDeviceIndexTypeUint8FeaturesEXT>();
    }

    try {
        device = physical_device.createDeviceUnique(device_chain.get());
    } catch (vk::ExtensionNotPresentError& err) {
        LOG_CRITICAL(Render_Vulkan, "Some required extensions are not available {}", err.what());
        return false;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    graphics_queue = device->getQueue(queue_family_index, 0);
    present_queue = device->getQueue(queue_family_index, 0);

    CreateAllocator();
    return true;
}

void Instance::CreateAllocator() {
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
    };

    const VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = physical_device,
        .device = *device,
        .pVulkanFunctions = &functions,
        .instance = *instance,
        .vulkanApiVersion = TargetVulkanApiVersion,
    };

    const VkResult result = vmaCreateAllocator(&allocator_info, &allocator);
    if (result != VK_SUCCESS) {
        UNREACHABLE_MSG("Failed to initialize VMA with error {}",
                        vk::to_string(vk::Result{result}));
    }
}

void Instance::CollectDeviceParameters() {
    const vk::StructureChain property_chain =
        physical_device
            .getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDriverProperties>();
    const vk::PhysicalDeviceDriverProperties driver =
        property_chain.get<vk::PhysicalDeviceDriverProperties>();

    driver_id = driver.driverID;
    vendor_name = driver.driverName.data();

    const std::string model_name{GetModelName()};
    const std::string driver_version = GetDriverVersionName();
    const std::string driver_name = fmt::format("{} {}", vendor_name, driver_version);
    const std::string api_version = GetReadableVersion(properties.apiVersion);
    const std::string extensions = fmt::format("{}", fmt::join(available_extensions, ", "));

    LOG_INFO(Render_Vulkan, "GPU_Vendor", vendor_name);
    LOG_INFO(Render_Vulkan, "GPU_Model", model_name);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Driver", driver_name);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Version", api_version);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Extensions", extensions);
}

void Instance::CollectToolingInfo() {
    if (!tooling_info) {
        return;
    }
    const auto tools = physical_device.getToolPropertiesEXT();
    for (const vk::PhysicalDeviceToolProperties& tool : tools) {
        const std::string_view name = tool.name;
        LOG_INFO(Render_Vulkan, "Attached debugging tool: {}", name);
        has_renderdoc = has_renderdoc || name == "RenderDoc";
        has_nsight_graphics = has_nsight_graphics || name == "NVIDIA Nsight Graphics";
    }
}

} // namespace Vulkan
