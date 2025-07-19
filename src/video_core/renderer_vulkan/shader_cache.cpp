// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cereal/archives/binary.hpp>

#include "common/config.h"
#include "common/hash.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "shader_cache.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/type.h"
#include "shader_recompiler/specialization.h"
#include "video_core/renderer_vulkan/shader_cache_serialization.h"

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
    if (Config::getShaderCachePreloadEnabled()) {
        return shader_cache.contains(shader_id);
    }

    return shader_registry.contains(shader_id);
}

void InitializeShaderCache() {
    if (!std::filesystem::exists(SHADER_CACHE_REGISTRY_PATH) ||
        std::filesystem::file_size(SHADER_CACHE_REGISTRY_PATH) == 0) {
        return;
    }
    std::ifstream registry_file(SHADER_CACHE_REGISTRY_PATH, std::ios::binary);
    cereal::BinaryInputArchive registry_ar(registry_file);
    while (registry_file.tellg() < std::filesystem::file_size(SHADER_CACHE_REGISTRY_PATH)) {
        std::string shader_key;
        u64 offset;
        registry_ar(shader_key, offset);
        shader_registry[shader_key] = offset;
    }
    if (Config::getShaderCachePreloadEnabled()) {
        std::ifstream blob_file(SHADER_CACHE_BLOB_PATH, std::ios::binary);
        for (auto const& [shader_key, offset] : shader_registry) {
            blob_file.seekg(offset, std::ios::beg);
            {
                cereal::BinaryInputArchive blob_ar(blob_file);
                std::string resources;
                std::vector<u32> spv;
                blob_ar(spv, resources);
                shader_cache[shader_key] = std::make_pair(spv, resources);
            }
        }
    }
}

void GetShader(std::string shader_id, Shader::Info& info, std::vector<u32>& spv) {
    std::string resources;
    if (Config::getShaderCachePreloadEnabled()) {
        auto& entry = shader_cache[shader_id];
        spv = entry.first;
        resources = entry.second;
    } else {
        std::ifstream blob_file(SHADER_CACHE_BLOB_PATH, std::ios::binary);
        blob_file.seekg(shader_registry[shader_id], std::ios::beg);
        cereal::BinaryInputArchive ar(blob_file);

        ar(spv, resources);
    }

    std::istringstream info_serialized(resources);
    DeserializeInfo(info_serialized, info);
}

void AddShader(std::string shader_id, std::vector<u32> spv, std::ostringstream& info_serialized) {
    std::ofstream registry_file(SHADER_CACHE_REGISTRY_PATH, std::ios::binary | std::ios::app);
    registry_file.seekp(0, std::ios::end);
    cereal::BinaryOutputArchive reg_ar(registry_file);

    std::ofstream blob_file(SHADER_CACHE_BLOB_PATH, std::ios::binary | std::ios::app);
    blob_file.seekp(0, std::ios::end);
    cereal::BinaryOutputArchive blob_ar(blob_file);

    u64 offset = static_cast<u64>(blob_file.tellp());
    reg_ar(shader_id, offset);

    std::string info_blob = info_serialized.str();
    blob_ar(spv, info_blob);

    shader_registry[shader_id] = offset;
    if (Config::getShaderCachePreloadEnabled()) {
        shader_cache[shader_id] = std::make_pair(spv, info_blob);
    }
}

void SerializeInfo(std::ostream& info_serialized, Shader::Info& info) {
    cereal::BinaryOutputArchive ar(info_serialized);
    ar << info.ud_mask;
    ar << info.gs_copy_data;
    ar << info.uses_patches;
    ar << info.buffers;
    ar << info.images;
    ar << info.samplers;
    ar << info.fmasks;
    ar << info.fs_interpolation;
    ar << info.tess_consts_ptr_base;
    ar << info.tess_consts_dword_offset;
    ar << info.stage;
    ar << info.l_stage;
    ar << info.pgm_hash;
    ar << info.pgm_base; // !
    ar << info.has_storage_images;
    ar << info.has_discard;
    ar << info.has_image_gather;
    ar << info.has_image_query;
    ar << info.uses_buffer_atomic_float_min_max;
    ar << info.uses_image_atomic_float_min_max;
    ar << info.uses_lane_id;
    ar << info.uses_group_quad;
    ar << info.uses_group_ballot;
    ar << info.shared_types;
    ar << info.uses_fp16;
    ar << info.uses_fp64;
    ar << info.uses_pack_10_11_11;
    ar << info.uses_unpack_10_11_11;
    ar << info.uses_buffer_int64_atomics;
    ar << info.uses_shared_int64_atomics;
    ar << info.stores_tess_level_outer;
    ar << info.stores_tess_level_inner;
    ar << info.translation_failed;
    ar << info.mrt_mask;
    ar << info.has_fetch_shader;
    ar << info.fetch_shader_sgpr_base; // !
    ar << info.readconst_types;
    ar << info.uses_dma;
    ar << info.srt_info.flattened_bufsize_dw;
}

void DeserializeInfo(std::istream& info_serialized, Shader::Info& info) {
    cereal::BinaryInputArchive ar(info_serialized);
    ar >> info.ud_mask;
    ar >> info.gs_copy_data;
    ar >> info.uses_patches;
    ar >> info.buffers;
    ar >> info.images;
    ar >> info.samplers;
    ar >> info.fmasks;
    ar >> info.fs_interpolation;
    ar >> info.tess_consts_ptr_base;
    ar >> info.tess_consts_dword_offset;
    ar >> info.stage;
    ar >> info.l_stage;
    ar >> info.pgm_hash;
    ar >> info.pgm_base; // !
    ar >> info.has_storage_images;
    ar >> info.has_discard;
    ar >> info.has_image_gather;
    ar >> info.has_image_query;
    ar >> info.uses_buffer_atomic_float_min_max;
    ar >> info.uses_image_atomic_float_min_max;
    ar >> info.uses_lane_id;
    ar >> info.uses_group_quad;
    ar >> info.uses_group_ballot;
    ar >> info.shared_types;
    ar >> info.uses_fp16;
    ar >> info.uses_fp64;
    ar >> info.uses_pack_10_11_11;
    ar >> info.uses_unpack_10_11_11;
    ar >> info.uses_buffer_int64_atomics;
    ar >> info.uses_shared_int64_atomics;
    ar >> info.stores_tess_level_outer;
    ar >> info.stores_tess_level_inner;
    ar >> info.translation_failed;
    ar >> info.mrt_mask;
    ar >> info.has_fetch_shader;
    ar >> info.fetch_shader_sgpr_base; // !
    ar >> info.readconst_types;
    ar >> info.uses_dma;
    ar >> info.srt_info.flattened_bufsize_dw;
}

} // namespace ShaderCache