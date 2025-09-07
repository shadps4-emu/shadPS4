// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/amdgpu/liverpool.h"

namespace Shader {
struct Info;
}

namespace Vulkan {

class Rasterizer;

/// Attempts to execute a shader using HLE if possible.
bool ExecuteShaderHLE(const Shader::Info& info, const AmdGpu::Liverpool::Regs& regs,
                      const AmdGpu::Liverpool::ComputeProgram& cs_program, Rasterizer& rasterizer);

} // namespace Vulkan
