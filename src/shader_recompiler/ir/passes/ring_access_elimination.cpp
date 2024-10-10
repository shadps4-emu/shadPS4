// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/recompiler.h"

namespace Shader::Optimization {

void RingAccessElimination(const IR::Program& program, const RuntimeInfo& runtime_info,
                           Stage stage) {
    auto& info = program.info;

    const auto& ForEachInstruction = [&](auto func) {
        for (IR::Block* block : program.blocks) {
            for (IR::Inst& inst : block->Instructions()) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                func(ir, inst);
            }
        }
    };

    switch (stage) {
    case Stage::Export: {
        ForEachInstruction([=](IR::IREmitter& ir, IR::Inst& inst) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::StoreBufferU32: {
                if (!inst.Flags<IR::BufferInstInfo>().ring_access) {
                    break;
                }

                const auto offset = inst.Flags<IR::BufferInstInfo>().inst_offset.Value();
                ASSERT(offset < runtime_info.es_info.vertex_data_size * 4);
                const auto data = ir.BitCast<IR::F32>(IR::U32{inst.Arg(2)});
                const auto attrib =
                    IR::Value{offset < 16 ? IR::Attribute::Position0
                                          : IR::Attribute::Param0 + (offset / 16 - 1)};
                const auto comp = (offset / 4) % 4;

                inst.ReplaceOpcode(IR::Opcode::SetAttribute);
                inst.ClearArgs();
                inst.SetArg(0, attrib);
                inst.SetArg(1, data);
                inst.SetArg(2, ir.Imm32(comp));
                break;
            }
            default:
                break;
            }
        });
        break;
    }
    case Stage::Geometry: {
        const auto& gs_info = runtime_info.gs_info;
        info.gs_copy_data = Shader::ParseCopyShader(gs_info.vs_copy);

        ForEachInstruction([&](IR::IREmitter& ir, IR::Inst& inst) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::LoadBufferU32: {
                if (!inst.Flags<IR::BufferInstInfo>().ring_access) {
                    break;
                }

                const auto shl_inst = inst.Arg(1).TryInstRecursive();
                const auto vertex_id = shl_inst->Arg(0).Resolve().U32() >> 2;
                const auto offset = inst.Arg(1).TryInstRecursive()->Arg(1);
                const auto bucket = offset.Resolve().U32() / 256u;
                const auto attrib = bucket < 4 ? IR::Attribute::Position0
                                               : IR::Attribute::Param0 + (bucket / 4 - 1);
                const auto comp = bucket % 4;

                auto attr_value = ir.GetAttribute(attrib, comp, vertex_id);
                inst.ReplaceOpcode(IR::Opcode::BitCastU32F32);
                inst.ClearArgs();
                inst.SetArg(0, attr_value);
                break;
            }
            case IR::Opcode::StoreBufferU32: {
                if (!inst.Flags<IR::BufferInstInfo>().ring_access) {
                    break;
                }

                const auto offset = inst.Flags<IR::BufferInstInfo>().inst_offset.Value();
                const auto data = ir.BitCast<IR::F32>(IR::U32{inst.Arg(2)});
                const auto comp_ofs = gs_info.output_vertices * 4u;
                const auto output_size = comp_ofs * gs_info.out_vertex_data_size;

                const auto vc_read_ofs = (((offset / comp_ofs) * comp_ofs) % output_size) * 16u;
                const auto& it = info.gs_copy_data.attr_map.find(vc_read_ofs);
                ASSERT(it != info.gs_copy_data.attr_map.cend());
                const auto& [attr, comp] = it->second;

                inst.ReplaceOpcode(IR::Opcode::SetAttribute);
                inst.ClearArgs();
                inst.SetArg(0, IR::Value{attr});
                inst.SetArg(1, data);
                inst.SetArg(2, ir.Imm32(comp));
                break;
            }
            default:
                break;
            }
        });
        break;
    }
    default:
        break;
    }
}

} // namespace Shader::Optimization
