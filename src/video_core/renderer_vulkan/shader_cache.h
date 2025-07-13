// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "shader_recompiler/info.h"
#include <shader_recompiler/specialization.h>

namespace ShaderCache {

u64 CalculateSpecializationHash(const Shader::StageSpecialization& spec);
void SerializeInfo(
    std::ostream& info_serialized, Shader::Info info);
void DeserializeInfo(std::istream& info_serialized, Shader::Info& info);

bool CheckShaderCache(std::string shader_id);
void GetShader(std::string shader_id, Shader::Info& info, std::vector<u32>& spv);
void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized);

} // namespace ShaderCache
