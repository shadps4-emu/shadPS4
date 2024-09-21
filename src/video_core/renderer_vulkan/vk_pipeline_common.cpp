// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/static_vector.hpp>

#include "shader_recompiler/info.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_common.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

boost::container::static_vector<vk::DescriptorImageInfo, 32> Pipeline::image_infos;

Pipeline::Pipeline(const Instance& instance_, Scheduler& scheduler_, DescriptorHeap& desc_heap_,
                   vk::PipelineCache pipeline_cache)
    : instance{instance_}, scheduler{scheduler_}, desc_heap{desc_heap_} {}

Pipeline::~Pipeline() = default;

void Pipeline::BindTextures(VideoCore::TextureCache& texture_cache, const Shader::Info& stage,
                            u32& binding, DescriptorWrites& set_writes) const {

    using ImageBindingInfo = std::tuple<VideoCore::ImageId, AmdGpu::Image, Shader::ImageResource>;
    boost::container::static_vector<ImageBindingInfo, 32> image_bindings;

    for (const auto& image_desc : stage.images) {
        const auto tsharp = image_desc.GetSharp(stage);
        if (tsharp.GetDataFmt() != AmdGpu::DataFormat::FormatInvalid) {
            VideoCore::ImageInfo image_info{tsharp, image_desc};
            const auto image_id = texture_cache.FindImage(image_info);
            auto& image = texture_cache.GetImage(image_id);
            image.flags |= VideoCore::ImageFlagBits::Bound;
            image_bindings.emplace_back(image_id, tsharp, image_desc);
        } else {
            image_bindings.emplace_back(VideoCore::ImageId{}, tsharp, image_desc);
        }

        if (texture_cache.IsMeta(tsharp.Address())) {
            LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a PS shader (texture)");
        }
    }

    // Second pass to re-bind images that were updated after binding
    for (auto [image_id, tsharp, desc] : image_bindings) {
        if (!image_id) {
            if (instance.IsNullDescriptorSupported()) {
                image_infos.emplace_back(VK_NULL_HANDLE, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
            } else {
                auto& null_image = texture_cache.GetImageView(VideoCore::NULL_IMAGE_VIEW_ID);
                image_infos.emplace_back(VK_NULL_HANDLE, *null_image.image_view,
                                         vk::ImageLayout::eGeneral);
            }
        } else {
            auto& image = texture_cache.GetImage(image_id);
            if (True(image.flags & VideoCore::ImageFlagBits::NeedsRebind)) {
                image_id = texture_cache.FindImage(image.info);
            }
            VideoCore::ImageViewInfo view_info{tsharp, desc};
            auto& image_view = texture_cache.FindTexture(image_id, view_info);
            image_infos.emplace_back(VK_NULL_HANDLE, *image_view.image_view,
                                     texture_cache.GetImage(image_id).last_state.layout);
            image.flags &=
                ~(VideoCore::ImageFlagBits::NeedsRebind | VideoCore::ImageFlagBits::Bound);
        }

        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = desc.is_storage ? vk::DescriptorType::eStorageImage
                                              : vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_infos.back(),
        });
    }
}

} // namespace Vulkan
