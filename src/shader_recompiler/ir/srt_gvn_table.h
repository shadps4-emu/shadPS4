// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "common/hash.h"
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
        if (auto it = value_numbers.find(v); it != value_numbers.end()) {
            return it->second;
        }
        if (auto* inst = v.TryInst()) {
            return ComputeInstValueNumber(inst);
        }
        return NextValueNumber(v);
    }

private:
    bool IsArgHashInst(IR::Inst* inst) {
        switch (inst->GetOpcode()) {
        case IR::Opcode::GetUserData:
        case IR::Opcode::CompositeConstructU32x2:
        case IR::Opcode::ReadConst:
        case IR::Opcode::ReadConstBuffer:
        case IR::Opcode::IAdd32:
        case IR::Opcode::ISub32:
        case IR::Opcode::IMul32:
        case IR::Opcode::ShiftLeftLogical32:
        case IR::Opcode::ShiftRightLogical32:
        case IR::Opcode::BitwiseAnd32:
        case IR::Opcode::BitwiseOr32:
        case IR::Opcode::BitwiseXor32:
        case IR::Opcode::BitwiseNot32:
        case IR::Opcode::UMin32:
        case IR::Opcode::UMax32:
        case IR::Opcode::BitFieldUExtract:
            ASSERT(!inst->MayHaveSideEffects());
            return true;
        default:
            return false;
        }
    }

    u32 ComputeInstValueNumber(IR::Inst* inst) {
        // Should always be checking before calling this functio
        ASSERT(!value_numbers.contains(IR::Value(inst)));
        u32 vn;
        if (IsArgHashInst(inst)) {
            InstVector iv = MakeInstVector(inst);
            if (auto it = iv_to_vn.find(iv); it != iv_to_vn.end()) {
                vn = it->second;
                value_numbers[IR::Value(inst)] = vn;
            } else {
                vn = NextValueNumber(IR::Value(inst));
                iv_to_vn.emplace(std::move(iv), vn);
            }
        } else {
            vn = NextValueNumber(IR::Value(inst));
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