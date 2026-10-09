// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <unordered_map>
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/resource.h"

namespace Shader {
namespace {

bool IsNormalOrZero(u32 bits) {
    const u32 exponent = (bits >> 23) & 0xFF;
    return exponent != 0xFF && (exponent != 0 || (bits & 0x7FFFFF) == 0);
}

std::optional<bool> CompareValues(GuardCompare op, u32 x, u32 y) {
    switch (op) {
    case GuardCompare::IEq:
        return x == y;
    case GuardCompare::INe:
        return x != y;
    case GuardCompare::SLt:
        return static_cast<s32>(x) < static_cast<s32>(y);
    case GuardCompare::SLe:
        return static_cast<s32>(x) <= static_cast<s32>(y);
    case GuardCompare::ULt:
        return x < y;
    case GuardCompare::ULe:
        return x <= y;
    default:
        break;
    }
    if (!IsNormalOrZero(x) || !IsNormalOrZero(y)) {
        return std::nullopt;
    }
    const f32 a = std::bit_cast<f32>(x);
    const f32 b = std::bit_cast<f32>(y);
    switch (op) {
    case GuardCompare::FEq:
        return a == b;
    case GuardCompare::FNe:
        return a != b;
    case GuardCompare::FLt:
        return a < b;
    case GuardCompare::FLe:
        return a <= b;
    default:
        return std::nullopt;
    }
}

std::optional<u32> LoadOperand(const GuardOperand& operand, std::span<const u32> flatbuf) {
    if (!operand.is_flatbuf) {
        return operand.value;
    }
    if (operand.value >= flatbuf.size()) {
        return std::nullopt;
    }
    return (flatbuf[operand.value] >> operand.shift) & operand.mask;
}

bool IsFalse(const GuardAtom& atom, std::span<const u32> flatbuf) {
    const std::optional<u32> lhs = LoadOperand(atom.lhs, flatbuf);
    const std::optional<u32> rhs = LoadOperand(atom.rhs, flatbuf);
    if (!lhs || !rhs) {
        return false;
    }
    const std::optional<bool> result = CompareValues(atom.op, *lhs, *rhs);
    return result && *result == atom.negate;
}

} // Anonymous namespace
} // namespace Shader

namespace Shader::Optimization {
namespace {

constexpr u32 MaxDepth = 32;

struct GuardTerms {
    std::array<u64, ResourceGuards::MaxTerms> terms{};
    u8 count{};

    static GuardTerms Always() {
        return Of(0);
    }

    static GuardTerms Never() {
        return {};
    }

    static GuardTerms Of(u64 term) {
        return {.terms = {term}, .count = 1};
    }

    bool IsTrivial() const {
        return count == 0 || terms[0] == 0;
    }

    GuardTerms And(const GuardTerms& other) const {
        GuardTerms result;
        for (u32 i = 0; i < count; ++i) {
            for (u32 j = 0; j < other.count; ++j) {
                result.Add(terms[i] | other.terms[j]);
            }
        }
        return result;
    }

    GuardTerms Or(const GuardTerms& other) const {
        GuardTerms result = *this;
        for (u32 i = 0; i < other.count; ++i) {
            result.Add(other.terms[i]);
        }
        return result;
    }

    void Add(u64 term) {
        for (u32 i = 0; i < count; ++i) {
            if ((terms[i] & term) == terms[i]) {
                return;
            }
        }
        u32 kept{};
        for (u32 i = 0; i < count; ++i) {
            if ((terms[i] & term) != term) {
                terms[kept++] = terms[i];
            }
        }
        count = static_cast<u8>(kept);
        if (count < ResourceGuards::MaxTerms) {
            terms[count++] = term;
            return;
        }
        u32 closest{};
        for (u32 i = 1; i < count; ++i) {
            if (std::popcount(terms[i] & term) > std::popcount(terms[closest] & term)) {
                closest = i;
            }
        }
        const u64 merged = terms[closest] & term;
        terms[closest] = terms[--count];
        Add(merged);
    }
};

GuardTerms Majority(const GuardTerms& a, const GuardTerms& b, const GuardTerms& c) {
    return a.And(b).Or(a.And(c)).Or(b.And(c));
}

struct CompareKind {
    GuardCompare op{};
    bool swap{};
    bool unordered{};
};

std::optional<CompareKind> CompareKindOf(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::IEqual32:
        return CompareKind{.op = GuardCompare::IEq};
    case IR::Opcode::INotEqual32:
        return CompareKind{.op = GuardCompare::INe};
    case IR::Opcode::SLessThan32:
        return CompareKind{.op = GuardCompare::SLt};
    case IR::Opcode::SLessThanEqual32:
        return CompareKind{.op = GuardCompare::SLe};
    case IR::Opcode::SGreaterThan32:
        return CompareKind{.op = GuardCompare::SLt, .swap = true};
    case IR::Opcode::SGreaterThanEqual32:
        return CompareKind{.op = GuardCompare::SLe, .swap = true};
    case IR::Opcode::ULessThan32:
        return CompareKind{.op = GuardCompare::ULt};
    case IR::Opcode::ULessThanEqual32:
        return CompareKind{.op = GuardCompare::ULe};
    case IR::Opcode::UGreaterThan32:
        return CompareKind{.op = GuardCompare::ULt, .swap = true};
    case IR::Opcode::UGreaterThanEqual32:
        return CompareKind{.op = GuardCompare::ULe, .swap = true};
    case IR::Opcode::FPOrdEqual32:
        return CompareKind{.op = GuardCompare::FEq};
    case IR::Opcode::FPUnordEqual32:
        return CompareKind{.op = GuardCompare::FEq, .unordered = true};
    case IR::Opcode::FPOrdNotEqual32:
        return CompareKind{.op = GuardCompare::FNe};
    case IR::Opcode::FPUnordNotEqual32:
        return CompareKind{.op = GuardCompare::FNe, .unordered = true};
    case IR::Opcode::FPOrdLessThan32:
        return CompareKind{.op = GuardCompare::FLt};
    case IR::Opcode::FPUnordLessThan32:
        return CompareKind{.op = GuardCompare::FLt, .unordered = true};
    case IR::Opcode::FPOrdLessThanEqual32:
        return CompareKind{.op = GuardCompare::FLe};
    case IR::Opcode::FPUnordLessThanEqual32:
        return CompareKind{.op = GuardCompare::FLe, .unordered = true};
    case IR::Opcode::FPOrdGreaterThan32:
        return CompareKind{.op = GuardCompare::FLt, .swap = true};
    case IR::Opcode::FPUnordGreaterThan32:
        return CompareKind{.op = GuardCompare::FLt, .swap = true, .unordered = true};
    case IR::Opcode::FPOrdGreaterThanEqual32:
        return CompareKind{.op = GuardCompare::FLe, .swap = true};
    case IR::Opcode::FPUnordGreaterThanEqual32:
        return CompareKind{.op = GuardCompare::FLe, .swap = true, .unordered = true};
    default:
        return std::nullopt;
    }
}

std::optional<u32> ImmU32(const IR::Value& value) {
    if (value.IsImmediate() && value.Type() == IR::Type::U32) {
        return value.U32();
    }
    return std::nullopt;
}

const IR::Inst* Unpacked(const IR::Value& value) {
    const IR::Inst* const vec = value.TryInst();
    if (!vec) {
        return nullptr;
    }
    if (vec->GetOpcode() == IR::Opcode::UnpackUint2x32) {
        return vec;
    }
    if (vec->GetOpcode() != IR::Opcode::CompositeConstructU32x2) {
        return nullptr;
    }
    const IR::Inst* const lo = vec->Arg(0).TryInst();
    const IR::Inst* const hi = vec->Arg(1).TryInst();
    if (!lo || !hi || lo->GetOpcode() != IR::Opcode::CompositeExtractU32x2 ||
        hi->GetOpcode() != IR::Opcode::CompositeExtractU32x2 || lo->Arg(0) != hi->Arg(0) ||
        lo->Arg(1) != IR::Value{0U} || hi->Arg(1) != IR::Value{1U}) {
        return nullptr;
    }
    const IR::Inst* const unpack = lo->Arg(0).TryInst();
    return unpack && unpack->GetOpcode() == IR::Opcode::UnpackUint2x32 ? unpack : nullptr;
}

const IR::Inst* ValueSelect(const IR::Value& value) {
    const IR::Inst* inst = value.TryInst();
    while (inst && (inst->GetOpcode() == IR::Opcode::BitCastU32F32 ||
                    inst->GetOpcode() == IR::Opcode::BitCastF32U32)) {
        inst = inst->Arg(0).TryInst();
    }
    if (inst && (inst->GetOpcode() == IR::Opcode::SelectU32 ||
                 inst->GetOpcode() == IR::Opcode::SelectF32)) {
        return inst;
    }
    return nullptr;
}

std::pair<IR::Value, bool> StripNegation(IR::Value value) {
    bool negated = false;
    for (const IR::Inst* inst = value.TryInst(); inst && inst->GetOpcode() == IR::Opcode::FPNeg32;
         inst = value.TryInst()) {
        value = inst->Arg(0);
        negated = !negated;
    }
    return {value, negated};
}

std::optional<std::pair<IR::Value, IR::Value>> Unnegated(const IR::Value& lhs,
                                                         const IR::Value& rhs) {
    const auto [a, a_negated] = StripNegation(lhs);
    const auto [b, b_negated] = StripNegation(rhs);
    if (a_negated == b_negated) {
        return std::pair{a, b};
    }
    const IR::Value& fixed = a_negated ? b : a;
    if (!fixed.IsImmediate() || fixed.Type() != IR::Type::F32) {
        return std::nullopt;
    }
    const IR::Value negated{-fixed.F32()};
    return a_negated ? std::pair{a, negated} : std::pair{negated, b};
}

bool IsRange(const IR::Value& low, const IR::Value& high) {
    return low.IsImmediate() && high.IsImmediate() && low.Type() == IR::Type::F32 &&
           high.Type() == IR::Type::F32 && low.F32() <= high.F32();
}

GuardOperand Bits(GuardOperand operand, u32 offset, u32 mask) {
    if (operand.is_flatbuf) {
        operand.shift = static_cast<u8>(operand.shift + offset);
        operand.mask = (operand.mask >> offset) & mask;
    } else {
        operand.value = (operand.value >> offset) & mask;
    }
    return operand;
}

std::optional<GuardOperand> Operand(const IR::Value& value, u32 depth);

std::optional<GuardOperand> Masked(const IR::Inst& inst, u32 depth) {
    const std::optional<u32> lhs_mask = ImmU32(inst.Arg(0));
    const std::optional<u32> rhs_mask = ImmU32(inst.Arg(1));
    if (lhs_mask.has_value() == rhs_mask.has_value()) {
        return std::nullopt;
    }
    const std::optional<GuardOperand> operand = Operand(inst.Arg(lhs_mask ? 1 : 0), depth + 1);
    if (!operand || !operand->is_flatbuf) {
        return std::nullopt;
    }
    return Bits(*operand, 0, lhs_mask ? *lhs_mask : *rhs_mask);
}

std::optional<GuardOperand> Extract(const IR::Value& base, u32 offset, u32 count, u32 depth) {
    const std::optional<GuardOperand> operand = Operand(base, depth + 1);
    if (!operand || !operand->is_flatbuf || count == 0 || offset > 32 || count > 32 ||
        offset + count > 32 || operand->shift + offset >= 32) {
        return std::nullopt;
    }
    return Bits(*operand, offset, static_cast<u32>((u64{1} << count) - 1));
}

std::optional<GuardOperand> Operand(const IR::Value& value, u32 depth) {
    if (value.IsImmediate()) {
        if (value.Type() == IR::Type::U32) {
            return GuardOperand{.value = value.U32()};
        }
        if (value.Type() == IR::Type::F32) {
            return GuardOperand{.value = std::bit_cast<u32>(value.F32())};
        }
        return std::nullopt;
    }
    if (depth >= MaxDepth) {
        return std::nullopt;
    }
    const IR::Inst* const inst = value.Inst();
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::ReadConst:
        if (const SharpLocation location = SharpLocationFromSource(inst, false);
            location != UNKNOWN_LOCATION) {
            return GuardOperand{.value = location, .is_flatbuf = true};
        }
        return std::nullopt;
    case IR::Opcode::ReadFirstLane:
    case IR::Opcode::BitCastU32F32:
    case IR::Opcode::BitCastF32U32:
        return Operand(inst->Arg(0), depth + 1);
    case IR::Opcode::BitwiseAnd32:
        return Masked(*inst, depth);
    case IR::Opcode::ShiftRightLogical32:
        if (const std::optional<u32> shift = ImmU32(inst->Arg(1)); shift && *shift < 32) {
            return Extract(inst->Arg(0), *shift, 32 - *shift, depth);
        }
        return std::nullopt;
    case IR::Opcode::BitFieldUExtract: {
        const std::optional<u32> offset = ImmU32(inst->Arg(1));
        const std::optional<u32> count = ImmU32(inst->Arg(2));
        if (offset && count) {
            return Extract(inst->Arg(0), *offset, *count, depth);
        }
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

std::optional<GuardOperand> FloatOperand(const IR::Value& value) {
    const IR::Inst* const inst = value.TryInst();
    const bool is_abs = inst && inst->GetOpcode() == IR::Opcode::FPAbs32;
    const std::optional<GuardOperand> operand = Operand(is_abs ? inst->Arg(0) : value, 0);
    if (!operand || operand->shift != 0 || operand->mask != ~0U) {
        return std::nullopt;
    }
    return is_abs ? Bits(*operand, 0, 0x7FFFFFFF) : *operand;
}

class AtomBuilder {
public:
    explicit AtomBuilder(ResourceGuards& table_) : table{table_} {}

    GuardTerms Leaf(const IR::Value& lhs, const IR::Value& rhs, GuardCompare op, bool negate) {
        const bool is_float = op >= GuardCompare::FEq;
        const std::optional<GuardOperand> a = is_float ? FloatOperand(lhs) : Operand(lhs, 0);
        const std::optional<GuardOperand> b = is_float ? FloatOperand(rhs) : Operand(rhs, 0);
        if (!a || !b) {
            return GuardTerms::Always();
        }
        return Atom({.lhs = *a, .rhs = *b, .op = op, .negate = negate});
    }

    GuardTerms Equality(const IR::Value& lhs, const IR::Value& rhs, bool equal, u32 depth) {
        if (depth >= MaxDepth) {
            return GuardTerms::Always();
        }
        for (const bool left : {true, false}) {
            const IR::Inst* const combined = (left ? lhs : rhs).TryInst();
            const IR::Value& other = left ? rhs : lhs;
            if (!combined || ImmU32(other) != 0U) {
                continue;
            }
            if (combined->GetOpcode() == IR::Opcode::BitwiseOr32) {
                return OrEqualsZero(*combined, other, equal, depth + 1);
            }
            if (combined->GetOpcode() == IR::Opcode::BitwiseXor32) {
                return XorEqualsZero(*combined, equal, depth + 1);
            }
        }
        return Leaf(lhs, rhs, GuardCompare::IEq, !equal);
    }

    GuardTerms FloatEquality(const IR::Value& lhs, const IR::Value& rhs, GuardCompare op,
                             bool negate) {
        if (const auto sides = Unnegated(lhs, rhs)) {
            return Leaf(sides->first, sides->second, op, negate);
        }
        return GuardTerms::Always();
    }

    GuardTerms Order(const IR::Value& small, const IR::Value& big, bool strict, bool nan_false,
                     u32 depth) {
        if (depth >= MaxDepth) {
            return GuardTerms::Always();
        }
        const auto [s, s_negated] = StripNegation(small);
        const auto [b, b_negated] = StripNegation(big);
        if (s_negated || b_negated) {
            if (const auto sides = Unnegated(small, big)) {
                return Order(sides->second, sides->first, strict, nan_false, depth + 1);
            }
            return GuardTerms::Always();
        }
        const auto below = [&](const IR::Value& value) {
            return Order(value, b, strict, nan_false, depth + 1);
        };
        const auto above = [&](const IR::Value& value) {
            return Order(s, value, strict, nan_false, depth + 1);
        };
        if (const std::optional<GuardTerms> terms = OrderOperation(s, true, nan_false, below)) {
            return *terms;
        }
        if (const std::optional<GuardTerms> terms = OrderOperation(b, false, nan_false, above)) {
            return *terms;
        }
        return Leaf(s, b, strict ? GuardCompare::FLt : GuardCompare::FLe, false);
    }

    GuardTerms Class(const IR::Inst& inst, bool polarity) {
        const std::optional<GuardOperand> value = FloatOperand(inst.Arg(0));
        if (!value) {
            return GuardTerms::Always();
        }
        const bool is_nan = inst.GetOpcode() == IR::Opcode::FPIsNan32;
        const GuardTerms special = Atom({.lhs = Bits(*value, 23, 0xFF),
                                         .rhs = {.value = 0xFF},
                                         .op = GuardCompare::IEq,
                                         .negate = !polarity});
        const GuardTerms payload = Atom({.lhs = Bits(*value, 0, 0x7FFFFF),
                                         .rhs = {},
                                         .op = GuardCompare::IEq,
                                         .negate = is_nan == polarity});
        return polarity ? special.And(payload) : special.Or(payload);
    }

private:
    GuardTerms OrEqualsZero(const IR::Inst& inst, const IR::Value& zero, bool equal, u32 depth) {
        const GuardTerms lhs = Equality(inst.Arg(0), zero, equal, depth);
        const GuardTerms rhs = Equality(inst.Arg(1), zero, equal, depth);
        return equal ? lhs.And(rhs) : lhs.Or(rhs);
    }

    GuardTerms XorEqualsZero(const IR::Inst& inst, bool equal, u32 depth) {
        return Equality(inst.Arg(0), inst.Arg(1), equal, depth);
    }

    static std::optional<GuardTerms> OrderOperation(const IR::Value& value, bool is_small,
                                                    bool nan_false, const auto& order) {
        const IR::Inst* const inst = value.TryInst();
        if (!inst) {
            return std::nullopt;
        }
        switch (inst->GetOpcode()) {
        case IR::Opcode::FPMin32:
            return is_small ? EitherArgument(*inst, order) : BothArguments(*inst, nan_false, order);
        case IR::Opcode::FPMax32:
            return is_small ? BothArguments(*inst, nan_false, order) : EitherArgument(*inst, order);
        case IR::Opcode::FPClamp32:
            return Clamp(*inst, is_small, nan_false, order);
        case IR::Opcode::FPMedTri32:
            return Median(*inst, order);
        default:
            return std::nullopt;
        }
    }

    static GuardTerms EitherArgument(const IR::Inst& inst, const auto& order) {
        return order(inst.Arg(0)).Or(order(inst.Arg(1)));
    }

    static GuardTerms BothArguments(const IR::Inst& inst, bool nan_false, const auto& order) {
        return nan_false ? order(inst.Arg(0)).And(order(inst.Arg(1))) : GuardTerms::Always();
    }

    static GuardTerms Clamp(const IR::Inst& inst, bool is_small, bool nan_false,
                            const auto& order) {
        if (!nan_false || !IsRange(inst.Arg(1), inst.Arg(2))) {
            return GuardTerms::Always();
        }
        if (is_small) {
            return order(inst.Arg(0)).And(order(inst.Arg(1))).Or(order(inst.Arg(2)));
        }
        return order(inst.Arg(0)).Or(order(inst.Arg(1))).And(order(inst.Arg(2)));
    }

    static GuardTerms Median(const IR::Inst& inst, const auto& order) {
        return Majority(order(inst.Arg(0)), order(inst.Arg(1)), order(inst.Arg(2)));
    }

    GuardTerms Atom(const GuardAtom& atom) {
        if (!atom.lhs.is_flatbuf && !atom.rhs.is_flatbuf) {
            return IsFalse(atom, {}) ? GuardTerms::Never() : GuardTerms::Always();
        }
        u32 index{};
        while (index < table.num_atoms && table.atoms[index] != atom) {
            ++index;
        }
        if (index == ResourceGuards::MaxAtoms) {
            return GuardTerms::Always();
        }
        if (index == table.num_atoms) {
            table.atoms[table.num_atoms++] = atom;
        }
        return GuardTerms::Of(u64{1} << index);
    }

    ResourceGuards& table;
};

class GuardBuilder {
public:
    explicit GuardBuilder(ResourceGuards& table) : atoms{table} {}

    GuardTerms Condition(const IR::Value& value, bool polarity) {
        for (auto& seen : necessary) {
            seen.clear();
        }
        return Necessary(value, polarity, 0);
    }

private:
    GuardTerms Necessary(const IR::Value& value, bool polarity, u32 depth) {
        if (const std::optional<bool> known = Known(value, depth)) {
            return *known == polarity ? GuardTerms::Always() : GuardTerms::Never();
        }
        if (value.IsImmediate() || depth >= MaxDepth) {
            return GuardTerms::Always();
        }
        const IR::Inst* const inst = value.Inst();
        auto& seen = necessary[polarity];
        if (const auto it = seen.find(inst); it != seen.end()) {
            return it->second;
        }
        seen.emplace(inst, GuardTerms::Always());
        const GuardTerms terms = NecessaryInst(*inst, polarity, depth + 1);
        seen[inst] = terms;
        return terms;
    }

    GuardTerms NecessaryInst(const IR::Inst& inst, bool polarity, u32 depth) {
        switch (inst.GetOpcode()) {
        case IR::Opcode::ConditionRef:
        case IR::Opcode::Ballot:
        case IR::Opcode::InverseBallot:
            return Necessary(inst.Arg(0), polarity, depth);
        case IR::Opcode::LogicalNot:
        case IR::Opcode::BitwiseNot64:
            return Necessary(inst.Arg(0), !polarity, depth);
        case IR::Opcode::LogicalAnd:
        case IR::Opcode::BitwiseAnd64:
            return Junction(inst, true, polarity, depth);
        case IR::Opcode::LogicalOr:
        case IR::Opcode::BitwiseOr64:
            return Junction(inst, false, polarity, depth);
        case IR::Opcode::LogicalXor:
        case IR::Opcode::BitwiseXor64:
            return Exclusive(inst, polarity, depth);
        case IR::Opcode::SelectU1:
        case IR::Opcode::SelectU64:
            return Select(inst, polarity, depth);
        case IR::Opcode::PackUint2x32:
            if (const IR::Inst* const unpack = Unpacked(inst.Arg(0))) {
                return Necessary(unpack->Arg(0), polarity, depth);
            }
            return GuardTerms::Always();
        case IR::Opcode::FPIsNan32:
        case IR::Opcode::FPIsInf32:
            return atoms.Class(inst, polarity);
        default:
            if (const std::optional<CompareKind> kind = CompareKindOf(inst.GetOpcode())) {
                return Compare(inst, *kind, polarity);
            }
            return GuardTerms::Always();
        }
    }

    GuardTerms Junction(const IR::Inst& inst, bool is_and, bool polarity, u32 depth) {
        const GuardTerms lhs = Necessary(inst.Arg(0), polarity, depth);
        if (polarity != is_and) {
            return lhs.Or(Necessary(inst.Arg(1), polarity, depth));
        }
        return lhs.count == 0 ? lhs : lhs.And(Necessary(inst.Arg(1), polarity, depth));
    }

    GuardTerms Exclusive(const IR::Inst& inst, bool polarity, u32 depth) {
        const auto side = [&](bool lhs_value) {
            const GuardTerms lhs = Necessary(inst.Arg(0), lhs_value, depth);
            return lhs.count == 0 ? lhs
                                  : lhs.And(Necessary(inst.Arg(1), lhs_value != polarity, depth));
        };
        return side(true).Or(side(false));
    }

    GuardTerms Select(const IR::Inst& inst, bool polarity, u32 depth) {
        return Choice(inst.Arg(0), depth, [&](bool taken) {
            return Necessary(inst.Arg(taken ? 1 : 2), polarity, depth);
        });
    }

    GuardTerms Choice(const IR::Value& cond, u32 depth, const auto& arm) {
        if (const std::optional<bool> taken = Known(cond, depth)) {
            return arm(*taken);
        }
        const auto when = [&](bool taken) {
            const GuardTerms terms = arm(taken);
            return terms.count == 0 ? terms : Necessary(cond, taken, depth).And(terms);
        };
        return when(true).Or(when(false));
    }

    GuardTerms Compare(const IR::Inst& inst, const CompareKind& kind, bool polarity) {
        return Comparison(inst.Arg(kind.swap ? 1 : 0), inst.Arg(kind.swap ? 0 : 1), kind.op,
                          !kind.unordered, polarity, 0);
    }

    GuardTerms Comparison(const IR::Value& lhs, const IR::Value& rhs, GuardCompare op, bool ordered,
                          bool polarity, u32 depth) {
        if (depth >= MaxDepth) {
            return GuardTerms::Always();
        }
        if (const std::optional<GuardTerms> terms =
                SelectComparison(lhs, rhs, op, ordered, polarity, depth)) {
            return *terms;
        }
        switch (op) {
        case GuardCompare::IEq:
        case GuardCompare::INe:
            return atoms.Equality(lhs, rhs, (op == GuardCompare::IEq) == polarity, depth + 1);
        case GuardCompare::FEq:
        case GuardCompare::FNe:
            return atoms.FloatEquality(lhs, rhs, op, !polarity);
        case GuardCompare::FLt:
        case GuardCompare::FLe:
            return atoms.Order(polarity ? lhs : rhs, polarity ? rhs : lhs,
                               (op == GuardCompare::FLt) == polarity, ordered == polarity,
                               depth + 1);
        default:
            return atoms.Leaf(lhs, rhs, op, !polarity);
        }
    }

    std::optional<GuardTerms> SelectComparison(const IR::Value& lhs, const IR::Value& rhs,
                                               GuardCompare op, bool ordered, bool polarity,
                                               u32 depth) {
        for (const bool left : {true, false}) {
            if (const IR::Inst* const select = ValueSelect(left ? lhs : rhs)) {
                return Choice(select->Arg(0), depth, [&](bool taken) {
                    const IR::Value arm = select->Arg(taken ? 1 : 2);
                    return Comparison(left ? arm : lhs, left ? rhs : arm, op, ordered, polarity,
                                      depth + 1);
                });
            }
        }
        return std::nullopt;
    }

    std::optional<bool> Known(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() == IR::Type::U1) {
                return value.U1();
            }
            if (value.Type() == IR::Type::U64 &&
                (value.U64() == 0 || value.U64() == std::numeric_limits<u64>::max())) {
                return value.U64() != 0;
            }
            return std::nullopt;
        }
        const IR::Inst* const inst = value.Inst();
        if (const auto it = known.find(inst); it != known.end()) {
            return it->second;
        }
        known.emplace(inst, std::nullopt);
        const std::optional<bool> result =
            depth < MaxDepth ? KnownInst(*inst, depth + 1) : std::nullopt;
        known[inst] = result;
        return result;
    }

    std::optional<bool> KnownInst(const IR::Inst& inst, u32 depth) {
        switch (inst.GetOpcode()) {
        case IR::Opcode::ConditionRef:
        case IR::Opcode::Ballot:
        case IR::Opcode::InverseBallot:
            return Known(inst.Arg(0), depth);
        case IR::Opcode::LogicalNot:
        case IR::Opcode::BitwiseNot64:
            if (const std::optional<bool> value = Known(inst.Arg(0), depth)) {
                return !*value;
            }
            return std::nullopt;
        case IR::Opcode::LogicalAnd:
        case IR::Opcode::BitwiseAnd64:
            return KnownJunction(inst, true, depth);
        case IR::Opcode::LogicalOr:
        case IR::Opcode::BitwiseOr64:
            return KnownJunction(inst, false, depth);
        case IR::Opcode::SelectU1:
        case IR::Opcode::SelectU64: {
            if (const std::optional<bool> taken = Known(inst.Arg(0), depth)) {
                return Known(inst.Arg(*taken ? 1 : 2), depth);
            }
            const std::optional<bool> on_true = Known(inst.Arg(1), depth);
            return on_true == Known(inst.Arg(2), depth) ? on_true : std::nullopt;
        }
        case IR::Opcode::PackUint2x32:
            if (const IR::Inst* const unpack = Unpacked(inst.Arg(0))) {
                return Known(unpack->Arg(0), depth);
            }
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }

    std::optional<bool> KnownJunction(const IR::Inst& inst, bool is_and, u32 depth) {
        const std::optional<bool> lhs = Known(inst.Arg(0), depth);
        const std::optional<bool> rhs = Known(inst.Arg(1), depth);
        if (lhs == !is_and || rhs == !is_and) {
            return !is_and;
        }
        if (lhs && rhs) {
            return is_and;
        }
        return std::nullopt;
    }

    AtomBuilder atoms;
    std::unordered_map<const IR::Inst*, std::optional<bool>> known;
    std::array<std::unordered_map<const IR::Inst*, GuardTerms>, 2> necessary;
};

class GuardCollector {
public:
    explicit GuardCollector(GuardBuilder& builder_) : builder{builder_} {}

    GuardTerms BlockTerms(IR::Block* block) {
        boost::container::small_vector<IR::Block*, 32> chain;
        GuardTerms block_terms = GuardTerms::Always();
        for (IR::Block* current = block; current;) {
            if (const auto it = terms.find(current); it != terms.end()) {
                block_terms = it->second;
                break;
            }
            chain.push_back(current);
            IR::Block* const idom = current->immediate_dominator;
            current = idom != current ? idom : nullptr;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            block_terms = block_terms.And(EdgeTerms(*it));
            terms.emplace(*it, block_terms);
        }
        return block_terms;
    }

private:
    GuardTerms EdgeTerms(IR::Block* block) {
        const IR::Block* const idom = block->immediate_dominator;
        if (!idom || idom == block || block->ImmPredecessors().size() != 1) {
            return GuardTerms::Always();
        }
        const IR::Block* const pred = block->ImmPredecessors().front();
        const Gcn::Block* const cfg = pred->cfg_block;
        if (pred->branch_cond.IsEmpty() || pred->ImmSuccessors().size() != 2 || !cfg ||
            !cfg->branch_true || !cfg->branch_false) {
            return GuardTerms::Always();
        }
        const IR::Block* const on_true = cfg->branch_true->ir_block;
        const IR::Block* const on_false = cfg->branch_false->ir_block;
        if (on_true == on_false || (block != on_true && block != on_false)) {
            return GuardTerms::Always();
        }
        return builder.Condition(pred->branch_cond, block == on_true);
    }

    GuardBuilder& builder;
    std::unordered_map<const IR::Block*, GuardTerms> terms;
};

template <typename T>
class GuardGroups {
public:
    u32 AddUse(const SharpFetch<T>& key, const GuardTerms& terms) {
        const auto it = std::ranges::find(entries, key, &Entry::key);
        const u32 index = static_cast<u32>(std::distance(entries.begin(), it));
        if (it == entries.end()) {
            entries.push_back({.key = key});
        }
        const bool is_guardable =
            key.summary != SharpFetch<T>::Summary::Invalid && key.load_mask != 0;
        Entry& entry = entries[index];
        entry.terms = entry.terms.Or(is_guardable ? terms : GuardTerms::Always());
        return index;
    }

    void AddTermsToAll(const GuardTerms& terms) {
        for (Entry& entry : entries) {
            entry.terms = entry.terms.Or(terms);
        }
    }

    void Emit(ResourceGuards& table) {
        for (Entry& entry : entries) {
            if (entry.terms.IsTrivial() || table.num_groups == ResourceGuards::MaxGroups) {
                continue;
            }
            entry.group = table.num_groups;
            auto& group = table.groups[table.num_groups++];
            group.terms = entry.terms.terms;
            group.num_terms = entry.terms.count;
        }
    }

    u8 GroupOf(u32 index) const {
        return index < entries.size() ? entries[index].group : NO_GUARD;
    }

private:
    struct Entry {
        SharpFetch<T> key{};
        GuardTerms terms{};
        u8 group{NO_GUARD};
    };

    boost::container::small_vector<Entry, 16> entries;
};

void BuildGuards(ResourceGuards& table, ResourceDiscoveryList& resources) {
    GuardBuilder builder{table};
    GuardCollector collector{builder};
    GuardGroups<AmdGpu::Image> images;
    GuardGroups<AmdGpu::Sampler> samplers;
    constexpr u32 NoUse = std::numeric_limits<u32>::max();
    boost::container::small_vector<std::array<u32, 2>, 32> uses(resources.size(), {NoUse, NoUse});
    boost::container::small_vector<GuardTerms, 4> lod_terms;
    for (size_t i = 0; i < resources.size(); ++i) {
        const IR::Inst& inst = *resources[i].user;
        if (!IsImageInstruction(inst)) {
            continue;
        }
        const GuardTerms terms = collector.BlockTerms(inst.GetParent());
        const bool is_storage =
            inst.GetOpcode() == IR::Opcode::ImageWrite || IsImageAtomicInstruction(inst);
        uses[i][0] =
            images.AddUse(ConstructSharpFetch<AmdGpu::Image>(resources[i].sharps[0], false),
                          is_storage ? GuardTerms::Always() : terms);
        if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
            uses[i][1] = samplers.AddUse(
                ConstructSharpFetch<AmdGpu::Sampler>(resources[i].sharps[1], false), terms);
        } else if (inst.GetOpcode() == IR::Opcode::ImageQueryLod) {
            lod_terms.push_back(terms);
        }
    }
    for (const GuardTerms& terms : lod_terms) {
        samplers.AddTermsToAll(terms);
    }
    images.Emit(table);
    samplers.Emit(table);
    for (size_t i = 0; i < resources.size(); ++i) {
        resources[i].guards = {images.GroupOf(uses[i][0]), samplers.GroupOf(uses[i][1])};
    }
}

} // Anonymous namespace

void ResourceGuardPass(IR::Program& program, ResourceDiscoveryList& resources) {
    Info& info = program.info;
    info.resource_guards = {};
    if (!info.skip_resource_guards) {
        IR::ComputeDominators(program.post_order_blocks);
        BuildGuards(info.resource_guards, resources);
        for (IR::Block* const block : program.blocks) {
            block->immediate_dominator = nullptr;
        }
    }
    if (info.key_info) {
        info.dead_resource_guards = info.key_info->resource_guards == info.resource_guards
                                        ? info.key_info->dead_resource_guards
                                        : 0;
    } else {
        info.RefreshResourceGuards();
    }
}

} // namespace Shader::Optimization

namespace Shader {

u32 ResourceGuards::EvaluateDead(std::span<const u32> flatbuf) const {
    if (num_groups == 0) {
        return 0;
    }
    u64 false_atoms{};
    for (u32 i = 0; i < num_atoms; ++i) {
        if (IsFalse(atoms[i], flatbuf)) {
            false_atoms |= u64{1} << i;
        }
    }
    if (false_atoms == 0) {
        return 0;
    }
    u32 dead{};
    for (u32 i = 0; i < num_groups; ++i) {
        const Group& group = groups[i];
        bool is_dead = group.num_terms != 0;
        for (u32 t = 0; is_dead && t < group.num_terms; ++t) {
            is_dead = (group.terms[t] & false_atoms) != 0;
        }
        dead |= static_cast<u32>(is_dead) << i;
    }
    return dead;
}

} // namespace Shader
