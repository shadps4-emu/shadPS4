// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/copy_shader.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/ir/attribute.h"

namespace Shader {

CopyShaderData ParseCopyShader(std::span<const u32> code) {
    Gcn::GcnCodeSlice code_slice{code.data(), code.data() + code.size()};
    Gcn::GcnDecodeContext decoder;

    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    ASSERT_MSG(code[0] == token_mov_vcchi, "First instruction is not s_mov_b32 vcc_hi, #imm");

    std::array<s32, 32> offsets{};
    offsets.fill(-1);

    std::array<s32, 256> sources{};
    sources.fill(-1);

    CopyShaderData data{};
    auto last_attr{IR::Attribute::Position0};
    while (!code_slice.atEnd()) {
        auto inst = decoder.decodeInstruction(code_slice);
        switch (inst.opcode) {
        case Gcn::Opcode::S_MOVK_I32: {
            sources[inst.dst[0].code] = inst.control.sopk.simm;
            break;
        }
        case Gcn::Opcode::EXP: {
            const auto& exp = inst.control.exp;
            const IR::Attribute semantic = static_cast<IR::Attribute>(exp.target);
            for (int i = 0; i < inst.src_count; ++i) {
                const auto ofs = offsets[inst.src[i].code];
                if (ofs != -1) {
                    data.attr_map[ofs] = {semantic, i};
                    if (semantic > last_attr) {
                        last_attr = semantic;
                    }
                }
            }
            break;
        }
        case Gcn::Opcode::BUFFER_LOAD_DWORD: {
            offsets[inst.src[1].code] = inst.control.mubuf.offset;
            if (inst.src[3].field != Gcn::OperandField::ConstZero) {
                const u32 index = inst.src[3].code;
                ASSERT(sources[index] != -1);
                offsets[inst.src[1].code] += sources[index];
            }
            break;
        }
        default:
            break;
        }
    }

    if (last_attr != IR::Attribute::Position0) {
        data.num_attrs = static_cast<u32>(last_attr) - static_cast<u32>(IR::Attribute::Param0) + 1;
    }

    return data;
}

} // namespace Shader
