// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>
#include <span>
#include <boost/container/static_vector.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "common/assert.h"
#include "common/config.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullability-completeness"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop

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

std::unordered_map<vk::Format, vk::FormatProperties> GetFormatProperties(
    vk::PhysicalDevice physical) {
    std::unordered_map<vk::Format, vk::FormatProperties> format_properties;
    for (const auto& format : LiverpoolToVK::GetAllFormats()) {
        format_properties.emplace(format, physical.getFormatProperties(format));
    }
    return format_properties;
}

std::string GetReadableVersion(u32 version) {
    return fmt::format("{}.{}.{}", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version),
                       VK_VERSION_PATCH(version));
}

} // Anonymous namespace

Instance::Instance(bool enable_validation, bool enable_crash_diagnostic)
    : instance{CreateInstance(Frontend::WindowSystemType::Headless, enable_validation,
                              enable_crash_diagnostic)},
      physical_devices{instance->enumeratePhysicalDevices()} {}

Instance::Instance(Frontend::WindowSDL& window, s32 physical_device_index,
                   bool enable_validation /*= false*/, bool enable_crash_diagnostic /*= false*/)
    : instance{CreateInstance(window.getWindowInfo().type, enable_validation,
                              enable_crash_diagnostic)},
      physical_devices{instance->enumeratePhysicalDevices()} {
    if (enable_validation) {
        debug_callback = CreateDebugCallback(*instance);
    }
    const std::size_t num_physical_devices = static_cast<u16>(physical_devices.size());
    ASSERT_MSG(num_physical_devices > 0, "No physical devices found");
    LOG_INFO(Render_Vulkan, "Found {} physical devices", num_physical_devices);

    if (physical_device_index < 0) {
        std::vector<
            std::tuple<size_t, vk::PhysicalDeviceProperties2, vk::PhysicalDeviceMemoryProperties>>
            properties2{};
        for (auto const& physical : physical_devices) {
            properties2.emplace_back(properties2.size(), physical.getProperties2(),
                                     physical.getMemoryProperties());
        }
        std::sort(properties2.begin(), properties2.end(), [](const auto& left, const auto& right) {
            const vk::PhysicalDeviceProperties& left_prop = std::get<1>(left).properties;
            const vk::PhysicalDeviceProperties& right_prop = std::get<1>(right).properties;
            if (left_prop.apiVersion >= TargetVulkanApiVersion &&
                right_prop.apiVersion < TargetVulkanApiVersion) {
                return true;
            }
            if (left_prop.deviceType != right_prop.deviceType) {
                return left_prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
            }
            constexpr auto get_mem = [](const vk::PhysicalDeviceMemoryProperties& mem) -> size_t {
                size_t max = 0;
                for (u32 i = 0; i < mem.memoryHeapCount; i++) {
                    const vk::MemoryHeap& heap = mem.memoryHeaps[i];
                    if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal && heap.size > max) {
                        max = heap.size;
                    }
                }
                return max;
            };
            size_t left_mem_size = get_mem(std::get<2>(left));
            size_t right_mem_size = get_mem(std::get<2>(right));
            return left_mem_size > right_mem_size;
        });
        physical_device = physical_devices[std::get<0>(properties2[0])];
    } else {
        ASSERT_MSG(physical_device_index < num_physical_devices,
                   "Invalid physical device index {} provided when only {} devices exist",
                   physical_device_index, num_physical_devices);

        physical_device = physical_devices[physical_device_index];
    }

    available_extensions = GetSupportedExtensions(physical_device);
    format_properties = GetFormatProperties(physical_device);
    properties = physical_device.getProperties();
    CollectDeviceParameters();
    ASSERT_MSG(properties.apiVersion >= TargetVulkanApiVersion,
               "Vulkan {}.{} is required, but only {}.{} is supported by device!",
               VK_VERSION_MAJOR(TargetVulkanApiVersion), VK_VERSION_MINOR(TargetVulkanApiVersion),
               VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion));

    CreateDevice();
    CollectToolingInfo();

    // Check and log format support details.
    for (const auto& key : format_properties | std::views::keys) {
        const auto format = key;
        if (!IsImageFormatSupported(format)) {
            const auto alternative = GetAlternativeFormat(format);
            if (IsImageFormatSupported(alternative)) {
                LOG_WARNING(Render_Vulkan,
                            "Format {} is not supported for images, falling back to {}.",
                            vk::to_string(format), vk::to_string(alternative));
            } else if (IsVertexFormatSupported(format)) {
                LOG_WARNING(Render_Vulkan, "Format {} is only supported for vertex buffers.",
                            vk::to_string(format));
            } else {
                LOG_ERROR(Render_Vulkan,
                          "Format {} is not supported and no suitable alternative is supported.",
                          vk::to_string(format));
            }
        }
    }
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
        vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT,
        vk::PhysicalDeviceCustomBorderColorFeaturesEXT,
        vk::PhysicalDeviceColorWriteEnableFeaturesEXT, vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR,
        vk::PhysicalDeviceDepthClipControlFeaturesEXT, vk::PhysicalDeviceRobustness2FeaturesEXT,
        vk::PhysicalDevicePortabilitySubsetFeaturesKHR>();
    const vk::StructureChain properties_chain = physical_device.getProperties2<
        vk::PhysicalDeviceProperties2, vk::PhysicalDevicePortabilitySubsetPropertiesKHR,
        vk::PhysicalDeviceExternalMemoryHostPropertiesEXT, vk::PhysicalDeviceVulkan11Properties,
        vk::PhysicalDevicePushDescriptorPropertiesKHR>();
    subgroup_size = properties_chain.get<vk::PhysicalDeviceVulkan11Properties>().subgroupSize;
    push_descriptor_props = properties_chain.get<vk::PhysicalDevicePushDescriptorPropertiesKHR>();
    LOG_INFO(Render_Vulkan, "Physical device subgroup size {}", subgroup_size);

    features = feature_chain.get().features;
    if (available_extensions.empty()) {
        LOG_CRITICAL(Render_Vulkan, "No extensions supported by device.");
        return false;
    }

    boost::container::static_vector<const char*, 25> enabled_extensions;
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
    shader_stencil_export = add_extension(VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME);
    external_memory_host = add_extension(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
    custom_border_color = add_extension(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
    add_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    depth_clip_control = add_extension(VK_EXT_DEPTH_CLIP_CONTROL_EXTENSION_NAME);
    add_extension(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);
    workgroup_memory_explicit_layout =
        add_extension(VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
    vertex_input_dynamic_state = add_extension(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

    // The next two extensions are required to be available together in order to support write masks
    color_write_en = add_extension(VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);
    color_write_en &= add_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
    const bool calibrated_timestamps = add_extension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
    const bool robustness = add_extension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
    const bool topology_restart =
        add_extension(VK_EXT_PRIMITIVE_TOPOLOGY_LIST_RESTART_EXTENSION_NAME);
    const bool maintenance5 = add_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);

    // These extensions are promoted by Vulkan 1.3, but for greater compatibility we use Vulkan 1.2
    // with extensions.
    tooling_info = add_extension(VK_EXT_TOOLING_INFO_EXTENSION_NAME);
    const bool maintenance4 = add_extension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
    add_extension(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
    add_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    add_extension(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);
    add_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

#ifdef __APPLE__
    // Required by Vulkan spec if supported.
    add_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

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

    const auto vk12_features = feature_chain.get<vk::PhysicalDeviceVulkan12Features>();
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
                .independentBlend = features.independentBlend,
                .geometryShader = features.geometryShader,
                .logicOp = features.logicOp,
                .depthBiasClamp = features.depthBiasClamp,
                .multiViewport = features.multiViewport,
                .samplerAnisotropy = features.samplerAnisotropy,
                .vertexPipelineStoresAndAtomics = features.vertexPipelineStoresAndAtomics,
                .fragmentStoresAndAtomics = features.fragmentStoresAndAtomics,
                .shaderImageGatherExtended = features.shaderImageGatherExtended,
                .shaderStorageImageExtendedFormats = features.shaderStorageImageExtendedFormats,
                .shaderStorageImageMultisample = features.shaderStorageImageMultisample,
                .shaderClipDistance = features.shaderClipDistance,
                .shaderFloat64 = features.shaderFloat64,
                .shaderInt64 = features.shaderInt64,
                .shaderInt16 = features.shaderInt16,
            },
        },
        vk::PhysicalDeviceVulkan11Features{
            .shaderDrawParameters = true,
        },
        vk::PhysicalDeviceVulkan12Features{
            .shaderFloat16 = vk12_features.shaderFloat16,
            .scalarBlockLayout = vk12_features.scalarBlockLayout,
            .uniformBufferStandardLayout = vk12_features.uniformBufferStandardLayout,
            .separateDepthStencilLayouts = vk12_features.separateDepthStencilLayouts,
            .hostQueryReset = vk12_features.hostQueryReset,
            .timelineSemaphore = vk12_features.timelineSemaphore,
        },
        vk::PhysicalDeviceMaintenance4FeaturesKHR{
            .maintenance4 = true,
        },
        vk::PhysicalDeviceMaintenance5FeaturesKHR{
            .maintenance5 = true,
        },
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR{
            .dynamicRendering = true,
        },
        vk::PhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT{
            .shaderDemoteToHelperInvocation = true,
        },
        vk::PhysicalDeviceCustomBorderColorFeaturesEXT{
            .customBorderColors = true,
            .customBorderColorWithoutFormat = true,
        },
        vk::PhysicalDeviceColorWriteEnableFeaturesEXT{
            .colorWriteEnable = true,
        },
        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{
            .extendedDynamicState3ColorWriteMask = true,
        },
        vk::PhysicalDeviceDepthClipControlFeaturesEXT{
            .depthClipControl = true,
        },
        vk::PhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR{
            .workgroupMemoryExplicitLayout = true,
            .workgroupMemoryExplicitLayoutScalarBlockLayout = true,
            .workgroupMemoryExplicitLayout8BitAccess = true,
            .workgroupMemoryExplicitLayout16BitAccess = true,
        },
        vk::PhysicalDeviceRobustness2FeaturesEXT{
            .nullDescriptor = true,
        },
        vk::PhysicalDeviceSynchronization2Features{
            .synchronization2 = true,
        },
        vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT{
            .vertexInputDynamicState = true,
        },
        vk::PhysicalDevicePrimitiveTopologyListRestartFeaturesEXT{
            .primitiveTopologyListRestart = true,
        },
#ifdef __APPLE__
        feature_chain.get<vk::PhysicalDevicePortabilitySubsetFeaturesKHR>(),
#endif
    };

    if (!maintenance4) {
        device_chain.unlink<vk::PhysicalDeviceMaintenance4FeaturesKHR>();
    }
    if (!maintenance5) {
        device_chain.unlink<vk::PhysicalDeviceMaintenance5FeaturesKHR>();
    }
    if (!custom_border_color) {
        device_chain.unlink<vk::PhysicalDeviceCustomBorderColorFeaturesEXT>();
    }
    if (!color_write_en) {
        device_chain.unlink<vk::PhysicalDeviceColorWriteEnableFeaturesEXT>();
        device_chain.unlink<vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>();
    }
    if (!depth_clip_control) {
        device_chain.unlink<vk::PhysicalDeviceDepthClipControlFeaturesEXT>();
    }
    if (!workgroup_memory_explicit_layout) {
        device_chain.unlink<vk::PhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR>();
    }
    if (!topology_restart) {
        device_chain.unlink<vk::PhysicalDevicePrimitiveTopologyListRestartFeaturesEXT>();
    }
    if (robustness) {
        device_chain.get<vk::PhysicalDeviceRobustness2FeaturesEXT>().nullDescriptor =
            feature_chain.get<vk::PhysicalDeviceRobustness2FeaturesEXT>().nullDescriptor;
    } else {
        device_chain.unlink<vk::PhysicalDeviceRobustness2FeaturesEXT>();
    }
    if (!vertex_input_dynamic_state) {
        device_chain.unlink<vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT>();
    }

    try {
        device = physical_device.createDeviceUnique(device_chain.get());
    } catch (vk::ExtensionNotPresentError& err) {
        LOG_CRITICAL(Render_Vulkan, "Some required extensions are not available {}", err.what());
        return false;
    } catch (vk::FeatureNotPresentError& err) {
        LOG_CRITICAL(Render_Vulkan, "Some required features are not available {}", err.what());
        return false;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    graphics_queue = device->getQueue(queue_family_index, 0);
    present_queue = device->getQueue(queue_family_index, 0);

    if (calibrated_timestamps) {
        const auto& time_domains = physical_device.getCalibrateableTimeDomainsEXT();
#if _WIN64
        const bool has_host_time_domain =
            std::find(time_domains.cbegin(), time_domains.cend(),
                      vk::TimeDomainEXT::eQueryPerformanceCounter) != time_domains.cend();
#elif __linux__
        const bool has_host_time_domain =
            std::find(time_domains.cbegin(), time_domains.cend(),
                      vk::TimeDomainEXT::eClockMonotonicRaw) != time_domains.cend();
#else
        // Tracy limitation means only Windows and Linux can use host time domain.
        // https://github.com/shadps4-emu/tracy/blob/c6d779d78508514102fbe1b8eb28bda10d95bb2a/public/tracy/TracyVulkan.hpp#L384-L389
        const bool has_host_time_domain = false;
#endif
        if (has_host_time_domain) {
            static constexpr std::string_view context_name{"vk_rasterizer"};
            profiler_context =
                TracyVkContextHostCalibrated(*instance, physical_device, *device,
                                             VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
                                             VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr);
            TracyVkContextName(profiler_context, context_name.data(), context_name.size());
        }
    }

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

    LOG_INFO(Render_Vulkan, "GPU_Vendor: {}", vendor_name);
    LOG_INFO(Render_Vulkan, "GPU_Model: {}", model_name);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Driver: {}", driver_name);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Version: {}", api_version);
    LOG_INFO(Render_Vulkan, "GPU_Vulkan_Extensions: {}", extensions);
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

bool Instance::IsImageFormatSupported(const vk::Format format) const {
    if (format == vk::Format::eUndefined) [[unlikely]] {
        return true;
    }

    const auto it = format_properties.find(format);
    if (it == format_properties.end()) {
        UNIMPLEMENTED_MSG("Properties of format {} have not been queried.", vk::to_string(format));
    }

    constexpr vk::FormatFeatureFlags optimal_flags = vk::FormatFeatureFlagBits::eTransferSrc |
                                                     vk::FormatFeatureFlagBits::eTransferDst |
                                                     vk::FormatFeatureFlagBits::eSampledImage;
    return (it->second.optimalTilingFeatures & optimal_flags) == optimal_flags;
}

bool Instance::IsVertexFormatSupported(const vk::Format format) const {
    if (format == vk::Format::eUndefined) [[unlikely]] {
        return true;
    }

    const auto it = format_properties.find(format);
    if (it == format_properties.end()) {
        UNIMPLEMENTED_MSG("Properties of format {} have not been queried.", vk::to_string(format));
    }

    constexpr vk::FormatFeatureFlags optimal_flags = vk::FormatFeatureFlagBits::eVertexBuffer;
    return (it->second.bufferFeatures & optimal_flags) == optimal_flags;
}

vk::Format Instance::GetAlternativeFormat(const vk::Format format) const {
    if (format == vk::Format::eB5G6R5UnormPack16) {
        return vk::Format::eR5G6B5UnormPack16;
    } else if (format == vk::Format::eD16UnormS8Uint) {
        return vk::Format::eD24UnormS8Uint;
    }
    return format;
}

vk::Format Instance::GetSupportedFormat(const vk::Format format) const {
    if (IsImageFormatSupported(format)) [[likely]] {
        return format;
    }
    const vk::Format alternative = GetAlternativeFormat(format);
    if (IsImageFormatSupported(alternative)) [[likely]] {
        return alternative;
    }
    return format;
}

vk::ComponentMapping Instance::GetSupportedComponentSwizzle(vk::Format format,
                                                            vk::ComponentMapping swizzle) const {
    if (IsImageFormatSupported(format)) [[likely]] {
        return swizzle;
    }

    vk::ComponentMapping supported_swizzle = swizzle;
    if (format == vk::Format::eB5G6R5UnormPack16) {
        // B5G6R5 -> R5G6B5
        std::swap(supported_swizzle.r, supported_swizzle.b);
    }
    return supported_swizzle;
}

} // namespace Vulkan
