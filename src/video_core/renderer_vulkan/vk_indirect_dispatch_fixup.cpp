// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/host_shaders/indirect_dispatch_fixup_comp.h"
#include "video_core/renderer_vulkan/vk_indirect_dispatch_fixup.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

static constexpr u32 CommandSize = sizeof(u32) * 3;

IndirectDispatchFixup::IndirectDispatchFixup(const Instance& instance_) : instance{instance_} {
    const auto device = instance.GetDevice();

    const std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{
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
    }};
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = u32(bindings.size()),
        .pBindings = bindings.data(),
    };
    desc_layout = Check(device.createDescriptorSetLayoutUnique(desc_layout_ci));

    const vk::PushConstantRange push_constant_range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(u32),
    };
    const vk::PipelineLayoutCreateInfo layout_ci = {
        .setLayoutCount = 1,
        .pSetLayouts = &(*desc_layout),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
    };
    pipeline_layout = Check(device.createPipelineLayoutUnique(layout_ci));

    const auto module = CompileSPV(INDIRECT_DISPATCH_FIXUP_COMP, device);
    SetObjectName(device, module, "Indirect Dispatch Fixup");

    const vk::PipelineShaderStageCreateInfo shader_ci = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = module,
        .pName = "main",
    };
    const vk::ComputePipelineCreateInfo pipeline_ci = {
        .stage = shader_ci,
        .layout = *pipeline_layout,
    };
    pipeline = Check(device.createComputePipelineUnique({}, pipeline_ci));
    SetObjectName(device, *pipeline, "Indirect Dispatch Fixup Pipeline");

    device.destroyShaderModule(module);
}

IndirectDispatchFixup::~IndirectDispatchFixup() = default;

std::pair<vk::Buffer, u32> IndirectDispatchFixup::Patch(vk::CommandBuffer cmdbuf,
                                                        VideoCore::BufferCache& buffer_cache,
                                                        vk::Buffer src_buffer, u32 src_offset,
                                                        u32 split_factor) {
    auto& stream_buffer = buffer_cache.GetStreamBuffer();
    const u64 dst_offset =
        stream_buffer.Reserve(CommandSize, instance.StorageMinAlignment(), true).value();
    const vk::Buffer dst_buffer = stream_buffer.Handle();

    const vk::DescriptorBufferInfo src_info = {
        .buffer = src_buffer,
        .offset = src_offset,
        .range = CommandSize,
    };
    const vk::DescriptorBufferInfo dst_info = {
        .buffer = dst_buffer,
        .offset = dst_offset,
        .range = CommandSize,
    };
    const std::array<vk::WriteDescriptorSet, 2> writes = {{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &src_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &dst_info,
        },
    }};

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0,
                         sizeof(split_factor), &split_factor);
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0, writes);
    cmdbuf.dispatch(1, 1, 1);

    const vk::BufferMemoryBarrier2 post_barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect,
        .dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead,
        .buffer = dst_buffer,
        .offset = dst_offset,
        .size = CommandSize,
    };
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &post_barrier,
    });

    return {dst_buffer, u32(dst_offset)};
}

} // namespace Vulkan
