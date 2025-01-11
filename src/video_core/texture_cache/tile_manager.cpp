// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/tile_manager.h"

#include "video_core/host_shaders/detilers/macro_32bpp_comp.h"
#include "video_core/host_shaders/detilers/macro_64bpp_comp.h"
#include "video_core/host_shaders/detilers/macro_8bpp_comp.h"
#include "video_core/host_shaders/detilers/micro_128bpp_comp.h"
#include "video_core/host_shaders/detilers/micro_16bpp_comp.h"
#include "video_core/host_shaders/detilers/micro_32bpp_comp.h"
#include "video_core/host_shaders/detilers/micro_64bpp_comp.h"
#include "video_core/host_shaders/detilers/micro_8bpp_comp.h"

// #include <boost/container/static_vector.hpp>
#include <magic_enum/magic_enum.hpp>
#include <vk_mem_alloc.h>

namespace VideoCore {

const DetilerContext* TileManager::GetDetiler(const ImageInfo& info) const {
    const auto bpp = info.num_bits * (info.props.is_block ? 16 : 1);
    switch (info.tiling_mode) {
    case AmdGpu::TilingMode::Texture_MicroTiled:
        switch (bpp) {
        case 8:
            return &detilers[DetilerType::Micro8];
        case 16:
            return &detilers[DetilerType::Micro16];
        case 32:
            return &detilers[DetilerType::Micro32];
        case 64:
            return &detilers[DetilerType::Micro64];
        case 128:
            return &detilers[DetilerType::Micro128];
        default:
            return nullptr;
        }
    case AmdGpu::TilingMode::Texture_Volume:
        switch (bpp) {
        case 8:
            return &detilers[DetilerType::Macro8];
        case 32:
            return &detilers[DetilerType::Macro32];
        case 64:
            return &detilers[DetilerType::Macro64];
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
        HostShaders::MICRO_8BPP_COMP,   HostShaders::MICRO_16BPP_COMP,
        HostShaders::MICRO_32BPP_COMP,  HostShaders::MICRO_64BPP_COMP,
        HostShaders::MICRO_128BPP_COMP, HostShaders::MACRO_8BPP_COMP,
        HostShaders::MACRO_32BPP_COMP,  HostShaders::MACRO_64BPP_COMP,
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
                                                  const ImageInfo& info) {
    if (!info.props.is_tiled) {
        return {in_buffer, in_offset};
    }

    const auto* detiler = GetDetiler(info);
    if (!detiler) {
        if (info.tiling_mode != AmdGpu::TilingMode::Texture_MacroTiled &&
            info.tiling_mode != AmdGpu::TilingMode::Display_MacroTiled &&
            info.tiling_mode != AmdGpu::TilingMode::Depth_MacroTiled) {
            LOG_ERROR(Render_Vulkan, "Unsupported tiled image: {} ({})",
                      vk::to_string(info.pixel_format), NameOf(info.tiling_mode));
        }
        return {in_buffer, in_offset};
    }

    const u32 image_size = info.guest_size;

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
    params.num_levels = info.resources.levels;
    params.pitch0 = info.pitch >> (info.props.is_block ? 2u : 0u);
    params.height = info.size.height;
    if (info.tiling_mode == AmdGpu::TilingMode::Texture_Volume) {
        ASSERT(info.resources.levels == 1);
        const auto tiles_per_row = info.pitch / 8u;
        const auto tiles_per_slice = tiles_per_row * ((info.size.height + 7u) / 8u);
        params.sizes[0] = tiles_per_row;
        params.sizes[1] = tiles_per_slice;
    } else {
        ASSERT(info.resources.levels <= 14);
        std::memset(&params.sizes, 0, sizeof(params.sizes));
        for (int m = 0; m < info.resources.levels; ++m) {
            params.sizes[m] = info.mips_layout[m].size * info.resources.layers +
                              (m > 0 ? params.sizes[m - 1] : 0);
        }
    }

    cmdbuf.pushConstants(*detiler->pl_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(params),
                         &params);

    ASSERT((image_size % 64) == 0);
    const auto bpp = info.num_bits * (info.props.is_block ? 16u : 1u);
    const auto num_tiles = image_size / (64 * (bpp / 8));
    cmdbuf.dispatch(num_tiles, 1, 1);
    return {out_buffer.first, 0};
}

} // namespace VideoCore
