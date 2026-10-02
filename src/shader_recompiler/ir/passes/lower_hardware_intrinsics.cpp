// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <span>
#include <vector>
#include <boost/container/small_vector.hpp>
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static void LowerCmpClass(IR::Block& block, IR::Inst& inst) {
    ASSERT_MSG(inst.Arg(1).IsImmediate(), "Unable to resolve compare operation");
    const auto class_mask = static_cast<IR::FloatClassFunc>(inst.Arg(1).U32());
    if ((class_mask & IR::FloatClassFunc::NaN) == IR::FloatClassFunc::NaN) {
        inst.ReplaceOpcode(IR::Opcode::FPIsNan32);
    } else if ((class_mask & IR::FloatClassFunc::Infinity) == IR::FloatClassFunc::Infinity) {
        inst.ReplaceOpcode(IR::Opcode::FPIsInf32);
    } else if ((class_mask & IR::FloatClassFunc::Finite) == IR::FloatClassFunc::Finite) {
        IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
        const IR::F32 value = IR::F32{inst.Arg(0)};
        inst.ReplaceUsesWithAndRemove(
            ir.LogicalNot(ir.LogicalOr(ir.FPIsNan(value), ir.FPIsInf(value))));
    } else {
        UNREACHABLE();
    }
}

static void LowerPackedAncillary(IR::Inst& value) {
    boost::container::small_vector<IR::Inst*, 4> extracts;
    for (const auto& [user, operand] : value.Uses()) {
        if (user->GetOpcode() == IR::Opcode::BitFieldUExtract && operand == 0 &&
            user->Arg(1).IsImmediate() && user->Arg(2).IsImmediate()) {
            extracts.push_back(user);
        }
    }
    for (IR::Inst* const inst : extracts) {
        const u32 offset = inst->Arg(1).U32();
        const u32 bits = inst->Arg(2).U32();
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        if (offset >= 8 && offset + bits <= 12) {
            const auto sample_index = ir.GetAttributeU32(IR::Attribute::SampleIndex);
            if (offset == 8 && bits == 4) {
                inst->ReplaceUsesWithAndRemove(sample_index);
            } else {
                inst->ReplaceUsesWithAndRemove(
                    ir.BitFieldExtract(sample_index, ir.Imm32(offset - 8), ir.Imm32(bits)));
            }
        } else if (offset >= 16 && offset + bits <= 27) {
            const auto mrt_index = ir.GetAttributeU32(IR::Attribute::RenderTargetIndex);
            if (offset == 16 && bits == 11) {
                inst->ReplaceUsesWithAndRemove(mrt_index);
            } else {
                inst->ReplaceUsesWithAndRemove(
                    ir.BitFieldExtract(mrt_index, ir.Imm32(offset - 16), ir.Imm32(bits)));
            }
        } else {
            UNREACHABLE_MSG("Unhandled bitfield extract from ancillary VGPR offset={}, bits={}",
                            offset, bits);
        }
    }
    value.ReplaceUsesWithAndRemove(IR::Value{0U});
}

static void LowerMaskedBitCount(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::U32 thread_mask{
        ir.GetAttributeU32(IR::Attribute::SubgroupLtMask, inst.Arg(2).U1() ? 1 : 0)};
    const IR::U32 masked_value{ir.BitCount(ir.BitwiseAnd(IR::U32{inst.Arg(0)}, thread_mask))};
    inst.ReplaceUsesWithAndRemove(ir.IAdd(masked_value, IR::U32{inst.Arg(1)}));
}

static bool IsLane(const IR::Inst* inst, u32 lane) {
    return inst->GetOpcode() == IR::Opcode::ReadLane && inst->Arg(1).IsImmediate() &&
           inst->Arg(1).U32() == lane;
}

static bool IsAllOnes(const IR::Value& value) {
    if (!value.IsImmediate()) {
        return false;
    }
    if (value.Type() == IR::Type::F32) {
        return std::bit_cast<u32>(value.F32()) == 0xFFFFFFFFu;
    }
    return value.Type() == IR::Type::U32 && value.U32() == 0xFFFFFFFFu;
}

static IR::Value UnwrapCondition(IR::Value value) {
    while (const auto* inst = value.TryInst()) {
        if (inst->GetOpcode() == IR::Opcode::ConditionRef) {
            value = inst->Arg(0);
        } else if (inst->GetOpcode() == IR::Opcode::LogicalNot && inst->Arg(0).TryInst() &&
                   inst->Arg(0).Inst()->GetOpcode() == IR::Opcode::LogicalNot) {
            value = inst->Arg(0).Inst()->Arg(0);
        } else {
            break;
        }
    }
    return value;
}

static IR::Value OtherArg(const IR::Inst& inst, IR::Value value) {
    return inst.Arg(0) == value ? inst.Arg(1) : inst.Arg(1) == value ? inst.Arg(0) : IR::Value{};
}

static IR::Value PhiFrom(const IR::Inst& phi, const IR::Block* block) {
    for (size_t i = 0; i < phi.NumArgs(); ++i) {
        if (phi.PhiBlock(i) == block) {
            return phi.Arg(i);
        }
    }
    return {};
}

static bool ClearsLowestBit(IR::Value value, IR::Value mask, IR::Value scan, IR::Value candidate) {
    const auto* clear = value.TryInst();
    if (clear && clear->GetOpcode() == IR::Opcode::BitwiseOr32) {
        const auto is_zero = [](IR::Value arg) {
            const auto* inst = arg.TryInst();
            return inst && inst->GetOpcode() == IR::Opcode::BitwiseAnd32 &&
                   (inst->Arg(0) == IR::Value{0u} || inst->Arg(1) == IR::Value{0u});
        };
        clear = is_zero(clear->Arg(0))   ? clear->Arg(1).TryInst()
                : is_zero(clear->Arg(1)) ? clear->Arg(0).TryInst()
                                         : nullptr;
    }
    if (!clear || clear->GetOpcode() != IR::Opcode::BitwiseAnd32) {
        return false;
    }
    const auto* inverse = OtherArg(*clear, mask).TryInst();
    const auto* bit = inverse && inverse->GetOpcode() == IR::Opcode::BitwiseNot32
                          ? inverse->Arg(0).TryInst()
                          : nullptr;
    const auto* index =
        bit && bit->GetOpcode() == IR::Opcode::ShiftLeftLogical32 && bit->Arg(0) == IR::Value{1u}
            ? bit->Arg(1).TryInst()
            : nullptr;
    return index && index->GetOpcode() == IR::Opcode::BitwiseAnd32 &&
           (OtherArg(*index, IR::Value{31u}) == scan ||
            OtherArg(*index, IR::Value{31u}) == candidate);
}

static bool IsIndependentBitMaskLoop(const IR::Program& program, IR::Inst& reduction,
                                     const IR::Inst& select,
                                     std::span<const IR::Inst* const> chain) {
    using Type = IR::AbstractSyntaxNode::Type;
    const std::span nodes{program.syntax_list};
    const auto location = std::ranges::find_if(nodes, [&](const auto& node) {
        return node.type == Type::Block && node.data.block == reduction.GetParent();
    });
    const size_t at = std::distance(nodes.begin(), location);
    if (at < 4 || at + 1 >= nodes.size() || nodes[at - 4].type != Type::Block ||
        nodes[at - 3].type != Type::Loop || nodes[at - 2].type != Type::Block ||
        nodes[at - 1].type != Type::Break || nodes[at + 1].type != Type::If) {
        return false;
    }
    const auto& loop = nodes[at - 3].data.loop;
    const auto& branch = nodes[at + 1].data.if_node;
    const auto finish = std::ranges::find_if(nodes.subspan(at + 2), [&](const auto& node) {
        return node.type == Type::EndIf && node.data.end_if.merge == branch.merge;
    });
    const size_t end_if = std::distance(nodes.begin(), finish);
    if (end_if + 3 >= nodes.size() || nodes[end_if + 1].type != Type::Block ||
        nodes[end_if + 1].data.block != branch.merge || nodes[end_if + 2].type != Type::Block ||
        nodes[end_if + 2].data.block != loop.continue_block ||
        nodes[end_if + 3].type != Type::Repeat) {
        return false;
    }
    const auto* header = nodes[at - 4].data.block;
    const auto& repeat = nodes[end_if + 3].data.repeat;
    const auto& stop = nodes[at - 1].data.break_node;
    if (repeat.loop_header != header || repeat.merge != loop.merge ||
        UnwrapCondition(repeat.cond) != IR::Value{true} || nodes[at - 2].data.block != loop.body ||
        stop.merge != loop.merge || stop.skip != reduction.GetParent()) {
        return false;
    }
    const auto body = nodes.subspan(at + 2, end_if - at - 2);
    const auto region = nodes.subspan(at - 2, end_if - at + 5);
    const auto contains = [](auto scope, const IR::Block* block) {
        return std::ranges::any_of(scope, [&](const auto& node) {
            return node.type == Type::Block && node.data.block == block;
        });
    };
    IR::Value candidate = select.Arg(1);
    if (candidate.TryInst() && candidate.Inst()->GetOpcode() == IR::Opcode::BitCastF32U32) {
        candidate = candidate.Inst()->Arg(0);
    }
    const auto* add = candidate.TryInst();
    const auto* scan =
        add && add->GetOpcode() == IR::Opcode::IAdd32 ? add->Arg(0).TryInst() : nullptr;
    const auto* base =
        add && add->GetOpcode() == IR::Opcode::IAdd32 ? add->Arg(1).TryInst() : nullptr;
    if (!scan || scan->GetOpcode() != IR::Opcode::FindILsb32 || !base ||
        base->GetOpcode() != IR::Opcode::IMul32 ||
        (base->Arg(0) != IR::Value{32u} && base->Arg(1) != IR::Value{32u}) ||
        base->GetParent() == header || contains(region, base->GetParent())) {
        return false;
    }
    const IR::Value mask = scan->Arg(0);
    const auto* mask_phi = mask.TryInst();
    const auto* active = select.Arg(0).TryInst();
    const auto* nonzero = active && active->GetOpcode() == IR::Opcode::LogicalAnd
                              ? active->Arg(1).TryInst()
                              : nullptr;
    const auto* exec_phi = active && active->GetOpcode() == IR::Opcode::LogicalAnd
                               ? active->Arg(0).TryInst()
                               : nullptr;
    const auto* stop_condition = UnwrapCondition(stop.cond).TryInst();
    if (!mask_phi || mask_phi->GetOpcode() != IR::Opcode::Phi || mask_phi->GetParent() != header ||
        !nonzero || nonzero->GetOpcode() != IR::Opcode::INotEqual32 ||
        OtherArg(*nonzero, mask) != IR::Value{0u} || !exec_phi ||
        exec_phi->GetOpcode() != IR::Opcode::Phi || exec_phi->GetParent() != header ||
        PhiFrom(*exec_phi, loop.continue_block) != select.Arg(0) || !stop_condition ||
        stop_condition->GetOpcode() != IR::Opcode::LogicalNot ||
        stop_condition->Arg(0) != select.Arg(0)) {
        return false;
    }
    const auto* gate = UnwrapCondition(branch.cond).TryInst();
    const auto* equal = gate && gate->GetOpcode() == IR::Opcode::LogicalAnd
                            ? OtherArg(*gate, select.Arg(0)).TryInst()
                            : nullptr;
    if (!equal || equal->GetOpcode() != IR::Opcode::IEqual32 || equal->UseCount() != 1 ||
        OtherArg(*equal, IR::Value{&reduction}) != candidate) {
        return false;
    }
    std::vector<const IR::Inst*> conditions{gate};
    for (IR::Value condition = branch.cond; condition.TryInst() != gate;) {
        const auto* wrapper = condition.TryInst();
        conditions.push_back(wrapper);
        condition = wrapper->Arg(0);
    }
    for (const auto* condition : conditions) {
        for (const auto& use : condition->Uses()) {
            if (!std::ranges::contains(conditions, use.user) &&
                !contains(body, use.user->GetParent())) {
                return false;
            }
        }
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const IR::Value condition = node.type == Type::If ? IR::Value{node.data.if_node.cond}
                                    : node.type == Type::Break
                                        ? IR::Value{node.data.break_node.cond}
                                    : node.type == Type::Repeat ? IR::Value{node.data.repeat.cond}
                                                                : IR::Value{};
        if (std::ranges::contains(conditions, condition.TryInst()) && (i < at + 1 || i >= end_if)) {
            return false;
        }
    }
    for (const auto& use : reduction.Uses()) {
        if (use.user != equal && !contains(body, use.user->GetParent())) {
            return false;
        }
    }
    for (const auto* inst : chain) {
        if (inst != &reduction && std::ranges::any_of(inst->Uses(), [&](const auto& use) {
                return !std::ranges::contains(chain, use.user);
            })) {
            return false;
        }
    }
    for (const auto& phi : header->Instructions()) {
        if (phi.GetOpcode() != IR::Opcode::Phi || phi.NumArgs() != 2) {
            return false;
        }
        if (&phi == exec_phi) {
            continue;
        }
        const auto* incoming = PhiFrom(phi, loop.continue_block).TryInst();
        if (!incoming || incoming->GetOpcode() != IR::Opcode::Phi || incoming->NumArgs() != 2 ||
            incoming->GetParent() != branch.merge ||
            PhiFrom(*incoming, reduction.GetParent()).TryInst() != &phi) {
            return false;
        }
        for (size_t i = 0; i < incoming->NumArgs(); ++i) {
            if (incoming->PhiBlock(i) != reduction.GetParent() &&
                (!contains(body, incoming->PhiBlock(i)) ||
                 (&phi == mask_phi &&
                  !ClearsLowestBit(incoming->Arg(i), mask, add->Arg(0), candidate)))) {
                return false;
            }
        }
    }
    for (size_t i = 0; i < region.size(); ++i) {
        const auto& node = region[i];
        if ((node.type == Type::Break && i != 1 &&
             ((i + at - 2 <= at + 1 || i + at - 2 >= end_if) ||
              !contains(body, node.data.break_node.merge))) ||
            ((node.type == Type::Loop || node.type == Type::Repeat) &&
             (i + at - 2 <= at + 1 || i + at - 2 >= end_if)) ||
            node.type == Type::Return || node.type == Type::Unreachable) {
            return false;
        }
        if (node.type != Type::Block) {
            continue;
        }
        for (const auto& inst : node.data.block->Instructions()) {
            if (std::ranges::contains(chain, &inst)) {
                continue;
            }
            const auto op = inst.GetOpcode();
            const auto name = IR::NameOf(op);
            if ((inst.MayHaveSideEffects() && op != IR::Opcode::ConditionRef) ||
                name.starts_with("Set") || name.starts_with("Group") ||
                name.starts_with("Shuffle") || name.starts_with("ReadLane") ||
                op == IR::Opcode::ReadFirstLane || name.starts_with("WriteLane") ||
                name.starts_with("Ballot") || name.starts_with("LoadShared") ||
                name.starts_with("MaskedBitCount") || name.starts_with("Quad") ||
                name.starts_with("DPdx") || name.starts_with("DPdy") ||
                name.starts_with("ImageGather") ||
                name.find("ImplicitLod") != std::string_view::npos ||
                op == IR::Opcode::ImageQueryLod || op == IR::Opcode::ImageSampleRaw ||
                op == IR::Opcode::Memtime || op == IR::Opcode::GetExec) {
                return false;
            }
        }
    }
    return true;
}

static void FoldWaveUMin(IR::Program& program, IR::Block& block, IR::Inst& inst) {
    if (inst.Arg(0).IsImmediate() || inst.Arg(1).IsImmediate()) {
        return;
    }
    const IR::Inst* const lane0{inst.Arg(0).Inst()};
    const IR::Inst* const lane32{inst.Arg(1).Inst()};
    const bool in_order{IsLane(lane0, 0) && IsLane(lane32, 32)};
    if (!in_order && !(IsLane(lane0, 32) && IsLane(lane32, 0))) {
        return;
    }
    if (lane0->GetParent() != &block || lane32->GetParent() != &block ||
        lane0->Arg(0).IsImmediate() || lane0->Arg(0) != lane32->Arg(0)) {
        return;
    }
    std::vector<const IR::Inst*> chain{&inst, lane0, lane32};
    IR::Value value{lane0->Arg(0)};
    for (u32 mask = 1; mask <= 16; mask <<= 1) {
        const IR::Inst* const min{value.Inst()};
        if (min->GetOpcode() != IR::Opcode::UMin32 || min->GetParent() != &block) {
            return;
        }
        IR::Value next{};
        for (u32 i = 0; i < 2; ++i) {
            const IR::Value shuffled{min->Arg(i)};
            const IR::Value other{min->Arg(1 - i)};
            if (shuffled.IsImmediate() || other.IsImmediate()) {
                continue;
            }
            const IR::Inst* const shuffle{shuffled.Inst()};
            if (shuffle->GetOpcode() == IR::Opcode::ShuffleXor && shuffle->GetParent() == &block &&
                shuffle->Arg(1).IsImmediate() && shuffle->Arg(1).U32() == mask &&
                shuffle->Arg(0) == other) {
                chain.push_back(min);
                chain.push_back(shuffle);
                next = other;
                break;
            }
        }
        if (next.IsEmpty()) {
            return;
        }
        value = next;
    }
    const IR::Inst* source{value.Inst()};
    if (source->GetOpcode() == IR::Opcode::BitCastU32F32 && !source->Arg(0).IsImmediate()) {
        source = source->Arg(0).Inst();
    }
    if ((source->GetOpcode() != IR::Opcode::SelectU32 &&
         source->GetOpcode() != IR::Opcode::SelectF32) ||
        source->GetParent() != &block || !IsAllOnes(source->Arg(2))) {
        return;
    }
    if (!IsIndependentBitMaskLoop(program, inst, *source, chain)) {
        return;
    }
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    IR::U32 minimum = ir.GroupUMin(IR::U32{value});
    if (program.info.hw_stage == HwStage::Fragment) {
        minimum = IR::U32{
            ir.Select(ir.GetAttributeU1(IR::Attribute::IsHelperInvocation), value, minimum)};
    }
    inst.ReplaceUsesWithAndRemove(minimum);
}

static void Lower(IR::Program& program, IR::Block& block, IR::Inst& inst, bool fold_wave_min) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::FPCmpClass32:
        return LowerCmpClass(block, inst);
    case IR::Opcode::GetAttributeU32:
        if (inst.Arg(0).Attribute() == IR::Attribute::PackedAncillary) {
            LowerPackedAncillary(inst);
        }
        break;
    case IR::Opcode::MaskedBitCount32:
        return LowerMaskedBitCount(block, inst);
    case IR::Opcode::UMin32:
        if (fold_wave_min) {
            FoldWaveUMin(program, block, inst);
        }
        break;
    default:
        break;
    }
}

void LowerHardwareIntrinsics(IR::Program& program, const Profile& profile) {
    const HwStage stage = program.info.hw_stage;
    const bool fold_wave_min = profile.supports_group_arithmetic &&
                               (stage == HwStage::Fragment || stage == HwStage::Compute);
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Lower(program, *block, inst, fold_wave_min);
        }
    }
}

} // namespace Shader::Optimization
