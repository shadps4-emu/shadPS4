#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "shader_recompiler/info.h"

namespace ShaderCache {

std::string CreateShaderID(u64 pgm_hash, size_t perm_idx, std::ostream& info_dump,
                           std::ostream& profile_dump);
void SerializeInfo(std::ostream& info_serialized, Shader::Info info);
void SerializeProfile(std::ostream& profile_serialized, Shader::Profile profile);
void DeserializeInfo(std::istream& info_serialized, Shader::Info& info);
void DeserializeProfile(std::istream& profile_serialized, Shader::Profile& profile);

bool CheckShaderCache(std::string shader_id);
void GetShader(std::string shader_id, Shader::Info& info, Shader::Profile& profile);
void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized,
               std::ostream& profile_serialized);

} // namespace ShaderCache
