// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan::LiverpoolToVK {

using Liverpool = AmdGpu::Liverpool;

vk::StencilOp StencilOp(Liverpool::StencilFunc op);

vk::CompareOp CompareOp(Liverpool::CompareFunc func);

vk::PrimitiveTopology PrimitiveType(Liverpool::PrimitiveType type);

vk::PolygonMode PolygonMode(Liverpool::PolygonMode mode);

vk::CullModeFlags CullMode(Liverpool::CullMode mode);

vk::Format SurfaceFormat(AmdGpu::DataFormat data_format, AmdGpu::NumberFormat num_format);

vk::Format DepthFormat(Liverpool::DepthBuffer::ZFormat z_format,
                       Liverpool::DepthBuffer::StencilFormat stencil_format);

void EmitQuadToTriangleListIndices(u8* out_indices, u32 num_vertices);

} // namespace Vulkan::LiverpoolToVK
