#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "shader_recompiler/info.h"

namespace ShaderCache {

std::string CreateShaderID(u64 pgm_hash, size_t perm_idx, std::ostream& info_dump);
void SerializeInfo(std::ostream& info_serialized, Shader::Info info);
void DeserializeInfo(std::istream& info_serialized, Shader::Info& info);

bool CheckShaderCache(std::string shader_id);
void GetShader(std::string shader_id, Shader::Info& info);
void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized);

} // namespace ShaderCache
