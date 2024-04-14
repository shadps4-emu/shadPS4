// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_descriptor_update_queue.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace Vulkan {

DescriptorUpdateQueue::DescriptorUpdateQueue(const Instance& instance, u32 descriptor_write_max_)
    : device{instance.GetDevice()}, descriptor_write_max{descriptor_write_max_} {
    descriptor_infos = std::make_unique<DescriptorInfoUnion[]>(descriptor_write_max);
    descriptor_writes = std::make_unique<vk::WriteDescriptorSet[]>(descriptor_write_max);
}

void DescriptorUpdateQueue::Flush() {
    if (descriptor_write_end == 0) {
        return;
    }
    device.updateDescriptorSets({std::span(descriptor_writes.get(), descriptor_write_end)}, {});
    descriptor_write_end = 0;
}

void DescriptorUpdateQueue::AddStorageImage(vk::DescriptorSet target, u8 binding,
                                            vk::ImageView image_view,
                                            vk::ImageLayout image_layout) {
    if (descriptor_write_end >= descriptor_write_max) [[unlikely]] {
        Flush();
    }

    auto& image_info = descriptor_infos[descriptor_write_end].image_info;
    image_info.sampler = VK_NULL_HANDLE;
    image_info.imageView = image_view;
    image_info.imageLayout = image_layout;

    descriptor_writes[descriptor_write_end++] = vk::WriteDescriptorSet{
        .dstSet = target,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .pImageInfo = &image_info,
    };
}

void DescriptorUpdateQueue::AddImageSampler(vk::DescriptorSet target, u8 binding, u8 array_index,
                                            vk::ImageView image_view, vk::Sampler sampler,
                                            vk::ImageLayout image_layout) {
    if (descriptor_write_end >= descriptor_write_max) [[unlikely]] {
        Flush();
    }

    auto& image_info = descriptor_infos[descriptor_write_end].image_info;
    image_info.sampler = sampler;
    image_info.imageView = image_view;
    image_info.imageLayout = image_layout;

    descriptor_writes[descriptor_write_end++] = vk::WriteDescriptorSet{
        .dstSet = target,
        .dstBinding = binding,
        .dstArrayElement = array_index,
        .descriptorCount = 1,
        .descriptorType =
            sampler ? vk::DescriptorType::eCombinedImageSampler : vk::DescriptorType::eSampledImage,
        .pImageInfo = &image_info,
    };
}

void DescriptorUpdateQueue::AddBuffer(vk::DescriptorSet target, u8 binding, vk::Buffer buffer,
                                      vk::DeviceSize offset, vk::DeviceSize size,
                                      vk::DescriptorType type) {
    if (descriptor_write_end >= descriptor_write_max) [[unlikely]] {
        Flush();
    }

    auto& buffer_info = descriptor_infos[descriptor_write_end].buffer_info;
    buffer_info.buffer = buffer;
    buffer_info.offset = offset;
    buffer_info.range = size;

    descriptor_writes[descriptor_write_end++] = vk::WriteDescriptorSet{
        .dstSet = target,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &buffer_info,
    };
}

void DescriptorUpdateQueue::AddTexelBuffer(vk::DescriptorSet target, u8 binding,
                                           vk::BufferView buffer_view) {
    if (descriptor_write_end >= descriptor_write_max) [[unlikely]] {
        Flush();
    }

    auto& buffer_info = descriptor_infos[descriptor_write_end].buffer_view;
    buffer_info = buffer_view;
    descriptor_writes[descriptor_write_end++] = vk::WriteDescriptorSet{
        .dstSet = target,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformTexelBuffer,
        .pTexelBufferView = &buffer_info,
    };
}

} // namespace Vulkan
