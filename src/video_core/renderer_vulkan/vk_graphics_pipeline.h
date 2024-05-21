// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/types.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

static constexpr u32 MaxShaderStages = 5;

class Instance;

using Liverpool = AmdGpu::Liverpool;

struct PipelineKey {
    Liverpool::DepthControl depth;
    Liverpool::StencilControl stencil;
    Liverpool::StencilRefMask stencil_ref_front;
    Liverpool::StencilRefMask stencil_ref_back;
    Liverpool::PrimitiveType prim_type;
    Liverpool::PolygonMode polygon_mode;
    Liverpool::CullMode cull_mode;
};
static_assert(std::has_unique_object_representations_v<PipelineKey>);

class GraphicsPipeline {
public:
    explicit GraphicsPipeline(const Instance& instance, const PipelineKey& key,
                              vk::PipelineCache pipeline_cache, vk::PipelineLayout layout,
                              std::array<vk::ShaderModule, MaxShaderStages> modules);
    ~GraphicsPipeline();

    [[nodiscard]] vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

private:
    const Instance& instance;
    vk::UniquePipeline pipeline;
    vk::PipelineLayout pipeline_layout;
    vk::PipelineCache pipeline_cache;
    PipelineKey key;
};

} // namespace Vulkan
