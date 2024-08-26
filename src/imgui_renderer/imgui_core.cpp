// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_events.h>
#include <imgui.h>
#include "common/config.h"
#include "imgui_core.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"

static void CheckVkResult(const vk::Result err) {
    LOG_ERROR(ImGui, "Vulkan error {}", vk::to_string(err));
}

namespace ImGui::Emulator {

void Initialize(const ::Vulkan::Instance& instance, const Frontend::WindowSDL& window,
                const u32 image_count, vk::Format surface_format,
                const vk::AllocationCallbacks* allocator) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.DisplaySize = ImVec2((float)window.getWidth(), (float)window.getHeight());
    ImGui::StyleColorsDark();

    Sdl::Init(window.GetSdlWindow());

    Vulkan::InitInfo vk_info{
        .instance = instance.GetInstance(),
        .physical_device = instance.GetPhysicalDevice(),
        .device = instance.GetDevice(),
        .queue_family = instance.GetPresentQueueFamilyIndex(),
        .queue = instance.GetPresentQueue(),
        .image_count = image_count,
        .min_allocation_size = 1024 * 1024,
        .pipeline_rendering_create_info{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &surface_format,
        },
        .allocator = allocator,
        .check_vk_result_fn = &CheckVkResult,
    };
    Vulkan::Init(vk_info);
}

void OnResize() {
    ImGuiIO& io = ImGui::GetIO();
    Sdl::OnResize();
}

void Shutdown(const vk::Device& device) {
    device.waitIdle();

    Vulkan::Shutdown();
    Sdl::Shutdown();
    ImGui::DestroyContext();
}

bool ProcessEvent(SDL_Event* event) {
    bool used = Sdl::ProcessEvent(event);
    if (!used) {
        return false;
    }
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return ImGui::GetIO().WantCaptureMouse;
    case SDL_EVENT_TEXT_INPUT:
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return ImGui::GetIO().WantCaptureKeyboard;
    default:
        return false;
    }
}

void NewFrame() {
    Vulkan::NewFrame();
    Sdl::NewFrame();
    const auto& io = ImGui::GetIO();

    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    if (ImGui::Begin("Frame timings")) {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate,
                    io.Framerate);
        ImGui::End();
    }
}

void Render(const vk::CommandBuffer& cmdbuf, ::Vulkan::Frame* frame) {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    if (Config::vkMarkersEnabled()) {
        cmdbuf.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{
            .pLabelName = "ImGui Render",
        });
    }
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                           vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {},
                           {vk::ImageMemoryBarrier{
                               .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                               .dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead,
                               .oldLayout = vk::ImageLayout::eUndefined,
                               .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = frame->image,
                               .subresourceRange{
                                   .aspectMask = vk::ImageAspectFlagBits::eColor,
                                   .baseMipLevel = 0,
                                   .levelCount = 1,
                                   .baseArrayLayer = 0,
                                   .layerCount = VK_REMAINING_ARRAY_LAYERS,
                               },
                           }});

    vk::RenderingAttachmentInfo color_attachments[1]{
        {
            .imageView = frame->image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
        },
    };
    vk::RenderingInfo render_info = {};
    render_info.renderArea = {
        .offset = {0, 0},
        .extent = {frame->width, frame->height},
    };
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = color_attachments;
    cmdbuf.beginRendering(render_info);
    Vulkan::RenderDrawData(*draw_data, cmdbuf);
    cmdbuf.endRendering();
    if (Config::vkMarkersEnabled()) {
        cmdbuf.endDebugUtilsLabelEXT();
    }
}

} // namespace ImGui::Emulator
