
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <vector>
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/srt_info.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/value.h"

enum class ReadConstOperand : u32 {

};

namespace Shader::Optimization {

class AssignOffsetsVisitor {
public:
    AssignOffsetsVisitor(SrtInfo& srt_info_)
        : srt_info(srt_info_), current_sharp_off_dw(0), current_cbuf_off_dw(0) {}

    void VisitRoots() {
        for (const IR::Inst* root : srt_info.srt_roots) {
            Visit(root);
        }

        srt_info.flattened_sharp_bufsize_dw = current_sharp_off_dw;
        srt_info.flattened_cbuf_bufsize_dw = current_cbuf_off_dw;
    }

    void Visit(const IR::Inst* inst) {
        SrtNode* node = srt_info.getNode(inst);

        ASSERT_MSG(std::popcount(node->use_kind.raw) <= 1, "Unhandled multiple use kinds in SRT");
        // TODO handle when a ReadConst is used as any combo of pointer, sharp,
        // normal u32
        // For now assume mutually exclusive
        if (node->use_kind.pointer_lo) {
            ASSERT(srt_info.pointer_uses.contains(inst));
            auto& use_list = srt_info.pointer_uses[inst];
            for (const auto& [off, use] : use_list) {
                Visit(use);
            }
        } else if (node->use_kind.pointer_hi) {
            // Ignore this
        } else if (node->use_kind.cbuffer) {
            node->flattened_cbuf_off_dw = current_cbuf_off_dw++;
        } else {
            // use_kind.sharp currently unused. If here, assume ReadConst use is for sharp
            node->flattened_sharp_off_dw = current_sharp_off_dw++;
        }
    }

private:
    SrtInfo& srt_info;

    u32 current_sharp_off_dw;
    u32 current_cbuf_off_dw;
};

class CodegenVisitor {
public:
    CodegenVisitor(SrtInfo& srt_info_) : srt_info(srt_info_) {}

    void VisitRoots() {
        for (const IR::Inst* root : srt_info.srt_roots) {
            Visit(root);
        }
    }

    void Visit(const IR::Inst* inst) {
        SrtNode* node = srt_info.getNode(inst);

        ASSERT_MSG(std::popcount(node->use_kind.raw) <= 1, "Unhandled multiple use kinds in SRT");
        if (node->use_kind.pointer_lo) {
            ASSERT(srt_info.pointer_uses.contains(inst));
            auto& use_list = srt_info.pointer_uses[inst];
            for (const auto& [off, use] : use_list) {
                Visit(use);
            }
        } else if (node->use_kind.pointer_hi) {
            // Ignore this
        } else if (node->use_kind.cbuffer) {
            // TODO
        } else {
            // Assume sharp for now
        }
    }

private:
    SrtInfo& srt_info;
};

void FlattenExtendedUserdataPass(IR::Program& program) {
    Shader::Info& info = program.info;
    SrtInfo& srt_info = program.info.srt_info;

    // Build tree
    for (IR::Block* const block : program.blocks) {
        for (const IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadConst) {
                const IR::Inst* spgpr_base = inst.Arg(0).InstRecursive();
                const IR::Value dword_off = inst.Arg(1);

                const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
                    if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                        inst->GetOpcode() == IR::Opcode::ReadConst) {
                        return inst;
                    }
                    return std::nullopt;
                };
                const auto base0 = IR::BreadthFirstSearch(spgpr_base->Arg(0), pred);
                const auto base1 = IR::BreadthFirstSearch(spgpr_base->Arg(1), pred);
                ASSERT_MSG(base0 && base1 && "ReadConst not from constant memory");

                const IR::Inst* ptr = base0.value();

                SrtNode* ptr_node = srt_info.getOrInsertNode(ptr);
                SrtNode* use_node = srt_info.getOrInsertNode(&inst);

                ptr_node->use_kind.pointer_lo.Assign(true);

                {
                    // set flag so we ignore the HI value during codegen
                    SrtNode* ptr_hi_node = srt_info.getOrInsertNode(base1.value());
                    ptr_hi_node->use_kind.pointer_hi.Assign(1);
                }

                // TODO figure out C++ way to do this without UserList constructor
                auto it = srt_info.pointer_uses.insert_or_assign(ptr, SrtInfo::UserList{});
                SrtInfo::UserList& user_list = it.first->second;

                ASSERT(dword_off.IsImmediate());
                u32 off_imm = dword_off.U32();
                user_list[off_imm] = &inst;

                if (ptr->GetOpcode() == IR::Opcode::GetUserData) {
                    srt_info.srt_roots.push_back(&inst);
                }
            }
        }
    }

    AssignOffsetsVisitor assign_off_vis(srt_info);
    assign_off_vis.VisitRoots();

    CodegenVisitor codegen_vis(srt_info);
    codegen_vis.VisitRoots();
}

} // namespace Shader::Optimization