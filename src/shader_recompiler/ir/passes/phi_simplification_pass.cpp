// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The Phi simplfication pass works as follows:
// - Gather all Phi instructions in program and apply Tarjan's algorithm to find SCCs.
// - Queue all SCCs in a worklist and process them in order.
//   If all external args have the same unary op move that operation to users.
// - The above transform can make can allow other SCCs to get folded so queue other SCCs,
//   that use the def of at least 1 Phi of the processed SCC to worklist.
// - Repeat until the worklist is empty
// - Compute "structural hash" of each SCC and group them into buckets.
//   Essentially two SCCs are in same group if they have same number of Phis, in same blocks and
//   internal edges are also same. External args are not checked because its checked later.
// - Queue all non trivial groups in a worklist and attempt to dedup SCCs inside each group.
//   This is accomplished by doing a full structural and SSA def equality check on external args.
// - Deduping an SCC can allow deduplication in other SCC groups so also queue user SCC groups that
//   might be affected

#include <limits>
#include <unordered_map>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/hash.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/ssa.h"

namespace Shader::Optimization {

static bool IsCompositeExtract(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::CompositeExtractU32x2:
    case IR::Opcode::CompositeExtractU32x3:
    case IR::Opcode::CompositeExtractU32x4:
        return true;
    default:
        return false;
    }
}

static bool IsDisallowedOp(IR::Opcode op) {
    return op == IR::Opcode::InverseBallot;
}

using InstList = boost::container::small_vector<IR::Inst*, 4>;

struct CommonOp {
    IR::Opcode op{IR::Opcode::Void};
    u32 extract_comp{};
};

struct PhiScc {
    bool queued{};
    bool dead{};
    u64 hash{};
    InstList phis;

    bool HasCommonOp(CommonOp& out) const noexcept {
        out = {};
        for (const IR::Inst* phi : phis) {
            for (size_t i = 0; i < phi->NumArgs(); ++i) {
                if (phi->Arg(i).IsImmediate()) {
                    return false;
                }
                IR::Inst* arg = phi->Arg(i).Inst();
                if (arg->GetOpcode() == IR::Opcode::Phi) {
                    if (arg->scc_index == phi->scc_index) {
                        continue;
                    }
                    return false;
                }
                if (out.op == IR::Opcode::Void) {
                    out.op = arg->GetOpcode();
                    if (IsCompositeExtract(out.op)) {
                        out.extract_comp = arg->Arg(1).U32();
                    } else if (arg->NumArgs() != 1 || IsDisallowedOp(out.op)) {
                        return false;
                    }
                } else if (arg->GetOpcode() != out.op) {
                    return false;
                } else if (IsCompositeExtract(out.op) && arg->Arg(1).U32() != out.extract_comp) {
                    return false;
                }
            }
        }
        ASSERT(out.op != IR::Opcode::Void);
        return true;
    }

    InstList SccUses() const noexcept {
        InstList uses;
        for (const IR::Inst* phi : phis) {
            for (auto [user, operand] : phi->Uses()) {
                if (user->GetOpcode() == IR::Opcode::Phi && user->scc_index != phi->scc_index) {
                    uses.push_back(user);
                }
            }
        }
        return uses;
    }

    void FoldCommonOp(CommonOp& common_op) noexcept {
        const IR::Type new_type{IR::ArgTypeOf(common_op.op, 0)};
        for (IR::Inst* phi : phis) {
            IR::Block* block = phi->GetParent();
            for (size_t i = 0; i < phi->NumArgs(); ++i) {
                IR::Inst* arg = phi->Arg(i).Inst();
                if (arg->GetOpcode() == IR::Opcode::Phi) {
                    continue;
                }
                phi->SetArg(i, arg->Arg(0));
            }
            phi->SetRegTag(IR::EMPTY_REG_TAG);
            phi->SetFlags(new_type);

            IR::Inst* replacement{};
            auto& uses = phi->Uses();
            for (auto it = uses.begin(); it != uses.end();) {
                auto [user, operand] = *it;
                ++it;
                if (user->GetOpcode() == IR::Opcode::Phi && user->scc_index == phi->scc_index) {
                    continue;
                }
                if (!replacement) {
                    const auto insert_point =
                        std::ranges::find_if_not(block->Instructions(), IR::IsPhi);
                    if (IsCompositeExtract(common_op.op)) {
                        replacement = &*block->PrependNewInst(
                            insert_point, common_op.op,
                            {IR::Value{phi}, IR::Value{common_op.extract_comp}});
                    } else {
                        replacement =
                            &*block->PrependNewInst(insert_point, common_op.op, {IR::Value{phi}});
                    }
                }
                user->SetArg(operand, IR::Value{replacement});
            }
        }
    }

    void ComputeStructureHash() noexcept {
        hash = HashCombine(0ull, phis.size());
        for (const IR::Inst* phi : phis) {
            hash = HashCombine(hash, reinterpret_cast<u64>(phi->GetParent()));
            for (size_t i = 0; i < phi->NumArgs(); ++i) {
                hash = HashCombine(hash, phi->Arg(i).IsImmediate());
                if (phi->Arg(i).IsImmediate()) {
                    hash = HashCombine(hash, phi->Arg(i).Hash());
                    continue;
                }
                const IR::Inst* arg = phi->Arg(i).Inst();
                const bool is_scc_arg =
                    arg->GetOpcode() == IR::Opcode::Phi && arg->scc_index == phi->scc_index;
                hash = HashCombine(hash, is_scc_arg);
                if (is_scc_arg) {
                    const auto it = std::ranges::find(phis, arg);
                    hash = HashCombine(hash, std::distance(phis.begin(), it));
                }
            }
        }
    }

    bool IsSameAs(const PhiScc& other) const noexcept {
        if (phis.size() != other.phis.size()) {
            return false;
        }
        for (size_t i = 0; i < phis.size(); ++i) {
            const IR::Inst* phi_a = phis[i];
            const IR::Inst* phi_b = other.phis[i];
            if (phi_a->GetParent() != phi_b->GetParent()) {
                return false;
            }
            for (size_t j = 0; j < phi_a->NumArgs(); ++j) {
                if (phi_a->Arg(j).IsImmediate() != phi_b->Arg(j).IsImmediate()) {
                    return false;
                }
                if (phi_a->Arg(j).IsImmediate()) {
                    if (phi_a->Arg(j).Hash() != phi_b->Arg(j).Hash()) {
                        return false;
                    }
                    continue;
                }
                const IR::Inst* arg_a = phi_a->Arg(j).Inst();
                const IR::Inst* arg_b = phi_b->Arg(j).Inst();
                if (arg_a->GetOpcode() != arg_b->GetOpcode()) {
                    return false;
                }
                if (arg_a->GetOpcode() == IR::Opcode::Phi) {
                    if (arg_a->Flags<IR::Type>() != arg_b->Flags<IR::Type>()) {
                        return false;
                    }
                    const bool is_scc_phi_a = arg_a->scc_index == phi_a->scc_index;
                    const bool is_scc_phi_b = arg_b->scc_index == phi_b->scc_index;
                    if (is_scc_phi_a != is_scc_phi_b) {
                        return false;
                    }
                    if (is_scc_phi_a) {
                        const auto it_a = std::ranges::find(phis, arg_a);
                        const auto it_b = std::ranges::find(other.phis, arg_b);
                        if (std::distance(phis.begin(), it_a) !=
                            std::distance(other.phis.begin(), it_b)) {
                            return false;
                        }
                        continue;
                    }
                }
                if (arg_a != arg_b) {
                    return false;
                }
            }
        }
        return true;
    }
};

struct PhiSccFinder {
    struct NodeInfo {
        u32 index{std::numeric_limits<u32>::max()};
        u32 lowlink{std::numeric_limits<u32>::max()};
        bool on_stack{};
    };
    std::unordered_map<IR::Inst*, NodeInfo> nodes;
    InstList stack;
    std::vector<PhiScc> sccs;
    u32 index_counter{0};

    explicit PhiSccFinder(std::span<IR::Inst*> all_phis) {
        nodes.reserve(all_phis.size());
        for (IR::Inst* phi : all_phis) {
            if (nodes[phi].index == std::numeric_limits<u32>::max()) {
                StrongConnect(phi);
            }
        }
        for (u32 i = 0; i < sccs.size(); i++) {
            for (IR::Inst* phi : sccs[i].phis) {
                phi->scc_index = i;
            }
        }
    }

private:
    void StrongConnect(IR::Inst* root) {
        struct Frame {
            IR::Inst* phi;
            size_t arg_idx;
        };
        boost::container::small_vector<Frame, 8> dfs;

        const auto push_phi = [&](IR::Inst* phi, NodeInfo& info) {
            info.index = index_counter;
            info.lowlink = index_counter++;
            info.on_stack = true;
            stack.push_back(phi);
            dfs.emplace_back(phi, 0);
        };

        push_phi(root, nodes[root]);

        while (!dfs.empty()) {
            auto& [phi, arg_idx] = dfs.back();
            auto& info = nodes[phi];
            bool pushed_child = false;

            for (; arg_idx < phi->NumArgs(); arg_idx++) {
                if (phi->Arg(arg_idx).IsImmediate()) {
                    continue;
                }
                IR::Inst* arg = phi->Arg(arg_idx).Inst();
                if (arg->GetOpcode() != IR::Opcode::Phi) {
                    continue;
                }

                auto& arg_info = nodes[arg];
                if (arg_info.index == std::numeric_limits<u32>::max()) {
                    arg_idx++;
                    push_phi(arg, arg_info);
                    pushed_child = true;
                    break;
                } else if (arg_info.on_stack) {
                    info.lowlink = std::min(info.lowlink, arg_info.index);
                }
            }
            if (pushed_child) {
                continue;
            }

            if (info.lowlink == info.index) {
                auto& scc = sccs.emplace_back();
                IR::Inst* w;
                do {
                    w = stack.back();
                    stack.pop_back();
                    nodes[w].on_stack = false;
                    scc.phis.push_back(w);
                } while (w != phi);
            }

            dfs.pop_back();
            if (!dfs.empty()) {
                auto& parent_info = nodes[dfs.back().phi];
                parent_info.lowlink = std::min(parent_info.lowlink, info.lowlink);
            }
        }
    }
};

static void FoldSccCommonOps(std::vector<PhiScc>& sccs) {
    std::vector<u32> worklist;
    worklist.reserve(sccs.size());
    for (u32 i = 0; i < sccs.size(); i++) {
        worklist.push_back(i);
        sccs[i].queued = true;
    }

    while (!worklist.empty()) {
        const u32 scc_idx = worklist.back();
        worklist.pop_back();

        auto& scc = sccs[scc_idx];
        scc.queued = false;

        CommonOp op;
        if (!scc.HasCommonOp(op)) {
            continue;
        }
        auto scc_uses = scc.SccUses();
        do {
            scc.FoldCommonOp(op);
        } while (scc.HasCommonOp(op));

        for (IR::Inst* const user : scc_uses) {
            if (!std::exchange(sccs[user->scc_index].queued, true)) {
                worklist.push_back(user->scc_index);
            }
        }
    }
}

static void DeduplicateSccs(std::vector<PhiScc>& sccs) {
    struct SccGroup {
        boost::container::small_vector<u32, 4> sccs;
        bool queued{};
    };
    std::unordered_map<u64, SccGroup, std::identity> scc_groups;
    for (size_t scc_idx = 0; scc_idx < sccs.size(); ++scc_idx) {
        auto& scc = sccs[scc_idx];
        scc.ComputeStructureHash();
        scc_groups[scc.hash].sccs.emplace_back(scc_idx);
    }

    std::vector<SccGroup*> worklist;
    worklist.reserve(scc_groups.size());
    for (auto it = scc_groups.begin(); it != scc_groups.end(); ++it) {
        auto& group = it->second;
        if (group.sccs.size() > 1) {
            worklist.push_back(&group);
            group.queued = true;
        }
    }

    while (!worklist.empty()) {
        auto* group = worklist.back();
        worklist.pop_back();
        group->queued = false;

        for (size_t i = 0; i < group->sccs.size(); ++i) {
            auto& scc = sccs[group->sccs[i]];
            if (scc.dead) {
                continue;
            }
            for (size_t j = i + 1; j < group->sccs.size(); ++j) {
                const u32 remove_idx = group->sccs[j];
                auto& remove = sccs[remove_idx];
                if (remove.dead) {
                    continue;
                }
                if (!scc.IsSameAs(remove)) {
                    continue;
                }
                for (size_t k = 0; k < remove.phis.size(); k++) {
                    IR::Inst* old_phi = remove.phis[k];
                    IR::Inst* new_phi = scc.phis[k];
                    for (auto it = old_phi->Uses().begin(); it != old_phi->Uses().end();) {
                        auto [user, operand] = *it;
                        ++it;
                        if (user->GetOpcode() == IR::Opcode::Phi) {
                            if (user->scc_index == old_phi->scc_index) {
                                continue;
                            }
                            auto& user_scc = sccs[user->scc_index];
                            auto& user_group = scc_groups[user_scc.hash];
                            if (!std::exchange(user_group.queued, true)) {
                                worklist.push_back(&user_group);
                            }
                        }
                        user->SetArg(operand, IR::Value{new_phi});
                    }
                }
                remove.dead = true;
            }
        }
    }
}

void PhiSimplificationPass(IR::Program& program) {
    std::vector<IR::Inst*> all_phis;
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::Phi) {
                break;
            }
            all_phis.push_back(&inst);
        }
    }

    PhiSccFinder finder{all_phis};
    auto& sccs = finder.sccs;
    FoldSccCommonOps(sccs);
    DeduplicateSccs(sccs);
}

} // namespace Shader::Optimization
