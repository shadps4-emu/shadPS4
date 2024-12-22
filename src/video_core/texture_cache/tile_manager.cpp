// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/tile_manager.h"

#include "video_core/host_shaders/detile_m32x1_comp.h"
#include "video_core/host_shaders/detile_m32x2_comp.h"
#include "video_core/host_shaders/detile_m32x4_comp.h"
#include "video_core/host_shaders/detile_m8x1_comp.h"
#include "video_core/host_shaders/detile_m8x2_comp.h"
#include "video_core/host_shaders/detile_macro32x1_comp.h"
#include "video_core/host_shaders/detile_macro32x2_comp.h"

#include <boost/container/static_vector.hpp>
#include <magic_enum/magic_enum.hpp>
#include <vk_mem_alloc.h>

namespace VideoCore {

static vk::Format DemoteImageFormatForDetiling(vk::Format format) {
    switch (format) {
    case vk::Format::eR8Uint:
    case vk::Format::eR8Unorm:
        return vk::Format::eR8Uint;
    case vk::Format::eR4G4B4A4UnormPack16:
    case vk::Format::eR5G5B5A1UnormPack16:
    case vk::Format::eR8G8Unorm:
    case vk::Format::eR16Sfloat:
    case vk::Format::eR16Unorm:
        return vk::Format::eR8G8Uint;
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR8G8B8A8Snorm:
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Uint:
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eB10G11R11UfloatPack32:
    case vk::Format::eA2B10G10R10UnormPack32:
        return vk::Format::eR32Uint;
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eR32G32Sfloat:
    case vk::Format::eR32G32Uint:
    case vk::Format::eR16G16B16A16Unorm:
    case vk::Format::eR16G16B16A16Uint:
    case vk::Format::eR16G16B16A16Sfloat:
        return vk::Format::eR32G32Uint;
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc5UnormBlock:
    case vk::Format::eBc5SnormBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eBc7UnormBlock:
    case vk::Format::eBc6HUfloatBlock:
    case vk::Format::eR32G32B32A32Uint:
    case vk::Format::eR32G32B32A32Sfloat:
        return vk::Format::eR32G32B32A32Uint;
    default:
        break;
    }

    // Log missing formats only once to avoid spamming the log.
    static constexpr size_t MaxFormatIndex = 256;
    static std::array<bool, MaxFormatIndex> logged_formats{};
    if (const u32 index = u32(format); !logged_formats[index]) {
        LOG_ERROR(Render_Vulkan, "Unexpected format for demotion {}", vk::to_string(format));
        logged_formats[index] = true;
    }
    return format;
}

const DetilerContext* TileManager::GetDetiler(const Image& image) const {
    const auto format = DemoteImageFormatForDetiling(image.info.pixel_format);

    switch (image.info.tiling_mode) {
    case AmdGpu::TilingMode::Texture_MicroTiled:
        switch (format) {
        case vk::Format::eR8Uint:
            return &detilers[DetilerType::Micro8x1];
        case vk::Format::eR8G8Uint:
            return &detilers[DetilerType::Micro8x2];
        case vk::Format::eR32Uint:
            return &detilers[DetilerType::Micro32x1];
        case vk::Format::eR32G32Uint:
            return &detilers[DetilerType::Micro32x2];
        case vk::Format::eR32G32B32A32Uint:
            return &detilers[DetilerType::Micro32x4];
        default:
            return nullptr;
        }
    case AmdGpu::TilingMode::Texture_Volume:
        switch (format) {
        case vk::Format::eR32Uint:
            return &detilers[DetilerType::Macro32x1];
        case vk::Format::eR32G32Uint:
            return &detilers[DetilerType::Macro32x2];
        default:
            return nullptr;
        }
        break;
    default:
        return nullptr;
    }
}

struct DetilerParams {
    u32 num_levels;
    u32 pitch0;
    u32 height;
    u32 sizes[14];
};

TileManager::TileManager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler)
    : instance{instance}, scheduler{scheduler} {
    static const std::array detiler_shaders{
        HostShaders::DETILE_M8X1_COMP,      HostShaders::DETILE_M8X2_COMP,
        HostShaders::DETILE_M32X1_COMP,     HostShaders::DETILE_M32X2_COMP,
        HostShaders::DETILE_M32X4_COMP,     HostShaders::DETILE_MACRO32X1_COMP,
        HostShaders::DETILE_MACRO32X2_COMP,
    };

    boost::container::static_vector<vk::DescriptorSetLayoutBinding, 2> bindings{
        {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    };

    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    auto desc_layout_result = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
    ASSERT_MSG(desc_layout_result.result == vk::Result::eSuccess,
               "Failed to create descriptor set layout: {}",
               vk::to_string(desc_layout_result.result));
    desc_layout = std::move(desc_layout_result.value);

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(DetilerParams),
    };

    for (int pl_id = 0; pl_id < DetilerType::Max; ++pl_id) {
        auto& ctx = detilers[pl_id];

        const auto& module = Vulkan::Compile(
            detiler_shaders[pl_id], vk::ShaderStageFlagBits::eCompute, instance.GetDevice());

        // Set module debug name
        auto module_name = magic_enum::enum_name(static_cast<DetilerType>(pl_id));
        Vulkan::SetObjectName(instance.GetDevice(), module, module_name);

        const vk::PipelineShaderStageCreateInfo shader_ci = {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = module,
            .pName = "main",
        };

        const vk::DescriptorSetLayout set_layout = *desc_layout;
        const vk::PipelineLayoutCreateInfo layout_info = {
            .setLayoutCount = 1U,
            .pSetLayouts = &set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_constants,
        };
        auto [layout_result, layout] = instance.GetDevice().createPipelineLayoutUnique(layout_info);
        ASSERT_MSG(layout_result == vk::Result::eSuccess, "Failed to create pipeline layout: {}",
                   vk::to_string(layout_result));
        ctx.pl_layout = std::move(layout);

        const vk::ComputePipelineCreateInfo compute_pipeline_ci = {
            .stage = shader_ci,
            .layout = *ctx.pl_layout,
        };
        auto result = instance.GetDevice().createComputePipelineUnique(
            /*pipeline_cache*/ {}, compute_pipeline_ci);
        if (result.result == vk::Result::eSuccess) {
            ctx.pl = std::move(result.value);
        } else {
            UNREACHABLE_MSG("Detiler pipeline creation failed!");
        }

        // Once pipeline is compiled, we don't need the shader module anymore
        instance.GetDevice().destroyShaderModule(module);
    }
}

TileManager::~TileManager() = default;

TileManager::ScratchBuffer TileManager::AllocBuffer(u32 size, bool is_storage /*= false*/) {
    const auto usage = vk::BufferUsageFlagBits::eStorageBuffer |
                       (is_storage ? vk::BufferUsageFlagBits::eTransferSrc
                                   : vk::BufferUsageFlagBits::eTransferDst);
    const vk::BufferCreateInfo buffer_ci{
        .size = size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info{
        .flags = !is_storage ? VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                             : static_cast<VmaAllocationCreateFlags>(0),
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = !is_storage ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                     : static_cast<VkMemoryPropertyFlags>(0),
    };

    VkBuffer buffer;
    VmaAllocation allocation;
    const auto buffer_ci_unsafe = static_cast<VkBufferCreateInfo>(buffer_ci);
    const auto result = vmaCreateBuffer(instance.GetAllocator(), &buffer_ci_unsafe, &alloc_info,
                                        &buffer, &allocation, nullptr);
    ASSERT(result == VK_SUCCESS);
    return {buffer, allocation};
}

void TileManager::Upload(ScratchBuffer buffer, const void* data, size_t size) {
    VmaAllocationInfo alloc_info{};
    vmaGetAllocationInfo(instance.GetAllocator(), buffer.second, &alloc_info);
    ASSERT(size <= alloc_info.size);
    void* ptr{};
    const auto result = vmaMapMemory(instance.GetAllocator(), buffer.second, &ptr);
    ASSERT(result == VK_SUCCESS);
    std::memcpy(ptr, data, size);
    vmaUnmapMemory(instance.GetAllocator(), buffer.second);
}

void TileManager::FreeBuffer(ScratchBuffer buffer) {
    vmaDestroyBuffer(instance.GetAllocator(), buffer.first, buffer.second);
}

std::pair<vk::Buffer, u32> TileManager::TryDetile(vk::Buffer in_buffer, u32 in_offset,
                                                  Image& image) {
    if (!image.info.props.is_tiled) {
        return {in_buffer, in_offset};
    }

    const auto* detiler = GetDetiler(image);
    if (!detiler) {
        if (image.info.tiling_mode != AmdGpu::TilingMode::Texture_MacroTiled &&
            image.info.tiling_mode != AmdGpu::TilingMode::Display_MacroTiled &&
            image.info.tiling_mode != AmdGpu::TilingMode::Depth_MacroTiled) {
            LOG_ERROR(Render_Vulkan, "Unsupported tiled image: {} ({})",
                      vk::to_string(image.info.pixel_format), NameOf(image.info.tiling_mode));
        }
        return {in_buffer, in_offset};
    }

    const u32 image_size = image.info.guest_size_bytes;

    // Prepare output buffer
    auto out_buffer = AllocBuffer(image_size, true);
    scheduler.DeferOperation([=, this]() { FreeBuffer(out_buffer); });

    auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *detiler->pl);

    const vk::DescriptorBufferInfo input_buffer_info{
        .buffer = in_buffer,
        .offset = in_offset,
        .range = image_size,
    };

    const vk::DescriptorBufferInfo output_buffer_info{
        .buffer = out_buffer.first,
        .offset = 0,
        .range = image_size,
    };

    std::vector<vk::WriteDescriptorSet> set_writes{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &input_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &output_buffer_info,
        },
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *detiler->pl_layout, 0,
                                set_writes);

    DetilerParams params;
    params.num_levels = image.info.resources.levels;
    params.pitch0 = image.info.pitch >> (image.info.props.is_block ? 2u : 0u);
    params.height = image.info.size.height;
    if (image.info.tiling_mode == AmdGpu::TilingMode::Texture_Volume) {
        ASSERT(image.info.resources.levels == 1);
        ASSERT(image.info.num_bits >= 32);
        const auto tiles_per_row = image.info.pitch / 8u;
        const auto tiles_per_slice = tiles_per_row * ((image.info.size.height + 7u) / 8u);
        params.sizes[0] = tiles_per_row;
        params.sizes[1] = tiles_per_slice;
    } else {

        ASSERT(image.info.resources.levels <= 14);
        std::memset(&params.sizes, 0, sizeof(params.sizes));
        for (int m = 0; m < image.info.resources.levels; ++m) {
            params.sizes[m] = image.info.mips_layout[m].size * image.info.resources.layers +
                              (m > 0 ? params.sizes[m - 1] : 0);
        }
    }

    cmdbuf.pushConstants(*detiler->pl_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(params),
                         &params);

    ASSERT((image_size % 64) == 0);
    const auto bpp = image.info.num_bits * (image.info.props.is_block ? 16u : 1u);
    const auto num_tiles = image_size / (64 * (bpp / 8));
    cmdbuf.dispatch(num_tiles, 1, 1);

    const vk::BufferMemoryBarrier post_barrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eTransferRead,
        .buffer = out_buffer.first,
        .size = image_size,
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                           vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
                           {}, post_barrier, {});

    return {out_buffer.first, 0};
}

} // namespace VideoCore
