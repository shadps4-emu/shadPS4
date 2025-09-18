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

    /// Returns true if anisotropic filtering is supported
    bool IsAnisotropicFilteringSupported() const {
        return features.samplerAnisotropy;
    }

    /// Returns true if depth bounds testing is supported
    bool IsDepthBoundsSupported() const {
        return features.depthBounds;
    }

    /// Returns true if 16-bit floats are supported in shaders
    bool IsShaderFloat16Supported() const {
        return vk12_features.shaderFloat16;
    }

    /// Returns true if 64-bit floats are supported in shaders
    bool IsShaderFloat64Supported() const {
        return features.shaderFloat64;
    }

    /// Returns true if 64-bit ints are supported in shaders
    bool IsShaderInt64Supported() const {
        return features.shaderInt64;
    }

    /// Returns true if 16-bit ints are supported in shaders
    bool IsShaderInt16Supported() const {
        return features.shaderInt16;
    }

    /// Returns true if 8-bit ints are supported in shaders
    bool IsShaderInt8Supported() const {
        return vk12_features.shaderInt8;
    }

    /// Returns true if VK_KHR_maintenance8 is supported
    bool IsMaintenance8Supported() const {
        return maintenance_8;
    }

    /// Returns true if VK_EXT_attachment_feedback_loop_layout is supported
    bool IsAttachmentFeedbackLoopLayoutSupported() const {
        return attachment_feedback_loop;
    }

    /// Returns true when VK_EXT_custom_border_color is supported
    bool IsCustomBorderColorSupported() const {
        return custom_border_color;
    }

    /// Returns true when VK_EXT_shader_stencil_export is supported
    bool IsShaderStencilExportSupported() const {
        return shader_stencil_export;
    }

    /// Returns true when VK_EXT_depth_clip_control is supported
    bool IsDepthClipControlSupported() const {
        return depth_clip_control;
    }

    /// Returns true when VK_EXT_depth_clip_enable is supported
    bool IsDepthClipEnableSupported() const {
        return depth_clip_enable;
    }

    /// Returns true when VK_EXT_depth_range_unrestricted is supported
    bool IsDepthRangeUnrestrictedSupported() const {
        return depth_range_unrestricted;
    }

    /// Returns true when the extendedDynamicState3ColorWriteMask feature o
    /// VK_EXT_extended_dynamic_state3 is supported.
    bool IsDynamicColorWriteMaskSupported() const {
        return dynamic_state_3 && dynamic_state_3_features.extendedDynamicState3ColorWriteMask;
    }

    /// Returns true when VK_EXT_vertex_input_dynamic_state is supported.
    bool IsVertexInputDynamicState() const {
        return vertex_input_dynamic_state;
    }

    /// Returns true when the robustBufferAccess2 feature of VK_EXT_robustness2 is supported.
    bool IsRobustBufferAccess2Supported() const {
        return robustness2 && robustness2_features.robustBufferAccess2;
    }

    /// Returns true when the nullDescriptor feature of VK_EXT_robustness2 is supported.
    bool IsNullDescriptorSupported() const {
        return robustness2 && robustness2_features.nullDescriptor;
    }

    /// Returns true when VK_KHR_fragment_shader_barycentric is supported.
    bool IsFragmentShaderBarycentricSupported() const {
        return fragment_shader_barycentric;
    }

    /// Returns true when VK_AMD_shader_explicit_vertex_parameter is supported.
    bool IsAmdShaderExplicitVertexParameterSupported() const {
        return amd_shader_explicit_vertex_parameter;
    }

    /// Returns true when VK_EXT_primitive_topology_list_restart is supported.
    bool IsListRestartSupported() const {
        return list_restart;
    }

    /// Returns true when VK_EXT_legacy_vertex_attributes is supported.
    bool IsLegacyVertexAttributesSupported() const {
        return legacy_vertex_attributes;
    }

    /// Returns true when VK_EXT_provoking_vertex is supported.
    bool IsProvokingVertexSupported() const {
        return provoking_vertex;
    }

    /// Returns true when VK_AMD_shader_image_load_store_lod is supported.
    bool IsImageLoadStoreLodSupported() const {
        return image_load_store_lod;
    }

    /// Returns true when VK_AMD_gcn_shader is supported.
    bool IsAmdGcnShaderSupported() const {
        return amd_gcn_shader;
    }

    /// Returns true when VK_AMD_shader_trinary_minmax is supported.
    bool IsAmdShaderTrinaryMinMaxSupported() const {
        return amd_shader_trinary_minmax;
    }

    /// Returns true when the shaderBufferFloat32AtomicMinMax feature of
    /// VK_EXT_shader_atomic_float2 is supported.
    bool IsShaderAtomicFloatBuffer32MinMaxSupported() const {
        return shader_atomic_float2 &&
               shader_atomic_float2_features.shaderBufferFloat32AtomicMinMax;
    }

    /// Returns true when the shaderImageFloat32AtomicMinMax feature of
    /// VK_EXT_shader_atomic_float2 is supported.
    bool IsShaderAtomicFloatImage32MinMaxSupported() const {
        return shader_atomic_float2 && shader_atomic_float2_features.shaderImageFloat32AtomicMinMax;
    }

    /// Returns true if 64-bit integer atomic operations can be used on buffers
    bool IsBufferInt64AtomicsSupported() const {
        return vk12_features.shaderBufferInt64Atomics;
    }

    /// Returns true if 64-bit integer atomic operations can be used on shared memory
    bool IsSharedInt64AtomicsSupported() const {
        return vk12_features.shaderSharedInt64Atomics;
    }

    /// Returns true when VK_KHR_workgroup_memory_explicit_layout is supported.
    bool IsWorkgroupMemoryExplicitLayoutSupported() const {
        return workgroup_memory_explicit_layout &&
               workgroup_memory_explicit_layout_features.workgroupMemoryExplicitLayout16BitAccess;
    }

    /// Returns true when geometry shaders are supported by the device
    bool IsGeometryStageSupported() const {
        return features.geometryShader;
    }

    /// Returns true when tessellation is supported by the device
    bool IsTessellationSupported() const {
        return features.tessellationShader;
    }

    /// Returns true when tessellation isolines are supported by the device
    bool IsTessellationIsolinesSupported() const {
        return !portability_subset || portability_features.tessellationIsolines;
    }

    /// Returns true when tessellation point mode is supported by the device
    bool IsTessellationPointModeSupported() const {
        return !portability_subset || portability_features.tessellationPointMode;
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

    /// Returns if the device is an integrated GPU.
    bool IsIntegrated() const {
        return properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
    }

    /// Returns the pipeline cache unique identifier
    const auto GetPipelineCacheUUID() const {
        return properties.pipelineCacheUUID;
    }

    /// Returns the minimum required alignment for uniforms
    vk::DeviceSize UniformMinAlignment() const {
        return properties.limits.minUniformBufferOffsetAlignment;
    }

    ///  Returns the maximum size of uniform buffers.
    vk::DeviceSize UniformMaxSize() const {
        return properties.limits.maxUniformBufferRange;
    }

    /// Returns the minimum required alignment for storage buffers
    vk::DeviceSize StorageMinAlignment() const {
        return properties.limits.minStorageBufferOffsetAlignment;
    }

    /// Returns the minimum alignemt required for accessing host-mapped device memory
    vk::DeviceSize NonCoherentAtomSize() const {
        return properties.limits.nonCoherentAtomSize;
    }

    /// Returns the subgroup size of the selected physical device.
    u32 SubgroupSize() const {
        return vk11_props.subgroupSize;
    }

    /// Returns the maximum size of compute shared memory.
    u32 MaxComputeSharedMemorySize() const {
        return properties.limits.maxComputeSharedMemorySize;
    }

    /// Returns the maximum sampler LOD bias.
    float MaxSamplerLodBias() const {
        return properties.limits.maxSamplerLodBias;
    }

    /// Returns the maximum sampler anisotropy.
    float MaxSamplerAnisotropy() const {
        return properties.limits.maxSamplerAnisotropy;
    }

    /// Returns the maximum number of push descriptors.
    u32 MaxPushDescriptors() const {
        return push_descriptor_props.maxPushDescriptors;
    }

    /// Returns the vulkan 1.2 physical device properties.
    const vk::PhysicalDeviceVulkan12Properties& GetVk12Properties() const noexcept {
        return vk12_props;
    }

    /// Returns the memory properties of the physical device.
    const vk::PhysicalDeviceMemoryProperties& GetMemoryProperties() const noexcept {
        return memory_properties;
    }

    /// Returns true if shaders can declare the ClipDistance attribute
    bool IsShaderClipDistanceSupported() const {
        return features.shaderClipDistance;
    }

    /// Returns the maximim viewport width.
    u32 GetMaxViewportWidth() const {
        return properties.limits.maxViewportDimensions[0];
    }

    /// Returns the maximum viewport height.
    u32 GetMaxViewportHeight() const {
        return properties.limits.maxViewportDimensions[1];
    }

    /// Returns the maximum render area width.
    u32 GetMaxFramebufferWidth() const {
        return properties.limits.maxFramebufferWidth;
    }

    /// Returns the maximum render area height.
    u32 GetMaxFramebufferHeight() const {
        return properties.limits.maxFramebufferHeight;
    }

    /// Returns the sample count flags supported by framebuffers.
    vk::SampleCountFlags GetFramebufferSampleCounts() const {
        return properties.limits.framebufferColorSampleCounts &
               properties.limits.framebufferDepthSampleCounts &
               properties.limits.framebufferStencilSampleCounts;
    }

    /// Returns whether disabling primitive restart is supported.
    bool IsPrimitiveRestartDisableSupported() const {
        return driver_id != vk::DriverId::eMoltenvk;
    }

    /// Returns true if logic ops are supported by the device.
    bool IsLogicOpSupported() const {
        return features.logicOp;
    }

    /// Returns whether the device can report memory usage.
    bool CanReportMemoryUsage() const {
        return supports_memory_budget;
    }

    /// Returns the amount of memory used.
    [[nodiscard]] u64 GetDeviceMemoryUsage() const;

    /// Returns the total memory budget available to the device.
    [[nodiscard]] u64 GetTotalMemoryBudget() const {
        return total_memory_budget;
    }

    /// Determines if a format is supported for a set of feature flags.
    [[nodiscard]] bool IsFormatSupported(vk::Format format, vk::FormatFeatureFlags2 flags) const;

private:
    /// Creates the logical device opportunistically enabling extensions
    bool CreateDevice();

    /// Creates the VMA allocator handle
    void CreateAllocator();

    /// Collects various information from the device.
    void CollectDeviceParameters();
    void CollectPhysicalMemoryInfo();
    void CollectToolingInfo() const;

    /// Gets the supported feature flags for a format.
    [[nodiscard]] vk::FormatFeatureFlags2 GetFormatFeatureFlags(vk::Format format) const;

private:
    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    vk::PhysicalDeviceVulkan11Properties vk11_props;
    vk::PhysicalDeviceVulkan12Properties vk12_props;
    vk::PhysicalDevicePushDescriptorPropertiesKHR push_descriptor_props;
    vk::PhysicalDeviceFeatures features;
    vk::PhysicalDeviceVulkan12Features vk12_features;
    vk::PhysicalDevicePortabilitySubsetFeaturesKHR portability_features;
    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT dynamic_state_3_features;
    vk::PhysicalDeviceRobustness2FeaturesEXT robustness2_features;
    vk::PhysicalDeviceShaderAtomicFloat2FeaturesEXT shader_atomic_float2_features;
    vk::PhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR
        workgroup_memory_explicit_layout_features;
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
    bool custom_border_color{};
    bool fragment_shader_barycentric{};
    bool amd_shader_explicit_vertex_parameter{};
    bool depth_clip_control{};
    bool depth_clip_enable{};
    bool dynamic_state_3{};
    bool depth_range_unrestricted{};
    bool vertex_input_dynamic_state{};
    bool robustness2{};
    bool list_restart{};
    bool legacy_vertex_attributes{};
    bool provoking_vertex{};
    bool shader_stencil_export{};
    bool image_load_store_lod{};
    bool amd_gcn_shader{};
    bool amd_shader_trinary_minmax{};
    bool shader_atomic_float2{};
    bool workgroup_memory_explicit_layout{};
    bool portability_subset{};
    bool maintenance_8{};
    bool attachment_feedback_loop{};
    bool supports_memory_budget{};
    u64 total_memory_budget{};
    std::vector<size_t> valid_heaps;
};

} // namespace Vulkan
