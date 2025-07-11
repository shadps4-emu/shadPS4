// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsSharedAccess(const IR::Inst& inst) {
    const auto opcode = inst.GetOpcode();
    switch (opcode) {
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU16:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::WriteSharedU64:
    case IR::Opcode::SharedAtomicIAdd32:
    case IR::Opcode::SharedAtomicISub32:
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicUMin32:
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicUMax32:
    case IR::Opcode::SharedAtomicInc32:
    case IR::Opcode::SharedAtomicDec32:
    case IR::Opcode::SharedAtomicAnd32:
    case IR::Opcode::SharedAtomicOr32:
    case IR::Opcode::SharedAtomicXor32:
    case IR::Opcode::SharedAtomicIAdd64:
    case IR::Opcode::SharedAtomicISub64:
    case IR::Opcode::SharedAtomicSMin64:
    case IR::Opcode::SharedAtomicUMin64:
    case IR::Opcode::SharedAtomicSMax64:
    case IR::Opcode::SharedAtomicUMax64:
    case IR::Opcode::SharedAtomicInc64:
    case IR::Opcode::SharedAtomicDec64:
    case IR::Opcode::SharedAtomicAnd64:
    case IR::Opcode::SharedAtomicOr64:
    case IR::Opcode::SharedAtomicXor64:
        return true;
    default:
        return false;
    }
}

IR::Type CalculateSharedMemoryTypes(IR::Program& program) {
    IR::Type used_types{IR::Type::Void};
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!IsSharedAccess(inst)) {
                continue;
            }
            switch (inst.GetOpcode()) {
            case IR::Opcode::LoadSharedU16:
            case IR::Opcode::WriteSharedU16:
                used_types |= IR::Type::U16;
                break;
            case IR::Opcode::LoadSharedU32:
            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::SharedAtomicIAdd32:
            case IR::Opcode::SharedAtomicISub32:
            case IR::Opcode::SharedAtomicSMin32:
            case IR::Opcode::SharedAtomicUMin32:
            case IR::Opcode::SharedAtomicSMax32:
            case IR::Opcode::SharedAtomicUMax32:
            case IR::Opcode::SharedAtomicInc32:
            case IR::Opcode::SharedAtomicDec32:
            case IR::Opcode::SharedAtomicAnd32:
            case IR::Opcode::SharedAtomicOr32:
            case IR::Opcode::SharedAtomicXor32:
                used_types |= IR::Type::U32;
                break;
            case IR::Opcode::LoadSharedU64:
            case IR::Opcode::WriteSharedU64:
            case IR::Opcode::SharedAtomicIAdd64:
            case IR::Opcode::SharedAtomicISub64:
            case IR::Opcode::SharedAtomicSMin64:
            case IR::Opcode::SharedAtomicUMin64:
            case IR::Opcode::SharedAtomicSMax64:
            case IR::Opcode::SharedAtomicUMax64:
            case IR::Opcode::SharedAtomicInc64:
            case IR::Opcode::SharedAtomicDec64:
            case IR::Opcode::SharedAtomicAnd64:
            case IR::Opcode::SharedAtomicOr64:
            case IR::Opcode::SharedAtomicXor64:
                used_types |= IR::Type::U64;
                break;
            default:
                break;
            }
        }
    }
    return used_types;
}

void SharedMemoryToStoragePass(IR::Program& program, const RuntimeInfo& runtime_info,
                               const Profile& profile) {
    if (program.info.stage != Stage::Compute) {
        return;
    }

    // Run this pass if:
    // * There are shared memory instructions.
    // * One of the following is true:
    //   * Requested shared memory size is too large for the host shared memory.
    //   * Workgroup explicit memory is not supported and multiple shared memory types are used.
    const u32 shared_memory_size = runtime_info.cs_info.shared_memory_size;
    const auto used_types = CalculateSharedMemoryTypes(program);
    if (used_types == IR::Type::Void || (shared_memory_size <= profile.max_shared_memory_size &&
                                         (profile.supports_workgroup_explicit_memory_layout ||
                                          std::popcount(static_cast<u32>(used_types)) == 1))) {
        return;
    }

    // Add a buffer binding for shared memory storage buffer.
    const u32 binding = static_cast<u32>(program.info.buffers.size());
    program.info.buffers.push_back({
        .used_types = used_types,
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
            const IR::U32 offset = ir.IMul(ir.GetAttributeU32(IR::Attribute::WorkgroupIndex),
                                           ir.Imm32(shared_memory_size));
            const IR::U32 address = ir.IAdd(IR::U32{inst.Arg(0)}, offset);
            switch (inst.GetOpcode()) {
            case IR::Opcode::SharedAtomicIAdd32:
            case IR::Opcode::SharedAtomicIAdd64:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIAdd(handle, address, inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicISub32:
            case IR::Opcode::SharedAtomicISub64:
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicISub(handle, address, inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicSMin32:
            case IR::Opcode::SharedAtomicUMin32:
            case IR::Opcode::SharedAtomicSMin64:
            case IR::Opcode::SharedAtomicUMin64: {
                const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMin32 ||
                                       inst.GetOpcode() == IR::Opcode::SharedAtomicSMin64;
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIMin(handle, address, inst.Arg(1), is_signed, {}));
                continue;
            }
            case IR::Opcode::SharedAtomicSMax32:
            case IR::Opcode::SharedAtomicUMax32:
            case IR::Opcode::SharedAtomicSMax64:
            case IR::Opcode::SharedAtomicUMax64: {
                const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMax32 ||
                                       inst.GetOpcode() == IR::Opcode::SharedAtomicSMax64;
                inst.ReplaceUsesWithAndRemove(
                    ir.BufferAtomicIMax(handle, address, inst.Arg(1), is_signed, {}));
                continue;
            }
            case IR::Opcode::SharedAtomicInc32:
                inst.ReplaceUsesWithAndRemove(ir.BufferAtomicInc(handle, address, {}));
                continue;
            case IR::Opcode::SharedAtomicDec32:
                inst.ReplaceUsesWithAndRemove(ir.BufferAtomicDec(handle, address, {}));
                continue;
            case IR::Opcode::SharedAtomicAnd32:
            case IR::Opcode::SharedAtomicAnd64:
                inst.ReplaceUsesWithAndRemove(ir.BufferAtomicAnd(handle, address, inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicOr32:
            case IR::Opcode::SharedAtomicOr64:
                inst.ReplaceUsesWithAndRemove(ir.BufferAtomicOr(handle, address, inst.Arg(1), {}));
                continue;
            case IR::Opcode::SharedAtomicXor32:
            case IR::Opcode::SharedAtomicXor64:
                inst.ReplaceUsesWithAndRemove(ir.BufferAtomicXor(handle, address, inst.Arg(1), {}));
                continue;
            case IR::Opcode::LoadSharedU16:
                inst.ReplaceUsesWithAndRemove(ir.LoadBufferU16(handle, address, {}));
                break;
            case IR::Opcode::LoadSharedU32:
                inst.ReplaceUsesWithAndRemove(ir.LoadBufferU32(1, handle, address, {}));
                break;
            case IR::Opcode::LoadSharedU64:
                inst.ReplaceUsesWithAndRemove(ir.LoadBufferU64(handle, address, {}));
                break;
            case IR::Opcode::WriteSharedU16:
                ir.StoreBufferU16(handle, address, IR::U16{inst.Arg(1)}, {});
                inst.Invalidate();
                break;
            case IR::Opcode::WriteSharedU32:
                ir.StoreBufferU32(1, handle, address, inst.Arg(1), {});
                inst.Invalidate();
                break;
            case IR::Opcode::WriteSharedU64:
                ir.StoreBufferU64(handle, address, IR::U64{inst.Arg(1)}, {});
                inst.Invalidate();
                break;
            default:
                UNREACHABLE();
            }
        }
    }
}

} // namespace Shader::Optimization
