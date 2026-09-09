// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <string>
#include <vector>

#include "common/types.h"
#include "video_core/host_shaders/host_shader.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

[[nodiscard]] vk::ShaderModule Compile(const HostShaders::ShaderSource& source,
                                       vk::ShaderStageFlagBits stage, vk::Device device,
                                       std::vector<std::string> defines = {});

[[nodiscard]] vk::ShaderModule CompileSPV(std::span<const u32> code, vk::Device device);

} // namespace Vulkan
