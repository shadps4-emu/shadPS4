// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer/backend_factory.h"

#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_headless/headless_backend.h"
#include "video_core/renderer_vulkan/vulkan_backend.h"

namespace VideoCore::Render {

namespace {

std::unique_ptr<IRenderBackend> g_backend;

constexpr std::string_view BackendName(const BackendKind kind) {
    switch (kind) {
    case BackendKind::Vulkan:
        return "Vulkan";
    case BackendKind::Headless:
        return "Headless";
    }
    return "Unknown";
}

} // namespace

std::unique_ptr<IRenderBackend> CreateBackend(const BackendCreateInfo& create_info) {
    std::unique_ptr<IRenderBackend> backend;
    switch (create_info.requested_backend) {
    case BackendKind::Vulkan:
        backend =
            std::make_unique<Vulkan::VulkanBackend>(create_info.window, create_info.liverpool);
        break;
    case BackendKind::Headless:
        backend =
            std::make_unique<Headless::HeadlessBackend>(create_info.window, create_info.liverpool);
        break;
    }

    LOG_INFO(Render, "Using renderer backend: {}", BackendName(backend->GetKind()));
    return backend;
}

IRenderBackend* TryGetRenderBackend() {
    return g_backend.get();
}

IRenderBackend& GetRenderBackend() {
    ASSERT_MSG(g_backend != nullptr, "Render backend was requested before initialization");
    return *g_backend;
}

void SetRenderBackend(std::unique_ptr<IRenderBackend> backend) {
    g_backend = std::move(backend);
}

} // namespace VideoCore::Render
