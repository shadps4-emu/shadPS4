// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/alignment.h"
#include "shader_recompiler/dispatch_data.h"
#include "video_core/host_shaders/prepare_dispatch_comp.h"
#include "video_core/renderer_vulkan/vk_dispatch_helper.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

struct DispatchHelper::Batch {
    StagingBufferRef storage;
    u64 tick;
    u64 used{};
};

DispatchHelper::DispatchHelper(const Instance& instance_, Scheduler& scheduler_,
                               StagingBufferPool& staging_pool_)
    : instance{instance_}, scheduler{scheduler_}, staging_pool{staging_pool_} {
    const auto limits = instance.GetPhysicalDevice().getProperties().limits;
    max_groups = {limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1],
                  limits.maxComputeWorkGroupCount[2]};
    stride = Common::AlignUp<u64>(
        sizeof(Shader::IndirectDispatchMetadata),
        std::max(limits.minStorageBufferOffsetAlignment, limits.nonCoherentAtomSize));
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{};
    for (u32 i = 0; i < bindings.size(); ++i) {
        bindings[i] =
            vk::DescriptorSetLayoutBinding{.binding = i,
                                           .descriptorType = vk::DescriptorType::eStorageBuffer,
                                           .descriptorCount = 1,
                                           .stageFlags = vk::ShaderStageFlagBits::eCompute};
    }
    const auto device = instance.GetDevice();
    descriptor_layout = Check(device.createDescriptorSetLayoutUnique({
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = u32(bindings.size()),
        .pBindings = bindings.data(),
    }));
    const vk::PushConstantRange push_constants{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = 32,
    };
    pipeline_layout = Check(device.createPipelineLayoutUnique({
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptor_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constants,
    }));
    const auto module = CompileSPV(PREPARE_DISPATCH_COMP, device);
    pipeline = Check(device.createComputePipelineUnique(
        {}, {
                .stage = {.stage = vk::ShaderStageFlagBits::eCompute,
                          .module = module,
                          .pName = "main"},
                .layout = *pipeline_layout,
            }));
    device.destroyShaderModule(module);
}

vk::DescriptorBufferInfo DispatchHelper::Prepare(const VideoCore::Buffer* arguments, u64 offset) {
    constexpr u64 BatchSize = 64_KB;
    ASSERT(stride <= BatchSize && offset % sizeof(u32) == 0);
    if (!batch || batch->tick != scheduler.CurrentTick() || batch->used + stride > BatchSize) {
        batch = std::make_shared<Batch>(Batch{
            .storage =
                staging_pool.Request(BatchSize, VideoCore::MemoryType::HostCached, stride, true),
            .tick = scheduler.CurrentTick(),
        });
        scheduler.DeferOperation([batch = batch, stride = stride, pool = &staging_pool] {
            batch->storage.Invalidate();
            for (u64 offset = 0; offset < batch->used; offset += stride) {
                Shader::IndirectDispatchMetadata data;
                std::memcpy(&data, batch->storage.mapped + offset, sizeof(data));
                ASSERT_MSG(data.status == 0,
                           "Unsupported indirect dispatch {}x{}x{} (preparation status {})",
                           data.guest[0], data.guest[1], data.guest[2], data.status);
            }
            pool->FreeDeferred(batch->storage);
        });
    }
    const u64 destination = batch->storage.offset + batch->used;
    batch->used += stride;
    const u64 source = Common::AlignDown(offset, instance.StorageMinAlignment());
    // prepare_dispatch.comp parameters: the device limits and the word offset of the arguments.
    const std::array<u32, 8> push{max_groups[0], max_groups[1], max_groups[2], 0,
                                  u32((offset - source) / sizeof(u32))};
    const std::array<vk::DescriptorBufferInfo, 2> buffers{{
        {.buffer = arguments->Handle(),
         .offset = source,
         .range = offset - source + 3 * sizeof(u32)},
        {.buffer = batch->storage.buffer->Handle(),
         .offset = destination,
         .range = sizeof(Shader::IndirectDispatchMetadata)},
    }};
    std::array<vk::WriteDescriptorSet, 2> writes{};
    for (u32 i = 0; i < writes.size(); ++i) {
        writes[i] = vk::WriteDescriptorSet{.dstSet = VK_NULL_HANDLE,
                                           .dstBinding = i,
                                           .descriptorCount = 1,
                                           .descriptorType = vk::DescriptorType::eStorageBuffer,
                                           .pBufferInfo = &buffers[i]};
    }
    scheduler.EndRendering();
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0, writes);
    cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push),
                         push.data());
    cmdbuf.dispatch(1, 1, 1);
    const vk::BufferMemoryBarrier2 barrier{
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader |
                        vk::PipelineStageFlagBits2::eDrawIndirect |
                        vk::PipelineStageFlagBits2::eHost,
        .dstAccessMask = vk::AccessFlagBits2::eShaderRead |
                         vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eHostRead,
        .buffer = buffers[1].buffer,
        .offset = destination,
        .size = sizeof(Shader::IndirectDispatchMetadata),
    };
    cmdbuf.pipelineBarrier2({.bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier});
    return buffers[1];
}
} // namespace Vulkan
