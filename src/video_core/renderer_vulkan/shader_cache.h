#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "shader_recompiler/info.h"

namespace ShaderCache {

std::string CreateShaderID(u64 pgm_hash, size_t perm_idx, std::ostream& info_dump,
                           std::ostream& profile_dump);
void DumpInfo(std::ostream& info_dump, Shader::Info info);
void DumpProfile(std::ostream& profile_dump, Shader::Profile profile);
bool CheckShaderCache(std::string shader_id);
bool GetShader(std::string shader_id);
void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_dump,
               std::ostream& profile_dump);

} // namespace ShaderCache
