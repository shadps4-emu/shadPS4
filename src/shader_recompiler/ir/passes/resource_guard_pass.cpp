// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <unordered_map>
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
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
        *this = Always();
    }
};

struct CompareKind {
    GuardCompare op{};
    bool swap{};
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
    case IR::Opcode::FPUnordEqual32:
        return CompareKind{.op = GuardCompare::FEq};
    case IR::Opcode::FPOrdNotEqual32:
    case IR::Opcode::FPUnordNotEqual32:
        return CompareKind{.op = GuardCompare::FNe};
    case IR::Opcode::FPOrdLessThan32:
    case IR::Opcode::FPUnordLessThan32:
        return CompareKind{.op = GuardCompare::FLt};
    case IR::Opcode::FPOrdLessThanEqual32:
    case IR::Opcode::FPUnordLessThanEqual32:
        return CompareKind{.op = GuardCompare::FLe};
    case IR::Opcode::FPOrdGreaterThan32:
    case IR::Opcode::FPUnordGreaterThan32:
        return CompareKind{.op = GuardCompare::FLt, .swap = true};
    case IR::Opcode::FPOrdGreaterThanEqual32:
    case IR::Opcode::FPUnordGreaterThanEqual32:
        return CompareKind{.op = GuardCompare::FLe, .swap = true};
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
    default:
        return std::nullopt;
    }
}

std::optional<GuardOperand> FloatOperand(const IR::Value& value) {
    const std::optional<GuardOperand> operand = Operand(value, 0);
    if (!operand || operand->shift != 0 || operand->mask != ~0U) {
        return std::nullopt;
    }
    return operand;
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

private:
    GuardTerms Atom(const GuardAtom& atom) {
        if (!atom.lhs.is_flatbuf && !atom.rhs.is_flatbuf) {
            return GuardTerms::Always();
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
        if (const std::optional<bool> known = Known(value)) {
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
        case IR::Opcode::SelectU1:
        case IR::Opcode::SelectU64:
            return Select(inst, polarity, depth);
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

    GuardTerms Select(const IR::Inst& inst, bool polarity, u32 depth) {
        const IR::Value cond = inst.Arg(0);
        if (const std::optional<bool> taken = Known(cond)) {
            return Necessary(inst.Arg(*taken ? 1 : 2), polarity, depth);
        }
        const auto when = [&](bool taken) {
            const GuardTerms terms = Necessary(inst.Arg(taken ? 1 : 2), polarity, depth);
            return terms.count == 0 ? terms : Necessary(cond, taken, depth).And(terms);
        };
        return when(true).Or(when(false));
    }

    GuardTerms Compare(const IR::Inst& inst, const CompareKind& kind, bool polarity) {
        const IR::Value lhs = inst.Arg(kind.swap ? 1 : 0);
        const IR::Value rhs = inst.Arg(kind.swap ? 0 : 1);
        switch (kind.op) {
        case GuardCompare::IEq:
        case GuardCompare::INe:
            return atoms.Leaf(lhs, rhs, GuardCompare::IEq,
                              (kind.op == GuardCompare::IEq) != polarity);
        case GuardCompare::FLt:
        case GuardCompare::FLe:
            return atoms.Leaf(polarity ? lhs : rhs, polarity ? rhs : lhs,
                              (kind.op == GuardCompare::FLt) == polarity ? GuardCompare::FLt
                                                                         : GuardCompare::FLe,
                              false);
        default:
            return atoms.Leaf(lhs, rhs, kind.op, !polarity);
        }
    }

    static std::optional<bool> Known(const IR::Value& value) {
        if (!value.IsImmediate()) {
            return std::nullopt;
        }
        if (value.Type() == IR::Type::U1) {
            return value.U1();
        }
        if (value.Type() == IR::Type::U64 &&
            (value.U64() == 0 || value.U64() == std::numeric_limits<u64>::max())) {
            return value.U64() != 0;
        }
        return std::nullopt;
    }

    AtomBuilder atoms;
    std::array<std::unordered_map<const IR::Inst*, GuardTerms>, 2> necessary;
};

struct ImageUse {
    u32 image{};
    u32 sampler{};
    bool has_sampler{};
    bool is_storage{};
};

std::optional<ImageUse> ImageUseOf(const IR::Inst& inst) {
    ImageUse use{};
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageSampleImplicitLod:
    case IR::Opcode::ImageSampleExplicitLod:
    case IR::Opcode::ImageSampleDrefImplicitLod:
    case IR::Opcode::ImageSampleDrefExplicitLod:
    case IR::Opcode::ImageGather:
    case IR::Opcode::ImageGatherDref:
    case IR::Opcode::ImageGradient:
    case IR::Opcode::ImageQueryLod:
        use.has_sampler = true;
        break;
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageRead:
        break;
    case IR::Opcode::ImageWrite:
        use.is_storage = true;
        break;
    default:
        if (!IsImageAtomicInstruction(inst)) {
            return std::nullopt;
        }
        use.is_storage = true;
        break;
    }
    const std::optional<u32> handle = ImmU32(inst.Arg(0));
    if (!handle) {
        return std::nullopt;
    }
    use.image = *handle & 0xFFFF;
    use.sampler = *handle >> 16;
    return use;
}

template <typename T>
bool IsGuardable(const SharpFetch<T>& fetch) {
    return fetch.summary != SharpFetch<T>::Summary::Invalid && fetch.load_mask != 0;
}

class GuardGroups {
public:
    explicit GuardGroups(size_t num_resources) : entries(num_resources) {}

    void AddUse(u32 binding, const GuardTerms& terms) {
        if (binding < entries.size()) {
            entries[binding] = entries[binding].Or(terms);
        }
    }

    void Emit(ResourceGuards& table, auto& resources) {
        for (size_t i = 0; i < resources.size(); ++i) {
            const GuardTerms& terms = entries[i];
            if (terms.IsTrivial() || !IsGuardable(resources[i].sharp_fetch) ||
                table.num_groups == ResourceGuards::MaxGroups) {
                continue;
            }
            resources[i].guard = table.num_groups;
            auto& group = table.groups[table.num_groups++];
            group.terms = terms.terms;
            group.num_terms = terms.count;
        }
    }

private:
    boost::container::small_vector<GuardTerms, 16> entries;
};

void BuildGuards(Info& info, const IR::AbstractSyntaxList& syntax_list) {
    GuardBuilder builder{info.resource_guards};
    GuardGroups images{info.images.size()};
    GuardGroups samplers{info.samplers.size()};
    boost::container::small_vector<GuardTerms, 16> scopes{GuardTerms::Always()};
    for (const IR::AbstractSyntaxNode& node : syntax_list) {
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::If:
            scopes.push_back(scopes.back().And(builder.Condition(node.data.if_node.cond, true)));
            break;
        case IR::AbstractSyntaxNode::Type::EndIf:
            scopes.pop_back();
            break;
        case IR::AbstractSyntaxNode::Type::Block:
            for (const IR::Inst& inst : node.data.block->Instructions()) {
                const std::optional<ImageUse> use = ImageUseOf(inst);
                if (!use) {
                    continue;
                }
                images.AddUse(use->image, use->is_storage ? GuardTerms::Always() : scopes.back());
                if (use->has_sampler) {
                    samplers.AddUse(use->sampler, scopes.back());
                }
            }
            break;
        default:
            break;
        }
    }
    images.Emit(info.resource_guards, info.images);
    samplers.Emit(info.resource_guards, info.samplers);
}

} // Anonymous namespace

void ResourceGuardPass(IR::Program& program) {
    Info& info = program.info;
    info.resource_guards = {};
    if (!info.skip_resource_guards) {
        BuildGuards(info, program.syntax_list);
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
