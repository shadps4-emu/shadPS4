// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/texture_cache/sampler.h"

namespace VideoCore {

Sampler::Sampler(const Vulkan::Instance& instance, const AmdGpu::Sampler& sampler,
                 const AmdGpu::Liverpool::BorderColorBufferBase& border_color_base) {
    if (sampler.force_degamma) {
        LOG_WARNING(Render_Vulkan, "Texture requires gamma correction");
    }
    using namespace Vulkan;
    const bool anisotropyEnable = instance.IsAnisotropicFilteringSupported() &&
                                  (AmdGpu::IsAnisoFilter(sampler.xy_mag_filter) ||
                                   AmdGpu::IsAnisoFilter(sampler.xy_min_filter));
    const float maxAnisotropy =
        anisotropyEnable ? std::clamp(sampler.MaxAniso(), 1.0f, instance.MaxSamplerAnisotropy())
                         : 1.0f;
    auto borderColor = LiverpoolToVK::BorderColor(sampler.border_color_type);
    if (!instance.IsCustomBorderColorSupported()) {
        LOG_WARNING(Render_Vulkan, "Custom border color is not supported, falling back to black");
        borderColor = vk::BorderColor::eFloatOpaqueBlack;
    }

    const auto customColor = [&]() -> std::optional<vk::SamplerCustomBorderColorCreateInfoEXT> {
        if (borderColor == vk::BorderColor::eFloatCustomEXT) {
            const auto borderColorIndex = sampler.border_color_ptr.Value();
            const auto borderColorBuffer = border_color_base.Address<std::array<float, 4>*>();
            const auto customBorderColorArray = borderColorBuffer[borderColorIndex];

            const vk::SamplerCustomBorderColorCreateInfoEXT ret{
                .customBorderColor =
                    vk::ClearColorValue{
                        .float32 = customBorderColorArray,
                    },
                .format = vk::Format::eR32G32B32A32Sfloat,
            };
            return ret;
        } else {
            return std::nullopt;
        }
    }();

    const vk::SamplerCreateInfo sampler_ci = {
        .pNext = customColor ? &*customColor : nullptr,
        .magFilter = LiverpoolToVK::Filter(sampler.xy_mag_filter),
        .minFilter = LiverpoolToVK::Filter(sampler.xy_min_filter),
        .mipmapMode = LiverpoolToVK::MipFilter(sampler.mip_filter),
        .addressModeU = LiverpoolToVK::ClampMode(sampler.clamp_x),
        .addressModeV = LiverpoolToVK::ClampMode(sampler.clamp_y),
        .addressModeW = LiverpoolToVK::ClampMode(sampler.clamp_z),
        .mipLodBias = std::min(sampler.LodBias(), instance.MaxSamplerLodBias()),
        .anisotropyEnable = anisotropyEnable,
        .maxAnisotropy = maxAnisotropy,
        .compareEnable = sampler.depth_compare_func != AmdGpu::DepthCompare::Never,
        .compareOp = LiverpoolToVK::DepthCompare(sampler.depth_compare_func),
        .minLod = sampler.MinLod(),
        .maxLod = sampler.MaxLod(),
        .borderColor = borderColor,
        .unnormalizedCoordinates = false, // Handled in shader due to Vulkan limitations.
    };
    auto [sampler_result, smplr] = instance.GetDevice().createSamplerUnique(sampler_ci);
    ASSERT_MSG(sampler_result == vk::Result::eSuccess, "Failed to create sampler: {}",
               vk::to_string(sampler_result));
    handle = std::move(smplr);
}

Sampler::~Sampler() = default;

} // namespace VideoCore
