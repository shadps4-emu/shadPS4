// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging.h"
#include "video_core/renderer_vulkan/vk_shader_stage_conversion.h"

namespace Vulkan::VkShaderStageConversion {

vk::ShaderStageFlagBits ShaderStage(AmdGpu::ShaderStage stage) {
    LOG_TRACE(Render_Vulkan, "Converting AmdGpu::ShaderStage: {}", static_cast<u32>(stage));
    switch (stage) {
    case AmdGpu::ShaderStage::Vertex:
        return vk::ShaderStageFlagBits::eVertex;
    case AmdGpu::ShaderStage::Hull:
        return vk::ShaderStageFlagBits::eTessellationControl;
    case AmdGpu::ShaderStage::Domain:
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case AmdGpu::ShaderStage::Geometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case AmdGpu::ShaderStage::Pixel:
        return vk::ShaderStageFlagBits::eFragment;
    case AmdGpu::ShaderStage::Compute:
        return vk::ShaderStageFlagBits::eCompute;
    default:
        LOG_ERROR(Render_Vulkan, "Invalid AmdGpu::ShaderStage: {}", static_cast<u32>(stage));
        UNREACHABLE();
        return vk::ShaderStageFlagBits::eVertex;
    }
}

bool IsGraphicsStage(AmdGpu::ShaderStage stage) {
    const bool is_graphics = (stage != AmdGpu::ShaderStage::Compute);
    LOG_TRACE(Render_Vulkan, "IsGraphicsStage: stage={}, result={}", static_cast<u32>(stage), is_graphics);
    switch (stage) {
    case AmdGpu::ShaderStage::Vertex:
    case AmdGpu::ShaderStage::Hull:
    case AmdGpu::ShaderStage::Domain:
    case AmdGpu::ShaderStage::Geometry:
    case AmdGpu::ShaderStage::Pixel:
        return true;
    case AmdGpu::ShaderStage::Compute:
        return false;
    default:
        LOG_ERROR(Render_Vulkan, "Unknown AmdGpu::ShaderStage in IsGraphicsStage: {}", static_cast<u32>(stage));
        UNREACHABLE();
        return false;
    }
}

bool IsTessellationStage(AmdGpu::ShaderStage stage) {
    const bool is_tess = (stage == AmdGpu::ShaderStage::Hull || stage == AmdGpu::ShaderStage::Domain);
    LOG_TRACE(Render_Vulkan, "IsTessellationStage: stage={}, result={}", static_cast<u32>(stage), is_tess);
    switch (stage) {
    case AmdGpu::ShaderStage::Hull:
    case AmdGpu::ShaderStage::Domain:
        return true;
    case AmdGpu::ShaderStage::Vertex:
    case AmdGpu::ShaderStage::Geometry:
    case AmdGpu::ShaderStage::Pixel:
    case AmdGpu::ShaderStage::Compute:
        return false;
    default:
        LOG_ERROR(Render_Vulkan, "Unknown AmdGpu::ShaderStage in IsTessellationStage: {}", static_cast<u32>(stage));
        UNREACHABLE();
        return false;
    }
}

vk::PipelineStageFlags PipelineStage(AmdGpu::ShaderStage stage) {
    LOG_TRACE(Render_Vulkan, "Mapping PipelineStage for AmdGpu::ShaderStage: {}", static_cast<u32>(stage));
    switch (stage) {
    case AmdGpu::ShaderStage::Vertex:
        return vk::PipelineStageFlagBits::eVertexShader;
    case AmdGpu::ShaderStage::Hull:
        return vk::PipelineStageFlagBits::eTessellationControlShader;
    case AmdGpu::ShaderStage::Domain:
        return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case AmdGpu::ShaderStage::Geometry:
        return vk::PipelineStageFlagBits::eGeometryShader;
    case AmdGpu::ShaderStage::Pixel:
        return vk::PipelineStageFlagBits::eFragmentShader;
    case AmdGpu::ShaderStage::Compute:
        return vk::PipelineStageFlagBits::eComputeShader;
    default:
        LOG_ERROR(Render_Vulkan, "Failed to map PipelineStage for stage: {}", static_cast<u32>(stage));
        UNREACHABLE();
        return vk::PipelineStageFlagBits::eVertexShader;
    }
}

u32 MaxTessellationPatchSize() {
    return 32;
}

u32 GetHullShaderOutputControlPoints(const AmdGpu::HullShaderInfo& info) {
    LOG_TRACE(Render_Vulkan, "HullShader output_control_points: {}", info.output_control_points);
    return info.output_control_points;
}

vk::PrimitiveTopology GetDomainShaderTopology(AmdGpu::TessellationDomain domain) {
    LOG_TRACE(Render_Vulkan, "GetDomainShaderTopology: domain={}", static_cast<u32>(domain));
    switch (domain) {
    case AmdGpu::TessellationDomain::Isoline:
    case AmdGpu::TessellationDomain::Triangle:
    case AmdGpu::TessellationDomain::Quad:
        return vk::PrimitiveTopology::ePatchList;
    default:
        LOG_ERROR(Render_Vulkan, "Unhandled TessellationDomain: {}", static_cast<u32>(domain));
        UNREACHABLE();
        return vk::PrimitiveTopology::ePatchList;
    }
}

vk::TessellationDomainOrigin TessellationDomainOrigin(AmdGpu::TessellationDomain domain) {
    LOG_TRACE(Render_Vulkan, "TessellationDomainOrigin mapping for domain: {}", static_cast<u32>(domain));
    switch (domain) {
    case AmdGpu::TessellationDomain::Isoline:
    case AmdGpu::TessellationDomain::Triangle:
        return vk::TessellationDomainOrigin::eLowerLeft;
    case AmdGpu::TessellationDomain::Quad:
        return vk::TessellationDomainOrigin::eUpperLeft;
    default:
        LOG_ERROR(Render_Vulkan, "Unknown TessellationDomain for origin mapping: {}", static_cast<u32>(domain));
        UNREACHABLE();
        return vk::TessellationDomainOrigin::eLowerLeft;
    }
}

const char* GetShaderStageName(AmdGpu::ShaderStage stage) {
    switch (stage) {
    case AmdGpu::ShaderStage::Vertex:
        return "Vertex";
    case AmdGpu::ShaderStage::Hull:
        return "Hull";
    case AmdGpu::ShaderStage::Domain:
        return "Domain";
    case AmdGpu::ShaderStage::Geometry:
        return "Geometry";
    case AmdGpu::ShaderStage::Pixel:
        return "Pixel";
    case AmdGpu::ShaderStage::Compute:
        return "Compute";
    default:
        LOG_DEBUG(Render_Vulkan, "Requesting name for unknown stage: {}", static_cast<u32>(stage));
        return "Unknown";
    }
}

} // namespace Vulkan::VkShaderStageConversion
