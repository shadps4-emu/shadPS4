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
struct ImageInfo;

class BlitHelper {
    static constexpr size_t MaxMsPipelines = 6;

public:
    explicit BlitHelper(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler);
    ~BlitHelper();

    void ReinterpretColorAsMsDepth(u32 width, u32 height, u32 num_samples, vk::Format pixel_format,
                                   vk::Image source, vk::Image dest);

    void BlitNonMsImageMsImage(u32 width, u32 height, u32 num_samples, vk::Format pixel_format,
                               bool is_depth, bool src_msaa, vk::Image source, vk::Image dest);

private:
    void CreateShaders();
    void CreatePipelineLayouts();

    struct MsPipelineKey {
        u32 num_samples;
        vk::Format color_format;
        vk::Format depth_format;
        bool src_msaa;

        auto operator<=>(const MsPipelineKey&) const noexcept = default;
    };
    void CreateColorToMSDepthPipeline(const MsPipelineKey& key);
    void CreateNonMsImageToMsImagePipeline(const MsPipelineKey& key);

private:
    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    vk::UniqueDescriptorSetLayout single_texture_descriptor_set_layout;
    vk::UniquePipelineLayout single_texture_pl_layout;
    vk::ShaderModule fs_tri_vertex;
    vk::ShaderModule fs_tri_layer_vertex;
    vk::ShaderModule color_to_ms_depth_frag;
    vk::ShaderModule non_ms_color_to_ms_color_frag;
    vk::ShaderModule non_ms_depth_to_ms_depth_frag;
    vk::ShaderModule ms_color_to_non_ms_color_frag;
    vk::ShaderModule ms_depth_to_non_ms_depth_frag;

    using MsPipeline = std::pair<MsPipelineKey, vk::UniquePipeline>;
    std::vector<MsPipeline> color_to_ms_depth_pl;
    std::vector<MsPipeline> non_ms_color_to_ms_color_pl;
};

} // namespace VideoCore
