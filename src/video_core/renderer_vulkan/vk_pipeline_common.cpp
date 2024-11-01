// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/static_vector.hpp>

#include "shader_recompiler/info.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_common.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

boost::container::static_vector<vk::DescriptorImageInfo, 32> Pipeline::image_infos;
boost::container::static_vector<vk::BufferView, 8> Pipeline::buffer_views;
boost::container::static_vector<vk::DescriptorBufferInfo, 32> Pipeline::buffer_infos;

Pipeline::Pipeline(const Instance& instance_, Scheduler& scheduler_, DescriptorHeap& desc_heap_,
                   vk::PipelineCache pipeline_cache)
    : instance{instance_}, scheduler{scheduler_}, desc_heap{desc_heap_} {}

Pipeline::~Pipeline() = default;

void Pipeline::BindBuffers(VideoCore::BufferCache& buffer_cache,
                           VideoCore::TextureCache& texture_cache, const Shader::Info& stage,
                           Shader::Backend::Bindings& binding, Shader::PushData& push_data,
                           DescriptorWrites& set_writes, BufferBarriers& buffer_barriers) const {
    using BufferBindingInfo = std::pair<VideoCore::BufferId, AmdGpu::Buffer>;
    static boost::container::static_vector<BufferBindingInfo, 32> buffer_bindings;

    buffer_bindings.clear();

    for (const auto& desc : stage.buffers) {
        const auto vsharp = desc.GetSharp(stage);
        if (!desc.is_gds_buffer && vsharp.base_address != 0 && vsharp.GetSize() > 0) {
            const auto buffer_id = buffer_cache.FindBuffer(vsharp.base_address, vsharp.GetSize());
            buffer_bindings.emplace_back(buffer_id, vsharp);
        } else {
            buffer_bindings.emplace_back(VideoCore::BufferId{}, vsharp);
        }
    }

    using TexBufferBindingInfo = std::pair<VideoCore::BufferId, AmdGpu::Buffer>;
    static boost::container::static_vector<TexBufferBindingInfo, 32> texbuffer_bindings;

    texbuffer_bindings.clear();

    for (const auto& desc : stage.texture_buffers) {
        const auto vsharp = desc.GetSharp(stage);
        if (vsharp.base_address != 0 && vsharp.GetSize() > 0 &&
            vsharp.GetDataFmt() != AmdGpu::DataFormat::FormatInvalid) {
            const auto buffer_id = buffer_cache.FindBuffer(vsharp.base_address, vsharp.GetSize());
            texbuffer_bindings.emplace_back(buffer_id, vsharp);
        } else {
            texbuffer_bindings.emplace_back(VideoCore::BufferId{}, vsharp);
        }
    }

    // Bind the flattened user data buffer as a UBO so it's accessible to the shader
    if (stage.has_readconst) {
        const auto [vk_buffer, offset] = buffer_cache.ObtainHostUBO(stage.flattened_ud_buf);
        buffer_infos.emplace_back(vk_buffer->Handle(), offset,
                                  stage.flattened_ud_buf.size() * sizeof(u32));
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &buffer_infos.back(),
        });
        ++binding.buffer;
    }

    // Second pass to re-bind buffers that were updated after binding
    for (u32 i = 0; i < buffer_bindings.size(); i++) {
        const auto& [buffer_id, vsharp] = buffer_bindings[i];
        const auto& desc = stage.buffers[i];
        const bool is_storage = desc.IsStorage(vsharp);
        if (!buffer_id) {
            if (desc.is_gds_buffer) {
                const auto* gds_buf = buffer_cache.GetGdsBuffer();
                buffer_infos.emplace_back(gds_buf->Handle(), 0, gds_buf->SizeBytes());
            } else if (instance.IsNullDescriptorSupported()) {
                buffer_infos.emplace_back(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE);
            } else {
                auto& null_buffer = buffer_cache.GetBuffer(VideoCore::NULL_BUFFER_ID);
                buffer_infos.emplace_back(null_buffer.Handle(), 0, VK_WHOLE_SIZE);
            }
        } else {
            const auto [vk_buffer, offset] = buffer_cache.ObtainBuffer(
                vsharp.base_address, vsharp.GetSize(), desc.is_written, false, buffer_id);
            const u32 alignment =
                is_storage ? instance.StorageMinAlignment() : instance.UniformMinAlignment();
            const u32 offset_aligned = Common::AlignDown(offset, alignment);
            const u32 adjust = offset - offset_aligned;
            ASSERT(adjust % 4 == 0);
            push_data.AddOffset(binding.buffer, adjust);
            buffer_infos.emplace_back(vk_buffer->Handle(), offset_aligned,
                                      vsharp.GetSize() + adjust);
        }

        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = is_storage ? vk::DescriptorType::eStorageBuffer
                                         : vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &buffer_infos.back(),
        });
        ++binding.buffer;
    }

    const auto null_buffer_view =
        instance.IsNullDescriptorSupported() ? VK_NULL_HANDLE : buffer_cache.NullBufferView();
    for (u32 i = 0; i < texbuffer_bindings.size(); i++) {
        const auto& [buffer_id, vsharp] = texbuffer_bindings[i];
        const auto& desc = stage.texture_buffers[i];
        vk::BufferView& buffer_view = buffer_views.emplace_back(null_buffer_view);
        if (buffer_id) {
            const u32 alignment = instance.TexelBufferMinAlignment();
            const auto [vk_buffer, offset] = buffer_cache.ObtainBuffer(
                vsharp.base_address, vsharp.GetSize(), desc.is_written, true, buffer_id);
            const u32 fmt_stride = AmdGpu::NumBits(vsharp.GetDataFmt()) >> 3;
            ASSERT_MSG(fmt_stride == vsharp.GetStride(),
                       "Texel buffer stride must match format stride");
            const u32 offset_aligned = Common::AlignDown(offset, alignment);
            const u32 adjust = offset - offset_aligned;
            ASSERT(adjust % fmt_stride == 0);
            push_data.AddOffset(binding.buffer, adjust / fmt_stride);
            buffer_view =
                vk_buffer->View(offset_aligned, vsharp.GetSize() + adjust, desc.is_written,
                                vsharp.GetDataFmt(), vsharp.GetNumberFmt());
            if (auto barrier =
                    vk_buffer->GetBarrier(desc.is_written ? vk::AccessFlagBits2::eShaderWrite
                                                          : vk::AccessFlagBits2::eShaderRead,
                                          vk::PipelineStageFlagBits2::eComputeShader)) {
                buffer_barriers.emplace_back(*barrier);
            }
            if (desc.is_written) {
                texture_cache.InvalidateMemoryFromGPU(vsharp.base_address, vsharp.GetSize());
            }
        }

        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = desc.is_written ? vk::DescriptorType::eStorageTexelBuffer
                                              : vk::DescriptorType::eUniformTexelBuffer,
            .pTexelBufferView = &buffer_view,
        });
        ++binding.buffer;
    }
}

void Pipeline::BindTextures(VideoCore::TextureCache& texture_cache, const Shader::Info& stage,
                            Shader::Backend::Bindings& binding,
                            DescriptorWrites& set_writes) const {

    using ImageBindingInfo = std::tuple<VideoCore::ImageId, AmdGpu::Image, Shader::ImageResource>;
    static boost::container::static_vector<ImageBindingInfo, 32> image_bindings;

    image_bindings.clear();

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
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = desc.is_storage ? vk::DescriptorType::eStorageImage
                                              : vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_infos.back(),
        });
    }

    for (const auto& sampler : stage.samplers) {
        auto ssharp = sampler.GetSharp(stage);
        if (sampler.disable_aniso) {
            const auto& tsharp = stage.images[sampler.associated_image].GetSharp(stage);
            if (tsharp.base_level == 0 && tsharp.last_level == 0) {
                ssharp.max_aniso.Assign(AmdGpu::AnisoRatio::One);
            }
        }
        const auto vk_sampler = texture_cache.GetSampler(ssharp);
        image_infos.emplace_back(vk_sampler, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampler,
            .pImageInfo = &image_infos.back(),
        });
    }
}

} // namespace Vulkan
