// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xxhash.h>
#include "common/types.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

static constexpr u32 MaxVertexBufferCount = 32;
static constexpr u32 MaxShaderStages = 5;

class Instance;
class Scheduler;

using Liverpool = AmdGpu::Liverpool;

struct GraphicsPipelineKey {
    std::array<size_t, MaxShaderStages> stage_hashes;
    std::array<vk::Format, Liverpool::NumColorBuffers> color_formats;
    vk::Format depth_format;

    Liverpool::DepthControl depth;
    float depth_bounds_min;
    float depth_bounds_max;
    float depth_bias_const_factor;
    float depth_bias_slope_factor;
    float depth_bias_clamp;
    u32 depth_bias_enable;
    u32 num_samples = 1;
    Liverpool::StencilControl stencil;
    Liverpool::StencilRefMask stencil_ref_front;
    Liverpool::StencilRefMask stencil_ref_back;
    Liverpool::PrimitiveType prim_type;
    u32 enable_primitive_restart;
    u32 primitive_restart_index;
    Liverpool::PolygonMode polygon_mode;
    Liverpool::CullMode cull_mode;
    Liverpool::FrontFace front_face;
    Liverpool::ClipSpace clip_space;
    Liverpool::ColorBufferMask cb_shader_mask{};
    std::array<Liverpool::BlendControl, Liverpool::NumColorBuffers> blend_controls;
    std::array<vk::ColorComponentFlags, Liverpool::NumColorBuffers> write_masks;

    bool operator==(const GraphicsPipelineKey& key) const noexcept {
        return std::memcmp(this, &key, sizeof(key)) == 0;
    }
};

class GraphicsPipeline {
public:
    explicit GraphicsPipeline(const Instance& instance, Scheduler& scheduler,
                              const GraphicsPipelineKey& key, vk::PipelineCache pipeline_cache,
                              std::span<const Shader::Info*, MaxShaderStages> infos,
                              std::array<vk::ShaderModule, MaxShaderStages> modules);
    ~GraphicsPipeline();

    void BindResources(const Liverpool::Regs& regs, VideoCore::BufferCache& buffer_cache,
                       VideoCore::TextureCache& texture_cache) const;

    vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    vk::PipelineLayout GetLayout() const {
        return *pipeline_layout;
    }

    const Shader::Info& GetStage(Shader::Stage stage) const noexcept {
        return stages[u32(stage)];
    }

    bool IsEmbeddedVs() const noexcept {
        static constexpr size_t EmbeddedVsHash = 0x9b2da5cf47f8c29f;
        return key.stage_hashes[u32(Shader::Stage::Vertex)] == EmbeddedVsHash;
    }

    auto GetWriteMasks() const {
        return key.write_masks;
    }

    bool IsDepthEnabled() const {
        return key.depth.depth_enable.Value();
    }

private:
    void BuildDescSetLayout();

private:
    const Instance& instance;
    Scheduler& scheduler;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    std::array<Shader::Info, MaxShaderStages> stages{};
    GraphicsPipelineKey key;
};

} // namespace Vulkan

template <>
struct std::hash<Vulkan::GraphicsPipelineKey> {
    std::size_t operator()(const Vulkan::GraphicsPipelineKey& key) const noexcept {
        return XXH3_64bits(&key, sizeof(key));
    }
};
