// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "video_core/renderer/backend_factory.h"

namespace AmdGpu {
struct Liverpool;
}

namespace Frontend {
class WindowSDL;
}

namespace Headless {

class HeadlessBackend final : public VideoCore::Render::IRenderBackend {
public:
    HeadlessBackend(Frontend::WindowSDL& window, AmdGpu::Liverpool& liverpool);
    ~HeadlessBackend() override;

    VideoCore::Render::BackendKind GetKind() const override;
    const VideoCore::Render::BackendCapabilities& GetCapabilities() const override;
    VideoCore::Render::IPresenter& GetPresenter() override;
    VideoCore::Render::IRasterizer& GetRasterizer() override;
    VideoCore::Render::IShaderDebugProvider* GetShaderDebugProvider() override;

private:
    class HeadlessPresenter;
    class HeadlessRasterizer;

    std::unique_ptr<HeadlessPresenter> presenter;
    std::unique_ptr<HeadlessRasterizer> rasterizer;
    VideoCore::Render::BackendCapabilities capabilities{};
};

} // namespace Headless
