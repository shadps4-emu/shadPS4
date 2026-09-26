// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include "common/assert.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "shader_recompiler/frontend/opcodes.h"

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

const u32* GetFetchShaderCode(const Info& info, u32 sgpr_base) {
    const u32* code;
    std::memcpy(&code, &info.user_data[sgpr_base], sizeof(code));
    return code;
}

bool ParseFetchShader(const Shader::Info& info, FetchShaderData& out_fetch_data) {
    if (!info.has_fetch_shader) {
        return false;
    }

    out_fetch_data.attributes.clear();

    struct VsharpLoad {
        u32 dword_offset{};
        u32 base_sgpr{};
    };
    std::array<VsharpLoad, 104> loads{};

    const auto* code = GetFetchShaderCode(info, info.fetch_shader_sgpr_base);
    const u32* ptr = code;
    while (true) {
        const u32 word0 = *(ptr++);
        const auto encoding = GetInstructionEncoding(word0);

        if (encoding == InstEncoding::SOP1) {
            const auto opcode = OpcodeSOP1((word0 >> 8) & 0xff);
            ASSERT(opcode == OpcodeSOP1::S_SETPC_B64);
            break;
        } else if (encoding == InstEncoding::SMRD) {
            const auto opcode = OpcodeSMRD((word0 >> 22) & 0x1f);
            const u32 sdst = (word0 >> 15) & 0x7f;
            const u32 sbase = (word0 >> 9) & 0x3f;
            const u32 offset = word0 & 0xff;
            ASSERT(opcode == OpcodeSMRD::S_LOAD_DWORDX4);
            loads[sdst] = VsharpLoad{offset, sbase * 2};
        } else if (encoding == InstEncoding::VOP2) {
            const auto opcode = OpcodeVOP2((word0 >> 25) & 0x3f);
            const u32 src0 = word0 & 0x1ff;
            const u32 vsrc1 = (word0 >> 9) & 0xff;
            const u32 vdst = (word0 >> 17) & 0xff;
            ASSERT(opcode == OpcodeVOP2::V_ADD_I32 && src0 < 16 && vsrc1 == vdst);
            if (vdst == 0) {
                // V0 is always the vertex offset
                out_fetch_data.vertex_offset_sgpr = src0;
            } else if (vdst == 3) {
                // V3 is always the instance offset
                out_fetch_data.instance_offset_sgpr = src0;
            } else {
                UNREACHABLE();
            }
        } else if (encoding == InstEncoding::MUBUF || encoding == InstEncoding::MTBUF) {
            // SRSRC is in units of 4 SPGRs while SBASE is in pairs of SGPRs
            const u32 word1 = *(ptr++);
            const u32 srsrc = ((word1 >> 16) & 0x1f) * 4;
            auto& attrib = out_fetch_data.attributes.emplace_back();
            attrib.dest_vgpr = (word1 >> 8) & 0xff;
            if (encoding == InstEncoding::MUBUF) {
                const auto opcode = OpcodeMUBUF((word0 >> 18) & 0x7f);
                ASSERT(opcode >= OpcodeMUBUF::BUFFER_LOAD_FORMAT_X &&
                       opcode <= OpcodeMUBUF::BUFFER_LOAD_FORMAT_XYZW);
                attrib.num_elements = (std::to_underlying(opcode) -
                                       std::to_underlying(OpcodeMUBUF::BUFFER_LOAD_FORMAT_X)) +
                                      1;
            } else {
                const auto opcode = OpcodeMTBUF((word0 >> 16) & 0x7);
                ASSERT(opcode >= OpcodeMTBUF::TBUFFER_LOAD_FORMAT_X &&
                       opcode <= OpcodeMTBUF::TBUFFER_LOAD_FORMAT_XYZW);
                attrib.num_elements = (std::to_underlying(opcode) -
                                       std::to_underlying(OpcodeMTBUF::TBUFFER_LOAD_FORMAT_X)) +
                                      1;
            }
            attrib.sgpr_base = loads[srsrc].base_sgpr;
            attrib.dword_offset = loads[srsrc].dword_offset;
            attrib.inst_offset = word0 & 0xfff;
            attrib.instance_data = word1 & 0xff;
            if (encoding == InstEncoding::MTBUF) {
                attrib.data_format = (word0 >> 19) & 0xf;
                attrib.num_format = (word0 >> 23) & 0x7;
            } else {
                attrib.data_format = 0;
                attrib.num_format = 0;
            }
        } else if (encoding == InstEncoding::SOPP) {
            const auto opcode = OpcodeSOPP((word0 >> 16) & 0x7f);
            ASSERT(opcode == OpcodeSOPP::S_WAITCNT);
        } else if (encoding == InstEncoding::VOP1) {
            const auto opcode = OpcodeVOP1((word0 >> 9) & 0xff);
            const u32 src0 = word0 & 0x1ff;
            const u32 vdst = (word0 >> 17) & 0xff;
            ASSERT(opcode == OpcodeVOP1::V_MOV_B32 && src0 == 242);
            LOG_WARNING(Render_Recompiler, "Fetch shader has V{} = 1.0 which is ignored", vdst);
        } else {
            UNREACHABLE_MSG("Unexpected instruction encoding in fetch shader {}", u32(encoding));
        }
    }

    out_fetch_data.size = (ptr - code) * sizeof(u32);
    return true;
}

} // namespace Shader::Gcn
