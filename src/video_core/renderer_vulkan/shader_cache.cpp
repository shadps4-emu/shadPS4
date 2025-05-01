#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include "common/path_util.h"
#include "common/io_file.h"
#include "common/binary_helper.h"
#include "shader_recompiler/info.h"

using u64 = uint64_t;

namespace ShaderCache {

const auto shader_cache_dir = Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "cache";

std::string CreateShaderID(u64 pgm_hash, size_t perm_idx, std::ostream& info_serialized, std::ostream& profile_serialized) {
    std::ostringstream info_stream, profile_stream;
    info_stream << pgm_hash << perm_idx;
    info_stream << info_serialized.rdbuf();
    profile_stream << profile_serialized.rdbuf();

    std::string combined_data = info_stream.str() + profile_stream.str();

    std::hash<std::string> hasher;
    size_t shader_id = hasher(combined_data);
    return std::to_string(shader_id);
}

void SerializeInfo(std::ostream& info_serialized, Shader::Info info) {
    writeBin(info_serialized, info.mrt_mask);
}

void DeserializeInfo(std::istream& info_serialized, Shader::Info& info) {
    readBin(info_serialized, info.mrt_mask);
}

void SerializeProfile(std::ostream& profile_serialized, Shader::Profile profile) {
    writeBin(profile_serialized, profile.has_broken_spirv_clamp);
}

void DeserializeProfile(std::istream& profile_serialized, Shader::Profile& profile) {
    readBin(profile_serialized, profile.has_broken_spirv_clamp);
}

bool CheckShaderCache(std::string shader_id) {
    return 0;
}

void GetShader(std::string shader_id, Shader::Info& info, Shader::Profile& profile) {
    std::string spirv_cache_filename = shader_id + ".spv ";
    std::filesystem::path spirv_cache_file_path = shader_cache_dir / spirv_cache_filename;
    Common::FS::IOFile spirv_cache_file(spirv_cache_file_path,
                                        Common::FS::FileAccessMode::Read);
    std::vector<u32> spv;
    spv.resize(spirv_cache_file.GetSize() / sizeof(u32));
    spirv_cache_file.Read(spv);
    spirv_cache_file.Close();

    std::filesystem::path resources_dump_file_path = shader_cache_dir / (shader_id + ".resources");
    Common::FS::IOFile resources_dump_file(resources_dump_file_path,
                                           Common::FS::FileAccessMode::Read);
    
       // Lese die Ressourcendaten
    std::vector<char> resources_data;
    resources_data.resize(resources_dump_file.GetSize());
    resources_dump_file.Read(resources_data);
    resources_dump_file.Close();

    // Verarbeite die gespeicherten Daten
    std::istringstream combined_stream(std::string(resources_data.begin(), resources_data.end()));

    // Deserialisiere info und profile
    std::istringstream info_stream;
    info_stream.str(std::string(resources_data.begin(), resources_data.end()));
    DeserializeInfo(info_stream, info);

    std::istringstream profile_stream;
    profile_stream.str(
        std::string(resources_data.begin() + info_stream.tellg(), resources_data.end()));
    DeserializeProfile(profile_stream, profile);
}

void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized, std::ostream& profile_serialized) {
    // SPIR-V-Datei speichern
    std::string spirv_cache_filename = shader_id + ".spv";
    std::filesystem::path spirv_cache_file_path = shader_cache_dir / spirv_cache_filename;
    Common::FS::IOFile shader_cache_file(spirv_cache_file_path, Common::FS::FileAccessMode::Write);
    shader_cache_file.WriteSpan(std::span<const u32>(spv));
    shader_cache_file.Close();

    // Resources-Datei vorbereiten
    std::filesystem::path resources_dump_file_path = shader_cache_dir / (shader_id + ".resources");
    Common::FS::IOFile resources_dump_file(resources_dump_file_path,
                                           Common::FS::FileAccessMode::Write);

    // Die Streams müssen zurückgesetzt werden, bevor wir sie lesen können
    if (std::ostringstream* info_oss = dynamic_cast<std::ostringstream*>(&info_serialized)) {
        std::string info_data = info_oss->str();
        resources_dump_file.WriteSpan(std::span<const char>(info_data.data(), info_data.size()));
    }

    if (std::ostringstream* profile_oss = dynamic_cast<std::ostringstream*>(&profile_serialized)) {
        std::string profile_data = profile_oss->str();
        resources_dump_file.WriteSpan(
            std::span<const char>(profile_data.data(), profile_data.size()));
    }

    resources_dump_file.Close();
}

} // namespace ShaderCache