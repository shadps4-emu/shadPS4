// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "common/elf_info.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/specialization.h"

namespace ShaderCache {

#define SHADER_CACHE_DIR                                                                           \
    (Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "cache" / "portable")

#define SHADER_CACHE_BLOB_PATH                                                                     \
    (SHADER_CACHE_DIR / (std::string{Common::ElfInfo::Instance().GameSerial()} + "_shaders.bin"))

#define SHADER_CACHE_REGISTRY_PATH                                                                 \
    (SHADER_CACHE_DIR /                                                                            \
     (std::string{Common::ElfInfo::Instance().GameSerial()} + "_shaders_registry.bin"))

inline std::map<std::string, u64> shader_registry; // shader_key:offset
inline std::map<std::string, std::pair<std::vector<u32>, std::string>> shader_cache;
// only used when preload active // shader_key:blob[spv,info]

u64 CalculateSpecializationHash(const Shader::StageSpecialization& spec);
void InitializeShaderCache();
void SerializeInfo(std::ostream& info_serialized, Shader::Info& info);
void DeserializeInfo(std::istream& info_serialized, Shader::Info& info);

bool CheckShaderCache(std::string shader_id);
void GetShader(std::string shader_id, Shader::Info& info, std::vector<u32>& spv);
void AddShader(std::string shader_id, std::vector<u32> spv, std::ostringstream& info_serialized);

} // namespace ShaderCache