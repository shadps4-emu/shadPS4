// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan::LiverpoolToVK {

using Liverpool = AmdGpu::Liverpool;

vk::StencilOp StencilOp(Liverpool::StencilFunc op);

vk::CompareOp CompareOp(Liverpool::CompareFunc func);

vk::PrimitiveTopology PrimitiveType(Liverpool::PrimitiveType type);

vk::PolygonMode PolygonMode(Liverpool::PolygonMode mode);

vk::CullModeFlags CullMode(Liverpool::CullMode mode);

vk::BlendFactor BlendFactor(Liverpool::BlendControl::BlendFactor factor);

vk::BlendOp BlendOp(Liverpool::BlendControl::BlendFunc func);

vk::SamplerAddressMode ClampMode(AmdGpu::ClampMode mode);

vk::CompareOp DepthCompare(AmdGpu::DepthCompare comp);

vk::Filter Filter(AmdGpu::Filter filter);

vk::SamplerReductionMode FilterMode(AmdGpu::FilterMode mode);

vk::SamplerMipmapMode MipFilter(AmdGpu::MipFilter filter);

vk::BorderColor BorderColor(AmdGpu::BorderColor color);

std::span<const vk::Format> GetAllFormats();

vk::Format SurfaceFormat(AmdGpu::DataFormat data_format, AmdGpu::NumberFormat num_format);

vk::Format AdjustColorBufferFormat(vk::Format base_format,
                                   Liverpool::ColorBuffer::SwapMode comp_swap, bool is_vo_surface);

vk::Format DepthFormat(Liverpool::DepthBuffer::ZFormat z_format,
                       Liverpool::DepthBuffer::StencilFormat stencil_format);

vk::ClearValue ColorBufferClearValue(const AmdGpu::Liverpool::ColorBuffer& color_buffer);

vk::SampleCountFlagBits NumSamples(u32 num_samples, vk::SampleCountFlags supported_flags);

void EmitQuadToTriangleListIndices(u8* out_indices, u32 num_vertices);

} // namespace Vulkan::LiverpoolToVK
