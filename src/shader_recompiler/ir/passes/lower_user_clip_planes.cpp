// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "common/logging/log.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static constexpr u32 NumUserClipPlanes = 6;

// Lowers the fixed-function user clip planes in PA_CL_UCP_*, enabled by the UCP_ENA bits of
// PA_CL_CLIP_CNTL, to clip distances. Planes apply to the post-transform position and change per
// draw, so they are supplied through a per-draw uniform buffer.
void LowerUserClipPlanes(IR::Program& program, const RuntimeInfo& runtime_info) {
    // vs_info is populated for the hardware vertex stage, whether it runs the vertex shader or,
    // under tessellation, the domain shader.
    if (program.info.stage != Stage::Vertex) {
        return;
    }

    // Permutations of a shader binary share one Info and therefore one descriptor set layout, so
    // the buffer is declared for every permutation, keeping each module's binding numbering in
    // sync with that layout. The list persists across permutation compiles, so an existing entry
    // is reused.
    auto it = std::ranges::find(program.info.buffers, BufferType::ClipPlanes,
                                &BufferResource::buffer_type);
    if (it == program.info.buffers.end()) {
        it = program.info.buffers.insert(it, {
                                                 .used_types = IR::Type::F32,
                                                 .inline_cbuf = AmdGpu::Buffer::Placeholder(
                                                     NumUserClipPlanes * 4 * sizeof(float)),
                                                 .buffer_type = BufferType::ClipPlanes,
                                             });
    }
    const u32 binding = static_cast<u32>(std::distance(program.info.buffers.begin(), it));

    const u32 enabled_mask = runtime_info.vs_info.user_clip_plane_mask;
    if (enabled_mask == 0) {
        return;
    }

    // GCN clips against user planes and shader-exported distances independently, but both share
    // the eight ClipDistance slots here, and the guest's exports own their indices. Lowering is
    // skipped for such shaders, which restores the previous behavior of clipping by the exported
    // distances alone.
    for (u32 i = 0; i < runtime_info.vs_info.num_outputs; ++i) {
        for (const auto output : runtime_info.vs_info.outputs[i]) {
            if (output >= Output::ClipDist0 && output <= Output::ClipDist7) {
                LOG_WARNING(Render_Vulkan,
                            "Skipping user clip plane lowering: shader {:#x} exports its own clip "
                            "distances",
                            program.info.pgm_hash);
                return;
            }
        }
    }

    // Clip distances are computed from the final exported position.
    std::array<IR::Value, 4> position{};
    IR::Block* target_block{};
    IR::Block::iterator target_it{};
    for (IR::Block* const block : program.blocks) {
        for (auto inst = block->begin(); inst != block->end(); ++inst) {
            if (inst->GetOpcode() != IR::Opcode::SetAttribute ||
                inst->Arg(0).Attribute() != IR::Attribute::Position0) {
                continue;
            }
            position[inst->Arg(2).U32()] = inst->Arg(1);
            target_block = block;
            target_it = std::next(inst);
        }
    }
    if (std::ranges::any_of(position, [](const IR::Value& v) { return v.IsEmpty(); })) {
        return;
    }

    IR::IREmitter ir{*target_block, target_it};
    const IR::U32 handle = ir.Imm32(binding);
    for (u32 plane = 0; plane < NumUserClipPlanes; ++plane) {
        if (((enabled_mask >> plane) & 1) == 0) {
            continue;
        }
        // Each plane equation is four consecutive dwords, matching the upload in BindBuffers.
        const IR::Value equation = ir.LoadBufferF32(4, handle, ir.Imm32(plane * 4), {});
        IR::F32 distance{ir.FPMul(IR::F32{position[0]}, IR::F32{ir.CompositeExtract(equation, 0)})};
        for (u32 comp = 1; comp < 4; ++comp) {
            distance = IR::F32{ir.FPFma(IR::F32{position[comp]},
                                        IR::F32{ir.CompositeExtract(equation, comp)}, distance)};
        }
        ir.SetAttribute(IR::Attribute::ClipDistance, distance, plane);
    }
}

} // namespace Shader::Optimization
