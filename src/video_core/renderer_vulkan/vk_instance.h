// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <unordered_map>

#include "video_core/renderer_vulkan/vk_platform.h"

#define TRACY_VK_USE_SYMBOL_TABLE
#include <tracy/TracyVulkan.hpp>

namespace Frontend {
class WindowSDL;
}

VK_DEFINE_HANDLE(VmaAllocator)

namespace Vulkan {

class Instance {
public:
    explicit Instance(bool validation = false, bool crash_diagnostic = false);
    explicit Instance(Frontend::WindowSDL& window, s32 physical_device_index,
                      bool enable_validation = false, bool enable_crash_diagnostic = false);
    ~Instance();

    /// Returns a formatted string for the driver version
    std::string GetDriverVersionName();

    /// Gets a compatibility format if the format is not supported.
    [[nodiscard]] vk::Format GetSupportedFormat(vk::Format format,
                                                vk::FormatFeatureFlags2 flags) const;

    /// Re-orders a component swizzle for format compatibility, if needed.
    [[nodiscard]] vk::ComponentMapping GetSupportedComponentSwizzle(
        vk::Format format, vk::ComponentMapping swizzle, vk::FormatFeatureFlags2 flags) const;

    /// Returns the Vulkan instance
    vk::Instance GetInstance() const {
        return *instance;
    }

    /// Returns the current physical device
    vk::PhysicalDevice GetPhysicalDevice() const {
        return physical_device;
    }

    /// Returns the Vulkan device
    vk::Device GetDevice() const {
        return *device;
    }

    /// Returns the VMA allocator handle
    VmaAllocator GetAllocator() const {
        return allocator;
    }

    /// Returns a list of the available physical devices
    std::span<const vk::PhysicalDevice> GetPhysicalDevices() const {
        return physical_devices;
    }

    /// Retrieve queue information
    u32 GetGraphicsQueueFamilyIndex() const {
        return queue_family_index;
    }

    u32 GetPresentQueueFamilyIndex() const {
        return queue_family_index;
    }

    vk::Queue GetGraphicsQueue() const {
        return graphics_queue;
    }

    vk::Queue GetPresentQueue() const {
        return present_queue;
    }

    TracyVkCtx GetProfilerContext() const {
        return profiler_context;
    }

    /// Returns true when a known debugging tool is attached.
    bool HasDebuggingToolAttached() const {
        return has_renderdoc || has_nsight_graphics;
    }

    /// Returns true if anisotropic filtering is supported
    bool IsAnisotropicFilteringSupported() const {
        return features.samplerAnisotropy;
    }

    /// Returns true when VK_EXT_custom_border_color is supported
    bool IsCustomBorderColorSupported() const {
        return custom_border_color;
    }

    /// Returns true when VK_EXT_fragment_shader_interlock is supported
    bool IsFragmentShaderInterlockSupported() const {
        return fragment_shader_interlock;
    }

    /// Returns true when VK_EXT_pipeline_creation_cache_control is supported
    bool IsPipelineCreationCacheControlSupported() const {
        return pipeline_creation_cache_control;
    }

    /// Returns true when VK_EXT_shader_stencil_export is supported
    bool IsShaderStencilExportSupported() const {
        return shader_stencil_export;
    }

    /// Returns true when VK_EXT_external_memory_host is supported
    bool IsExternalMemoryHostSupported() const {
        return external_memory_host;
    }

    /// Returns true when VK_EXT_depth_clip_control is supported
    bool IsDepthClipControlSupported() const {
        return depth_clip_control;
    }

    /// Returns true when VK_EXT_color_write_enable is supported
    bool IsColorWriteEnableSupported() const {
        return color_write_en;
    }

    /// Returns true when VK_EXT_vertex_input_dynamic_state is supported.
    bool IsVertexInputDynamicState() const {
        return vertex_input_dynamic_state;
    }

    /// Returns true when the nullDescriptor feature of VK_EXT_robustness2 is supported.
    bool IsNullDescriptorSupported() const {
        return null_descriptor;
    }

    /// Returns true when VK_KHR_maintenance5 is supported.
    bool IsMaintenance5Supported() const {
        return maintenance5;
    }

    bool IsListRestartSupported() const {
        return list_restart;
    }

    /// Returns the vendor ID of the physical device
    u32 GetVendorID() const {
        return properties.vendorID;
    }

    /// Returns the device ID of the physical device
    u32 GetDeviceID() const {
        return properties.deviceID;
    }

    /// Returns the driver ID.
    vk::DriverId GetDriverID() const {
        return driver_id;
    }

    /// Returns the current driver version provided in Vulkan-formatted version numbers.
    u32 GetDriverVersion() const {
        return properties.driverVersion;
    }

    /// Returns the current Vulkan API version provided in Vulkan-formatted version numbers.
    u32 ApiVersion() const {
        return properties.apiVersion;
    }

    /// Returns the vendor name reported from Vulkan.
    std::string_view GetVendorName() const {
        return vendor_name;
    }

    /// Returns the list of available extensions.
    std::span<const std::string> GetAvailableExtensions() const {
        return available_extensions;
    }

    /// Returns the device name.
    std::string_view GetModelName() const {
        return properties.deviceName;
    }

    /// Returns the pipeline cache unique identifier
    const auto GetPipelineCacheUUID() const {
        return properties.pipelineCacheUUID;
    }

    /// Returns the minimum required alignment for uniforms
    vk::DeviceSize UniformMinAlignment() const {
        return properties.limits.minUniformBufferOffsetAlignment;
    }

    /// Returns the minimum required alignment for storage buffers
    vk::DeviceSize StorageMinAlignment() const {
        return properties.limits.minStorageBufferOffsetAlignment;
    }

    /// Returns the minimum required alignment for texel buffers
    vk::DeviceSize TexelBufferMinAlignment() const {
        return properties.limits.minTexelBufferOffsetAlignment;
    }

    /// Returns the minimum alignemt required for accessing host-mapped device memory
    vk::DeviceSize NonCoherentAtomSize() const {
        return properties.limits.nonCoherentAtomSize;
    }

    /// Returns the subgroup size of the selected physical device.
    u32 SubgroupSize() const {
        return subgroup_size;
    }

    /// Returns the maximum supported elements in a texel buffer
    u32 MaxTexelBufferElements() const {
        return properties.limits.maxTexelBufferElements;
    }

    /// Returns the maximum number of push descriptors.
    u32 MaxPushDescriptors() const {
        return push_descriptor_props.maxPushDescriptors;
    }

    /// Returns true if shaders can declare the ClipDistance attribute
    bool IsShaderClipDistanceSupported() const {
        return features.shaderClipDistance;
    }

    /// Returns the minimum imported host pointer alignment
    u64 GetMinImportedHostPointerAlignment() const {
        return min_imported_host_pointer_alignment;
    }

    /// Returns the sample count flags supported by framebuffers.
    vk::SampleCountFlags GetFramebufferSampleCounts() const {
        return properties.limits.framebufferColorSampleCounts &
               properties.limits.framebufferDepthSampleCounts &
               properties.limits.framebufferStencilSampleCounts;
    }

private:
    /// Creates the logical device opportunistically enabling extensions
    bool CreateDevice();

    /// Creates the VMA allocator handle
    void CreateAllocator();

    /// Collects telemetry information from the device.
    void CollectDeviceParameters();
    void CollectToolingInfo();

    /// Determines if a format is supported for a set of feature flags.
    [[nodiscard]] bool IsFormatSupported(vk::Format format, vk::FormatFeatureFlags2 flags) const;

private:
    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDevicePushDescriptorPropertiesKHR push_descriptor_props;
    vk::PhysicalDeviceFeatures features;
    vk::DriverIdKHR driver_id;
    vk::UniqueDebugUtilsMessengerEXT debug_callback{};
    std::string vendor_name;
    VmaAllocator allocator{};
    vk::Queue present_queue;
    vk::Queue graphics_queue;
    std::vector<vk::PhysicalDevice> physical_devices;
    std::vector<std::string> available_extensions;
    std::unordered_map<vk::Format, vk::FormatProperties3> format_properties;
    TracyVkCtx profiler_context{};
    u32 queue_family_index{0};
    bool image_view_reinterpretation{true};
    bool timeline_semaphores{};
    bool custom_border_color{};
    bool fragment_shader_interlock{};
    bool pipeline_creation_cache_control{};
    bool fragment_shader_barycentric{};
    bool shader_stencil_export{};
    bool external_memory_host{};
    bool depth_clip_control{};
    bool workgroup_memory_explicit_layout{};
    bool color_write_en{};
    bool vertex_input_dynamic_state{};
    bool null_descriptor{};
    bool maintenance5{};
    bool list_restart{};
    u64 min_imported_host_pointer_alignment{};
    u32 subgroup_size{};
    bool tooling_info{};
    bool debug_utils_supported{};
    bool has_nsight_graphics{};
    bool has_renderdoc{};
};

} // namespace Vulkan
