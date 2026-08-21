// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iterator>
#include <set>
#include <utility>
#include <vector>

#include "common/hack_features.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

namespace {

bool IsStoreBuffer(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::StoreBufferU8:
    case IR::Opcode::StoreBufferU16:
    case IR::Opcode::StoreBufferU32:
    case IR::Opcode::StoreBufferU32x2:
    case IR::Opcode::StoreBufferU32x3:
    case IR::Opcode::StoreBufferU32x4:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::StoreBufferF32:
    case IR::Opcode::StoreBufferF32x2:
    case IR::Opcode::StoreBufferF32x3:
    case IR::Opcode::StoreBufferF32x4:
        return true;
    default:
        return false;
    }
}

bool IsLoadBuffer(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::LoadBufferF32:
    case IR::Opcode::LoadBufferF32x2:
    case IR::Opcode::LoadBufferF32x3:
    case IR::Opcode::LoadBufferF32x4:
        return true;
    default:
        return false;
    }
}

} // namespace

void InsertSsbsoStoreLoadBarrier(IR::Program& program, const Profile& profile) {
    // GCN hardware forwards a same-address buffer store to a following buffer load within the same
    // dispatch, so the original shader relies on that implicit forwarding. NVIDIA drivers do not
    // guarantee it for non-coherent SSBO accesses, making the load observe stale data. Insert a
    // device memory barrier after stores of any buffer that is both stored and loaded. Only needed
    // on NVIDIA, and gated behind the 1886 hack feature for now.
    if (!profile.needs_ssbo_store_load_barrier || !Common::HackFeatures::isTheOrder1886) {
        return;
    }

    std::set<u32> stored_buffers;
    std::set<u32> loaded_buffers;
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsStoreBuffer(inst)) {
                stored_buffers.insert(inst.Arg(0).U32());
            } else if (IsLoadBuffer(inst)) {
                loaded_buffers.insert(inst.Arg(0).U32());
            }
        }
    }

    std::vector<std::pair<IR::Block*, IR::Block::iterator>> insert_at;
    for (IR::Block* block : program.blocks) {
        for (auto it = block->Instructions().begin(); it != block->Instructions().end(); ++it) {
            IR::Inst& inst = *it;
            if (!IsStoreBuffer(inst)) {
                continue;
            }
            const u32 binding = inst.Arg(0).U32();
            if (stored_buffers.count(binding) && loaded_buffers.count(binding)) {
                insert_at.emplace_back(block, std::next(it));
            }
        }
    }

    for (auto [block, pos] : insert_at) {
        IR::IREmitter ir{*block, pos};
        ir.DeviceMemoryBarrier();
    }
}

} // namespace Shader::Optimization
