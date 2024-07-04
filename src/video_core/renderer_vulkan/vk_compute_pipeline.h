// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Core {
class MemoryManager;
}

namespace VideoCore {
class TextureCache;
}

namespace Vulkan {

class Instance;
class Scheduler;
class StreamBuffer;

class ComputePipeline {
public:
    explicit ComputePipeline(const Instance& instance, Scheduler& scheduler,
                             vk::PipelineCache pipeline_cache, const Shader::Info* info,
                             u64 compute_key, vk::ShaderModule module);
    ~ComputePipeline();

    [[nodiscard]] vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    bool BindResources(Core::MemoryManager* memory, StreamBuffer& staging,
                       VideoCore::TextureCache& texture_cache) const;

private:
    const Instance& instance;
    Scheduler& scheduler;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    u64 compute_key;
    Shader::Info info{};
};

} // namespace Vulkan
