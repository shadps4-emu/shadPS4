// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/backend/bindings.h"
#include "shader_recompiler/info.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Scheduler;
class DescriptorHeap;

class Pipeline {
public:
    Pipeline(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
             vk::PipelineCache pipeline_cache);
    virtual ~Pipeline();

    vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    vk::PipelineLayout GetLayout() const noexcept {
        return *pipeline_layout;
    }

    using DescriptorWrites = boost::container::small_vector<vk::WriteDescriptorSet, 16>;
    using BufferBarriers = boost::container::small_vector<vk::BufferMemoryBarrier2, 16>;

    void BindBuffers(VideoCore::BufferCache& buffer_cache, VideoCore::TextureCache& texture_cache,
                     const Shader::Info& stage, Shader::Backend::Bindings& binding,
                     Shader::PushData& push_data, DescriptorWrites& set_writes,
                     BufferBarriers& buffer_barriers) const;

    void BindTextures(VideoCore::TextureCache& texture_cache, const Shader::Info& stage,
                      Shader::Backend::Bindings& binding, DescriptorWrites& set_writes) const;

protected:
    const Instance& instance;
    Scheduler& scheduler;
    DescriptorHeap& desc_heap;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    static boost::container::static_vector<vk::DescriptorImageInfo, 32> image_infos;
    static boost::container::static_vector<vk::BufferView, 8> buffer_views;
    static boost::container::static_vector<vk::DescriptorBufferInfo, 32> buffer_infos;
};

} // namespace Vulkan
