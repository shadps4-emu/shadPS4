// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "common/assert.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/regs_color.h"
#include "video_core/amdgpu/regs_depth.h"
#include "video_core/amdgpu/regs_primitive.h"
#include "video_core/amdgpu/regs_vertex.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan::LiverpoolToVK {

vk::StencilOp StencilOp(AmdGpu::StencilFunc op);

vk::CompareOp CompareOp(AmdGpu::CompareFunc func);

bool IsPrimitiveCulled(AmdGpu::PrimitiveType type);

vk::PrimitiveTopology PrimitiveType(AmdGpu::PrimitiveType type);

vk::PolygonMode PolygonMode(AmdGpu::PolygonMode mode);

vk::CullModeFlags CullMode(AmdGpu::CullMode mode);

vk::FrontFace FrontFace(AmdGpu::FrontFace mode);

vk::BlendFactor BlendFactor(AmdGpu::BlendControl::BlendFactor factor);

bool IsDualSourceBlendFactor(AmdGpu::BlendControl::BlendFactor factor);

vk::BlendOp BlendOp(AmdGpu::BlendControl::BlendFunc func);

void NormalizeMinMaxBlend(vk::BlendOp& op, vk::BlendFactor& src, vk::BlendFactor& dst,
                          AmdGpu::NumberFormat format);

vk::LogicOp LogicOp(AmdGpu::ColorControl::LogicOp logic_op);

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

struct DepthFormatInfo {
    AmdGpu::DepthBuffer::ZFormat z_format;
    AmdGpu::DepthBuffer::StencilFormat stencil_format;
    vk::Format vk_format;
    vk::FormatFeatureFlags2 flags;
};
std::span<const DepthFormatInfo> DepthFormats();

vk::Format DepthFormat(AmdGpu::DepthBuffer::ZFormat z_format,
                       AmdGpu::DepthBuffer::StencilFormat stencil_format);

vk::ClearValue ColorBufferClearValue(const AmdGpu::ColorBuffer& color_buffer);

vk::SampleCountFlagBits NumSamples(u32 num_samples, vk::SampleCountFlags supported_flags);

// Table 8.13 Data and Image Formats [Sea Islands Series Instruction Set Architecture]
constexpr size_t amd_gpu_data_format_bit_size = 6;   // All values are under 64
constexpr size_t amd_gpu_number_format_bit_size = 4; // All values are under 16
constexpr size_t surface_format_table_size =
    (1u << amd_gpu_data_format_bit_size) * (1u << amd_gpu_number_format_bit_size);

extern const std::array<vk::Format, surface_format_table_size> surface_format_table;

constexpr size_t GetSurfaceFormatTableIndex(AmdGpu::DataFormat data_format,
                                            AmdGpu::NumberFormat num_format) {
    DEBUG_ASSERT(u32(data_format) < 1 << amd_gpu_data_format_bit_size);
    DEBUG_ASSERT(u32(num_format) < 1 << amd_gpu_number_format_bit_size);
    size_t result = static_cast<size_t>(num_format) |
                    (static_cast<size_t>(data_format) << amd_gpu_number_format_bit_size);
    return result;
}

constexpr vk::Format SurfaceFormat(AmdGpu::DataFormat data_format,
                                   AmdGpu::NumberFormat num_format) noexcept {
    vk::Format result = surface_format_table[GetSurfaceFormatTableIndex(data_format, num_format)];
    bool found =
        result != vk::Format::eUndefined || data_format == AmdGpu::DataFormat::FormatInvalid;
    ASSERT_MSG(found, "Unknown data_format={} and num_format={}", static_cast<u32>(data_format),
               static_cast<u32>(num_format));
    return result;
}

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
