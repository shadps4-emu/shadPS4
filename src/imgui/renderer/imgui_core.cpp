// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL_events.h>
#include <imgui.h>
#include "common/config.h"
#include "common/path_util.h"
#include "imgui/imgui_layer.h"
#include "imgui_core.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"

static void CheckVkResult(const vk::Result err) {
    LOG_ERROR(ImGui, "Vulkan error {}", vk::to_string(err));
}

static std::vector<ImGui::Layer*> layers;

// Update layers before rendering to allow layer changes to be applied during rendering.
// Using deque to keep the order of changes in case a Layer is removed then added again between
// frames.
std::deque<std::pair<bool, ImGui::Layer*>>& GetChangeLayers() {
    static std::deque<std::pair<bool, ImGui::Layer*>>* change_layers =
        new std::deque<std::pair<bool, ImGui::Layer*>>;
    return *change_layers;
}

static std::mutex change_layers_mutex{};

namespace ImGui {

namespace Core {

void Initialize(const ::Vulkan::Instance& instance, const Frontend::WindowSDL& window,
                const u32 image_count, vk::Format surface_format,
                const vk::AllocationCallbacks* allocator) {

    const auto config_path = GetUserPath(Common::FS::PathType::UserDir) / "imgui.ini";
    const auto log_path = GetUserPath(Common::FS::PathType::LogDir) / "imgui_log.txt";

    CreateContext();
    ImGuiIO& io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.DisplaySize = ImVec2((float)window.getWidth(), (float)window.getHeight());
    io.IniFilename = SDL_strdup(config_path.string().c_str());
    io.LogFilename = SDL_strdup(log_path.string().c_str());
    StyleColorsDark();

    Sdl::Init(window.GetSdlWindow());

    const Vulkan::InitInfo vk_info{
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
    Sdl::OnResize();
}

void Shutdown(const vk::Device& device) {
    device.waitIdle();

    const ImGuiIO& io = GetIO();
    const auto ini_filename = (void*)io.IniFilename;
    const auto log_filename = (void*)io.LogFilename;

    Vulkan::Shutdown();
    Sdl::Shutdown();
    DestroyContext();

    SDL_free(ini_filename);
    SDL_free(log_filename);
}

bool ProcessEvent(SDL_Event* event) {
    Sdl::ProcessEvent(event);
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return GetIO().WantCaptureMouse;
    case SDL_EVENT_TEXT_INPUT:
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return GetIO().WantCaptureKeyboard;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        return (GetIO().BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    default:
        return false;
    }
}

void NewFrame() {
    {
        std::scoped_lock lock{change_layers_mutex};
        while (!GetChangeLayers().empty()) {
            const auto [to_be_added, layer] = GetChangeLayers().front();
            if (to_be_added) {
                layers.push_back(layer);
            } else {
                const auto [begin, end] = std::ranges::remove(layers, layer);
                layers.erase(begin, end);
            }
            GetChangeLayers().pop_front();
        }
    }

    Vulkan::NewFrame();
    Sdl::NewFrame();
    ImGui::NewFrame();

    bool capture_gamepad = false;
    for (auto* layer : layers) {
        layer->Draw();
        if (layer->ShouldGrabGamepad()) {
            capture_gamepad = true;
        }
    }
    if (capture_gamepad) {
        GetIO().BackendFlags |= ImGuiBackendFlags_HasGamepad;
    } else {
        GetIO().BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    }
}

void Render(const vk::CommandBuffer& cmdbuf, ::Vulkan::Frame* frame) {
    ImGui::Render();
    ImDrawData* draw_data = GetDrawData();
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    if (Config::vkMarkersEnabled()) {
        cmdbuf.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{
            .pLabelName = "ImGui Render",
        });
    }

    vk::RenderingAttachmentInfo color_attachments[1]{
        {
            .imageView = frame->image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
        },
    };
    vk::RenderingInfo render_info = {};
    render_info.renderArea = vk::Rect2D{
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

} // namespace Core

void Layer::AddLayer(Layer* layer) {
    std::scoped_lock lock{change_layers_mutex};
    GetChangeLayers().emplace_back(true, layer);
}

void Layer::RemoveLayer(Layer* layer) {
    std::scoped_lock lock{change_layers_mutex};
    GetChangeLayers().emplace_back(false, layer);
}

} // namespace ImGui
