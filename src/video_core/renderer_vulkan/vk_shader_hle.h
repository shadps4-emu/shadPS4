// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Shader {
struct Info;
}

namespace Vulkan {

class Rasterizer;

/// Attempts to execute a shader using HLE if possible.
bool ExecuteShaderHLE(const Shader::Info& info, Rasterizer& rasterizer);

} // namespace Vulkan
