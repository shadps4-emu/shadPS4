#include <iostream>
#include <vector>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "common/hash.h"
#include "common/path_util.h"
#include "common/io_file.h"
#include "common/binary_helper.h"
#include "common/logging/log.h"
#include "shader_recompiler/ir/type.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/specialization.h"

using u64 = uint64_t;
using u32 = uint32_t;

namespace ShaderCache {

const auto shader_cache_dir = Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "cache";
std::unordered_map<std::string, std::vector<u32>> g_ud_storage;

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
        hash = HashCombine(hash, runtime_info.ls_info.links_with_tcs);
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
        hash = HashCombine(hash, vs_attrib.num_components);
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
        hash = HashCombine(hash, spec.info->uses_shared);
        hash = HashCombine(hash, spec.info->uses_fp16);
        hash = HashCombine(hash, spec.info->uses_fp64);
        hash = HashCombine(hash, spec.info->uses_pack_10_11_11);
        hash = HashCombine(hash, spec.info->uses_unpack_10_11_11);
        hash = HashCombine(hash, spec.info->stores_tess_level_outer);
        hash = HashCombine(hash, spec.info->stores_tess_level_inner);
        hash = HashCombine(hash, spec.info->translation_failed);
        hash = HashCombine(hash, spec.info->has_readconst);
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

void SerializeInfo(std::ostream& info_serialized, Shader::Info info) {
    writeBin(info_serialized, info.ud_mask.mask);

    u32 bufferCount = static_cast<u32>(info.buffers.size());
    writeBin(info_serialized, bufferCount); // Buffer Amount

    for (const auto& buffer : info.buffers) {
        writeBin(info_serialized, buffer.sharp_idx);
        writeBin(info_serialized, static_cast<u32>(buffer.used_types));
        writeBin(info_serialized, static_cast<u32>(buffer.buffer_type));
        writeBin(info_serialized, buffer.instance_attrib);
        writeBin(info_serialized, static_cast<u8>(buffer.is_written ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(buffer.is_formatted ? 1 : 0));

        writeBin(info_serialized, buffer.inline_cbuf.base_address);
        writeBin(info_serialized, buffer.inline_cbuf._padding0);
        writeBin(info_serialized, buffer.inline_cbuf.stride);
        writeBin(info_serialized, buffer.inline_cbuf.cache_swizzle);
        writeBin(info_serialized, buffer.inline_cbuf.swizzle_enable);
        writeBin(info_serialized, buffer.inline_cbuf.num_records);
        writeBin(info_serialized, buffer.inline_cbuf.dst_sel_x);
        writeBin(info_serialized, buffer.inline_cbuf.dst_sel_y);
        writeBin(info_serialized, buffer.inline_cbuf.dst_sel_z);
        writeBin(info_serialized, buffer.inline_cbuf.dst_sel_w);
        writeBin(info_serialized, buffer.inline_cbuf.num_format);
        writeBin(info_serialized, buffer.inline_cbuf.data_format);
        writeBin(info_serialized, buffer.inline_cbuf.element_size);
        writeBin(info_serialized, buffer.inline_cbuf.index_stride);
        writeBin(info_serialized, buffer.inline_cbuf.add_tid_enable);
        writeBin(info_serialized, buffer.inline_cbuf._padding1);
        writeBin(info_serialized, buffer.inline_cbuf.type);

    }

    // Image-Resources
    u32 imageCount = static_cast<u32>(info.images.size());
    writeBin(info_serialized, imageCount); // Image Amount

    for (const auto& image : info.images) {
        writeBin(info_serialized, image.sharp_idx);
        writeBin(info_serialized, static_cast<u8>(image.is_depth ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_atomic ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_array ? 1 : 0));
        writeBin(info_serialized, static_cast<u8>(image.is_written ? 1 : 0));
    }

    // Sampler-Resources
    u32 samplerCount = static_cast<u32>(info.samplers.size());
    writeBin(info_serialized, samplerCount); // Sampler Amount

    for (const auto& sampler : info.samplers) {
        writeBin(info_serialized, sampler.sharp_idx);
        writeBin(info_serialized, sampler.associated_image);
        writeBin(info_serialized, sampler.disable_aniso);

        writeBin(info_serialized, sampler.inline_sampler.raw0);
        writeBin(info_serialized, sampler.inline_sampler.raw1);


    }

    // FMask-Resources
    u32 fmaskCount = static_cast<u32>(info.fmasks.size());
    writeBin(info_serialized, fmaskCount); // FMask Amount

    for (const auto& fmask : info.fmasks) {
        writeBin(info_serialized, fmask.sharp_idx);
    }

    // GS Copy Data
    u32 mapCount = static_cast<u32>(info.gs_copy_data.attr_map.size());
    writeBin(info_serialized, mapCount);

    for (auto const& [loc, attr_pair] : info.gs_copy_data.attr_map) {
        writeBin(info_serialized, loc);
        // Das erste Element des Paars ist ein Shader::IR::Attribute, ein Enum
        writeBin(info_serialized, static_cast<u32>(attr_pair.first));
        // Das zweite Element ist ein u32
        writeBin(info_serialized, attr_pair.second);
    }
    
    // SRT Info
    u32 srtCount = static_cast<u32>(info.srt_info.srt_reservations.size());
    writeBin(info_serialized, srtCount);

    for (const auto& res : info.srt_info.srt_reservations) {
        writeBin(info_serialized, res.sgpr_base);
        writeBin(info_serialized, res.dword_offset);
        writeBin(info_serialized, res.num_dwords);
    }

    writeBin(info_serialized, info.srt_info.flattened_bufsize_dw);
    bool has_walker_func = (info.srt_info.walker_func != nullptr);
    writeBin(info_serialized, static_cast<u8>(has_walker_func ? 1 : 0));

    if (has_walker_func) {
        // Größe des JIT-Codes ermitteln
        const u8* walker_start = reinterpret_cast<const u8*>(info.srt_info.walker_func);

        // Wir müssen die Größe des generierten Codes bestimmen
        // Dies kann aus der Xbyak::CodeGenerator Instanz extrahiert werden
        // Alternativ können wir die Distanz zum nächsten Ret-Befehl berechnen
        size_t walker_size = 0;
        const u8* ptr = walker_start;
        const u32 MAX_CODE_SIZE = 4096; // Sicherheitsbegrenzung

        // Suche nach Ret-Befehl (C3 in x86/x64)
        for (walker_size = 0; walker_size < MAX_CODE_SIZE; walker_size++) {
            // Einfacher Ret-Befehl (C3)
            if (ptr[walker_size] == 0xC3) {
                walker_size++; // Ret einschließen
                break;
            }
        }

        // Speichere Größe und JIT-Code
        writeBin(info_serialized, static_cast<u32>(walker_size));
        info_serialized.write(reinterpret_cast<const char*>(walker_start), walker_size);
    }
    // Flat UD

    u32 flatCount = static_cast<u32>(info.flattened_ud_buf.size());
    writeBin(info_serialized, flatCount);

    for (const auto& flat : info.flattened_ud_buf) {
        writeBin(info_serialized, flat);
    }

    // Tessellation Data
    writeBin(info_serialized, info.tess_consts_ptr_base);
    writeBin(info_serialized, info.tess_consts_dword_offset);

    // Flags
    writeBin(info_serialized, static_cast<u8>(info.has_storage_images ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.has_discard ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.has_image_gather ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.has_image_query ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_lane_id ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_group_quad ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_group_ballot ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_shared ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_fp16 ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_fp64 ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_pack_10_11_11 ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.uses_unpack_10_11_11 ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.stores_tess_level_outer ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.stores_tess_level_inner ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.translation_failed ? 1 : 0));
    writeBin(info_serialized, static_cast<u8>(info.has_readconst ? 1 : 0));

    // MRT Mask
    writeBin(info_serialized, info.mrt_mask);

    // Fetch

    writeBin(info_serialized, static_cast<u8>(info.has_fetch_shader ? 1 : 0));
    writeBin(info_serialized, info.fetch_shader_sgpr_base);

    // Stage
    writeBin(info_serialized, info.stage);
    writeBin(info_serialized, info.l_stage);
    writeBin(info_serialized, info.pgm_hash);

    // AttributeFlags für loads
    u32 loads_size = static_cast<u32>(info.loads.flags.size());
    writeBin(info_serialized, loads_size);
    for (size_t i = 0; i < info.loads.flags.size(); ++i) {
        writeBin(info_serialized, info.loads.flags[i]);
    }

    // AttributeFlags für stores
    u32 stores_size = static_cast<u32>(info.stores.flags.size());
    writeBin(info_serialized, stores_size);
    for (size_t i = 0; i < info.stores.flags.size(); ++i) {
        writeBin(info_serialized, info.stores.flags[i]);
    }

    // UserData
    u32 userDataSize = static_cast<u32>(info.user_data.size());
    writeBin(info_serialized, userDataSize);
    for (size_t i = 0; i < info.user_data.size(); ++i) {
        writeBin(info_serialized, info.user_data[i]);
    }

    // Pgm Base
    writeBin(info_serialized, info.pgm_base);
}

void DeserializeInfo(std::istream& info_serialized, Shader::Info& info) {
    // UD Mask
    readBin(info_serialized, info.ud_mask.mask);

    // Buffer-Resources
    u32 bufferCount;
    readBin(info_serialized, bufferCount);

    info.buffers.clear();
    info.buffers.reserve(bufferCount);
    for (u32 i = 0; i < bufferCount; ++i) {
        Shader::BufferResource buffer;
        readBin(info_serialized, buffer.sharp_idx);
        u32 used_types;
        readBin(info_serialized, used_types);
        buffer.used_types = static_cast<Shader::IR::Type>(used_types);
        u32 buffer_type;
        readBin(info_serialized, buffer_type);
        buffer.buffer_type = static_cast<Shader::BufferType>(buffer_type);
        readBin(info_serialized, buffer.instance_attrib);
        u8 is_written;
        readBin(info_serialized, is_written);
        buffer.is_written = (is_written == 1);
        u8 is_formatted;
        readBin(info_serialized, is_formatted);
        buffer.is_formatted = (is_formatted == 1);
        
        u64 base_address;
        readBin(info_serialized, base_address);
        buffer.inline_cbuf.base_address = base_address;

        u64 padding0;
        readBin(info_serialized, padding0);
        buffer.inline_cbuf._padding0 = padding0;

        u64 stride;
        readBin(info_serialized, stride);
        buffer.inline_cbuf.stride = stride;

        u64 cache_swizzle;
        readBin(info_serialized, cache_swizzle);
        buffer.inline_cbuf.cache_swizzle = cache_swizzle;

        u64 swizzle_enable;
        readBin(info_serialized, swizzle_enable);
        buffer.inline_cbuf.swizzle_enable = swizzle_enable;

        readBin(info_serialized, buffer.inline_cbuf.num_records);

        u32 dst_sel_x;
        readBin(info_serialized, dst_sel_x);
        buffer.inline_cbuf.dst_sel_x = dst_sel_x;

        u32 dst_sel_y;
        readBin(info_serialized, dst_sel_y);
        buffer.inline_cbuf.dst_sel_y = dst_sel_y;

        u32 dst_sel_z;
        readBin(info_serialized, dst_sel_z);
        buffer.inline_cbuf.dst_sel_z = dst_sel_z;

        u32 dst_sel_w;
        readBin(info_serialized, dst_sel_w);
        buffer.inline_cbuf.dst_sel_w = dst_sel_w;

        u32 num_format;
        readBin(info_serialized, num_format);
        buffer.inline_cbuf.num_format = num_format;

        u32 data_format;
        readBin(info_serialized, data_format);
        buffer.inline_cbuf.data_format = data_format;

        u32 element_size;
        readBin(info_serialized, element_size);
        buffer.inline_cbuf.element_size = element_size;

        u32 index_stride;
        readBin(info_serialized, index_stride);
        buffer.inline_cbuf.index_stride = index_stride;

        u32 add_tid_enable;
        readBin(info_serialized, add_tid_enable);
        buffer.inline_cbuf.add_tid_enable = add_tid_enable;

        u32 padding1;
        readBin(info_serialized, padding1);
        buffer.inline_cbuf._padding1 = padding1;

        u32 type;
        readBin(info_serialized, type);
        buffer.inline_cbuf.type = type;

        info.buffers.push_back(std::move(buffer));
    }

    // Image-Resources
    u32 imageCount;
    readBin(info_serialized, imageCount);

    info.images.clear();
    info.images.reserve(imageCount);
    for (u32 i = 0; i < imageCount; ++i) {
        Shader::ImageResource image;
        readBin(info_serialized, image.sharp_idx);
        u8 is_depth;
        readBin(info_serialized, is_depth);
        image.is_depth = (is_depth == 1);
        u8 is_atomic;
        readBin(info_serialized, is_atomic);
        image.is_atomic = (is_atomic == 1);
        u8 is_array;
        readBin(info_serialized, is_array);
        image.is_array = (is_array == 1);
        u8 is_written;
        readBin(info_serialized, is_written);
        image.is_written = (is_written == 1);
        info.images.push_back(std::move(image));
    }

    // Sampler-Resources
    u32 samplerCount;
    readBin(info_serialized, samplerCount);

    info.samplers.clear();
    info.samplers.reserve(samplerCount);
    for (u32 i = 0; i < samplerCount; ++i) {
        Shader::SamplerResource sampler;
        readBin(info_serialized, sampler.sharp_idx);

        u32 associated_image;
        readBin(info_serialized, associated_image);
        sampler.associated_image = associated_image;

        u32 disable_aniso;
        readBin(info_serialized, disable_aniso);
        sampler.disable_aniso = disable_aniso;

        // Inline-Sampler deserialisieren
        readBin(info_serialized, sampler.inline_sampler.raw0);
        readBin(info_serialized, sampler.inline_sampler.raw1);
        
        info.samplers.push_back(std::move(sampler));
    }

    // FMask-Resources
    u32 fmaskCount;
    readBin(info_serialized, fmaskCount);

    info.fmasks.clear();
    info.fmasks.reserve(fmaskCount);
    for (u32 i = 0; i < fmaskCount; ++i) {
        Shader::FMaskResource fmask;
        readBin(info_serialized, fmask.sharp_idx);
        info.fmasks.push_back(std::move(fmask));
    }

    // GS Copy Data
    u32 mapCount;
    readBin(info_serialized, mapCount);

    info.gs_copy_data.attr_map.clear();
    for (u32 i = 0; i < mapCount; ++i) {
        u32 loc;
        u32 attribute_value;
        u32 idx;
        readBin(info_serialized, loc);
        readBin(info_serialized, attribute_value);
        readBin(info_serialized, idx);

        // Umwandeln des numerischen Werts zurück in das Shader::IR::Attribute-Enum
        Shader::IR::Attribute attribute = static_cast<Shader::IR::Attribute>(attribute_value);

        // Einfügen in die Map mit dem richtigen Paar-Typ
        info.gs_copy_data.attr_map.emplace(loc, std::make_pair(attribute, idx));
    }

    // SRT Info
    u32 srtCount;
    readBin(info_serialized, srtCount);

    info.srt_info.srt_reservations.clear();
    info.srt_info.srt_reservations.resize(srtCount);
    for (u32 i = 0; i < srtCount; ++i) {
        auto& res = info.srt_info.srt_reservations[i];
        readBin(info_serialized, res.sgpr_base);
        readBin(info_serialized, res.dword_offset);
        readBin(info_serialized, res.num_dwords);
    }

    readBin(info_serialized, info.srt_info.flattened_bufsize_dw);
    // Laden des walker_func JIT-Codes
    u8 has_walker_func;
    readBin(info_serialized, has_walker_func);

    if (has_walker_func == 1) {
        // Größe des JIT-Codes lesen
        u32 walker_size;
        readBin(info_serialized, walker_size);

        // Speicher für ausführbaren Code allokieren
        void* code_memory = nullptr;
#ifdef _WIN32
        code_memory =
            VirtualAlloc(nullptr, walker_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
        code_memory = mmap(nullptr, walker_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

        if (!code_memory) {
            LOG_ERROR(Render_Vulkan, "Konnte keinen ausführbaren Speicher für JIT-Code allokieren");
            return;
        }

        // JIT-Code laden
        info_serialized.read(reinterpret_cast<char*>(code_memory), walker_size);

        // JIT-Funktion zuweisen
        info.srt_info.walker_func = reinterpret_cast<Shader::PFN_SrtWalker>(code_memory);
    }
    // Flat UD

    u32 flatCount;
    readBin(info_serialized, flatCount);

    info.flattened_ud_buf.clear();
    u32 required_size = std::max(flatCount, info.srt_info.flattened_bufsize_dw);
    info.flattened_ud_buf.resize(required_size);

    for (u32 i = 0; i < flatCount; ++i) {
        readBin(info_serialized, info.flattened_ud_buf[i]);
    }

    // Tessellation Data
    readBin(info_serialized, info.tess_consts_ptr_base);
    readBin(info_serialized, info.tess_consts_dword_offset);

    // Flags
    u8 flag_value;
    readBin(info_serialized, flag_value);
    info.has_storage_images = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.has_discard = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.has_image_gather = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.has_image_query = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_lane_id = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_group_quad = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_group_ballot = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_shared = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_fp16 = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_fp64 = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_pack_10_11_11 = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.uses_unpack_10_11_11 = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.stores_tess_level_outer = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.stores_tess_level_inner = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.translation_failed = (flag_value == 1);
    readBin(info_serialized, flag_value);
    info.has_readconst = (flag_value == 1);

    // MRT Mask
    readBin(info_serialized, info.mrt_mask);

    // Fetch Shader
    u8 has_fetch_shader;
    readBin(info_serialized, has_fetch_shader);
    info.has_fetch_shader = (has_fetch_shader == 1);
    readBin(info_serialized, info.fetch_shader_sgpr_base);

    // Stage
    readBin(info_serialized, info.stage);
    readBin(info_serialized, info.l_stage);
    readBin(info_serialized, info.pgm_hash);

    // AttributeFlags für loads
    u32 loads_size;
    readBin(info_serialized, loads_size);
    for (size_t i = 0; i < loads_size; ++i) {
        readBin(info_serialized, info.loads.flags[i]);
    }

    // AttributeFlags für stores
    u32 stores_size;
    readBin(info_serialized, stores_size);
    for (size_t i = 0; i < stores_size; ++i) {
        readBin(info_serialized, info.stores.flags[i]);
    }

    // UserData
    u32 userDataSize;
    readBin(info_serialized, userDataSize);

    static std::vector<u32> temp_user_data_storage;
    temp_user_data_storage.clear();
    temp_user_data_storage.resize(userDataSize);

    for (u32 i = 0; i < userDataSize; ++i) {
        readBin(info_serialized, temp_user_data_storage[i]);
    }

    info.user_data = std::span<const u32>(temp_user_data_storage);

    // Pgm Base
    readBin(info_serialized, info.pgm_base);


    // Check if there are any remaining bytes in the stream
    if (info_serialized.peek() != EOF) {
        LOG_WARNING(Render_Vulkan, "Es sind noch Bytes im Stream übrig");
    }
}

bool CheckShaderCache(std::string shader_id) {
    // Überprüfen, ob das Verzeichnis existiert
    if (!std::filesystem::exists(shader_cache_dir)) {
        LOG_INFO(Render_Vulkan, "Shader-Cache-Verzeichnis existiert nicht");
        return false;
    }

    // Überprüfen, ob sowohl die SPIR-V-Datei als auch die Ressourcendatei existieren
    std::filesystem::path spirv_cache_file_path = shader_cache_dir / (shader_id + ".spv");
    std::filesystem::path resources_file_path = shader_cache_dir / (shader_id + ".resources");

    if (!std::filesystem::exists(spirv_cache_file_path)) {
        LOG_DEBUG(Render_Vulkan, "SPIR-V-Datei nicht gefunden: {}", spirv_cache_file_path.string());
        return false;
    }

    if (!std::filesystem::exists(resources_file_path)) {
        LOG_DEBUG(Render_Vulkan, "Ressourcendatei nicht gefunden: {}",
                  resources_file_path.string());
        return false;
    }

    // Überprüfen, ob die Dateien lesbar und nicht leer sind
    Common::FS::IOFile spirv_file(spirv_cache_file_path, Common::FS::FileAccessMode::Read);
    Common::FS::IOFile resources_file(resources_file_path, Common::FS::FileAccessMode::Read);

    const bool spirv_valid = spirv_file.IsOpen() && spirv_file.GetSize() > 0;
    const bool resources_valid = resources_file.IsOpen() && resources_file.GetSize() > 0;

    spirv_file.Close();
    resources_file.Close();

    if (!spirv_valid || !resources_valid) {
        LOG_WARNING(Render_Vulkan, "Ungueltige Dateien im Shader-Cache für ID: {}", shader_id);
        // Fehlerhafte Dateien entfernen, um zukünftige Probleme zu vermeiden
        if (std::filesystem::exists(spirv_cache_file_path)) {
            std::filesystem::remove(spirv_cache_file_path);
        }
        if (std::filesystem::exists(resources_file_path)) {
            std::filesystem::remove(resources_file_path);
        }
        return false;
    }

    LOG_INFO(Render_Vulkan, "Shader mit ID {} im Cache gefunden", shader_id);
    return true;
}

void GetShader(std::string shader_id, Shader::Info& info, std::vector<u32>& spv) {
    std::string spirv_cache_filename = shader_id + ".spv";
    std::filesystem::path spirv_cache_file_path = shader_cache_dir / spirv_cache_filename;
    Common::FS::IOFile spirv_cache_file(spirv_cache_file_path,
                                        Common::FS::FileAccessMode::Read);
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