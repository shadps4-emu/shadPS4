// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"

namespace Vulkan::LiverpoolToVK {

vk::StencilOp StencilOp(Liverpool::StencilFunc op) {
    switch (op) {
    case Liverpool::StencilFunc::Keep:
        return vk::StencilOp::eKeep;
    case Liverpool::StencilFunc::Zero:
        return vk::StencilOp::eZero;
    case Liverpool::StencilFunc::AddClamp:
        return vk::StencilOp::eIncrementAndClamp;
    case Liverpool::StencilFunc::SubClamp:
        return vk::StencilOp::eDecrementAndClamp;
    case Liverpool::StencilFunc::Invert:
        return vk::StencilOp::eInvert;
    case Liverpool::StencilFunc::AddWrap:
        return vk::StencilOp::eIncrementAndWrap;
    case Liverpool::StencilFunc::SubWrap:
        return vk::StencilOp::eDecrementAndWrap;
    default:
        UNREACHABLE();
        return vk::StencilOp::eKeep;
    }
}

vk::CompareOp CompareOp(Liverpool::CompareFunc func) {
    switch (func) {
    case Liverpool::CompareFunc::Always:
        return vk::CompareOp::eAlways;
    case Liverpool::CompareFunc::Equal:
        return vk::CompareOp::eEqual;
    case Liverpool::CompareFunc::GreaterEqual:
        return vk::CompareOp::eGreaterOrEqual;
    case Liverpool::CompareFunc::Greater:
        return vk::CompareOp::eGreater;
    case Liverpool::CompareFunc::LessEqual:
        return vk::CompareOp::eLessOrEqual;
    case Liverpool::CompareFunc::Less:
        return vk::CompareOp::eLess;
    case Liverpool::CompareFunc::NotEqual:
        return vk::CompareOp::eNotEqual;
    case Liverpool::CompareFunc::Never:
        return vk::CompareOp::eNever;
    default:
        UNREACHABLE();
        return vk::CompareOp::eAlways;
    }
}

vk::PrimitiveTopology PrimitiveType(Liverpool::PrimitiveType type) {
    switch (type) {
    case Liverpool::PrimitiveType::PointList:
        return vk::PrimitiveTopology::ePointList;
    case Liverpool::PrimitiveType::LineList:
        return vk::PrimitiveTopology::eLineList;
    case Liverpool::PrimitiveType::LineStrip:
        return vk::PrimitiveTopology::eLineStrip;
    case Liverpool::PrimitiveType::TriangleList:
        return vk::PrimitiveTopology::eTriangleList;
    case Liverpool::PrimitiveType::TriangleFan:
        return vk::PrimitiveTopology::eTriangleFan;
    case Liverpool::PrimitiveType::TriangleStrip:
        return vk::PrimitiveTopology::eTriangleStrip;
    case Liverpool::PrimitiveType::AdjLineList:
        return vk::PrimitiveTopology::eLineListWithAdjacency;
    case Liverpool::PrimitiveType::AdjLineStrip:
        return vk::PrimitiveTopology::eLineStripWithAdjacency;
    case Liverpool::PrimitiveType::AdjTriangleList:
        return vk::PrimitiveTopology::eTriangleListWithAdjacency;
    case Liverpool::PrimitiveType::AdjTriangleStrip:
        return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
    case Liverpool::PrimitiveType::QuadList:
        // Needs to generate index buffer on the fly.
        return vk::PrimitiveTopology::eTriangleList;
    default:
        UNREACHABLE();
        return vk::PrimitiveTopology::eTriangleList;
    }
}

vk::PolygonMode PolygonMode(Liverpool::PolygonMode mode) {
    switch (mode) {
    case Liverpool::PolygonMode::Point:
        return vk::PolygonMode::ePoint;
    case Liverpool::PolygonMode::Line:
        return vk::PolygonMode::eLine;
    case Liverpool::PolygonMode::Fill:
        return vk::PolygonMode::eFill;
    default:
        UNREACHABLE();
        return vk::PolygonMode::eFill;
    }
}

vk::CullModeFlags CullMode(Liverpool::CullMode mode) {
    switch (mode) {
    case Liverpool::CullMode::None:
        return vk::CullModeFlagBits::eNone;
    case Liverpool::CullMode::Front:
        return vk::CullModeFlagBits::eFront;
    case Liverpool::CullMode::Back:
        return vk::CullModeFlagBits::eBack;
    case Liverpool::CullMode::FrontAndBack:
        return vk::CullModeFlagBits::eFrontAndBack;
    default:
        UNREACHABLE();
        return vk::CullModeFlagBits::eNone;
    }
}

vk::Format SurfaceFormat(AmdGpu::DataFormat data_format, AmdGpu::NumberFormat num_format) {
    if (data_format == AmdGpu::DataFormat::Format32_32_32_32 &&
        num_format == AmdGpu::NumberFormat::Float) {
        return vk::Format::eR32G32B32A32Sfloat;
    }
    if (data_format == AmdGpu::DataFormat::Format32_32_32 &&
        num_format == AmdGpu::NumberFormat::Uint) {
        return vk::Format::eR32G32B32Uint;
    }
    if (data_format == AmdGpu::DataFormat::Format8_8_8_8 &&
        num_format == AmdGpu::NumberFormat::Unorm) {
        return vk::Format::eR8G8B8A8Unorm;
    }
    if (data_format == AmdGpu::DataFormat::Format8_8_8_8 &&
        num_format == AmdGpu::NumberFormat::Srgb) {
        return vk::Format::eR8G8B8A8Srgb;
    }
    UNREACHABLE();
}

vk::Format DepthFormat(Liverpool::DepthBuffer::ZFormat z_format,
                       Liverpool::DepthBuffer::StencilFormat stencil_format) {
    UNREACHABLE();
}

void EmitQuadToTriangleListIndices(u8* out_ptr, u32 num_vertices) {
    static constexpr u16 NumVerticesPerQuad = 4;
    u16* out_data = reinterpret_cast<u16*>(out_ptr);
    for (u16 i = 0; i < num_vertices; i += NumVerticesPerQuad) {
        *out_data++ = i;
        *out_data++ = i + 1;
        *out_data++ = i + 2;
        *out_data++ = i + 2;
        *out_data++ = i;
        *out_data++ = i + 3;
    }
}

} // namespace Vulkan::LiverpoolToVK
