// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>
#include "shader_recompiler/info.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Scheduler;

class ComputePipeline {
public:
    explicit ComputePipeline(const Instance& instance, Scheduler& scheduler,
                             vk::PipelineCache pipeline_cache, u64 compute_key,
                             const Shader::Info& info, vk::ShaderModule module);
    ~ComputePipeline();

    [[nodiscard]] vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    bool BindResources(VideoCore::BufferCache& buffer_cache,
                       VideoCore::TextureCache& texture_cache) const;

private:
    const Instance& instance;
    Scheduler& scheduler;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    u64 compute_key;
    const Shader::Info* info;
};

} // namespace Vulkan
