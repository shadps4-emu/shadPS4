// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "common/types.h"

namespace Shader {
struct Profile;
struct RuntimeInfo;
} // namespace Shader

std::vector<u32> TranslateToSpirv(u64 raw_gcn_inst);
std::vector<u32> TranslateToSpirv(std::span<const u64> raw_gcn_insts);
std::vector<u32> TranslateFragmentPullModelToSpirv(const Shader::Profile& profile,
                                                   const Shader::RuntimeInfo& runtime_info);
std::vector<u32> TranslateFragmentFrontFaceToSpirv(const Shader::Profile& profile,
                                                   const Shader::RuntimeInfo& runtime_info);
std::vector<u32> TranslateFragmentSampleCoverageToSpirv(const Shader::Profile& profile,
                                                        const Shader::RuntimeInfo& runtime_info);
