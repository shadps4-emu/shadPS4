// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"

namespace AmdGpu {
struct Liverpool;
}

namespace VideoCore {
class TextureCache;
}

namespace Vulkan {

class Scheduler;
class GraphicsPipeline;

class Rasterizer {
public:
    explicit Rasterizer(const Instance& instance, Scheduler& scheduler,
                        VideoCore::TextureCache& texture_cache, AmdGpu::Liverpool* liverpool);
    ~Rasterizer();

    /// Performs a draw call with an index buffer.
    void DrawIndex();

    /// Updates graphics state that is not part of the bound pipeline.
    void UpdateDynamicState();

private:
    /// Updates viewport and scissor from liverpool registers.
    void UpdateViewportScissorState();

    /// Updates depth and stencil pipeline state from liverpool registers.
    void UpdateDepthStencilState();

private:
    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::TextureCache& texture_cache;
    AmdGpu::Liverpool* liverpool;
    PipelineCache pipeline_cache;
    StreamBuffer vertex_index_buffer;
};

} // namespace Vulkan
