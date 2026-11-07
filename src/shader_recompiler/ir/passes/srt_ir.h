// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>
#include <unordered_map>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "common/types.h"

namespace Shader::Optimization::Srt {

enum class Opcode : u8 {
    True,
    False,
    Imm32,
    LoadFlatbuf,
    IAdd32,
    ISub32,
    IMul32,
    ShiftLeftLogical32,
    ShiftRightLogical32,
    BitwiseAnd32,
    BitwiseOr32,
    BitwiseXor32,
    BitwiseNot32,
    UMin32,
    UMax32,
    BitFieldUExtract,
    IEqual,
    INotEqual,
    SLessThan,
    SLessThanEqual,
    SGreaterThan,
    SGreaterThanEqual,
    ULessThan,
    ULessThanEqual,
    UGreaterThan,
    UGreaterThanEqual,
    FPOrdEqual,
    FPOrdNotEqual,
    FPOrdLessThan,
    FPOrdLessThanEqual,
    FPOrdGreaterThan,
    FPOrdGreaterThanEqual,
    FPUnordEqual,
    FPUnordNotEqual,
    FPUnordLessThan,
    FPUnordLessThanEqual,
    FPUnordGreaterThan,
    FPUnordGreaterThanEqual,
    LogicalAnd,
    LogicalOr,
};

using Ref = u16;

constexpr Ref TrueRef = 0;
constexpr Ref FalseRef = 1;

struct Inst {
    Opcode opcode{};
    u32 imm{};
    std::array<Ref, 3> args{};

    bool operator==(const Inst&) const = default;
};

constexpr bool IsFloatCompare(Opcode opcode) {
    return opcode >= Opcode::FPOrdEqual && opcode <= Opcode::FPUnordGreaterThanEqual;
}

inline Opcode Negate(Opcode opcode) {
    switch (opcode) {
    case Opcode::IEqual:
        return Opcode::INotEqual;
    case Opcode::INotEqual:
        return Opcode::IEqual;
    case Opcode::SLessThan:
        return Opcode::SGreaterThanEqual;
    case Opcode::SLessThanEqual:
        return Opcode::SGreaterThan;
    case Opcode::SGreaterThan:
        return Opcode::SLessThanEqual;
    case Opcode::SGreaterThanEqual:
        return Opcode::SLessThan;
    case Opcode::ULessThan:
        return Opcode::UGreaterThanEqual;
    case Opcode::ULessThanEqual:
        return Opcode::UGreaterThan;
    case Opcode::UGreaterThan:
        return Opcode::ULessThanEqual;
    case Opcode::UGreaterThanEqual:
        return Opcode::ULessThan;
    case Opcode::FPOrdEqual:
        return Opcode::FPUnordNotEqual;
    case Opcode::FPOrdNotEqual:
        return Opcode::FPUnordEqual;
    case Opcode::FPOrdLessThan:
        return Opcode::FPUnordGreaterThanEqual;
    case Opcode::FPOrdLessThanEqual:
        return Opcode::FPUnordGreaterThan;
    case Opcode::FPOrdGreaterThan:
        return Opcode::FPUnordLessThanEqual;
    case Opcode::FPOrdGreaterThanEqual:
        return Opcode::FPUnordLessThan;
    case Opcode::FPUnordEqual:
        return Opcode::FPOrdNotEqual;
    case Opcode::FPUnordNotEqual:
        return Opcode::FPOrdEqual;
    case Opcode::FPUnordLessThan:
        return Opcode::FPOrdGreaterThanEqual;
    case Opcode::FPUnordLessThanEqual:
        return Opcode::FPOrdGreaterThan;
    case Opcode::FPUnordGreaterThan:
        return Opcode::FPOrdLessThanEqual;
    case Opcode::FPUnordGreaterThanEqual:
        return Opcode::FPOrdLessThan;
    default:
        UNREACHABLE_MSG("Opcode {} is not a compare", static_cast<u32>(opcode));
    }
}

class Program {
public:
    static constexpr size_t MaxInsts = 8192;

    Program() {
        Add({.opcode = Opcode::True});
        Add({.opcode = Opcode::False});
    }

    bool IsFull() const {
        return insts.size() >= MaxInsts;
    }

    Ref Imm(u32 value) {
        return Add({.opcode = Opcode::Imm32, .imm = value});
    }

    Ref Load(u32 dword) {
        return Add({.opcode = Opcode::LoadFlatbuf, .imm = dword});
    }

    Ref Op(Opcode opcode, Ref a, Ref b = TrueRef, Ref c = TrueRef) {
        return Add({.opcode = opcode, .args = {a, b, c}});
    }

    Ref And(Ref a, Ref b) {
        if (a == FalseRef || b == FalseRef) {
            return FalseRef;
        }
        if (a == TrueRef || a == b) {
            return b;
        }
        return b == TrueRef ? a : Op(Opcode::LogicalAnd, a, b);
    }

    Ref Or(Ref a, Ref b) {
        if (a == TrueRef || b == TrueRef) {
            return TrueRef;
        }
        if (a == FalseRef || a == b) {
            return b;
        }
        return b == FalseRef ? a : Op(Opcode::LogicalOr, a, b);
    }

    const Inst& operator[](Ref ref) const {
        return insts[ref];
    }

private:
    struct InstHash {
        size_t operator()(const Inst& inst) const {
            size_t hash = std::hash<u64>{}((u64(inst.opcode) << 32) | inst.imm);
            for (const Ref arg : inst.args) {
                hash = hash * 31 + arg;
            }
            return hash;
        }
    };

    Ref Add(const Inst& inst) {
        const auto [it, is_new] = refs.try_emplace(inst, static_cast<Ref>(insts.size()));
        if (is_new) {
            insts.push_back(inst);
        }
        return it->second;
    }

    boost::container::small_vector<Inst, 64> insts;
    std::unordered_map<Inst, Ref, InstHash> refs;
};

} // namespace Shader::Optimization::Srt
