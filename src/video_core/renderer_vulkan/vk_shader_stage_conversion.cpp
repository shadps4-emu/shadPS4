// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "video_core/renderer_vulkan/vk_shader_stage_conversion.h"

namespace Vulkan::VkShaderStageConversion {

vk::ShaderStageFlagBits ShaderStage(AmdGpu::ShaderStage stage) {
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
        UNREACHABLE();
        return vk::ShaderStageFlagBits::eVertex;
    }
}

bool IsGraphicsStage(AmdGpu::ShaderStage stage) {
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
        UNREACHABLE();
        return false;
    }
}

bool IsTessellationStage(AmdGpu::ShaderStage stage) {
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
        UNREACHABLE();
        return false;
    }
}

vk::PipelineStageFlags PipelineStage(AmdGpu::ShaderStage stage) {
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
        UNREACHABLE();
        return vk::PipelineStageFlagBits::eVertexShader;
    }
}

u32 MaxTessellationPatchSize() {
    // PS4 GCN tessellation patch size limit.
    return 32;
}

u32 GetHullShaderOutputControlPoints(const AmdGpu::HullShaderInfo& info) {
    return info.output_control_points;
}

vk::PrimitiveTopology GetDomainShaderTopology(AmdGpu::TessellationDomain domain) {
    switch (domain) {
    case AmdGpu::TessellationDomain::Isoline:
    case AmdGpu::TessellationDomain::Triangle:
    case AmdGpu::TessellationDomain::Quad:
        return vk::PrimitiveTopology::ePatchList;
    default:
        UNREACHABLE();
        return vk::PrimitiveTopology::ePatchList;
    }
}

vk::TessellationDomainOrigin TessellationDomainOrigin(AmdGpu::TessellationDomain domain) {
    switch (domain) {
    case AmdGpu::TessellationDomain::Isoline:
    case AmdGpu::TessellationDomain::Triangle:
        return vk::TessellationDomainOrigin::eLowerLeft;
    case AmdGpu::TessellationDomain::Quad:
        return vk::TessellationDomainOrigin::eUpperLeft;
    default:
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
        UNREACHABLE();
        return "Unknown";
    }
}

} // namespace Vulkan::VkShaderStageConversion
