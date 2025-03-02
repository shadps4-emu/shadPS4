// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "common/hash.h"
#include "common/types.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {

// Does global value numbering on a subset of instructions that are used
// for loads from shader resource tables.
// Inspiration from spirv-opt

class SrtGvnTable {
public:
    using ValueNumberTable = std::unordered_map<IR::Value, u32>;
    using ValueNum = u32;

    SrtGvnTable() : value_numbers(), next_num(0) {}

    u32 GetValueNumber(IR::Inst* inst) {
        return GetValueNumber(IR::Value{inst});
    }

    u32 GetValueNumber(IR::Value v) {
        v = v.Resolve();
        if (auto it = value_numbers.find(v); it != value_numbers.end()) {
            return it->second;
        }
        if (auto inst = v.TryInstRecursive()) {
            return ComputeInstValueNumber(inst);
        }
        return NextValueNumber(v);
    }

private:
    u32 ComputeInstValueNumber(IR::Inst* inst) {
        ASSERT(!value_numbers.contains(
            IR::Value(inst))); // Should always be checking before calling this function

        if (inst->MayHaveSideEffects()) {
            return NextValueNumber(IR::Value(inst));
        }

        u32 vn;

        switch (inst->GetOpcode()) {
        case IR::Opcode::Phi: {
            const auto pred = [](IR::Inst* inst) -> std::optional<IR::Inst*> {
                if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                    inst->GetOpcode() == IR::Opcode::CompositeConstructU32x2 ||
                    inst->GetOpcode() == IR::Opcode::ReadConst) {
                    return inst;
                }
                return std::nullopt;
            };
            IR::Inst* source = IR::BreadthFirstSearch(inst, pred).value();
            vn = GetValueNumber(source);
            value_numbers[IR::Value(inst)] = vn;
            break;
        }
        case IR::Opcode::GetUserData:
        case IR::Opcode::CompositeConstructU32x2:
        case IR::Opcode::ReadConst: {
            InstVector iv = MakeInstVector(inst);
            if (auto it = iv_to_vn.find(iv); it != iv_to_vn.end()) {
                vn = it->second;
                value_numbers[IR::Value(inst)] = vn;
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
};

} // namespace Shader::Optimization