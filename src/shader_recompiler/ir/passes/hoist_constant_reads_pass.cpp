// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include "common/assert.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {

namespace {
inline u32 HashCombine(const u32 seed, const u32 hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

struct ValueHash {
    size_t operator()(const IR::Value& val) {
        u32 h = 0;
        if (auto inst = val.TryInstRecursive()) {
            // return inst;
        }
        // h = HashCombine(, h);
    }
};
} // namespace

struct ValueNumberTable {
    std::unordered_map<const IR::Value, u32> value_to_num;

    u32 num;
};

namespace {
u64 GetValueNumber(const IR::Inst* inst, ValueNumberTable& value_numbers) {
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::CompositeConstructU32x2:
    case IR::Opcode::ReadConst:
    default:
    }
}
} // namespace

void HoistConstantReadsPass(IR::Program& program) {
    ValueNumberTable value_numbers;

    auto compute_value_number = [&](const IR::Inst* inst) -> u64 {

    }

    for (auto r_it = program.post_order_blocks.rbegin(); r_it != program.post_order_blocks.rend();
         r_it++) {
        IR::Block* block = *r_it;
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadConst) {
            }
        }
    }
}

} // namespace Shader::Optimization