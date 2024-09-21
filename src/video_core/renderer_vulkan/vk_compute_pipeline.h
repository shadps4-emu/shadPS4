// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/renderer_vulkan/vk_pipeline_common.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Scheduler;
class DescriptorHeap;

class ComputePipeline : public Pipeline {
public:
    ComputePipeline(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
                    vk::PipelineCache pipeline_cache, u64 compute_key, const Shader::Info& info,
                    vk::ShaderModule module);
    ~ComputePipeline();

    bool BindResources(VideoCore::BufferCache& buffer_cache,
                       VideoCore::TextureCache& texture_cache) const;

private:
    u64 compute_key;
    const Shader::Info* info;
    bool uses_push_descriptors{};
};

} // namespace Vulkan
