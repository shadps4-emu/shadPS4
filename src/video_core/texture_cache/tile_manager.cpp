// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/buffer_cache/buffer.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/tile_manager.h"

#include "video_core/host_shaders/tiling_comp.h"

#include <magic_enum/magic_enum.hpp>
#include <vk_mem_alloc.h>

namespace VideoCore {

struct TilingInfo {
    u32 bank_swizzle;
    u32 num_slices;
    u32 num_mips;
    std::array<ImageInfo::MipInfo, 16> mips;
};

TileManager::TileManager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                         StreamBuffer& stream_buffer_)
    : instance{instance}, scheduler{scheduler}, stream_buffer{stream_buffer_} {
    const auto device = instance.GetDevice();
    const std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {{
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
        {
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    }};

    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    auto desc_layout_result = device.createDescriptorSetLayoutUnique(desc_layout_ci);
    ASSERT_MSG(desc_layout_result.result == vk::Result::eSuccess,
               "Failed to create descriptor set layout: {}",
               vk::to_string(desc_layout_result.result));
    desc_layout = std::move(desc_layout_result.value);

    const vk::DescriptorSetLayout set_layout = *desc_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 0U,
        .pPushConstantRanges = nullptr,
    };
    auto [layout_result, layout] = device.createPipelineLayoutUnique(layout_info);
    ASSERT_MSG(layout_result == vk::Result::eSuccess, "Failed to create pipeline layout: {}",
               vk::to_string(layout_result));
    pl_layout = std::move(layout);
}

TileManager::~TileManager() = default;

TileManager::ScratchBuffer TileManager::GetScratchBuffer(u32 size) {
    constexpr auto usage =
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

    const vk::BufferCreateInfo buffer_ci = {
        .size = size,
        .usage = usage,
    };

    const VmaAllocationCreateInfo alloc_info{
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VkBuffer buffer;
    VmaAllocation allocation;
    const auto buffer_ci_unsafe = static_cast<VkBufferCreateInfo>(buffer_ci);
    const auto result = vmaCreateBuffer(instance.GetAllocator(), &buffer_ci_unsafe, &alloc_info,
                                        &buffer, &allocation, nullptr);
    ASSERT(result == VK_SUCCESS);
    return {buffer, allocation};
}

vk::Pipeline TileManager::GetTilingPipeline(const ImageInfo& info, bool is_tiler) {
    const u32 pl_id = u32(info.tile_mode) * NUM_BPPS + std::bit_width(info.num_bits) - 4;
    auto& tiling_pipelines = is_tiler ? tilers : detilers;
    if (auto pipeline = *tiling_pipelines[pl_id]; pipeline != VK_NULL_HANDLE) {
        return pipeline;
    }

    const auto device = instance.GetDevice();
    const auto micro_tile_mode = AmdGpu::GetMicroTileMode(info.tile_mode);
    std::vector<std::string> defines = {
        fmt::format("BITS_PER_PIXEL={}", info.num_bits),
        fmt::format("NUM_SAMPLES={}", info.num_samples),
        fmt::format("ARRAY_MODE={}", u32(info.array_mode)),
        fmt::format("MICRO_TILE_MODE={}", u32(micro_tile_mode)),
        fmt::format("MICRO_TILE_THICKNESS={}", AmdGpu::GetMicroTileThickness(info.array_mode)),
    };
    if (AmdGpu::IsMacroTiled(info.array_mode)) {
        const auto macro_tile_mode =
            AmdGpu::CalculateMacrotileMode(info.tile_mode, info.num_bits, info.num_samples);
        const u32 num_banks = AmdGpu::GetNumBanks(macro_tile_mode);
        defines.emplace_back(
            fmt::format("PIPE_CONFIG={}", u32(AmdGpu::GetPipeConfig(info.tile_mode))));
        defines.emplace_back(fmt::format("BANK_WIDTH={}", AmdGpu::GetBankWidth(macro_tile_mode)));
        defines.emplace_back(fmt::format("BANK_HEIGHT={}", AmdGpu::GetBankHeight(macro_tile_mode)));
        defines.emplace_back(fmt::format("NUM_BANKS={}", num_banks));
        defines.emplace_back(fmt::format("NUM_BANK_BITS={}", std::bit_width(num_banks) - 1));
        defines.emplace_back(fmt::format(
            "TILE_SPLIT_BYTES={}", AmdGpu::CalculateTileSplit(info.tile_mode, info.array_mode,
                                                              micro_tile_mode, info.num_bits)));
        defines.emplace_back(
            fmt::format("MACRO_TILE_ASPECT={}", AmdGpu::GetMacrotileAspect(macro_tile_mode)));
    }
    if (is_tiler) {
        defines.emplace_back(fmt::format("IS_TILER=1"));
    }

    const auto& module = Vulkan::Compile(HostShaders::TILING_COMP,
                                         vk::ShaderStageFlagBits::eCompute, device, defines);
    const auto module_name = fmt::format("{}_{} {}", magic_enum::enum_name(info.tile_mode),
                                         info.num_bits, is_tiler ? "tiler" : "detiler");
    LOG_WARNING(Render_Vulkan, "Compiling shader {}", module_name);
    for (const auto& def : defines) {
        LOG_WARNING(Render_Vulkan, "#define {}", def);
    }
    Vulkan::SetObjectName(device, module, module_name);
    const vk::PipelineShaderStageCreateInfo shader_ci = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = module,
        .pName = "main",
    };
    const vk::ComputePipelineCreateInfo compute_pipeline_ci = {
        .stage = shader_ci,
        .layout = *pl_layout,
    };
    auto [result, pipeline] =
        device.createComputePipelineUnique(VK_NULL_HANDLE, compute_pipeline_ci);
    ASSERT_MSG(result == vk::Result::eSuccess, "Detiler pipeline creation failed {}",
               vk::to_string(result));
    tiling_pipelines[pl_id] = std::move(pipeline);
    device.destroyShaderModule(module);
    return *tiling_pipelines[pl_id];
}

TileManager::Result TileManager::DetileImage(vk::Buffer in_buffer, u32 in_offset,
                                             const ImageInfo& info) {
    if (!info.props.is_tiled) {
        return {in_buffer, in_offset};
    }

    TilingInfo params{};
    params.bank_swizzle = info.bank_swizzle;
    params.num_slices = info.props.is_volume ? info.size.depth : info.resources.layers;
    params.num_mips = info.resources.levels;
    for (u32 mip = 0; mip < params.num_mips; ++mip) {
        auto& mip_info = params.mips[mip];
        mip_info = info.mips_layout[mip];
        if (info.props.is_block) {
            mip_info.pitch = std::max((mip_info.pitch + 3) / 4, 1U);
            mip_info.height = std::max((mip_info.height + 3) / 4, 1U);
        }
    }

    const vk::DescriptorBufferInfo params_buffer_info{
        .buffer = stream_buffer.Handle(),
        .offset = stream_buffer.Copy(&params, sizeof(params), instance.UniformMinAlignment()),
        .range = sizeof(params),
    };

    const auto [out_buffer, out_allocation] = GetScratchBuffer(info.guest_size);
    scheduler.DeferOperation([this, out_buffer, out_allocation]() {
        vmaDestroyBuffer(instance.GetAllocator(), out_buffer, out_allocation);
    });

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, GetTilingPipeline(info, false));

    const vk::DescriptorBufferInfo tiled_buffer_info{
        .buffer = in_buffer,
        .offset = in_offset,
        .range = info.guest_size,
    };

    const vk::DescriptorBufferInfo linear_buffer_info{
        .buffer = out_buffer,
        .offset = 0,
        .range = info.guest_size,
    };

    const std::array<vk::WriteDescriptorSet, 3> set_writes = {{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &tiled_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &linear_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &params_buffer_info,
        },
    }};
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pl_layout, 0, set_writes);

    const auto dim_x = (info.guest_size / (info.num_bits / 8)) / 64;
    cmdbuf.dispatch(dim_x, 1, 1);
    return {out_buffer, 0};
}

void TileManager::TileImage(vk::Image in_image, std::span<vk::BufferImageCopy> buffer_copies,
                            vk::Buffer out_buffer, u32 out_offset, const ImageInfo& info) {
    if (!info.props.is_tiled) {
        for (auto& copy : buffer_copies) {
            copy.bufferOffset += out_offset;
        }
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.copyImageToBuffer(in_image, vk::ImageLayout::eTransferSrcOptimal, out_buffer,
                                 buffer_copies);
        return;
    }

    TilingInfo params{};
    params.bank_swizzle = info.bank_swizzle;
    params.num_slices = info.props.is_volume ? info.size.depth : info.resources.layers;
    params.num_mips = static_cast<u32>(buffer_copies.size());
    for (u32 mip = 0; mip < params.num_mips; ++mip) {
        auto& mip_info = params.mips[mip];
        mip_info = info.mips_layout[mip];
        if (info.props.is_block) {
            mip_info.pitch = std::max((mip_info.pitch + 3) / 4, 1U);
            mip_info.height = std::max((mip_info.height + 3) / 4, 1U);
        }
    }

    const vk::DescriptorBufferInfo params_buffer_info{
        .buffer = stream_buffer.Handle(),
        .offset = stream_buffer.Copy(&params, sizeof(params), instance.UniformMinAlignment()),
        .range = sizeof(params),
    };

    const auto [temp_buffer, temp_allocation] = GetScratchBuffer(info.guest_size);
    scheduler.DeferOperation([this, temp_buffer, temp_allocation]() {
        vmaDestroyBuffer(instance.GetAllocator(), temp_buffer, temp_allocation);
    });

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.copyImageToBuffer(in_image, vk::ImageLayout::eTransferSrcOptimal, temp_buffer,
                             buffer_copies);
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, GetTilingPipeline(info, true));

    const vk::DescriptorBufferInfo tiled_buffer_info{
        .buffer = out_buffer,
        .offset = out_offset,
        .range = info.guest_size,
    };

    const vk::DescriptorBufferInfo linear_buffer_info{
        .buffer = temp_buffer,
        .offset = 0,
        .range = info.guest_size,
    };

    const std::array<vk::WriteDescriptorSet, 3> set_writes = {{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &tiled_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &linear_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &params_buffer_info,
        },
    }};
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pl_layout, 0, set_writes);

    const auto dim_x = (info.guest_size / (info.num_bits / 8)) / 64;
    cmdbuf.dispatch(dim_x, 1, 1);
}

} // namespace VideoCore
