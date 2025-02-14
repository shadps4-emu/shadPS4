// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsSharedAccess(const IR::Inst& inst) {
    const auto opcode = inst.GetOpcode();
    switch (opcode) {
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::WriteSharedU64:
    case IR::Opcode::SharedAtomicAnd32:
    case IR::Opcode::SharedAtomicIAdd32:
    case IR::Opcode::SharedAtomicOr32:
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicUMax32:
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicUMin32:
    case IR::Opcode::SharedAtomicXor32:
        return true;
    default:
        return false;
    }
}

void SharedMemoryToStoragePass(IR::Program& program, const RuntimeInfo& runtime_info,
                               const Profile& profile) {
    if (program.info.stage != Stage::Compute) {
        return;
    }
    // Only perform the transform if the host shared memory is insufficient.
    const u32 shared_memory_size = runtime_info.cs_info.shared_memory_size;
    if (shared_memory_size <= profile.max_shared_memory_size) {
        return;
    }
    // Add buffer binding for shared memory storage buffer.
    const u32 binding = static_cast<u32>(program.info.buffers.size());
    program.info.buffers.push_back({
        .used_types = IR::Type::U32,
        .inline_cbuf = AmdGpu::Buffer::Null(),
        .buffer_type = BufferType::SharedMemory,
        .is_written = true,
    });
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!IsSharedAccess(inst)) {
                continue;
            }
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            const IR::U32 handle = ir.Imm32(binding);
            // Replace shared atomics first
            switch (inst.GetOpcode()) {
            case IR::Opcode::SharedAtomicAnd32:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicAnd(handle, inst.Arg(0), inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicIAdd32:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIAdd(handle, inst.Arg(0), inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicOr32:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicOr(handle, inst.Arg(0), inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicSMax32:
            case IR::Opcode::SharedAtomicUMax32: {
                const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMax32;
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIMax(handle, inst.Arg(0), inst.Arg(1), is_signed, {}));
                continue;
            }
            case IR::Opcode::SharedAtomicSMin32:
            case IR::Opcode::SharedAtomicUMin32: {
                const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMin32;
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIMin(handle, inst.Arg(0), inst.Arg(1), is_signed, {}));
                continue;
            }
            case IR::Opcode::SharedAtomicXor32:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicXor(handle, inst.Arg(0), inst.Arg(1), {}));
                continue;
            default:
                break;
            }
            // Replace shared operations.
            const IR::U32 offset = ir.IMul(ir.GetAttributeU32(IR::Attribute::WorkgroupIndex),
                                           ir.Imm32(shared_memory_size));
            const IR::U32 address = ir.IAdd(IR::U32{inst.Arg(0)}, offset);
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU32:
                inst.ReplaceUsesWithAndRemove(ir.LoadBufferU32(1, handle, address, {}));
                break;
            case IR::Opcode::LoadSharedU64:
                inst.ReplaceUsesWithAndRemove(ir.LoadBufferU32(2, handle, address, {}));
                break;
            case IR::Opcode::WriteSharedU32:
                ir.StoreBufferU32(1, handle, address, inst.Arg(1), {});
                inst.Invalidate();
                break;
            case IR::Opcode::WriteSharedU64:
                ir.StoreBufferU32(2, handle, address, inst.Arg(1), {});
                inst.Invalidate();
                break;
            default:
                break;
            }
        }
    }
}

} // namespace Shader::Optimization
