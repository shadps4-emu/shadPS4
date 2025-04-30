#include <iostream>
#include <vector>
#include <functional>

namespace ShaderCache {

const auto shader_cache_dir = Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "cache";
std::string CreateShaderID(std::ostream& info_dump, std::ostream& profile_dump) {
    std::ostringstream info_stream, profile_stream;
    info_stream << info_dump.rdbuf();
    profile_stream << profile_dump.rdbuf();

    std::string combined_data = info_stream.str() + profile_stream.str();

    std::hash<std::string> hasher;
    size_t shader_id = hasher(combined_data);
    return std::to_string(shader_id);
}

void GetShader{

}
void AddShader(std::vector<u32> spv, std::ostream& info_dump, std::ostream& profile_dump) {
    
    std::string spirv_cache_filename = shader_name + ".spv ";
}

}