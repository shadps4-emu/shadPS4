// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <unordered_map>
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/resource.h"

namespace Shader::Optimization {
namespace {

class GuardBuilder {
public:
    explicit GuardBuilder(ResourceGuards& table_) : table{table_} {
        table = {};
        table.nodes[NodeFalse].op = GuardOp::False;
        table.nodes[NodeTrue].op = GuardOp::True;
        table.num_nodes = static_cast<u8>(NodeTrue + 1);
    }

    u16 Condition(const IR::Value& value, bool polarity) {
        const u8 cond = Bool(value, 0);
        const u8 root = polarity ? cond : Not(cond);
        if (root <= NodeTrue || !has_leaf[root]) {
            return 0;
        }
        for (u32 i = 0; i < table.num_conds; ++i) {
            if (table.conds[i] == root) {
                return static_cast<u16>(1U << i);
            }
        }
        if (table.num_conds == ResourceGuards::MaxConds) {
            return 0;
        }
        table.conds[table.num_conds] = root;
        return static_cast<u16>(1U << table.num_conds++);
    }

private:
    enum View : u32 {
        ViewBool,
        ViewWord,
        ViewFloat,
        ViewLanes,
        NumViews,
    };

    static constexpr u8 NodeUnknown = 0;
    static constexpr u8 NodeFalse = 1;
    static constexpr u8 NodeTrue = 2;
    static constexpr u8 NotBuilt = 0xFF;
    static constexpr u32 MaxDepth = 32;

    u8 Make(GuardOp op, u8 a = NodeUnknown, u8 b = NodeUnknown, u8 c = NodeUnknown, u32 imm = 0) {
        const GuardNode node{.op = op, .a = a, .b = b, .c = c, .imm = imm};
        for (u32 i = 0; i < table.num_nodes; ++i) {
            if (table.nodes[i] == node) {
                return static_cast<u8>(i);
            }
        }
        if (table.num_nodes == ResourceGuards::MaxNodes) {
            return NodeUnknown;
        }
        const u8 index = table.num_nodes++;
        table.nodes[index] = node;
        has_leaf[index] = op == GuardOp::Flatbuf || has_leaf[a] || has_leaf[b] || has_leaf[c];
        return index;
    }

    u8 Leaf(GuardOp op, u32 imm) {
        return Make(op, NodeUnknown, NodeUnknown, NodeUnknown, imm);
    }

    u8 Strict(GuardOp op, u8 a) {
        return a == NodeUnknown ? NodeUnknown : Make(op, a);
    }

    u8 Strict(GuardOp op, u8 a, u8 b) {
        return a == NodeUnknown || b == NodeUnknown ? NodeUnknown : Make(op, a, b);
    }

    u8 Strict(GuardOp op, u8 a, u8 b, u8 c) {
        if (a == NodeUnknown || b == NodeUnknown || c == NodeUnknown) {
            return NodeUnknown;
        }
        return Make(op, a, b, c);
    }

    u8 Not(u8 a) {
        if (a == NodeFalse) {
            return NodeTrue;
        }
        if (a == NodeTrue) {
            return NodeFalse;
        }
        if (table.nodes[a].op == GuardOp::Not) {
            return table.nodes[a].a;
        }
        return Strict(GuardOp::Not, a);
    }

    u8 And(u8 a, u8 b) {
        if (a == NodeFalse || b == NodeFalse) {
            return NodeFalse;
        }
        if (a == NodeTrue || a == b) {
            return b;
        }
        if (b == NodeTrue) {
            return a;
        }
        return Make(GuardOp::And, a, b);
    }

    u8 Or(u8 a, u8 b) {
        if (a == NodeTrue || b == NodeTrue) {
            return NodeTrue;
        }
        if (a == NodeFalse || a == b) {
            return b;
        }
        if (b == NodeFalse) {
            return a;
        }
        return Make(GuardOp::Or, a, b);
    }

    u8 Xor(u8 a, u8 b) {
        if (a == NodeFalse) {
            return b;
        }
        if (b == NodeFalse) {
            return a;
        }
        if (a == NodeTrue) {
            return Not(b);
        }
        if (b == NodeTrue) {
            return Not(a);
        }
        return Strict(GuardOp::Xor, a, b);
    }

    u8 Select(u8 cond, u8 a, u8 b) {
        if (cond == NodeTrue || a == b) {
            return a;
        }
        if (cond == NodeFalse) {
            return b;
        }
        if (cond == NodeUnknown && (a == NodeUnknown || b == NodeUnknown)) {
            return NodeUnknown;
        }
        return Make(GuardOp::Select, cond, a, b);
    }

    template <typename Func>
    u8 Memo(const IR::Inst* inst, View view, u32 depth, Func&& func) {
        auto [it, inserted] = memo.try_emplace(inst);
        std::array<u8, NumViews>& entry = it->second;
        if (inserted) {
            entry.fill(NotBuilt);
        }
        if (entry[view] == NotBuilt) {
            entry[view] = NodeUnknown;
            entry[view] = depth < MaxDepth ? func(depth + 1) : NodeUnknown;
        }
        return entry[view];
    }

    u8 Compare(GuardOp op, const IR::Inst& inst, u32 depth, bool swap) {
        const u8 lhs = Word(inst.Arg(0), depth);
        const u8 rhs = Word(inst.Arg(1), depth);
        return swap ? Strict(op, rhs, lhs) : Strict(op, lhs, rhs);
    }

    u8 FloatCompare(GuardOp op, const IR::Inst& inst, u32 depth, bool swap) {
        const u8 lhs = Float(inst.Arg(0), depth);
        const u8 rhs = Float(inst.Arg(1), depth);
        return swap ? Strict(op, rhs, lhs) : Strict(op, lhs, rhs);
    }

    u8 WordBinary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 lhs = Word(inst.Arg(0), depth);
        const u8 rhs = Word(inst.Arg(1), depth);
        return Strict(op, lhs, rhs);
    }

    u8 FloatBinary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 lhs = Float(inst.Arg(0), depth);
        const u8 rhs = Float(inst.Arg(1), depth);
        return Strict(op, lhs, rhs);
    }

    u8 FloatTernary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 x = Float(inst.Arg(0), depth);
        const u8 y = Float(inst.Arg(1), depth);
        const u8 z = Float(inst.Arg(2), depth);
        return Strict(op, x, y, z);
    }

    u8 Bool(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U1) {
                return NodeUnknown;
            }
            return value.U1() ? NodeTrue : NodeFalse;
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewBool, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::ConditionRef:
                return Bool(inst->Arg(0), d);
            case IR::Opcode::LogicalNot:
                return Not(Bool(inst->Arg(0), d));
            case IR::Opcode::LogicalAnd: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return And(lhs, rhs);
            }
            case IR::Opcode::LogicalOr: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return Or(lhs, rhs);
            }
            case IR::Opcode::LogicalXor: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return Xor(lhs, rhs);
            }
            case IR::Opcode::SelectU1: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Bool(inst->Arg(1), d);
                const u8 rhs = Bool(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::InverseBallot:
                return Lanes(inst->Arg(0), d);
            case IR::Opcode::IEqual32:
                return Compare(GuardOp::IEq, *inst, d, false);
            case IR::Opcode::INotEqual32:
                return Compare(GuardOp::INe, *inst, d, false);
            case IR::Opcode::SLessThan32:
                return Compare(GuardOp::SLt, *inst, d, false);
            case IR::Opcode::SLessThanEqual32:
                return Compare(GuardOp::SLe, *inst, d, false);
            case IR::Opcode::SGreaterThan32:
                return Compare(GuardOp::SLt, *inst, d, true);
            case IR::Opcode::SGreaterThanEqual32:
                return Compare(GuardOp::SLe, *inst, d, true);
            case IR::Opcode::ULessThan32:
                return Compare(GuardOp::ULt, *inst, d, false);
            case IR::Opcode::ULessThanEqual32:
                return Compare(GuardOp::ULe, *inst, d, false);
            case IR::Opcode::UGreaterThan32:
                return Compare(GuardOp::ULt, *inst, d, true);
            case IR::Opcode::UGreaterThanEqual32:
                return Compare(GuardOp::ULe, *inst, d, true);
            case IR::Opcode::FPOrdEqual32:
            case IR::Opcode::FPUnordEqual32:
                return FloatCompare(GuardOp::FEq, *inst, d, false);
            case IR::Opcode::FPOrdNotEqual32:
            case IR::Opcode::FPUnordNotEqual32:
                return FloatCompare(GuardOp::FNe, *inst, d, false);
            case IR::Opcode::FPOrdLessThan32:
            case IR::Opcode::FPUnordLessThan32:
                return FloatCompare(GuardOp::FLt, *inst, d, false);
            case IR::Opcode::FPOrdLessThanEqual32:
            case IR::Opcode::FPUnordLessThanEqual32:
                return FloatCompare(GuardOp::FLe, *inst, d, false);
            case IR::Opcode::FPOrdGreaterThan32:
            case IR::Opcode::FPUnordGreaterThan32:
                return FloatCompare(GuardOp::FLt, *inst, d, true);
            case IR::Opcode::FPOrdGreaterThanEqual32:
            case IR::Opcode::FPUnordGreaterThanEqual32:
                return FloatCompare(GuardOp::FLe, *inst, d, true);
            case IR::Opcode::FPIsNan32:
            case IR::Opcode::FPIsInf32:
                return Strict(GuardOp::FIsNanOrInf, Float(inst->Arg(0), d));
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Word(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U32) {
                return NodeUnknown;
            }
            return Leaf(GuardOp::ImmU32, value.U32());
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewWord, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::GetUserData:
                return Leaf(GuardOp::Flatbuf, static_cast<u32>(inst->Arg(0).ScalarReg()));
            case IR::Opcode::ReadConst:
                if (const u32 offset = inst->Flags<u32>(); offset != 0) {
                    return Leaf(GuardOp::Flatbuf, offset);
                }
                return NodeUnknown;
            case IR::Opcode::ReadFirstLane:
                return Word(inst->Arg(0), d);
            case IR::Opcode::BitCastU32F32:
                return Strict(GuardOp::AsU32, Float(inst->Arg(0), d));
            case IR::Opcode::SelectU32: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Word(inst->Arg(1), d);
                const u8 rhs = Word(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::BitwiseAnd32:
                return WordBinary(GuardOp::BitAnd, *inst, d);
            case IR::Opcode::BitwiseOr32:
                return WordBinary(GuardOp::BitOr, *inst, d);
            case IR::Opcode::BitwiseXor32:
                return WordBinary(GuardOp::BitXor, *inst, d);
            case IR::Opcode::ShiftRightLogical32:
                return WordBinary(GuardOp::Shr, *inst, d);
            case IR::Opcode::BitFieldUExtract: {
                const u8 base = Word(inst->Arg(0), d);
                const u8 offset = Word(inst->Arg(1), d);
                const u8 count = Word(inst->Arg(2), d);
                return Strict(GuardOp::UExtract, base, offset, count);
            }
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Float(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::F32) {
                return NodeUnknown;
            }
            return Leaf(GuardOp::ImmF32, std::bit_cast<u32>(value.F32()));
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewFloat, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::BitCastF32U32:
                return Strict(GuardOp::AsF32, Word(inst->Arg(0), d));
            case IR::Opcode::SelectF32: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Float(inst->Arg(1), d);
                const u8 rhs = Float(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::FPAdd32:
                return FloatBinary(GuardOp::FAdd, *inst, d);
            case IR::Opcode::FPSub32:
                return FloatBinary(GuardOp::FSub, *inst, d);
            case IR::Opcode::FPMin32:
                return FloatBinary(GuardOp::FMin, *inst, d);
            case IR::Opcode::FPMax32:
                return FloatBinary(GuardOp::FMax, *inst, d);
            case IR::Opcode::FPNeg32:
                return Strict(GuardOp::FNeg, Float(inst->Arg(0), d));
            case IR::Opcode::FPAbs32:
                return Strict(GuardOp::FAbs, Float(inst->Arg(0), d));
            case IR::Opcode::FPMedTri32:
                return FloatTernary(GuardOp::FMed3, *inst, d);
            case IR::Opcode::FPClamp32:
                return FloatTernary(GuardOp::FClamp, *inst, d);
            case IR::Opcode::FPSaturate32: {
                const u8 x = Float(inst->Arg(0), d);
                const u8 zero = Leaf(GuardOp::ImmF32, std::bit_cast<u32>(0.0f));
                const u8 one = Leaf(GuardOp::ImmF32, std::bit_cast<u32>(1.0f));
                return Strict(GuardOp::FClamp, x, zero, one);
            }
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Lanes(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U64) {
                return NodeUnknown;
            }
            if (value.U64() == 0) {
                return NodeFalse;
            }
            return value.U64() == std::numeric_limits<u64>::max() ? NodeTrue : NodeUnknown;
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewLanes, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::Ballot:
                return Bool(inst->Arg(0), d);
            case IR::Opcode::BitwiseAnd64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return And(lhs, rhs);
            }
            case IR::Opcode::BitwiseOr64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return Or(lhs, rhs);
            }
            case IR::Opcode::BitwiseXor64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return Xor(lhs, rhs);
            }
            case IR::Opcode::BitwiseNot64:
                return Not(Lanes(inst->Arg(0), d));
            case IR::Opcode::SelectU64: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Lanes(inst->Arg(1), d);
                const u8 rhs = Lanes(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::PackUint2x32:
                return PackedLanes(inst->Arg(0), d);
            default:
                return NodeUnknown;
            }
        });
    }

    u8 PackedLanes(const IR::Value& value, u32 depth) {
        const IR::Inst* const vec = value.TryInst();
        if (!vec) {
            return NodeUnknown;
        }
        if (vec->GetOpcode() == IR::Opcode::UnpackUint2x32) {
            return Lanes(vec->Arg(0), depth);
        }
        if (vec->GetOpcode() != IR::Opcode::CompositeConstructU32x2) {
            return NodeUnknown;
        }
        const IR::Inst* const lo = vec->Arg(0).TryInst();
        const IR::Inst* const hi = vec->Arg(1).TryInst();
        if (!lo || !hi || lo->GetOpcode() != IR::Opcode::CompositeExtractU32x2 ||
            hi->GetOpcode() != IR::Opcode::CompositeExtractU32x2 || lo->Arg(0) != hi->Arg(0) ||
            lo->Arg(1) != IR::Value{0U} || hi->Arg(1) != IR::Value{1U}) {
            return NodeUnknown;
        }
        const IR::Inst* const unpack = lo->Arg(0).TryInst();
        if (!unpack || unpack->GetOpcode() != IR::Opcode::UnpackUint2x32) {
            return NodeUnknown;
        }
        return Lanes(unpack->Arg(0), depth);
    }

    ResourceGuards& table;
    std::array<bool, ResourceGuards::MaxNodes> has_leaf{};
    std::unordered_map<const IR::Inst*, std::array<u8, NumViews>> memo;
};

class GuardCollector {
public:
    explicit GuardCollector(GuardBuilder& builder_) : builder{builder_} {}

    u16 BlockTerm(IR::Block* block) {
        boost::container::small_vector<IR::Block*, 32> chain;
        u16 term{};
        for (IR::Block* current = block; current;) {
            if (const auto it = terms.find(current); it != terms.end()) {
                term = it->second;
                break;
            }
            chain.push_back(current);
            IR::Block* const idom = current->immediate_dominator;
            current = idom != current ? idom : nullptr;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            term |= EdgeTerm(*it);
            terms.emplace(*it, term);
        }
        return term;
    }

private:
    u16 EdgeTerm(IR::Block* block) {
        const IR::Block* const idom = block->immediate_dominator;
        if (!idom || idom == block || block->ImmPredecessors().size() != 1) {
            return 0;
        }
        const IR::Block* const pred = block->ImmPredecessors().front();
        const Gcn::Block* const cfg = pred->cfg_block;
        if (pred->branch_cond.IsEmpty() || pred->ImmSuccessors().size() != 2 || !cfg ||
            !cfg->branch_true || !cfg->branch_false) {
            return 0;
        }
        const IR::Block* const on_true = cfg->branch_true->ir_block;
        const IR::Block* const on_false = cfg->branch_false->ir_block;
        if (on_true == on_false || (block != on_true && block != on_false)) {
            return 0;
        }
        return builder.Condition(pred->branch_cond, block == on_true);
    }

    GuardBuilder& builder;
    std::unordered_map<const IR::Block*, u16> terms;
};

template <typename T>
class GuardGroups {
public:
    u32 AddUse(const SharpFetch<T>& key, u16 term) {
        const auto it = std::ranges::find(entries, key, &Entry::key);
        const u32 index = static_cast<u32>(std::distance(entries.begin(), it));
        if (it == entries.end()) {
            entries.push_back({.key = key});
        }
        const bool is_guardable =
            key.summary != SharpFetch<T>::Summary::Invalid && key.load_mask != 0;
        AddTerm(entries[index], is_guardable ? term : u16{0});
        return index;
    }

    void AddTermToAll(u16 term) {
        for (Entry& entry : entries) {
            AddTerm(entry, term);
        }
    }

    void Emit(ResourceGuards& table) {
        for (Entry& entry : entries) {
            if (entry.is_live || entry.num_terms == 0 ||
                table.num_groups == ResourceGuards::MaxGroups) {
                continue;
            }
            entry.group = table.num_groups;
            auto& group = table.groups[table.num_groups++];
            group.terms = entry.terms;
            group.num_terms = entry.num_terms;
        }
    }

    u8 GroupOf(u32 index) const {
        return index < entries.size() ? entries[index].group : NO_GUARD;
    }

private:
    struct Entry {
        SharpFetch<T> key{};
        std::array<u16, ResourceGuards::MaxTerms> terms{};
        u8 num_terms{};
        u8 group{NO_GUARD};
        bool is_live{};
    };

    static void AddTerm(Entry& entry, u16 term) {
        if (entry.is_live) {
            return;
        }
        if (term == 0) {
            entry.is_live = true;
            return;
        }
        for (u32 i = 0; i < entry.num_terms; ++i) {
            if ((entry.terms[i] & term) == entry.terms[i]) {
                return;
            }
        }
        u32 kept{};
        for (u32 i = 0; i < entry.num_terms; ++i) {
            if ((entry.terms[i] & term) != term) {
                entry.terms[kept++] = entry.terms[i];
            }
        }
        entry.num_terms = static_cast<u8>(kept);
        if (entry.num_terms < ResourceGuards::MaxTerms) {
            entry.terms[entry.num_terms++] = term;
            return;
        }
        u32 closest{};
        for (u32 i = 1; i < entry.num_terms; ++i) {
            if (std::popcount(static_cast<u32>(entry.terms[i] & term)) >
                std::popcount(static_cast<u32>(entry.terms[closest] & term))) {
                closest = i;
            }
        }
        entry.terms[closest] &= term;
        entry.is_live = entry.terms[closest] == 0;
    }

    boost::container::small_vector<Entry, 16> entries;
};

} // Anonymous namespace

void ResourceGuardPass(IR::Program& program, ResourceDiscoveryList& resources) {
    Info& info = program.info;
    info.resource_guards = {};
    if (!info.skip_resource_guards) {
        IR::ComputeDominators(program.post_order_blocks);
        GuardBuilder builder{info.resource_guards};
        GuardCollector collector{builder};
        GuardGroups<AmdGpu::Image> images;
        GuardGroups<AmdGpu::Sampler> samplers;
        constexpr u32 NoUse = std::numeric_limits<u32>::max();
        boost::container::small_vector<std::array<u32, 2>, 32> uses(resources.size(),
                                                                    {NoUse, NoUse});
        boost::container::small_vector<u16, 4> lod_terms;
        for (size_t i = 0; i < resources.size(); ++i) {
            const IR::Inst& inst = *resources[i].user;
            if (!IsImageInstruction(inst)) {
                continue;
            }
            const u16 term = collector.BlockTerm(inst.GetParent());
            const bool is_storage =
                inst.GetOpcode() == IR::Opcode::ImageWrite || IsImageAtomicInstruction(inst);
            uses[i][0] =
                images.AddUse(ConstructSharpFetch<AmdGpu::Image>(resources[i].sharps[0], false),
                              is_storage ? u16{0} : term);
            if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
                uses[i][1] = samplers.AddUse(
                    ConstructSharpFetch<AmdGpu::Sampler>(resources[i].sharps[1], false), term);
            } else if (inst.GetOpcode() == IR::Opcode::ImageQueryLod) {
                lod_terms.push_back(term);
            }
        }
        for (const u16 term : lod_terms) {
            samplers.AddTermToAll(term);
        }
        images.Emit(info.resource_guards);
        samplers.Emit(info.resource_guards);
        for (size_t i = 0; i < resources.size(); ++i) {
            resources[i].guards = {images.GroupOf(uses[i][0]), samplers.GroupOf(uses[i][1])};
        }
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
namespace {

enum class GuardKind : u8 {
    Unknown,
    Bool,
    Word,
    Float,
};

struct GuardValue {
    GuardKind kind{};
    bool float_op{};
    u32 bits{};
};

GuardValue BoolValue(bool value) {
    return {.kind = GuardKind::Bool, .bits = value ? 1U : 0U};
}

GuardValue WordValue(u32 value) {
    return {.kind = GuardKind::Word, .bits = value};
}

GuardValue FloatValue(u32 bits, bool float_op) {
    return {.kind = GuardKind::Float, .float_op = float_op, .bits = bits};
}

bool IsNormalOrZero(const GuardValue& value) {
    if (value.kind != GuardKind::Float) {
        return false;
    }
    const u32 exponent = (value.bits >> 23) & 0xFF;
    return exponent != 0xFF && (exponent != 0 || (value.bits & 0x7FFFFF) == 0);
}

double AsDouble(const GuardValue& value) {
    return static_cast<double>(std::bit_cast<float>(value.bits));
}

GuardValue FloatResult(double result) {
    if (!(std::abs(result) <= static_cast<double>(std::numeric_limits<float>::max()))) {
        return {};
    }
    const float rounded = static_cast<float>(result);
    if (static_cast<double>(rounded) != result) {
        return {};
    }
    const GuardValue value = FloatValue(std::bit_cast<u32>(rounded), true);
    return IsNormalOrZero(value) ? value : GuardValue{};
}

GuardValue FloatSum(double x, double y) {
    const double sum = x + y;
    const double y_part = sum - x;
    const double error = (x - (sum - y_part)) + (y - y_part);
    return error == 0.0 ? FloatResult(sum) : GuardValue{};
}

GuardValue Extract(u32 base, u32 offset, u32 count) {
    if (offset > 32 || count > 32 || offset + count > 32) {
        return {};
    }
    if (count == 0) {
        return WordValue(0);
    }
    const u32 shifted = base >> offset;
    return WordValue(count == 32 ? shifted : shifted & ((1U << count) - 1));
}

GuardValue WordOp(GuardOp op, u32 x, u32 y) {
    switch (op) {
    case GuardOp::BitAnd:
        return WordValue(x & y);
    case GuardOp::BitOr:
        return WordValue(x | y);
    case GuardOp::BitXor:
        return WordValue(x ^ y);
    case GuardOp::Shr:
        return y < 32 ? WordValue(x >> y) : GuardValue{};
    case GuardOp::IEq:
        return BoolValue(x == y);
    case GuardOp::INe:
        return BoolValue(x != y);
    case GuardOp::SLt:
        return BoolValue(static_cast<s32>(x) < static_cast<s32>(y));
    case GuardOp::SLe:
        return BoolValue(static_cast<s32>(x) <= static_cast<s32>(y));
    case GuardOp::ULt:
        return BoolValue(x < y);
    case GuardOp::ULe:
        return BoolValue(x <= y);
    default:
        return {};
    }
}

GuardValue FloatOp(GuardOp op, const GuardValue& a, const GuardValue& b, const GuardValue& c,
                   bool nearest) {
    switch (op) {
    case GuardOp::FNeg:
        return FloatValue(a.bits ^ 0x80000000U, a.float_op);
    case GuardOp::FAbs:
        return FloatValue(a.bits & 0x7FFFFFFFU, true);
    case GuardOp::FAdd:
        return nearest ? FloatSum(AsDouble(a), AsDouble(b)) : GuardValue{};
    case GuardOp::FSub:
        return nearest ? FloatSum(AsDouble(a), -AsDouble(b)) : GuardValue{};
    case GuardOp::FMin:
        return FloatValue(AsDouble(a) < AsDouble(b) ? a.bits : b.bits, true);
    case GuardOp::FMax:
        return FloatValue(AsDouble(a) > AsDouble(b) ? a.bits : b.bits, true);
    case GuardOp::FMed3: {
        const double x = AsDouble(a);
        const double y = AsDouble(b);
        const double z = AsDouble(c);
        const double median = std::max(std::min(x, y), std::min(std::max(x, y), z));
        return FloatValue(median == x ? a.bits : (median == y ? b.bits : c.bits), true);
    }
    case GuardOp::FClamp: {
        const double x = AsDouble(a);
        const double lo = AsDouble(b);
        const double hi = AsDouble(c);
        if (lo > hi) {
            return {};
        }
        return FloatValue(x < lo ? b.bits : (x > hi ? c.bits : a.bits), true);
    }
    case GuardOp::FEq:
        return BoolValue(AsDouble(a) == AsDouble(b));
    case GuardOp::FNe:
        return BoolValue(AsDouble(a) != AsDouble(b));
    case GuardOp::FLt:
        return BoolValue(AsDouble(a) < AsDouble(b));
    case GuardOp::FLe:
        return BoolValue(AsDouble(a) <= AsDouble(b));
    case GuardOp::FIsNanOrInf:
        return BoolValue(false);
    default:
        return {};
    }
}

u32 FloatOperandCount(GuardOp op) {
    switch (op) {
    case GuardOp::FNeg:
    case GuardOp::FAbs:
    case GuardOp::FIsNanOrInf:
        return 1;
    case GuardOp::FAdd:
    case GuardOp::FSub:
    case GuardOp::FMin:
    case GuardOp::FMax:
    case GuardOp::FEq:
    case GuardOp::FNe:
    case GuardOp::FLt:
    case GuardOp::FLe:
        return 2;
    case GuardOp::FMed3:
    case GuardOp::FClamp:
        return 3;
    default:
        return 0;
    }
}

GuardValue EvaluateNode(const GuardNode& node, std::span<const GuardValue> values,
                        std::span<const u32> flatbuf, bool nearest) {
    const GuardValue& a = values[node.a];
    const GuardValue& b = values[node.b];
    const GuardValue& c = values[node.c];
    const auto is_bool = [](const GuardValue& value, bool expected) {
        return value.kind == GuardKind::Bool && (value.bits != 0) == expected;
    };
    const auto are_words = [](std::initializer_list<const GuardValue*> operands) {
        return std::ranges::all_of(
            operands, [](const GuardValue* operand) { return operand->kind == GuardKind::Word; });
    };
    switch (node.op) {
    case GuardOp::False:
        return BoolValue(false);
    case GuardOp::True:
        return BoolValue(true);
    case GuardOp::Flatbuf:
        return node.imm < flatbuf.size() ? WordValue(flatbuf[node.imm]) : GuardValue{};
    case GuardOp::ImmU32:
        return WordValue(node.imm);
    case GuardOp::ImmF32:
        return FloatValue(node.imm, false);
    case GuardOp::AsU32:
        if (a.kind != GuardKind::Float || (a.float_op && (a.bits & 0x7FFFFFFFU) == 0)) {
            return {};
        }
        return WordValue(a.bits);
    case GuardOp::AsF32:
        return a.kind == GuardKind::Word ? FloatValue(a.bits, false) : GuardValue{};
    case GuardOp::Not:
        return a.kind == GuardKind::Bool ? BoolValue(a.bits == 0) : GuardValue{};
    case GuardOp::And:
        if (is_bool(a, false) || is_bool(b, false)) {
            return BoolValue(false);
        }
        return is_bool(a, true) && is_bool(b, true) ? BoolValue(true) : GuardValue{};
    case GuardOp::Or:
        if (is_bool(a, true) || is_bool(b, true)) {
            return BoolValue(true);
        }
        return is_bool(a, false) && is_bool(b, false) ? BoolValue(false) : GuardValue{};
    case GuardOp::Xor:
        if (a.kind != GuardKind::Bool || b.kind != GuardKind::Bool) {
            return {};
        }
        return BoolValue(a.bits != b.bits);
    case GuardOp::Select:
        if (a.kind == GuardKind::Bool) {
            return a.bits != 0 ? b : c;
        }
        if (b.kind != GuardKind::Unknown && b.kind == c.kind && b.bits == c.bits) {
            return {.kind = b.kind, .float_op = b.float_op || c.float_op, .bits = b.bits};
        }
        return {};
    case GuardOp::UExtract:
        return are_words({&a, &b, &c}) ? Extract(a.bits, b.bits, c.bits) : GuardValue{};
    default:
        break;
    }
    if (const u32 count = FloatOperandCount(node.op); count != 0) {
        const std::array operands{&a, &b, &c};
        for (u32 i = 0; i < count; ++i) {
            if (!IsNormalOrZero(*operands[i])) {
                return {};
            }
        }
        return FloatOp(node.op, a, b, c, nearest);
    }
    return are_words({&a, &b}) ? WordOp(node.op, a.bits, b.bits) : GuardValue{};
}

} // Anonymous namespace

u32 ResourceGuards::EvaluateDead(std::span<const u32> flatbuf) const {
    if (num_groups == 0) {
        return 0;
    }
    std::array<GuardValue, MaxNodes> values{};
    const bool nearest = std::fegetround() == FE_TONEAREST;
    for (u32 i = 0; i < num_nodes; ++i) {
        values[i] = EvaluateNode(nodes[i], values, flatbuf, nearest);
    }
    u32 false_conds{};
    for (u32 i = 0; i < num_conds; ++i) {
        const GuardValue& value = values[conds[i]];
        if (value.kind == GuardKind::Bool && value.bits == 0) {
            false_conds |= 1U << i;
        }
    }
    if (false_conds == 0) {
        return 0;
    }
    u32 dead{};
    for (u32 i = 0; i < num_groups; ++i) {
        const Group& group = groups[i];
        bool is_dead = group.num_terms != 0;
        for (u32 t = 0; is_dead && t < group.num_terms; ++t) {
            is_dead = (group.terms[t] & false_conds) != 0;
        }
        dead |= static_cast<u32>(is_dead) << i;
    }
    return dead;
}

} // namespace Shader
