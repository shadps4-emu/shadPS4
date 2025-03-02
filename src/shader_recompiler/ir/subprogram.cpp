// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/flat_set.hpp>
#include "shader_recompiler/ir/conditional_tree.h"
#include "shader_recompiler/ir/subprogram.h"
#include "shader_recompiler/ir/post_order.h"

namespace Shader::IR {

SubProgram::SubProgram(Program* super_program, Pools& pools) : super_program(super_program), pools(pools) {}

Block* SubProgram::AddBlock(Block* orig_block) {
    auto it = orig_block_to_block.find(orig_block);
    if (it != orig_block_to_block.end()) {
        return it->second;
    }
    auto block = pools.block_pool.Create(pools.inst_pool);
    orig_block_to_block[orig_block] = block;
    return block;
}

Inst* SubProgram::AddInst(Inst* orig_inst) {
    auto it = orig_inst_to_inst.find(orig_inst);
    if (it != orig_inst_to_inst.end()) {
        return it->second;
    }
    Block* block = AddBlock(orig_inst->GetParent());
    Inst inst(orig_inst->GetOpcode(), orig_inst->Flags<u32>());
    if (orig_inst->GetOpcode() == Opcode::Phi) {
        AddPhi(orig_inst, &inst);
    } else {
        for (size_t i = 0; i < orig_inst->NumArgs(); ++i) {
            SetArg(&inst, i, orig_inst->Arg(i));
        }
    }
    auto insertion_point = block->end();
    if (block->back().GetOpcode() == Opcode::ConditionRef) {
        --insertion_point;
    }
    return &(*block->PrependNewInst(insertion_point, inst));
}

Block* SubProgram::GetBlock(Block* orig_block) {
    auto it = orig_block_to_block.find(orig_block);
    if (it != orig_block_to_block.end()) {
        return it->second;
    }
    return nullptr;
}

Inst* SubProgram::GetInst(Inst* orig_inst) {
    auto it = orig_inst_to_inst.find(orig_inst);
    if (it != orig_inst_to_inst.end()) {
        return it->second;
    }
    return nullptr;
}

Program SubProgram::GetSubProgram() {
    Program sub_program(super_program->info);
    BuildBlockListAndASL(sub_program);
    sub_program.post_order_blocks = PostOrder(sub_program.syntax_list.front());
    AddConditionalTreeFromASL(sub_program.syntax_list);
    for (Block* block : sub_program.blocks) {
        block->SsaSeal();
    }
    return sub_program;
}

void SubProgram::AddPhi(Inst* orig_phi, Inst* phi) {
    // Current IR only has Phis with 2 arguments.
    ASSERT(orig_phi->NumArgs() == 2);
    Block* orig_block0 = orig_phi->PhiBlock(0);
    Block* orig_block1 = orig_phi->PhiBlock(1);
    Block* block0 = AddBlock(orig_block0);
    Block* block1 = AddBlock(orig_block1);
    const Value& arg0 = orig_phi->Arg(0);
    const Value& arg1 = orig_phi->Arg(1);
    AddPhiOperand(phi, block0, arg0);
    AddPhiOperand(phi, block1, arg1);
    const auto get_conds = [orig_block0, orig_block1]() -> std::pair<const Block::ConditionalData&, const Block::ConditionalData&> {
        const Block::ConditionalData& cond0 = orig_block0->CondData();
        const Block::ConditionalData& cond1 = orig_block1->CondData();
        if (cond0.depth > cond1.depth) {
            return {cond0, cond1};
        }
        return {cond1, cond0};
    };
    const auto& [start_cond, target_cond] = get_conds();
    const Block::ConditionalData* cond = &start_cond;
    while (cond->depth > target_cond.depth) {
        if (cond->asl_node->type == AbstractSyntaxNode::Type::If) {
            AddInst(cond->asl_node->data.if_node.cond.InstRecursive());
        } else if (cond->asl_node->type == AbstractSyntaxNode::Type::Loop) {
            AddInst(&cond->asl_node->data.loop.continue_block->back());
        }
        if (orig_phi->GetParent()->CondData().asl_node == cond->asl_node) {
            break;
        }
        cond = cond->parent;
    }
}

void SubProgram::SetArg(Inst* inst, size_t index, const Value& arg) {
    if (arg.IsImmediate()) {
        inst->SetArg(index, arg);
    } else {
        inst->SetArg(index, Value(AddInst(arg.InstRecursive())));
    }
}

void SubProgram::AddPhiOperand(Inst* phi, Block* block, const Value& arg) {
    if (arg.IsImmediate()) {
        phi->AddPhiOperand(block, arg);
    } else {
        phi->AddPhiOperand(block, Value(AddInst(arg.InstRecursive())));
    }
}

void SubProgram::BuildBlockListAndASL(Program& sub_program) {
    boost::container::flat_set<Block*> filter_blocks;
    for (const AbstractSyntaxNode& orig_asl_node : super_program->syntax_list) {
        AbstractSyntaxNode asl_node;
        asl_node.type = orig_asl_node.type;
        Block* orig_block = orig_asl_node.data.block;
        switch (orig_asl_node.type) {
        case AbstractSyntaxNode::Type::Block: {
            Block* block = GetBlock(orig_block);
            if (!block) {
                continue;
            }
            if (!sub_program.syntax_list.empty()) {
                Block* last_block = sub_program.blocks.back();
                if (!last_block->HasImmSuccessor(block)) {
                    last_block->AddBranch(block);
                }
            }
            asl_node.data.block = block;
            sub_program.blocks.push_back(block);
            break;
        }
        case AbstractSyntaxNode::Type::If: {
            Inst* cond = GetInst(orig_asl_node.data.if_node.cond.InstRecursive());
            if (!cond) {
                continue;
            }
            Block* block = cond->GetParent();
            Block* merge_block = AddBlock(orig_asl_node.data.if_node.merge);
            Block* body_block = AddBlock(orig_asl_node.data.if_node.body);
            asl_node.data.if_node.cond = U1(Value(cond));
            asl_node.data.if_node.body = body_block;
            asl_node.data.if_node.merge = merge_block;
            block->AddBranch(merge_block);
            block->AddBranch(body_block);
            filter_blocks.insert(merge_block);
            break;
        }
        case AbstractSyntaxNode::Type::EndIf: {
            Block* merge_block = GetBlock(orig_asl_node.data.end_if.merge);
            if (!filter_blocks.contains(merge_block)) {
                continue;
            }
            asl_node.data.end_if.merge = merge_block;
            break;
        }
        case AbstractSyntaxNode::Type::Loop: {
            Block* continue_block = GetBlock(orig_asl_node.data.loop.continue_block);
            if (!continue_block) {
                continue;
            }
            if (continue_block->back().GetOpcode() != Opcode::ConditionRef) {
                continue;
            }
            Block* merge_block = AddBlock(orig_asl_node.data.loop.merge);
            asl_node.data.loop.body = AddBlock(orig_asl_node.data.loop.body);
            asl_node.data.loop.continue_block = continue_block;
            asl_node.data.loop.merge = merge_block;
            filter_blocks.insert(merge_block);
            break;
        }
        case AbstractSyntaxNode::Type::Repeat: {
            Inst* cond = GetInst(orig_asl_node.data.repeat.cond.InstRecursive());
            if (!cond) {
                continue;
            }
            Block* block = cond->GetParent();
            Block* merge_block = AddBlock(orig_asl_node.data.repeat.merge);
            Block* loop_header_block = AddBlock(orig_asl_node.data.repeat.loop_header);
            asl_node.data.repeat.cond = U1(Value(cond));
            asl_node.data.repeat.loop_header = loop_header_block;
            asl_node.data.repeat.merge = merge_block;
            block->AddBranch(loop_header_block);
            block->AddBranch(merge_block);
            break;
        }
        case AbstractSyntaxNode::Type::Break: {
            Inst* cond = GetInst(orig_asl_node.data.break_node.cond.InstRecursive());
            if (!cond) {
                continue;
            }
            Block* block = cond->GetParent();
            Block* merge_block = AddBlock(orig_asl_node.data.break_node.merge);
            Block* skip_block = AddBlock(orig_asl_node.data.break_node.skip);
            asl_node.data.break_node.cond = U1(Value(&block->back()));
            asl_node.data.break_node.merge = merge_block;
            asl_node.data.break_node.skip = skip_block;
            block->AddBranch(merge_block);
            block->AddBranch(skip_block);
            break;
        }
        case AbstractSyntaxNode::Type::Unreachable:
            continue;
        default:
            break;
        }
        sub_program.syntax_list.push_back(asl_node);
    }
    for (Block* block : sub_program.blocks) {
        block->has_multiple_predecessors = block->ImmPredecessors().size() > 1;
    }
}

} // namespace Shader::IR