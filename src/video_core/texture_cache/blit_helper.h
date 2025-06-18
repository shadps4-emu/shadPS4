// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tsl/robin_map.h>

#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

namespace VideoCore {

class Image;
class ImageView;

class BlitHelper {
    static constexpr size_t MaxMsPipelines = 6;

public:
    explicit BlitHelper(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler);
    ~BlitHelper();

    void BlitColorToMsDepth(Image& source, Image& dest);

private:
    void CreateShaders();
    void CreatePipelineLayouts();

    struct DepthPipelineKey {
        u32 num_samples;
        vk::Format depth_format;

        auto operator<=>(const DepthPipelineKey&) const noexcept = default;
    };
    vk::Pipeline GetDepthToMsPipeline(const DepthPipelineKey& key);
    void CreateColorToMSDepthPipeline(const DepthPipelineKey& key);

private:
    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    vk::UniqueDescriptorSetLayout single_texture_descriptor_set_layout;
    vk::UniquePipelineLayout single_texture_pl_layout;
    vk::ShaderModule fs_tri_vertex;
    vk::ShaderModule color_to_ms_depth_frag;

    using DepthPipeline = std::pair<DepthPipelineKey, vk::UniquePipeline>;
    std::vector<DepthPipeline> color_to_ms_depth_pl{};
};

} // namespace VideoCore
