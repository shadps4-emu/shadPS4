// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"

namespace {

// TODO clean this up. Maybe remove
// from https://github.com/chaotic-cx/mesa-mirror/blob/main/src/amd/compiler/README.md
// basically logical stage x hw stage permutations
enum class SwHwStagePerm {
    vertex_vs,
    fragment_fs,
    vertex_ls,
    tess_control_hs,
    tess_eval_vs,
    vertex_es,
    geometry_gs,
    gs_copy_vs,
    tess_eval_es,
    compute_cs,
};

static SwHwStagePerm GetSwHwStagePerm(Shader::Stage hw_stage, Shader::LogicalStage sw_stage) {
    using namespace Shader;
    switch (sw_stage) {
    case LogicalStage::Fragment:
        ASSERT(hw_stage == Stage::Fragment);
        return SwHwStagePerm::fragment_fs;
    case LogicalStage::Vertex: {
        switch (hw_stage) {
        case Stage::Vertex:
            return SwHwStagePerm::vertex_vs;
        case Stage::Export:
            return SwHwStagePerm::vertex_es;
        case Stage::Local:
            return SwHwStagePerm::vertex_ls;
        default:
            UNREACHABLE();
        }
    } break;
    case LogicalStage::TessellationControl:
        ASSERT(hw_stage == Stage::Hull);
        return SwHwStagePerm::tess_control_hs;
    case LogicalStage::TessellationEval: {
        switch (hw_stage) {
        case Stage::Vertex:
            return SwHwStagePerm::tess_eval_vs;
        case Stage::Export:
            return SwHwStagePerm::tess_eval_es;
        default:
            UNREACHABLE();
        }
    }
    case LogicalStage::Geometry:
        ASSERT(hw_stage == Stage::Geometry);
        return SwHwStagePerm::geometry_gs;
    case LogicalStage::GsCopy:
        ASSERT(hw_stage == Stage::Vertex);
        return SwHwStagePerm::gs_copy_vs;
    case LogicalStage::Compute:
        ASSERT(hw_stage == Stage::Compute);
        return SwHwStagePerm::compute_cs;
    default:
        UNREACHABLE();
    }
}

}; // namespace

namespace Shader::Optimization {

void RingAccessElimination(const IR::Program& program, const RuntimeInfo& runtime_info) {
    auto& info = program.info;

    Stage stage = info.stage;
    LogicalStage l_stage = info.l_stage;
    SwHwStagePerm stage_perm = GetSwHwStagePerm(stage, l_stage);

    const auto& ForEachInstruction = [&](auto func) {
        for (IR::Block* block : program.blocks) {
            for (IR::Inst& inst : block->Instructions()) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                func(ir, inst);
            }
        }
    };

    switch (stage_perm) {
    case SwHwStagePerm::vertex_ls: {
        ForEachInstruction([=](IR::IREmitter& ir, IR::Inst& inst) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::WriteSharedU64:
            case IR::Opcode::WriteSharedU32: {
                bool is_composite = opcode == IR::Opcode::WriteSharedU64;
                u32 num_components = opcode == IR::Opcode::WriteSharedU32 ? 1 : 2;

                u32 offset = 0;
                const auto* addr = inst.Arg(0).InstRecursive();
                if (addr->GetOpcode() == IR::Opcode::IAdd32) {
                    ASSERT(addr->Arg(1).IsImmediate());
                    offset = addr->Arg(1).U32();
                }
                IR::Value data = inst.Arg(1).Resolve();
                for (s32 i = 0; i < num_components; i++) {
                    const auto attrib = IR::Attribute::Param0 + (offset / 16);
                    const auto comp = (offset / 4) % 4;
                    const IR::U32 value = IR::U32{is_composite ? data.Inst()->Arg(i) : data};
                    ir.SetAttribute(attrib, ir.BitCast<IR::F32, IR::U32>(value), comp);
                    offset += 4;
                }
                inst.Invalidate();
                break;
            }
            default:
                break;
            }
        });
        break;
    }
    case SwHwStagePerm::vertex_es: {
        ForEachInstruction([=](IR::IREmitter& ir, IR::Inst& inst) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::StoreBufferU32: {
                const auto info = inst.Flags<IR::BufferInstInfo>();
                if (!info.system_coherent || !info.globally_coherent) {
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
    case SwHwStagePerm::geometry_gs: {
        const auto& gs_info = runtime_info.gs_info;
        info.gs_copy_data = Shader::ParseCopyShader(gs_info.vs_copy);

        ForEachInstruction([&](IR::IREmitter& ir, IR::Inst& inst) {
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::LoadBufferU32: {
                const auto info = inst.Flags<IR::BufferInstInfo>();
                if (!info.system_coherent || !info.globally_coherent) {
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
                const auto buffer_info = inst.Flags<IR::BufferInstInfo>();
                if (!buffer_info.system_coherent || !buffer_info.globally_coherent) {
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
