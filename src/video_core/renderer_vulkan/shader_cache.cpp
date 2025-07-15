// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <fstream>
#include <cereal/archives/binary.hpp>
#include "common/hash.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/type.h"
#include "shader_recompiler/specialization.h"
#include "video_core/renderer_vulkan/shader_cache_serialization.h"

#include "shader_cache.h"

using u64 = uint64_t;
using u32 = uint32_t;

namespace ShaderCache {


u64 CalculateSpecializationHash(const Shader::StageSpecialization& spec) {
    u64 hash = 0;

    const auto& runtime_info = spec.runtime_info;
    hash = HashCombine(hash, static_cast<u32>(runtime_info.stage));
    hash = HashCombine(hash, runtime_info.num_user_data);
    hash = HashCombine(hash, runtime_info.num_input_vgprs);
    hash = HashCombine(hash, runtime_info.num_allocated_vgprs);
    hash = HashCombine(hash, static_cast<u32>(runtime_info.fp_denorm_mode32));
    hash = HashCombine(hash, static_cast<u32>(runtime_info.fp_round_mode32));

    switch (runtime_info.stage) {
    case Shader::Stage::Local:
        hash = HashCombine(hash, runtime_info.ls_info.ls_stride);
        break;
    case Shader::Stage::Export:
        hash = HashCombine(hash, runtime_info.es_info.vertex_data_size);
        break;
    case Shader::Stage::Vertex:
        hash = HashCombine(hash, runtime_info.vs_info.num_outputs);
        for (size_t i = 0;
             i < runtime_info.vs_info.num_outputs && i < runtime_info.vs_info.outputs.size(); ++i) {
            const auto& output_map = runtime_info.vs_info.outputs[i];
            for (const auto& output : output_map) {
                hash = HashCombine(hash, static_cast<u32>(output));
            }
        }
        hash = HashCombine(hash, runtime_info.vs_info.emulate_depth_negative_one_to_one);
        hash = HashCombine(hash, runtime_info.vs_info.clip_disable);
        hash = HashCombine(hash, static_cast<u32>(runtime_info.vs_info.tess_type));
        hash = HashCombine(hash, static_cast<u32>(runtime_info.vs_info.tess_topology));
        hash = HashCombine(hash, static_cast<u32>(runtime_info.vs_info.tess_partitioning));
        hash = HashCombine(hash, runtime_info.vs_info.hs_output_cp_stride);
        break;
    case Shader::Stage::Hull:
        hash = HashCombine(hash, runtime_info.hs_info.num_input_control_points);
        hash = HashCombine(hash, runtime_info.hs_info.num_threads);
        hash = HashCombine(hash, static_cast<u32>(runtime_info.hs_info.tess_type));
        hash = HashCombine(hash, runtime_info.hs_info.ls_stride);
        hash = HashCombine(hash, runtime_info.hs_info.hs_output_cp_stride);
        hash = HashCombine(hash, runtime_info.hs_info.hs_output_base);
        break;
    case Shader::Stage::Geometry:
        hash = HashCombine(hash, runtime_info.gs_info.num_invocations);
        hash = HashCombine(hash, runtime_info.gs_info.output_vertices);
        hash = HashCombine(hash, runtime_info.gs_info.in_vertex_data_size);
        hash = HashCombine(hash, runtime_info.gs_info.out_vertex_data_size);
        hash = HashCombine(hash, static_cast<u32>(runtime_info.gs_info.in_primitive));
        for (const auto& out_prim : runtime_info.gs_info.out_primitive) {
            hash = HashCombine(hash, static_cast<u32>(out_prim));
        }
        hash = HashCombine(hash, runtime_info.gs_info.vs_copy_hash);
        break;
    case Shader::Stage::Fragment:
        hash = HashCombine(hash, runtime_info.fs_info.en_flags.raw);
        hash = HashCombine(hash, runtime_info.fs_info.addr_flags.raw);
        hash = HashCombine(hash, runtime_info.fs_info.num_inputs);

        for (u32 i = 0;
             i < runtime_info.fs_info.num_inputs && i < runtime_info.fs_info.inputs.size(); ++i) {
            const auto& input = runtime_info.fs_info.inputs[i];
            hash = HashCombine(hash, input.param_index);
            hash = HashCombine(hash, input.is_default);
            hash = HashCombine(hash, input.is_flat);
            hash = HashCombine(hash, input.default_value);
        }

        for (const auto& color_buffer : runtime_info.fs_info.color_buffers) {
            hash = HashCombine(hash, static_cast<u32>(color_buffer.num_format));
            hash = HashCombine(hash, static_cast<u32>(color_buffer.num_conversion));
            hash = HashCombine(hash, static_cast<u32>(color_buffer.export_format));
            hash = HashCombine(hash, color_buffer.needs_unorm_fixup);
            hash = HashCombine(hash, color_buffer.swizzle.r);
            hash = HashCombine(hash, color_buffer.swizzle.g);
            hash = HashCombine(hash, color_buffer.swizzle.b);
            hash = HashCombine(hash, color_buffer.swizzle.a);
        }
        break;
    case Shader::Stage::Compute:
        hash = HashCombine(hash, runtime_info.cs_info.shared_memory_size);
        for (u32 i = 0; i < 3; ++i) {
            hash = HashCombine(hash, runtime_info.cs_info.workgroup_size[i]);
            hash = HashCombine(hash, runtime_info.cs_info.tgid_enable[i]);
        }
        break;
    }

    if (spec.fetch_shader_data) {
        const auto& fetch_shader = *spec.fetch_shader_data;
        hash = HashCombine(hash, fetch_shader.size);
        hash = HashCombine(hash, static_cast<u64>(fetch_shader.vertex_offset_sgpr));
        hash = HashCombine(hash, static_cast<u64>(fetch_shader.instance_offset_sgpr));

        for (const auto& attr : fetch_shader.attributes) {
            hash = HashCombine(hash, static_cast<u64>(attr.semantic));
            hash = HashCombine(hash, static_cast<u64>(attr.dest_vgpr));
            hash = HashCombine(hash, static_cast<u64>(attr.num_elements));
            hash = HashCombine(hash, static_cast<u64>(attr.sgpr_base));
            hash = HashCombine(hash, static_cast<u64>(attr.dword_offset));
            hash = HashCombine(hash, static_cast<u64>(attr.instance_data));
        }
    }

    for (const auto& vs_attrib : spec.vs_attribs) {
        hash = HashCombine(hash, static_cast<u32>(vs_attrib.num_class));
        hash = HashCombine(hash, vs_attrib.dst_select.r);
        hash = HashCombine(hash, vs_attrib.dst_select.g);
        hash = HashCombine(hash, vs_attrib.dst_select.b);
        hash = HashCombine(hash, vs_attrib.dst_select.a);
    }

    const std::string bitset_str = spec.bitset.to_string();
    for (size_t i = 0; i < bitset_str.size(); i += 8) {
        size_t end = std::min(i + 8, bitset_str.size());
        std::string chunk = bitset_str.substr(i, end - i);
        u8 value = 0;
        for (size_t j = 0; j < chunk.size(); ++j) {
            if (chunk[j] == '1') {
                value |= (1 << j);
            }
        }
        hash = HashCombine(hash, value);
    }

    for (const auto& buffer : spec.buffers) {
        hash = HashCombine(hash, buffer.stride);
        hash = HashCombine(hash, buffer.is_storage);
        hash = HashCombine(hash, buffer.is_formatted);
        hash = HashCombine(hash, buffer.swizzle_enable);

        if (buffer.is_formatted) {
            hash = HashCombine(hash, buffer.data_format);
            hash = HashCombine(hash, buffer.num_format);
            hash = HashCombine(hash, buffer.dst_select.r);
            hash = HashCombine(hash, buffer.dst_select.g);
            hash = HashCombine(hash, buffer.dst_select.b);
            hash = HashCombine(hash, buffer.dst_select.a);
            hash = HashCombine(hash, static_cast<u32>(buffer.num_conversion));
        }

        if (buffer.swizzle_enable) {
            hash = HashCombine(hash, buffer.index_stride);
            hash = HashCombine(hash, buffer.element_size);
        }
    }

    for (const auto& image : spec.images) {
        hash = HashCombine(hash, static_cast<u32>(image.type));
        hash = HashCombine(hash, image.is_integer);
        hash = HashCombine(hash, image.is_storage);
        hash = HashCombine(hash, image.is_cube);

        if (image.is_storage) {
            hash = HashCombine(hash, image.dst_select.r);
            hash = HashCombine(hash, image.dst_select.g);
            hash = HashCombine(hash, image.dst_select.b);
            hash = HashCombine(hash, image.dst_select.a);
        }

        hash = HashCombine(hash, static_cast<u32>(image.num_conversion));
    }

    for (const auto& fmask : spec.fmasks) {
        hash = HashCombine(hash, fmask.width);
        hash = HashCombine(hash, fmask.height);
    }

    for (const auto& sampler : spec.samplers) {
        hash = HashCombine(hash, sampler.force_unnormalized);
    }

    hash = HashCombine(hash, spec.start.buffer);
    hash = HashCombine(hash, spec.start.unified);
    hash = HashCombine(hash, spec.start.user_data);

    if (spec.info) {
        hash = HashCombine(hash, spec.info->pgm_hash);
        hash = HashCombine(hash, static_cast<u32>(spec.info->stage));
        hash = HashCombine(hash, static_cast<u32>(spec.info->l_stage));
        hash = HashCombine(hash, spec.info->has_storage_images);
        hash = HashCombine(hash, spec.info->has_discard);
        hash = HashCombine(hash, spec.info->has_image_gather);
        hash = HashCombine(hash, spec.info->has_image_query);
        hash = HashCombine(hash, spec.info->uses_lane_id);
        hash = HashCombine(hash, spec.info->uses_group_quad);
        hash = HashCombine(hash, spec.info->uses_group_ballot);
        hash = HashCombine(hash, spec.info->uses_fp16);
        hash = HashCombine(hash, spec.info->uses_fp64);
        hash = HashCombine(hash, spec.info->uses_pack_10_11_11);
        hash = HashCombine(hash, spec.info->uses_unpack_10_11_11);
        hash = HashCombine(hash, spec.info->stores_tess_level_outer);
        hash = HashCombine(hash, spec.info->stores_tess_level_inner);
        hash = HashCombine(hash, spec.info->translation_failed);
        hash = HashCombine(hash, spec.info->mrt_mask);
        hash = HashCombine(hash, spec.info->has_fetch_shader);
        hash = HashCombine(hash, spec.info->fetch_shader_sgpr_base);

        for (size_t i = 0; i < spec.info->loads.flags.size(); ++i) {
            hash = HashCombine(hash, spec.info->loads.flags[i]);
        }

        for (size_t i = 0; i < spec.info->stores.flags.size(); ++i) {
            hash = HashCombine(hash, spec.info->stores.flags[i]);
        }

        hash = HashCombine(hash, spec.info->ud_mask.mask);

        hash = HashCombine(hash, spec.info->uses_patches);
    }

    return hash;
}

bool CheckShaderCache(std::string shader_id) {
    std::filesystem::path spirv_cache_file_path = SHADER_CACHE_DIR / static_cast<std::filesystem::path>(shader_id + ".spv");
    std::filesystem::path resources_file_path = SHADER_CACHE_DIR / static_cast<std::filesystem::path>(shader_id + ".resources");
;

    if (!std::filesystem::exists(spirv_cache_file_path)) {
        return false;
    }

    if (!std::filesystem::exists(resources_file_path)) {
        return false;
    }

    Common::FS::IOFile spirv_file(spirv_cache_file_path, Common::FS::FileAccessMode::Read);
    Common::FS::IOFile resources_file(resources_file_path, Common::FS::FileAccessMode::Read);

    const bool spirv_valid = spirv_file.IsOpen() && spirv_file.GetSize() > 0;
    const bool resources_valid = resources_file.IsOpen() && resources_file.GetSize() > 0;

    spirv_file.Close();
    resources_file.Close();

    if (!spirv_valid || !resources_valid) {
        LOG_WARNING(Render_Vulkan, "Invalid cache file for shader with ID: {}", shader_id);
        if (std::filesystem::exists(spirv_cache_file_path)) {
            std::filesystem::remove(spirv_cache_file_path);
        }
        if (std::filesystem::exists(resources_file_path)) {
            std::filesystem::remove(resources_file_path);
        }
        return false;
    }

    LOG_INFO(Render_Vulkan, "Found shader with ID {} in the cache", shader_id);
    return true;
}

void GetShader(std::string shader_id, Shader::Info& info, std::vector<u32>& spv) {
    // read spirv
    std::filesystem::path spirv_cache_filename = shader_id + ".spv";
    std::filesystem::path spirv_cache_file_path = SHADER_CACHE_DIR / spirv_cache_filename;
    Common::FS::IOFile spirv_cache_file(spirv_cache_file_path, Common::FS::FileAccessMode::Read);
    spv.resize(spirv_cache_file.GetSize() / sizeof(u32));
    spirv_cache_file.Read(spv);
    spirv_cache_file.Close();

    // read resources
    std::filesystem::path resource_dump_filename = shader_id + ".resources";
    std::filesystem::path resources_dump_file_path = SHADER_CACHE_DIR / resource_dump_filename;
    Common::FS::IOFile resources_dump_file(resources_dump_file_path,
                                           Common::FS::FileAccessMode::Read);

    std::vector<char> resources_data;
    resources_data.resize(resources_dump_file.GetSize());
    resources_dump_file.Read(resources_data);
    resources_dump_file.Close();

    std::istringstream combined_stream(std::string(resources_data.begin(), resources_data.end()));

    std::istringstream info_stream;
    info_stream.str(std::string(resources_data.begin(), resources_data.end()));
}

void AddShader(std::string shader_id, std::vector<u32> spv, std::ostream& info_serialized) {
    std::filesystem::path spirv_cache_filename = shader_id + ".spv";
    std::filesystem::path spirv_cache_file_path = SHADER_CACHE_DIR / spirv_cache_filename;
    Common::FS::IOFile shader_cache_file(spirv_cache_file_path, Common::FS::FileAccessMode::Write);
    shader_cache_file.WriteSpan(std::span<const u32>(spv));
    shader_cache_file.Close();

    std::filesystem::path resource_dump_filename = shader_id + ".resources";
    std::filesystem::path resources_dump_file_path = SHADER_CACHE_DIR / resource_dump_filename;
    Common::FS::IOFile resources_dump_file(resources_dump_file_path,
                                           Common::FS::FileAccessMode::Write);

    if (std::ostringstream* info_oss = dynamic_cast<std::ostringstream*>(&info_serialized)) {
        std::string info_data = info_oss->str();
        resources_dump_file.WriteSpan(std::span<const char>(info_data.data(), info_data.size()));
    }

    resources_dump_file.Close();
}

void SerializeInfo(std::ostream& info_serialized, Shader::Info &info) {
    cereal::BinaryOutputArchive ar(info_serialized);
    ar << info.ud_mask;
    ar << info.gs_copy_data;
    ar << info.uses_patches;
    ar << info.buffers;
    ar << info.images;
    ar << info.samplers;
    ar << info.fmasks;

}

} // namespace ShaderCache