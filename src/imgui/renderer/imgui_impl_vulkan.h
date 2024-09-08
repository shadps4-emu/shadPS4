// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_vulkan.h from Dear ImGui repository

#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
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

vk::DescriptorSet AddTexture(vk::Sampler sampler, vk::ImageView image_view,
                             vk::ImageLayout image_layout);

void RemoveTexture(vk::DescriptorSet descriptor_set);

bool Init(InitInfo info);
void Shutdown();
void NewFrame();
void RenderDrawData(ImDrawData& draw_data, vk::CommandBuffer command_buffer,
                    vk::Pipeline pipeline = VK_NULL_HANDLE);

} // namespace ImGui::Vulkan