// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iterator>
#include <unordered_map>
#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "common/hash.h"
#include "common/types.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {

// This could be general GVN pass in future
// inspiration from spirv-opt

// TODO make sure identity handled ok

// Deduplicate reads from constant memory
// 1. Do GVN on GetUserData, ReadConst, and CompositeConstructU32x2 (pointers)
// 2. Hoist the distinct values to the entry block
//      - Trivial for readconst with immediate offset, and a pointer arg which has already been
//      hoisted
// 3. Replace duplicates of the hoisted values

// Note: use non-const IR::Value/IR::Inst, because IR::Value(const IR::Inst *) invokes
// explicit Value(bool value) noexcept; lol

// TODO delete, for debugger
void PrintBlock(const Shader::IR::Block* block) {
    std::string s = Shader::IR::DumpBlock(*block);
    printf("%s\n", s.c_str());
}

class ConstantReadGvnTable {
public:
    using ValueNumberTable = std::unordered_map<IR::Value, u32>;
    using ValueNum = u32;

    ConstantReadGvnTable() : value_numbers(), next_num(0) {}

    void FillTable(IR::Program& program) {
        for (auto r_it = program.post_order_blocks.rbegin();
             r_it != program.post_order_blocks.rend(); r_it++) {
            IR::Block* block = *r_it;
            for (IR::Inst& inst : block->Instructions()) {
                switch (inst.GetOpcode()) {
                // Temp workaround
                case IR::Opcode::Phi:
                    TryRemoveTrivialPhi(&inst);
                    break;
                case IR::Opcode::GetUserData:
                case IR::Opcode::CompositeConstructU32x2:
                case IR::Opcode::ReadConst:
                    GetValueNumber(&inst);
                    break;
                default:
                    break;
                }
            }
        }
    }

    u32 GetValueNumber(IR::Inst* inst) {
        return GetValueNumber(IR::Value{inst});
    }

    u32 GetValueNumber(IR::Value v) {
        if (auto it = value_numbers.find(v); it != value_numbers.end()) {
            return it->second;
        }
        if (auto inst = v.TryInstRecursive()) {
            return ComputeInstValueNumber(inst);
        }
        return NextValueNumber(v);
    }

    // Call after FillTable
    IR::Value GetCanonicalValue(u32 vn) {
        return IR::Value(pick_one_inst[vn]);
    }

    IR::Value GetCanonicalValue(IR::Inst* inst) {
        return GetCanonicalValue(GetValueNumber(inst));
    }

private:
    u32 ComputeInstValueNumber(IR::Inst* inst) {
        if (inst->MayHaveSideEffects()) {
            return NextValueNumber(IR::Value(inst));
        }

        u32 vn;

        switch (inst->GetOpcode()) {
        case IR::Opcode::GetUserData:
        case IR::Opcode::CompositeConstructU32x2:
        case IR::Opcode::ReadConst: {
            InstVector iv = MakeInstVector(inst);
            if (auto it = iv_to_vn.find(iv); it != iv_to_vn.end()) {
                vn = it->second;
            } else {
                vn = NextValueNumber(IR::Value(inst));
                iv_to_vn.emplace(std::move(iv), vn);
            }
            break;
        }
        default:
            vn = NextValueNumber(IR::Value(inst));
            break;
        }

        value_numbers[IR::Value(inst)] = vn;
        pick_one_inst.try_emplace(vn, inst);
        return vn;
    }

    u32 NextValueNumber(IR::Value v) {
        u32 rv = next_num++;
        value_numbers[v] = rv;
        return rv;
    }

    ValueNumberTable value_numbers;
    u32 next_num;

    using InstVector = boost::container::small_vector<u32, 8>;

    InstVector MakeInstVector(IR::Inst* inst) {
        ASSERT(inst->GetOpcode() != IR::Opcode::Identity);
        InstVector iv;
        iv.reserve(2 + inst->NumArgs());
        iv.push_back(static_cast<u32>(inst->GetOpcode()));
        iv.push_back(inst->Flags<u32>());
        for (auto i = 0; i < inst->NumArgs(); i++) {
            iv.push_back(GetValueNumber(inst->Arg(i)));
        }
        return iv;
    }

    // Temp workaround for something like this:
    // [0000555558a5baf8] %297   = Phi [ %24, {Block $1} ], [ %297, {Block $5} ] (uses: 4)
    // [0000555558a4e038] %305   = CompositeConstructU32x2 %297, %296 (uses: 4)
    // [0000555558a4e0a8] %306   = ReadConst %305, #0 (uses: 2)
    // Should probably be fixed in ssa_rewrite
    std::optional<IR::Value> TryRemoveTrivialPhi(IR::Inst* phi) {
        IR::Value single_source{};

        for (auto i = 0; i < phi->NumArgs(); i++) {
            IR::Value v = phi->Arg(i);
            if (auto inst = v.TryInstRecursive()) {
                v = IR::Value(inst);
            };
            if (v == IR::Value(phi)) {
                continue;
            }
            if (!single_source.IsEmpty() && single_source != v) {
                return std::nullopt;
            }
            single_source = v;
        }

        ASSERT(!single_source.IsEmpty());
        phi->ReplaceUsesWith(single_source);
        return single_source;
    }

    struct HashInstVector {
        size_t operator()(const InstVector& iv) const {
            u32 h = 0;
            for (auto vn : iv) {
                h = HashCombine(vn, h);
            }
            return h;
        }
    };

    std::unordered_map<InstVector, u32, HashInstVector> iv_to_vn;
    // used to redirect insts to a single source if they are duplicates.
    // For now, we can just hoist the canonical inst to the entry block, but in general GVN we'd
    // need to put it in a block that dominates the uses and is dominated by its operands
    std::unordered_map<u32, IR::Inst*> pick_one_inst;
};

void HoistConstantReadsPass(IR::Program& program) {
    ConstantReadGvnTable table;
    table.FillTable(program);

    IR::Block* entry_bb = *program.blocks.begin();
    IR::Block::iterator insert_point = entry_bb->end();
    for (auto it = entry_bb->rbegin(); it != entry_bb->rend(); it++) {
        if (it->GetOpcode() == IR::Opcode::GetUserData) {
            insert_point = IR::Block::InstructionList::s_iterator_to(*it);
            ++insert_point;
            break;
        }
    }

    boost::container::set<u32> hoisted_vns;

    // IdentityRemovalPass(program.blocks);

    auto can_hoist_all_args = [&](IR::Inst* inst) {
        for (auto i = 0; i < inst->NumArgs(); i++) {
            IR::Value v = inst->Arg(i);
            if (auto arg_inst = v.TryInstRecursive()) {
                // we are already hoisting to a point after the last GetUserData
                if (arg_inst->GetOpcode() != IR::Opcode::GetUserData &&
                    !hoisted_vns.contains(table.GetValueNumber(arg_inst))) {
                    return false;
                }
            } else if (!v.IsImmediate()) {
                return false;
            }
        }
        return true;
    };

    struct HoistInfo {
        IR::Inst* inst;
        IR::Block* src_block;
    };
    boost::container::small_vector<HoistInfo, 16> hoists;

    for (auto r_it = program.post_order_blocks.rbegin(); r_it != program.post_order_blocks.rend();
         r_it++) {
        IR::Block* block = *r_it;
        for (IR::Inst& inst : block->Instructions()) {
            switch (inst.GetOpcode()) {
            case IR::Opcode::GetUserData:
                break;
            case IR::Opcode::CompositeConstructU32x2:
            case IR::Opcode::ReadConst: {
                if (!can_hoist_all_args(&inst)) {
                    break;
                }

                u32 vn = table.GetValueNumber(&inst);
                IR::Value canon = table.GetCanonicalValue(vn);
                if (canon != IR::Value(&inst)) {
                    inst.ReplaceUsesWith(canon);
                    break;
                }

                hoists.emplace_back(&inst, block);
                hoisted_vns.insert(vn);
                break;
            }
            default:
                break;
            }
        }
    }

    for (HoistInfo& h_info : hoists) {
        // Copy logic from IdentityRemoval
        // (Just add another pass before this?)
        IR::Inst* inst = h_info.inst;
        IR::Block* src_block = h_info.src_block;
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            IR::Value arg;
            while ((arg = inst->Arg(i)).IsIdentity()) {
                inst->SetArg(i, arg.Inst()->Arg(0));
            }
        }

        // Hoist to entry block
        entry_bb->MoveInst(insert_point, *inst, *src_block);
    }
}

} // namespace Shader::Optimization