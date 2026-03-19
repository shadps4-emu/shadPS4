// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "video_core/renderer/backend.h"
#include "video_core/renderer/presenter.h"
#include "video_core/renderer/rasterizer.h"
#include "video_core/renderer/shader_debug_provider.h"

namespace Frontend {
class WindowSDL;
}

namespace AmdGpu {
struct Liverpool;
}

namespace VideoCore::Render {

struct BackendCreateInfo {
    Frontend::WindowSDL& window;
    AmdGpu::Liverpool& liverpool;
    BackendKind requested_backend{BackendKind::Vulkan};
};

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual BackendKind GetKind() const = 0;
    virtual const BackendCapabilities& GetCapabilities() const = 0;
    virtual IPresenter& GetPresenter() = 0;
    virtual IRasterizer& GetRasterizer() = 0;
    virtual IShaderDebugProvider* GetShaderDebugProvider() = 0;
};

std::unique_ptr<IRenderBackend> CreateBackend(const BackendCreateInfo& create_info);
IRenderBackend* TryGetRenderBackend();
IRenderBackend& GetRenderBackend();
void SetRenderBackend(std::unique_ptr<IRenderBackend> backend);

} // namespace VideoCore::Render
