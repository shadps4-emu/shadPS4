// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xxhash.h>
#include "common/types.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Core {
class MemoryManager;
}

namespace Vulkan {

static constexpr u32 MaxVertexBufferCount = 32;
static constexpr u32 MaxShaderStages = 5;

class Instance;
class Scheduler;

using Liverpool = AmdGpu::Liverpool;

struct PipelineKey {
    std::array<size_t, MaxShaderStages> stage_hashes;
    std::array<vk::Format, Liverpool::NumColorBuffers> color_formats;
    vk::Format depth_format;

    Liverpool::DepthControl depth;
    Liverpool::StencilControl stencil;
    Liverpool::StencilRefMask stencil_ref_front;
    Liverpool::StencilRefMask stencil_ref_back;
    Liverpool::PrimitiveType prim_type;
    Liverpool::PolygonMode polygon_mode;
    Liverpool::CullMode cull_mode;

    bool operator==(const PipelineKey& key) const noexcept {
        return std::memcmp(this, &key, sizeof(PipelineKey)) == 0;
    }
};
static_assert(std::has_unique_object_representations_v<PipelineKey>);

class GraphicsPipeline {
public:
    explicit GraphicsPipeline(const Instance& instance, Scheduler& scheduler,
                              const PipelineKey& key, vk::PipelineCache pipeline_cache,
                              std::span<const Shader::Info*, MaxShaderStages> infos,
                              std::array<vk::ShaderModule, MaxShaderStages> modules);
    ~GraphicsPipeline();

    void BindResources(Core::MemoryManager* memory) const;

    [[nodiscard]] vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

private:
    const Instance& instance;
    Scheduler& scheduler;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    std::array<Shader::Info, MaxShaderStages> stages;
    PipelineKey key;
};

} // namespace Vulkan

template <>
struct std::hash<Vulkan::PipelineKey> {
    std::size_t operator()(const Vulkan::PipelineKey& key) const noexcept {
        return XXH3_64bits(&key, sizeof(key));
    }
};
