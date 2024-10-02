
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/srt_info.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

using namespace Xbyak::util;

// TODO make sure no problems with identity and Insts being used in maps

namespace Shader {
// Hacky. TODO refactor info RuntimeInfo or smtn
FlatSharpBuffer::FlatSharpBuffer(const Info& info) {
    info.RunSrtWalker(*this);
}

} // namespace Shader

namespace Shader::Optimization {

namespace {
static IR::Value GetReadConstOff(const IR::Inst* inst) {
    ASSERT(inst->GetOpcode() == IR::Opcode::ReadConst);
    return inst->Arg(1);
}

static IR::ScalarReg GetUserDataSgprBase(const IR::Inst* inst) {
    ASSERT(inst->GetOpcode() == IR::Opcode::GetUserData);
    return inst->Arg(0).ScalarReg();
}

static inline void PushPtr(Xbyak::CodeGenerator& c, u32 off_dw) {
    c.push(rdi);
    c.mov(rdi, ptr[rdi + (off_dw << 2)]);
    c.mov(r10, 0xFFFFFFFFFFFFULL);
    c.and_(rdi, r10);
}

static inline void PopPtr(Xbyak::CodeGenerator& c) {
    c.pop(rdi);
};

static void VisitPointer(u32 off_dw, const IR::Inst* subtree, Info& info, Xbyak::CodeGenerator& c) {
    PushPtr(c, off_dw);
    SrtInfo& srt_info = info.srt_info;
    const SrtInfo::PtrUserList* use_list = srt_info.GetUsesAsPointer(subtree);
    ASSERT(use_list);

    // First copy all the src data from this tree level
    // That way, all data that was contiguous in the guest SRT is also contiguous in the
    // flattened buffer.
    // TODO src and dst are contiguous. Optimize with wider loads/stores
    // TODO if this subtree is dynamically indexed, don't compact it (keep it sparse)
    for (const auto [src_off_dw, use] : *use_list) {
        u32 dst_off_dw = srt_info.flattened_bufsize_dw++;

        c.mov(r10, dword[rdi + (src_off_dw << 2)]);
        c.mov(dword[rsi + (dst_off_dw << 2)], r10);

        srt_info.srt_node_to_flat_off_dw[use] = dst_off_dw;
    }

    // Then visit any children used as pointers
    for (const auto [src_off_dw, use] : *use_list) {
        if (srt_info.GetUsesAsPointer(use)) {
            VisitPointer(src_off_dw, use, info, c);
        }
    }

    PopPtr(c);
}

static void GenerateSrtProgram(Shader::Info& info) {
    SrtInfo& srt_info = info.srt_info;
    Xbyak::CodeGenerator& c = info.srt_codegen;
    srt_info.flattened_bufsize_dw = NumUserDataRegs + 4 * srt_info.fetch_reservations.size();

    c.inLocalLabel();

    // Special case for V# step rate buffers in fetch shader
    for (auto i = 0; i < srt_info.fetch_reservations.size(); i++) {
        SrtInfo::FetchShaderReservation res = srt_info.fetch_reservations[i];
        // get pointer to V#
        c.mov(r10, dword[rdi + (res.sgpr_base << 2)]);

        u32 src_off = res.dword_offset << 2;
        // 4 dwords per V#
        u32 dst_off = (NumUserDataRegs + 4 * i) << 2;

        c.mov(r11, dword[r10 + src_off]);
        c.mov(dword[rsi + dst_off], r11);

        c.mov(r11, dword[r10 + (src_off + 4)]);
        c.mov(dword[rsi + (dst_off + 4)], r11);

        c.mov(r11, dword[r10 + (src_off + 8)]);
        c.mov(dword[rsi + (dst_off + 8)], r11);

        c.mov(r11, dword[r10 + (src_off + 12)]);
        c.mov(dword[rsi + (dst_off + 12)], r11);
    }

    for (const auto& [sgpr_base, root] : srt_info.srt_roots) {
        VisitPointer(static_cast<u32>(sgpr_base), root, info, c);
    }

    c.ret();
}

}; // namespace

void FlattenExtendedUserdataPass(IR::Program& program) {
    Shader::Info& info = program.info;
    SrtInfo& srt_info = program.info.srt_info;

    // HoistConstantReads should have put all SRT reads into the entry block
    const IR::Block* entry_bb = *program.blocks.begin();

    for (const IR::Inst& inst : *entry_bb) {
        if (inst.GetOpcode() == IR::Opcode::ReadConst) {
            if (!GetReadConstOff(&inst).IsImmediate()) {
                continue;
            }

            const IR::Inst* spgpr_base = inst.Arg(0).InstRecursive();

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

            auto it = srt_info.pointer_uses.try_emplace(ptr, SrtInfo::PtrUserList{});
            SrtInfo::PtrUserList& user_list = it.first->second;

            user_list[GetReadConstOff(&inst).U32()] = &inst;

            if (ptr->GetOpcode() == IR::Opcode::GetUserData) {
                IR::ScalarReg ud_reg = GetUserDataSgprBase(ptr);
                srt_info.srt_roots[ud_reg] = ptr;
            }
        }
    }

    GenerateSrtProgram(info);
}

} // namespace Shader::Optimization