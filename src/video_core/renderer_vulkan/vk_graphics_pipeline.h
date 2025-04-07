// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/static_vector.hpp>
#include <xxhash.h>

#include "common/types.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/renderer_vulkan/vk_pipeline_common.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

static constexpr u32 MaxShaderStages = static_cast<u32>(Shader::LogicalStage::NumLogicalStages);
static constexpr u32 MaxVertexBufferCount = 32;

class Instance;
class Scheduler;
class DescriptorHeap;

using Liverpool = AmdGpu::Liverpool;

template <typename T>
using VertexInputs = boost::container::static_vector<T, MaxVertexBufferCount>;

struct GraphicsPipelineKey {
    std::array<size_t, MaxShaderStages> stage_hashes;
    u32 num_color_attachments;
    std::array<vk::Format, Liverpool::NumColorBuffers> color_formats;
    std::array<Shader::PsColorBuffer, Liverpool::NumColorBuffers> color_buffers;
    vk::Format depth_format;
    vk::Format stencil_format;

    u32 num_samples;
    u32 mrt_mask;
    AmdGpu::PrimitiveType prim_type;
    u32 enable_primitive_restart;
    u32 primitive_restart_index;
    Liverpool::PolygonMode polygon_mode;
    Liverpool::CullMode cull_mode;
    Liverpool::FrontFace front_face;
    Liverpool::ClipSpace clip_space;
    Liverpool::ColorBufferMask cb_shader_mask;
    std::array<Liverpool::BlendControl, Liverpool::NumColorBuffers> blend_controls;
    std::array<vk::ColorComponentFlags, Liverpool::NumColorBuffers> write_masks;
    std::array<vk::Format, MaxVertexBufferCount> vertex_buffer_formats;
    u32 patch_control_points;

    bool operator==(const GraphicsPipelineKey& key) const noexcept {
        return std::memcmp(this, &key, sizeof(key)) == 0;
    }
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
                     const Shader::Profile& profile, const GraphicsPipelineKey& key,
                     vk::PipelineCache pipeline_cache,
                     std::span<const Shader::Info*, MaxShaderStages> stages,
                     std::span<const Shader::RuntimeInfo, MaxShaderStages> runtime_infos,
                     std::optional<const Shader::Gcn::FetchShaderData> fetch_shader,
                     std::span<const vk::ShaderModule> modules);
    ~GraphicsPipeline();

    const std::optional<const Shader::Gcn::FetchShaderData>& GetFetchShader() const noexcept {
        return fetch_shader;
    }

    auto GetWriteMasks() const {
        return key.write_masks;
    }

    auto GetMrtMask() const {
        return key.mrt_mask;
    }

    [[nodiscard]] bool IsPrimitiveListTopology() const {
        return key.prim_type == AmdGpu::PrimitiveType::PointList ||
               key.prim_type == AmdGpu::PrimitiveType::LineList ||
               key.prim_type == AmdGpu::PrimitiveType::TriangleList ||
               key.prim_type == AmdGpu::PrimitiveType::AdjLineList ||
               key.prim_type == AmdGpu::PrimitiveType::AdjTriangleList ||
               key.prim_type == AmdGpu::PrimitiveType::RectList ||
               key.prim_type == AmdGpu::PrimitiveType::QuadList;
    }

    /// Gets the attributes and bindings for vertex inputs.
    template <typename Attribute, typename Binding>
    void GetVertexInputs(VertexInputs<Attribute>& attributes, VertexInputs<Binding>& bindings,
                         VertexInputs<AmdGpu::Buffer>& guest_buffers) const;

private:
    void BuildDescSetLayout();

private:
    GraphicsPipelineKey key;
    std::optional<const Shader::Gcn::FetchShaderData> fetch_shader{};
};

} // namespace Vulkan

template <>
struct std::hash<Vulkan::GraphicsPipelineKey> {
    std::size_t operator()(const Vulkan::GraphicsPipelineKey& key) const noexcept {
        return XXH3_64bits(&key, sizeof(key));
    }
};
