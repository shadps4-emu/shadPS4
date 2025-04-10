
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <boost/container/flat_map.hpp>
#include "common/arch.h"
#include "common/config.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "shader_recompiler/info.h"
#ifdef ARCH_X86_64
#include "shader_recompiler/backend/asm_x64/emit_x64.h"
#endif
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/num_executions.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/srt.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/srt_gvn_table.h"
#include "shader_recompiler/ir/subprogram.h"
#include "shader_recompiler/ir/value.h"
#include "src/common/arch.h"
#include "src/common/decoder.h"

using namespace Xbyak::util;

static Xbyak::CodeGenerator g_srt_codegen(32_MB);

namespace {

static void DumpSrtProgram(const Shader::Info& info, const u8* code, size_t codesize) {
#ifdef ARCH_X86_64
    using namespace Common::FS;

    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}_{:#018x}.srtprogram.txt", info.stage, info.pgm_hash);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Write, FileType::TextFile};

    u64 address = reinterpret_cast<u64>(code);
    u64 code_end = address + codesize;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    while (address < code_end && ZYAN_SUCCESS(Common::Decoder::Instance()->decodeInstruction(
                                     instruction, operands, reinterpret_cast<void*>(address)))) {
        std::string s =
            Common::Decoder::Instance()->disassembleInst(instruction, operands, address);
        s += "\n";
        file.WriteString(s);
        address += instruction.length;
    }
#endif
}

using namespace Shader;

struct PassInfo {
    struct ReadConstData {
        u32 offset_dw;
        u32 count_dw;
        IR::Inst* unique_inst;
        IR::Inst* original_inst;
    };

    Optimization::SrtGvnTable gvn_table;
    // pick a single inst for a given value number
    std::unordered_map<u32, IR::Inst*> vn_to_inst;
    // map of all readconsts to their subprogram insts
    boost::container::small_flat_map<IR::Inst*, IR::Inst*, 32> all_readconsts;
    // subprogram insts mapped to their readconst data
    boost::container::small_flat_map<IR::Inst*, ReadConstData, 32> readconst_data;

    // Incremented during SRT program generation
    u32 dst_off_dw = 0;

    // Return a single instruction that this instruction is identical to, according
    // to value number
    // The "original" is arbitrary. Here it's the first instruction found for a given value number
    IR::Inst* DeduplicateInstruction(IR::Inst* inst) {
        auto it = vn_to_inst.try_emplace(gvn_table.GetValueNumber(inst), inst);
        return it.first->second;
    }
};
} // namespace

namespace Shader::Optimization {

namespace {

static inline void PushPtr(Xbyak::CodeGenerator& c, u32 off_dw) {
    c.push(rdi);
    c.mov(rdi, ptr[rdi + (off_dw << 2)]);
    c.mov(r10, 0xFFFFFFFFFFFFULL);
    c.and_(rdi, r10);
}

static inline void PopPtr(Xbyak::CodeGenerator& c) {
    c.pop(rdi);
};

static IR::U32 WrapInstWithCounter(IR::Inst* inst, u32 inital_value, IR::Block* first_block) {
    const IR::Block::ConditionalData* loop_data = &inst->GetParent()->CondData();
    while (loop_data != nullptr &&
           loop_data->asl_node->type != IR::AbstractSyntaxNode::Type::Loop) {
        loop_data = loop_data->parent;
    }
    ASSERT(loop_data != nullptr);
    IR::Block* loop_body = loop_data->asl_node->data.loop.body;
    // We are putting the Phi node in the loop header so that the counter is
    // incremented each time the loop is executed. We point the Phi node to the
    // first block so that the counter is not reset each time the loop is
    // executed (nested loops)
    IR::IREmitter ir_inst(*inst->GetParent(), ++IR::Block::InstructionList::s_iterator_to(*inst));
    IR::IREmitter ir_loop_header(*loop_body->ImmPredecessors().front());
    IR::Inst* phi = ir_loop_header.Phi(IR::Type::U32).Inst();
    IR::U32 inc = ir_inst.IAdd(IR::U32(phi), ir_inst.Imm32(1));
    phi->AddPhiOperand(first_block, ir_loop_header.Imm32(inital_value));
    phi->AddPhiOperand(inst->GetParent(), inc);
    return IR::U32(phi);
}

static IR::Program GenerateSrtReadConstsSubProgram(IR::Program& program, PassInfo& pass_info, Pools& pools) {
    IR::SubProgram sub_gen(&program, pools);
    for (auto& [inst, sub_inst] : pass_info.all_readconsts) {
        sub_inst = sub_gen.AddInst(inst);
        pass_info.readconst_data[sub_inst] = {0, 0, pass_info.DeduplicateInstruction(sub_inst),
                                              inst};
    }
    IR::Program sub_program = sub_gen.GetSubProgram();
    IR::Block* original_first_block = program.blocks.front();
    IR::Block* sub_first_block = sub_program.blocks.front();
    for (auto& [inst, data] : pass_info.readconst_data) {
        if (inst != data.unique_inst) {
            PassInfo::ReadConstData& unique_data = pass_info.readconst_data[data.unique_inst];
            data.offset_dw = unique_data.offset_dw;
            // In this context, count_dw is always the same as unique_data.count_dw
            // There are no duplicate instructions in different loops
            data.count_dw = unique_data.count_dw;
        } else {
            u32 count = static_cast<u32>(IR::GetNumExecutions(inst));
            ASSERT_MSG(count > 0, "Dynamic loop range not supported yet");
            data.count_dw = count;
            data.offset_dw = pass_info.dst_off_dw;
            pass_info.dst_off_dw += count;
            IR::U32 save_offset;
            if (data.count_dw > 1) {
                save_offset = WrapInstWithCounter(inst, data.offset_dw, sub_first_block);
            } else {
                IR::IREmitter ir(*inst);
                save_offset = ir.Imm32(data.offset_dw);
            }
            IR::IREmitter ir(*inst->GetParent(),
                             ++IR::Block::InstructionList::s_iterator_to(*inst));
            ir.SetUserData(save_offset, IR::U32(inst));
        }
        data.original_inst->SetFlags<u32>(1);
        IR::IREmitter ir(*data.original_inst);
        data.original_inst->SetArg(0, ir.Imm32(0));
        if (data.count_dw > 1) {
            IR::U32 counter =
                WrapInstWithCounter(data.original_inst, data.offset_dw, original_first_block);
            data.original_inst->SetArg(1, counter);
        } else {
            data.original_inst->SetArg(1, ir.Imm32(data.offset_dw));
        }
        
    }
    DeadCodeEliminationPass(sub_program);
    IR::DumpProgram(sub_program, sub_program.info, "srt");
    return sub_program;
}

static void GenerateSrtProgram(IR::Program& program, PassInfo& pass_info, Pools& pools) {
#ifdef ARCH_X86_64
    Xbyak::CodeGenerator& c = g_srt_codegen;
    Shader::Info& info = program.info;

    if (info.srt_info.srt_reservations.empty() && pass_info.all_readconsts.empty()) {
        return;
    }

    info.srt_info.walker_func = c.getCurr<PFN_SrtWalker>();

    pass_info.dst_off_dw = NumUserDataRegs;

    // Special case for V# step rate buffers in fetch shader
    for (const auto [sgpr_base, dword_offset, num_dwords] : info.srt_info.srt_reservations) {
        // get pointer to V#
        if (sgpr_base != IR::NumScalarRegs) {
            PushPtr(c, sgpr_base);
        }
        u32 src_off = dword_offset << 2;

        for (auto j = 0; j < num_dwords; j++) {
            c.mov(r11d, ptr[rdi + src_off]);
            c.mov(ptr[rsi + (pass_info.dst_off_dw << 2)], r11d);

            src_off += 4;
            ++pass_info.dst_off_dw;
        }
        if (sgpr_base != IR::NumScalarRegs) {
            PopPtr(c);
        }
    }

    ASSERT(pass_info.dst_off_dw == info.srt_info.flattened_bufsize_dw);

    if (!pass_info.all_readconsts.empty()) {
        IR::Program sub_program = GenerateSrtReadConstsSubProgram(program, pass_info, pools);
        Backend::X64::EmitX64(sub_program, c);
    }

    info.srt_info.flattened_bufsize_dw = pass_info.dst_off_dw;

    c.ret();
    c.ready();

    if (Config::dumpShaders()) {
        size_t codesize = c.getCurr() - reinterpret_cast<const u8*>(info.srt_info.walker_func);
        DumpSrtProgram(info, reinterpret_cast<const u8*>(info.srt_info.walker_func), codesize);
    }
#elif
    if (info.srt_info.srt_reservations.empty() && pass_info.all_readconsts.empty()) {
        UNREACHABLE_MSG("SRT program generation only supported on x86_64");
    }
#endif
}

}; // namespace

void FlattenExtendedUserdataPass(IR::Program& program, Pools& pools) {
    Shader::Info& info = program.info;
    PassInfo pass_info;

    for (auto it = program.post_order_blocks.rbegin(); it != program.post_order_blocks.rend();
         ++it) {
        IR::Block* block = *it;
        for (auto& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadConst) {
                pass_info.all_readconsts[&inst] = nullptr;
            }
        }
    }

    GenerateSrtProgram(program, pass_info, pools);
    info.RefreshFlatBuf();
}

} // namespace Shader::Optimization
