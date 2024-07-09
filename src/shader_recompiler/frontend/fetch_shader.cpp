// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/static_vector.hpp>
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/fetch_shader.h"

namespace Shader::Gcn {

/**
 * s_load_dwordx4 s[8:11], s[2:3], 0x00
 * s_load_dwordx4 s[12:15], s[2:3], 0x04
 * s_load_dwordx4 s[16:19], s[2:3], 0x08
 * s_waitcnt     lgkmcnt(0)
 * buffer_load_format_xyzw v[4:7], v0, s[8:11], 0 idxen
 * buffer_load_format_xyz v[8:10], v0, s[12:15], 0 idxen
 * buffer_load_format_xy v[12:13], v0, s[16:19], 0 idxen
 * s_waitcnt     0
 * s_setpc_b64   s[0:1]

 * s_load_dwordx4  s[4:7], s[2:3], 0x0
 * s_waitcnt       lgkmcnt(0)
 * buffer_load_format_xyzw v[4:7], v0, s[4:7], 0 idxen
 * s_load_dwordx4  s[4:7], s[2:3], 0x8
 * s_waitcnt       lgkmcnt(0)
 * buffer_load_format_xyzw v[8:11], v0, s[4:7], 0 idxen
 * s_waitcnt       vmcnt(0) & expcnt(0) & lgkmcnt(0)
 * s_setpc_b64     s[0:1]

 * A normal fetch shader looks like the above, the instructions are generated
 * using input semantics on cpu side. Load instructions can either be separate or interleaved
 * We take the reverse way, extract the original input semantics from these instructions.
 **/

std::vector<VertexAttribute> ParseFetchShader(const u32* code, u32* out_size) {
    std::vector<VertexAttribute> attributes;
    GcnCodeSlice code_slice(code, code + std::numeric_limits<u32>::max());
    GcnDecodeContext decoder;

    struct VsharpLoad {
        u32 dword_offset{};
        s32 base_sgpr{};
        s32 dst_reg{-1};
    };
    boost::container::static_vector<VsharpLoad, 16> loads;

    u32 semantic_index = 0;
    while (!code_slice.atEnd()) {
        const auto inst = decoder.decodeInstruction(code_slice);
        *out_size += inst.length;

        if (inst.opcode == Opcode::S_SETPC_B64) {
            break;
        }

        if (inst.inst_class == InstClass::ScalarMemRd) {
            loads.emplace_back(inst.control.smrd.offset, inst.src[0].code * 2, inst.dst[0].code);
            continue;
        }

        if (inst.inst_class == InstClass::VectorMemBufFmt) {
            // SRSRC is in units of 4 SPGRs while SBASE is in pairs of SGPRs
            const u32 base_sgpr = inst.src[2].code * 4;

            // Find the load instruction that loaded the V# to the SPGR.
            // This is so we can determine its index in the vertex table.
            const auto it = std::ranges::find_if(
                loads, [&](VsharpLoad& load) { return load.dst_reg == base_sgpr; });

            auto& attrib = attributes.emplace_back();
            attrib.semantic = semantic_index++;
            attrib.dest_vgpr = inst.src[1].code;
            attrib.num_elements = inst.control.mubuf.count;
            attrib.sgpr_base = it->base_sgpr;
            attrib.dword_offset = it->dword_offset;

            // Store instance id rate
            attrib.instance_data = inst.src[0].code;

            // Mark load as used.
            it->dst_reg = -1;
        }
    }

    return attributes;
}

} // namespace Shader::Gcn
