// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "common/assert.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan::LiverpoolToVK {

using Liverpool = AmdGpu::Liverpool;

vk::StencilOp StencilOp(Liverpool::StencilFunc op);

vk::CompareOp CompareOp(Liverpool::CompareFunc func);

bool IsPrimitiveCulled(AmdGpu::PrimitiveType type);

vk::PrimitiveTopology PrimitiveType(AmdGpu::PrimitiveType type);

vk::PolygonMode PolygonMode(Liverpool::PolygonMode mode);

vk::CullModeFlags CullMode(Liverpool::CullMode mode);

vk::FrontFace FrontFace(Liverpool::FrontFace mode);

vk::BlendFactor BlendFactor(Liverpool::BlendControl::BlendFactor factor);

bool IsDualSourceBlendFactor(Liverpool::BlendControl::BlendFactor factor);

vk::BlendOp BlendOp(Liverpool::BlendControl::BlendFunc func);

vk::LogicOp LogicOp(Liverpool::ColorControl::LogicOp logic_op);

vk::SamplerAddressMode ClampMode(AmdGpu::ClampMode mode);

vk::CompareOp DepthCompare(AmdGpu::DepthCompare comp);

vk::Filter Filter(AmdGpu::Filter filter);

vk::SamplerReductionMode FilterMode(AmdGpu::FilterMode mode);

vk::SamplerMipmapMode MipFilter(AmdGpu::MipFilter filter);

vk::BorderColor BorderColor(AmdGpu::BorderColor color);

vk::ComponentSwizzle ComponentSwizzle(AmdGpu::CompSwizzle comp_swizzle);

vk::ComponentMapping ComponentMapping(AmdGpu::CompMapping comp_mapping);

struct SurfaceFormatInfo {
    AmdGpu::DataFormat data_format;
    AmdGpu::NumberFormat number_format;
    vk::Format vk_format;
    vk::FormatFeatureFlags2 flags;
};
std::span<const SurfaceFormatInfo> SurfaceFormats();

vk::Format SurfaceFormat(AmdGpu::DataFormat data_format, AmdGpu::NumberFormat num_format);

struct DepthFormatInfo {
    Liverpool::DepthBuffer::ZFormat z_format;
    Liverpool::DepthBuffer::StencilFormat stencil_format;
    vk::Format vk_format;
    vk::FormatFeatureFlags2 flags;
};
std::span<const DepthFormatInfo> DepthFormats();

vk::Format DepthFormat(Liverpool::DepthBuffer::ZFormat z_format,
                       Liverpool::DepthBuffer::StencilFormat stencil_format);

vk::ClearValue ColorBufferClearValue(const AmdGpu::Liverpool::ColorBuffer& color_buffer);

vk::SampleCountFlagBits NumSamples(u32 num_samples, vk::SampleCountFlags supported_flags);

static inline bool IsFormatDepthCompatible(vk::Format fmt) {
    switch (fmt) {
    // 32-bit float compatible
    case vk::Format::eD32Sfloat:
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Uint:
    // 16-bit unorm compatible
    case vk::Format::eD16Unorm:
    case vk::Format::eR16Unorm:
        return true;
    default:
        return false;
    }
}

static inline bool IsFormatStencilCompatible(vk::Format fmt) {
    switch (fmt) {
    // 8-bit uint compatible
    case vk::Format::eS8Uint:
    case vk::Format::eR8Uint:
    case vk::Format::eR8Unorm:
        return true;
    default:
        return false;
    }
}

static inline vk::Format PromoteFormatToDepth(vk::Format fmt) {
    if (fmt == vk::Format::eR32Sfloat || fmt == vk::Format::eR32Uint) {
        return vk::Format::eD32Sfloat;
    } else if (fmt == vk::Format::eR16Unorm) {
        return vk::Format::eD16Unorm;
    }
    UNREACHABLE_MSG("Unexpected depth format {}", vk::to_string(fmt));
}

} // namespace Vulkan::LiverpoolToVK
