// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_vulkan.h from Dear ImGui repository

#pragma once

#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"

struct ImDrawData;

namespace ImGui::Vulkan {

struct InitInfo {
    vk::Instance instance;
    vk::PhysicalDevice physical_device;
    vk::Device device;
    uint32_t queue_family;
    vk::Queue queue;
    uint32_t image_count;               // >= 2
    vk::DeviceSize min_allocation_size; // Minimum allocation size
    vk::PipelineCache pipeline_cache;
    uint32_t subpass;
    vk::PipelineRenderingCreateInfoKHR pipeline_rendering_create_info;

    // (Optional) Allocation, Logging
    const vk::AllocationCallbacks* allocator{};
    void (*check_vk_result_fn)(vk::Result err);
};

// Prepare all resources needed for uploading textures
// Caller should clean up the returned data.
struct UploadTextureData {
    vk::Image image;
    vk::ImageView image_view;
    vk::DescriptorSet descriptor_set;
    vk::DeviceMemory image_memory;

    vk::CommandBuffer command_buffer; // Submit to the queue
    vk::Buffer upload_buffer;
    vk::DeviceMemory upload_buffer_memory;

    void Upload();

    void Destroy();
};

vk::DescriptorSet AddTexture(vk::ImageView image_view, vk::ImageLayout image_layout,
                             vk::Sampler sampler = VK_NULL_HANDLE);

UploadTextureData UploadTexture(const void* data, vk::Format format, u32 width, u32 height,
                                size_t size);

void RemoveTexture(vk::DescriptorSet descriptor_set);

bool Init(InitInfo info);
void Shutdown();
void RenderDrawData(ImDrawData& draw_data, vk::CommandBuffer command_buffer,
                    vk::Pipeline pipeline = VK_NULL_HANDLE);

} // namespace ImGui::Vulkan