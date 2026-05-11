// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <vulkan/vulkan.hpp>

namespace AmdGpu {

enum class ShaderStage : u32 {
    Vertex = 0,
    Hull = 1,
    Domain = 2,
    Geometry = 3,
    Pixel = 4,
    Compute = 5,
};

enum class TessellationDomain : u32 {
    Isoline = 0,
    Triangle = 1,
    Quad = 2,
};

enum class TessellationPartitioning : u32 {
    Integer = 0,
    PowTwo = 1,
    FractionalOdd = 2,
    FractionalEven = 3,
};

enum class TessellationOutputPrimitive : u32 {
    Point = 0,
    Line = 1,
    TriangleCw = 2,
    TriangleCcw = 3,
};

struct HullShaderInfo {
    u32 output_control_points;
    TessellationDomain domain;
    TessellationPartitioning partitioning;
    TessellationOutputPrimitive output_primitive;
    bool use_even_distribution;
};

struct DomainShaderInfo {
    TessellationDomain domain;
};

} // namespace AmdGpu

namespace Vulkan::VkShaderStageConversion {

/**
 * Converts AmdGpu shader stage to Vulkan shader stage flag bits.
 * Hull shaders map to tessellation control shaders in Vulkan.
 */
vk::ShaderStageFlagBits ShaderStage(AmdGpu::ShaderStage stage);

/**
 * Returns true if the shader stage is part of the graphics pipeline.
 * Includes vertex, hull, domain, geometry, and pixel shaders.
 */
bool IsGraphicsStage(AmdGpu::ShaderStage stage);

/**
 * Returns true if the shader stage is a tessellation stage.
 * Includes hull (tessellation control) and domain (tessellation evaluation) shaders.
 */
bool IsTessellationStage(AmdGpu::ShaderStage stage);

/**
 * Converts AmdGpu shader stage to Vulkan pipeline stage flags.
 */
vk::PipelineStageFlags PipelineStage(AmdGpu::ShaderStage stage);

/**
 * Returns the maximum tessellation patch size supported by the hardware.
 */
u32 MaxTessellationPatchSize();

/**
 * Extracts the number of output control points from hull shader info.
 */
u32 GetHullShaderOutputControlPoints(const AmdGpu::HullShaderInfo& info);

/**
 * Converts tessellation domain to the appropriate primitive topology for domain shaders.
 */
vk::PrimitiveTopology GetDomainShaderTopology(AmdGpu::TessellationDomain domain);

/**
 * Determines the tessellation domain origin based on the domain type.
 */
vk::TessellationDomainOrigin TessellationDomainOrigin(AmdGpu::TessellationDomain domain);

/**
 * Returns a human-readable name for the shader stage (for debugging/logging).
 */
const char* GetShaderStageName(AmdGpu::ShaderStage stage);

} // namespace Vulkan::VkShaderStageConversion
