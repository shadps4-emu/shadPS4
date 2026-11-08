// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// GCN code reduces a value across the wave by enabling every lane (exec = ~exec | exec), feeding
// the reduction identity to the previously inactive lanes, combining lanes with xor swizzles and
// reading the partial results back with v_readlane. In SPIR-V, invocations that already left the
// enclosing control flow cannot be re-enabled, so those cross-lane reads return undefined values.
// This pass recognizes such reductions and replaces them with a subgroup reduction over the
// invocations that are active, which is equivalent because the inactive lanes only contribute the
// identity.

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/logging/log.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

namespace {

constexpr u32 WaveSize = 64;

struct Reduction {
    IR::Opcode group_op;
    u32 identity;
    bool idempotent;
};

std::optional<Reduction> GetReduction(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::IAdd32:
        return Reduction{IR::Opcode::GroupIAdd32, 0u, false};
    case IR::Opcode::SMin32:
        return Reduction{IR::Opcode::GroupSMin32, 0x7fffffffu, true};
    case IR::Opcode::UMin32:
        return Reduction{IR::Opcode::GroupUMin32, 0xffffffffu, true};
    case IR::Opcode::SMax32:
        return Reduction{IR::Opcode::GroupSMax32, 0x80000000u, true};
    case IR::Opcode::UMax32:
        return Reduction{IR::Opcode::GroupUMax32, 0u, true};
    case IR::Opcode::BitwiseAnd32:
        return Reduction{IR::Opcode::GroupBitwiseAnd32, 0xffffffffu, true};
    case IR::Opcode::BitwiseOr32:
        return Reduction{IR::Opcode::GroupBitwiseOr32, 0u, true};
    case IR::Opcode::BitwiseXor32:
        return Reduction{IR::Opcode::GroupBitwiseXor32, 0u, false};
    default:
        return std::nullopt;
    }
}

IR::Inst* Producer(const IR::Value& value, IR::Opcode opcode) {
    if (value.IsImmediate()) {
        return nullptr;
    }
    IR::Inst* const inst = value.Inst();
    return inst->GetOpcode() == opcode ? inst : nullptr;
}

bool IsImmediateBits(const IR::Value& value, u32 bits) {
    if (!value.IsImmediate()) {
        return false;
    }
    switch (value.Type()) {
    case IR::Type::U32:
        return value.U32() == bits;
    case IR::Type::F32:
        return std::bit_cast<u32>(value.F32()) == bits;
    default:
        return false;
    }
}

bool IsAllLanesImmediate(const IR::Value& exec) {
    if (exec.IsImmediate()) {
        return exec.Type() == IR::Type::U1 && exec.U1();
    }
    const IR::Inst* const inverse = Producer(exec, IR::Opcode::InverseBallot);
    return inverse && inverse->Arg(0).IsImmediate() &&
           inverse->Arg(0).U64() == std::numeric_limits<u64>::max();
}

// Returns the exec mask that was active before an exec write which enables every lane.
std::optional<IR::Value> MatchWholeWaveExec(const IR::Value& exec,
                                            const std::optional<IR::Value>& prev_exec) {
    if (IsAllLanesImmediate(exec)) {
        return prev_exec;
    }
    const IR::Inst* const inverse = Producer(exec, IR::Opcode::InverseBallot);
    if (!inverse) {
        return std::nullopt;
    }
    const IR::Inst* const mask = Producer(inverse->Arg(0), IR::Opcode::BitwiseOr64);
    if (!mask) {
        return std::nullopt;
    }
    for (u32 i = 0; i < 2; ++i) {
        const IR::Inst* const inverted = Producer(mask->Arg(i), IR::Opcode::BitwiseNot64);
        const IR::Inst* const ballot = Producer(mask->Arg(1 - i), IR::Opcode::Ballot);
        if (!inverted || !ballot) {
            continue;
        }
        const IR::Inst* const inverted_ballot = Producer(inverted->Arg(0), IR::Opcode::Ballot);
        if (inverted_ballot && inverted_ballot->Arg(0) == ballot->Arg(0)) {
            return ballot->Arg(0);
        }
    }
    return std::nullopt;
}

struct WholeWaveRegions {
    std::unordered_map<const IR::Inst*, u32> region_of;
    std::vector<IR::Value> saved_exec;
};

WholeWaveRegions FindWholeWaveRegions(IR::Program& program) {
    WholeWaveRegions regions;
    for (IR::Block* const block : program.blocks) {
        std::optional<IR::Value> exec;
        std::optional<u32> region;
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::SetExec) {
                const IR::Value new_exec = inst.Arg(0);
                region.reset();
                if (const auto saved = MatchWholeWaveExec(new_exec, exec)) {
                    region = static_cast<u32>(regions.saved_exec.size());
                    regions.saved_exec.push_back(*saved);
                }
                exec = new_exec;
                continue;
            }
            if (region) {
                regions.region_of.emplace(&inst, *region);
            }
        }
    }
    return regions;
}

class SubgroupMask {
public:
    bool Add(u32 mask) {
        if (mask >= WaveSize) {
            return false;
        }
        const bool independent = (offsets & (u64{1} << mask)) == 0;
        u64 extended = offsets;
        for (u32 offset = 0; offset < WaveSize; ++offset) {
            if (offsets & (u64{1} << offset)) {
                extended |= u64{1} << (offset ^ mask);
            }
        }
        offsets = extended;
        return independent;
    }

    bool Contains(u32 offset) const {
        return (offsets & (u64{1} << offset)) != 0;
    }

private:
    u64 offsets{1};
};

bool CollectReadLanes(IR::Inst* inst, IR::Opcode op, IR::Value& source,
                      boost::container::small_vector<IR::Inst*, 4>& read_lanes) {
    for (u32 i = 0; i < 2; ++i) {
        const IR::Value arg = inst->Arg(i);
        if (arg.IsImmediate()) {
            return false;
        }
        IR::Inst* const arg_inst = arg.Inst();
        if (arg_inst->GetOpcode() == op) {
            if (!CollectReadLanes(arg_inst, op, source, read_lanes)) {
                return false;
            }
            continue;
        }
        if (arg_inst->GetOpcode() != IR::Opcode::ReadLane || !arg_inst->Arg(1).IsImmediate() ||
            arg_inst->Arg(1).U32() >= WaveSize) {
            return false;
        }
        if (source.IsEmpty()) {
            source = arg_inst->Arg(0);
        } else if (source != arg_inst->Arg(0)) {
            return false;
        }
        read_lanes.push_back(arg_inst);
    }
    return true;
}

bool TryLowerReduction(IR::Inst& root, const Reduction& reduction,
                       const WholeWaveRegions& regions) {
    const IR::Opcode op = root.GetOpcode();

    IR::Value source{};
    boost::container::small_vector<IR::Inst*, 4> read_lanes;
    if (!CollectReadLanes(&root, op, source, read_lanes)) {
        return false;
    }

    const auto region_it = regions.region_of.find(read_lanes.front());
    if (region_it == regions.region_of.end()) {
        return false;
    }
    const u32 region = region_it->second;
    const auto in_region = [&](const IR::Inst* inst) {
        const auto it = regions.region_of.find(inst);
        return it != regions.region_of.end() && it->second == region;
    };
    if (!std::ranges::all_of(read_lanes, in_region)) {
        return false;
    }

    SubgroupMask butterfly;
    IR::Value value = source;
    while (IR::Inst* const step = Producer(value, op)) {
        IR::Value next{};
        for (u32 i = 0; i < 2; ++i) {
            IR::Inst* const shuffle = Producer(step->Arg(i), IR::Opcode::ShuffleXor);
            if (shuffle && shuffle->Arg(0) == step->Arg(1 - i) && shuffle->Arg(1).IsImmediate() &&
                in_region(shuffle)) {
                if (!butterfly.Add(shuffle->Arg(1).U32()) && !reduction.idempotent) {
                    return false;
                }
                next = step->Arg(1 - i);
                break;
            }
        }
        if (next.IsEmpty()) {
            break;
        }
        value = next;
    }
    if (value.IsImmediate()) {
        return false;
    }

    IR::Inst* const input = value.Inst();
    IR::Value select_value = value;
    if (input->GetOpcode() == IR::Opcode::BitCastU32F32) {
        select_value = input->Arg(0);
    }
    IR::Inst* const select = select_value.IsImmediate() ? nullptr : select_value.Inst();
    if (!select ||
        (select->GetOpcode() != IR::Opcode::SelectU32 &&
         select->GetOpcode() != IR::Opcode::SelectF32) ||
        !IsImmediateBits(select->Arg(2), reduction.identity) || !in_region(select) ||
        select->Arg(0) != regions.saved_exec[region]) {
        return false;
    }

    std::array<u32, WaveSize> coverage{};
    for (const IR::Inst* const read_lane : read_lanes) {
        const u32 lane = read_lane->Arg(1).U32();
        for (u32 offset = 0; offset < WaveSize; ++offset) {
            if (butterfly.Contains(offset)) {
                ++coverage[lane ^ offset];
            }
        }
    }
    const bool covers_wave = std::ranges::all_of(
        coverage, [&](u32 count) { return reduction.idempotent ? count >= 1 : count == 1; });
    if (!covers_wave) {
        return false;
    }

    IR::Block* const block = input->GetParent();
    const auto insertion_point = ++IR::Block::InstructionList::s_iterator_to(*input);
    const auto result = block->PrependNewInst(insertion_point, reduction.group_op, {value});
    LOG_INFO(Render_Recompiler, "Lowering whole wave {} reduction to {}", op, reduction.group_op);
    root.ReplaceUsesWithAndRemove(IR::Value{&*result});
    return true;
}

} // Anonymous namespace

void WaveReductionPass(IR::Program& program, const Profile& profile) {
    if (profile.subgroup_size != WaveSize ||
        !(profile.subgroup_arithmetic_stages & (1u << static_cast<u32>(program.info.hw_stage)))) {
        return;
    }
    const WholeWaveRegions regions = FindWholeWaveRegions(program);
    if (regions.saved_exec.empty()) {
        return;
    }
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (const auto reduction = GetReduction(inst.GetOpcode())) {
                TryLowerReduction(inst, *reduction, regions);
            }
        }
    }
}

} // namespace Shader::Optimization
