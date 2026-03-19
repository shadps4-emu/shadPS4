// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "video_core/renderer/backend_factory.h"
#include "video_core/renderer/presenter.h"
#include "video_core/renderer/rasterizer.h"
#include "video_core/renderer/shader_debug_provider.h"

namespace AmdGpu {
struct Liverpool;
}

namespace Frontend {
class WindowSDL;
}

namespace Vulkan {

class Presenter;
class Rasterizer;

class VulkanBackend final : public VideoCore::Render::IRenderBackend {
public:
    VulkanBackend(Frontend::WindowSDL& window, AmdGpu::Liverpool& liverpool);
    ~VulkanBackend() override;

    VideoCore::Render::BackendKind GetKind() const override;
    const VideoCore::Render::BackendCapabilities& GetCapabilities() const override;
    VideoCore::Render::IPresenter& GetPresenter() override;
    VideoCore::Render::IRasterizer& GetRasterizer() override;
    VideoCore::Render::IShaderDebugProvider* GetShaderDebugProvider() override;

private:
    class FrameHandle;
    class PresenterProxy;
    class RasterizerProxy;
    class ShaderDebugProvider;

    std::unique_ptr<Presenter> presenter;
    std::unique_ptr<PresenterProxy> presenter_proxy;
    std::unique_ptr<RasterizerProxy> rasterizer_proxy;
    std::unique_ptr<ShaderDebugProvider> shader_debug_provider;
    VideoCore::Render::BackendCapabilities capabilities{};
};

} // namespace Vulkan
