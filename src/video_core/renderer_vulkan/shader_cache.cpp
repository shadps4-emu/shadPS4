#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include "common/path_util.h"
#include "common/io_file.h"
#include "common/binary_helper.h"
#include "common/logging/log.h"
#include "shader_recompiler/info.h"

using u64 = uint64_t;
using u32 = uint32_t;

namespace ShaderCache {

const auto shader_cache_dir = Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "cache";

std::string CreateShaderID(u64 pgm_hash, size_t perm_idx, std::ostream& info_serialized) {
    std::ostringstream data_stream;
    data_stream << pgm_hash << perm_idx;
    data_stream << info_serialized.rdbuf();
    std::hash<std::string> hasher;
    size_t shader_id = hasher(data_stream.str());
    return std::to_string(shader_id);
}

void SerializeInfo(std::ostream& info_serialized, Shader::Info info) {
    // UD Mask
    writeBin(info_serialized, info.ud_mask.mask);

    // Buffer-Resources
    u32 count = static_cast<u32>(info.buffers.size());
    writeBin(info_serialized, count); // Buffer Amount
    LOG_INFO(Render_Recompiler, "ShaderCache: Buffer count: {}", info.buffers.size());

    for (const auto& buffer : info.buffers) {
        writeBin(info_serialized, buffer.sharp_idx);
        writeBin(info_serialized, static_cast<u32>(buffer.used_types));
        writeBin(info_serialized, static_cast<u32>(buffer.buffer_type));
        writeBin(info_serialized, buffer.instance_attrib);
        writeBin(info_serialized, static_cast<u8>(buffer.is_written ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(buffer.is_formatted ? 1 : 0));
    }

    // Image-Resources
    count = static_cast<u32>(info.images.size());
    writeBin(info_serialized, count); // Image Amount
    LOG_INFO(Render_Recompiler, "ShaderCache: Image count: {}", info.images.size());
    for (const auto& image : info.images) {
        writeBin(info_serialized, image.sharp_idx);
        writeBin(info_serialized, static_cast<u8>(image.is_depth ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_atomic ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_array ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_written ? 1 : 0));
    }

    // Sampler-Resources
    count = static_cast<u32>(info.samplers.size());
    writeBin(info_serialized, count); // Sampler Amount
    LOG_INFO(Render_Recompiler, "ShaderCache: Sampler count: {}", info.samplers.size());
    for (const auto& sampler : info.samplers) {
        writeBin(info_serialized, sampler.sharp_idx);
    }

    // FMask-Resources
    count = static_cast<u32>(info.fmasks.size());
    writeBin(info_serialized, count); // FMask Amount
    LOG_INFO(Render_Recompiler, "ShaderCache: FMask count: {}", info.fmasks.size());
    for (const auto& fmask : info.fmasks) {
        writeBin(info_serialized, fmask.sharp_idx);
    }

    // GS Copy Data
    writeBin(info_serialized, info.gs_copy_data.num_attrs);

    u32 mapCount = static_cast<u32>(info.gs_copy_data.attr_map.size());
    writeBin(info_serialized, mapCount);

    for (auto const& [loc, idx] : info.gs_copy_data.attr_map) {
        writeBin(info_serialized, loc);
        writeBin(info_serialized, idx);
    }
    
    // SRT Info
    u32 srtCount = static_cast<u32>(info.srt_info.srt_reservations.size());
    writeBin(info_serialized, count);

    for (const auto& res : info.srt_info.srt_reservations) {
        writeBin(info_serialized, res.sgpr_base);
        writeBin(info_serialized, res.dword_offset);
        writeBin(info_serialized, res.num_dwords);
    }

    // MRT Mask
    writeBin(info_serialized, info.mrt_mask);
}

void DeserializeInfo(std::istream& info_serialized, Shader::Info& info) {
    readBin(info_serialized, info.mrt_mask);
}

bool CheckShaderCache(std::string shader_id) {
    return 0;
}

void GetShader(std::string shader_id, Shader::Info& info) {
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

}

void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized) {
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

    resources_dump_file.Close();
}

} // namespace ShaderCache