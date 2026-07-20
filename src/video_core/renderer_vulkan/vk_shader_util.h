// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "common/types.h"
#include "video_core/host_shaders/host_shader.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

/**
 * @brief Creates a vulkan shader module from GLSL by converting it to SPIR-V using glslang.
 *
 * @param source The named GLSL source used to identify its persistent cache.
 * @param stage The
 * pipeline stage the shader will be used in.
 * @param device The vulkan device handle.
 */
vk::ShaderModule Compile(const HostShaders::ShaderSource& source, vk::ShaderStageFlagBits stage,
                         vk::Device device, std::vector<std::string> defines = {});

/**
 * @brief Creates a vulkan shader module from SPIR-V bytecode.
 * @param code The SPIR-V bytecode data.
 * @param device The vulkan device handle
 */
vk::ShaderModule CompileSPV(std::span<const u32> code, vk::Device device);

} // namespace Vulkan
