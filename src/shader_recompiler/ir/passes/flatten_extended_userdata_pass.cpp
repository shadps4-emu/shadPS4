// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <queue>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/arch.h"
#include "common/decoder.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/signal_context.h"
#include "core/emulator_settings.h"
#include "core/signals.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/passes/srt.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/srt_gvn_table.h"
#include "shader_recompiler/ir/value.h"

#ifdef ARCH_X86_64

using namespace Xbyak::util;

static Xbyak::CodeGenerator g_srt_codegen(32_MB);
static const u8* g_srt_codegen_start = nullptr;

namespace Shader {

PFN_SrtWalker RegisterWalkerCode(const u8* ptr, size_t size) {
    const auto func_addr = (PFN_SrtWalker)g_srt_codegen.getCurr();
    g_srt_codegen.db(ptr, size);
    g_srt_codegen.ready();
    return func_addr;
}

} // namespace Shader

namespace {

static void DumpSrtProgram(const Shader::Info& info, const u8* code, size_t codesize) {
    using namespace Common::FS;

    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}_{:#018x}.srtprogram.txt", info.hw_stage, info.pgm_hash);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Create, FileType::TextFile};

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
}

static bool SrtWalkerSignalHandler(void* context, void* fault_address) {
    // Only handle if the fault address is within the SRT code range
    const u8* code_start = g_srt_codegen_start;
    const u8* code_end = code_start + g_srt_codegen.getSize();
    const void* code = Common::GetRip(context);
    if (code < code_start || code >= code_end) {
        return false; // Not in SRT code range
    }

    // Patch instruction to zero register
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(instruction, operands,
                                                                       const_cast<void*>(code), 15);

    ASSERT(ZYAN_SUCCESS(status) && instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
           operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
           operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY);

    size_t len = instruction.length;
    const size_t patch_size = 3;
    u8* code_patch = const_cast<u8*>(reinterpret_cast<const u8*>(code));

    // We can only encounter rdi or r10d as the first operand in a
    // fault memory access for SRT walker.
    switch (operands[0].reg.value) {
    case ZYDIS_REGISTER_RDI:
        // mov rdi, [rdi + (off_dw << 2)] -> xor rdi, rdi
        code_patch[0] = 0x48;
        code_patch[1] = 0x31;
        code_patch[2] = 0xFF;
        break;
    case ZYDIS_REGISTER_R10D:
        // mov r10d, [rdi + (off_dw << 2)] -> xor r10d, r10d
        code_patch[0] = 0x45;
        code_patch[1] = 0x31;
        code_patch[2] = 0xD2;
        break;
    default:
        UNREACHABLE_MSG("Unsupported register for SRT walker patch");
        return false;
    }

    // Fill nops
    memset(code_patch + patch_size, 0x90, len - patch_size);

    LOG_WARNING(Render_Recompiler, "Patched SRT walker at {}, fault address {}", code,
                fault_address);

    return true;
}

using namespace Shader;

struct PassInfo {
    struct PointerListCompare {
        bool operator()(const IR::Value& a, const IR::Value& b) const {
            // Keep order with the following criteria:
            // 1. If immediate, sort by immediate value
            // 2. If not immediate, sort by instruction order
            // 3. Immediates come first, then instructions
            if (a.IsImmediate() && b.IsImmediate()) {
                return a.U32() < b.U32();
            } else if (!a.IsImmediate() && !b.IsImmediate()) {
                auto a_inst = a.Inst();
                auto b_inst = b.Inst();
                auto a_block = a_inst->GetParent();
                auto b_block = b_inst->GetParent();
                if (a_block != b_block) {
                    return a_block < b_block; // Sort by block address
                }

                auto it = a_block->Instructions().iterator_to(*a_inst);
                auto end = a_block->Instructions().end();
                for (; it != end; ++it) {
                    if (&*it == b_inst) {
                        return true;
                    }
                }
                return false;
            } else {
                return a.IsImmediate(); // Immediates come first
            }
        }
    };

    // map offset to inst
    using PtrUserList =
        boost::container::flat_map<IR::Value, Shader::IR::Inst*, PointerListCompare>;

    Optimization::SrtGvnTable gvn_table;
    // keys are GetUserData, ReadConst or ReadConstBuffer instructions that are used as pointers
    std::unordered_map<IR::Inst*, PtrUserList> pointer_uses;
    // GetUserData instructions corresponding to sgpr_base of SRT roots
    boost::container::small_flat_map<IR::ScalarReg, IR::Inst*, 1> srt_roots;

    // pick a single inst for a given value number
    std::unordered_map<u32, IR::Inst*> vn_to_inst;

    // Bumped during codegen to assign offsets to readconsts
    u16 dst_off_dw;

    PtrUserList* GetUsesAsPointer(IR::Inst* inst) {
        auto it = pointer_uses.find(inst);
        if (it != pointer_uses.end()) {
            return &it->second;
        }
        return nullptr;
    }

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

static inline u16 GetFlatbufOffset(const IR::Inst* inst) {
    if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<IR::BufferInstInfo>();
        return inst_info.flatbuf_off_dw;
    }
    if (inst->GetOpcode() == IR::Opcode::ReadConst) {
        return inst->Flags<u16>();
    }
    UNREACHABLE_MSG("Instruction not supported");
}

static inline void SetFlatbufOffset(IR::Inst* inst, u16 offset) {
    if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<IR::BufferInstInfo>();
        inst_info.flatbuf_off_dw.Assign(offset);
        inst->SetFlags(inst_info);
        return;
    }
    if (inst->GetOpcode() == IR::Opcode::ReadConst) {
        inst->SetFlags(offset);
        return;
    }
    UNREACHABLE_MSG("Instruction not supported");
}

static bool ComputeOffset(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                          const IR::Value& off_dw);

#define ABORT_ON_FAILURE(expr)                                                                     \
    if (!(expr)) {                                                                                 \
        return false;                                                                              \
    }

#define POP_ABORT_ON_FAILURE(expr)                                                                 \
    if (!(expr)) {                                                                                 \
        c.add(rsp, 8);                                                                             \
        return false;                                                                              \
    }

static bool EmitComputeOffsetIAdd32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                                    IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.add(reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.add(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.add(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetISub32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                                    IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.neg(reg);
        c.add(reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.sub(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.neg(reg);
        c.add(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetIMul32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                                    IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.imul(reg, reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.imul(reg, reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.imul(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetShiftLeftLogical32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                                PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.shl(reg, inst->Arg(0).U32());
        c.mov(ecx, reg);
        c.mov(reg, inst->Arg(0).U32());
        c.shl(reg, cl);
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.shl(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.pop(rcx);
        c.shl(reg, cl);
    }
    return true;
}

static bool EmitComputeOffsetShiftRightLogical32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                                 PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.mov(ecx, reg);
        c.mov(reg, inst->Arg(0).U32());
        c.shr(reg, cl);
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.shr(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.pop(rcx);
        c.shr(reg, cl);
    }
    return true;
}

static bool EmitComputeOffsetBitwiseAnd32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                          PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.and_(reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.and_(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.and_(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetBitwiseOr32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                         PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.or_(reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.or_(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.or_(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetBitwiseXor32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                          PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.xor_(reg, inst->Arg(0).U32());
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.xor_(reg, inst->Arg(1).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.xor_(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetBitwiseNot32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                          PassInfo& pass_info, IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
    c.not_(reg);
    return true;
}

static bool EmitComputeOffsetUMin32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                                    IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.mov(ecx, inst->Arg(0).U32());
        c.cmp(reg, ecx);
        c.cmova(reg, ecx);
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.mov(ecx, inst->Arg(1).U32());
        c.cmp(reg, ecx);
        c.cmova(reg, ecx);
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.cmp(reg, dword[rsp]);
        c.cmova(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetUMax32(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                                    IR::Inst* inst) {
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.mov(ecx, inst->Arg(0).U32());
        c.cmp(reg, ecx);
        c.cmovb(reg, ecx);
    } else if (inst->Arg(1).IsImmediate()) {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.mov(ecx, inst->Arg(1).U32());
        c.cmp(reg, ecx);
        c.cmovb(reg, ecx);
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
        c.push(reg.cvt64());
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.cmp(reg, dword[rsp]);
        c.cmovb(reg, dword[rsp]);
        c.add(rsp, 8);
    }
    return true;
}

static bool EmitComputeOffsetBitFieldUExtract(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg,
                                              PassInfo& pass_info, IR::Inst* inst) {
    // We asume that the count is always less than 32, Is this correct?
    ASSERT(!inst->AreAllArgsImmediates());
    if (inst->Arg(0).IsImmediate()) {
        c.mov(reg, inst->Arg(0).U32());
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(0)));
    }
    bool in_stack = false;
    if (inst->Arg(1).IsImmediate()) {
        c.shr(reg, inst->Arg(1).U32());
    } else {
        c.push(reg.cvt64());
        in_stack = true;
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(1)));
        c.mov(ecx, reg);
        c.shr(dword[rsp], cl);
    }
    if (inst->Arg(2).IsImmediate()) {
        if (in_stack) {
            c.pop(reg.cvt64());
        }
        c.and_(reg, (1U << inst->Arg(2).U32()) - 1);
    } else {
        if (!in_stack) {
            c.push(reg.cvt64());
        }
        POP_ABORT_ON_FAILURE(ComputeOffset(c, reg, pass_info, inst->Arg(2)));
        c.mov(ecx, reg);
        c.mov(edx, 1);
        c.shl(edx, cl);
        c.dec(edx);
        c.and_(dword[rsp], edx);
        c.pop(reg.cvt64());
    }
    return true;
}

static bool IsAllowedOffsetInstruction(const IR::Inst* inst) {
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::ReadConst:
    case IR::Opcode::ReadConstBuffer:
    case IR::Opcode::IAdd32:
    case IR::Opcode::ISub32:
    case IR::Opcode::IMul32:
    case IR::Opcode::ShiftLeftLogical32:
    case IR::Opcode::ShiftRightLogical32:
    case IR::Opcode::BitwiseAnd32:
    case IR::Opcode::BitwiseOr32:
    case IR::Opcode::BitwiseXor32:
    case IR::Opcode::BitwiseNot32:
    case IR::Opcode::UMin32:
    case IR::Opcode::UMax32:
    case IR::Opcode::BitFieldUExtract:
        return true;
    default:
        return false;
    }
}

static bool ComputeOffset(Xbyak::CodeGenerator& c, Xbyak::Reg32 reg, PassInfo& pass_info,
                          const IR::Value& off_dw) {
    ASSERT(reg != ecx && reg != edx);
    auto inst = off_dw.Inst();
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
        c.mov(reg, ptr[rsi + (static_cast<u32>(inst->Arg(0).ScalarReg()) << 2)]);
        return true;
    case IR::Opcode::ReadConst:
    case IR::Opcode::ReadConstBuffer:
        if (u16 offset = GetFlatbufOffset(pass_info.DeduplicateInstruction(inst)); offset != 0) {
            c.mov(reg, ptr[rsi + (offset << 2)]);
            return true;
        }
        return false;
    case IR::Opcode::IAdd32:
        ABORT_ON_FAILURE(EmitComputeOffsetIAdd32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::ISub32:
        ABORT_ON_FAILURE(EmitComputeOffsetISub32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::IMul32:
        ABORT_ON_FAILURE(EmitComputeOffsetIMul32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::ShiftLeftLogical32:
        ABORT_ON_FAILURE(EmitComputeOffsetShiftLeftLogical32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::ShiftRightLogical32:
        ABORT_ON_FAILURE(EmitComputeOffsetShiftRightLogical32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::BitwiseAnd32:
        ABORT_ON_FAILURE(EmitComputeOffsetBitwiseAnd32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::BitwiseOr32:
        ABORT_ON_FAILURE(EmitComputeOffsetBitwiseOr32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::BitwiseXor32:
        ABORT_ON_FAILURE(EmitComputeOffsetBitwiseXor32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::BitwiseNot32:
        ABORT_ON_FAILURE(EmitComputeOffsetBitwiseNot32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::UMin32:
        ABORT_ON_FAILURE(EmitComputeOffsetUMin32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::UMax32:
        ABORT_ON_FAILURE(EmitComputeOffsetUMax32(c, reg, pass_info, inst));
        return true;
    case IR::Opcode::BitFieldUExtract:
        ABORT_ON_FAILURE(EmitComputeOffsetBitFieldUExtract(c, reg, pass_info, inst));
        return true;
    default:
        LOG_ERROR(Render_Recompiler, "Unexpected instruction for offset computation, {}",
                  magic_enum::enum_name(inst->GetOpcode()));
        return false;
    }
}

static inline bool PushPtr(Xbyak::CodeGenerator& c, PassInfo& pass_info, const IR::Value& off_dw) {
    c.push(rdi);
    if (off_dw.IsImmediate()) {
        c.mov(rdi, ptr[rdi + (off_dw.U32() << 2)]);
    } else {
        ABORT_ON_FAILURE(ComputeOffset(c, r10d, pass_info, off_dw));
        c.shl(r10d, 2);
        c.mov(r10d, r10d);
        c.mov(rdi, ptr[rdi + r10]);
    }
    c.mov(r10, 0xFFFFFFFFFFFFULL);
    c.and_(rdi, r10);
    return true;
}

static inline void PopPtr(Xbyak::CodeGenerator& c) {
    c.pop(rdi);
}

static void VisitPointer(const IR::Value& off_dw, IR::Inst* subtree, PassInfo& pass_info,
                         Xbyak::CodeGenerator& c) {
    if (subtree->GetOpcode() == IR::Opcode::ReadConst && subtree->Flags<u16>() == 0 ||
        subtree->GetOpcode() == IR::Opcode::ReadConstBuffer &&
            subtree->Flags<IR::BufferInstInfo>().flatbuf_off_dw == 0) {
        return;
    }

    if (!PushPtr(c, pass_info, off_dw)) {
        LOG_ERROR(Render_Recompiler, "Failed to compute offset for SRT walker");
        return;
    }
    PassInfo::PtrUserList* use_list = pass_info.GetUsesAsPointer(subtree);
    ASSERT(use_list);

    // First copy all the src data from this tree level
    // That way, all data that was contiguous in the guest SRT is also contiguous in the
    // flattened buffer.
    // TODO src and dst are contiguous. Optimize with wider loads/stores
    // TODO if this subtree is dynamically indexed, don't compact it (keep it sparse)
    for (auto [src_off_dw, use] : *use_list) {
        if (src_off_dw.IsImmediate()) {
            c.mov(r10d, ptr[rdi + (src_off_dw.U32() << 2)]);
        } else {
            if (!ComputeOffset(c, r10d, pass_info, src_off_dw)) {
                LOG_ERROR(Render_Recompiler, "Failed to compute offset for SRT walker");
                continue;
            }
            c.shl(r10d, 2);
            c.mov(r10d, r10d);
            c.mov(r10d, dword[rdi + r10]);
        }
        c.mov(ptr[rsi + (pass_info.dst_off_dw << 2)], r10d);

        SetFlatbufOffset(use, pass_info.dst_off_dw);
        pass_info.dst_off_dw++;
    }

    // Then visit any children used as pointers
    for (const auto [src_off_dw, use] : *use_list) {
        if (pass_info.GetUsesAsPointer(use)) {
            VisitPointer(src_off_dw, use, pass_info, c);
        }
    }

    PopPtr(c);
}

constexpr u32 MaxGuardAtoms = 64;
constexpr u32 MaxGuardTerms = 4;
constexpr u32 MaxGuardDepth = 32;

enum class GuardCompare : u8 {
    IEq,
    INe,
    SLt,
    SLe,
    ULt,
    ULe,
    FEq,
    FNe,
    FLt,
    FLe,
};

struct GuardOperand {
    u32 immediate{};
    u32 mask{~0U};
    SharpLocation location{UNKNOWN_LOCATION};
    u8 shift{};

    bool operator==(const GuardOperand&) const = default;
};

struct GuardAtom {
    GuardOperand lhs{};
    GuardOperand rhs{};
    GuardCompare op{};
    bool negate{};

    bool operator==(const GuardAtom&) const = default;
};

using GuardAtoms = boost::container::static_vector<GuardAtom, MaxGuardAtoms>;

struct GuardTerms {
    boost::container::static_vector<u64, MaxGuardTerms> terms;

    static GuardTerms Always() {
        return Of(0);
    }

    static GuardTerms Never() {
        return {};
    }

    static GuardTerms Of(u64 term) {
        return {.terms = {term}};
    }

    bool IsTrivial() const {
        return terms.empty() || terms[0] == 0;
    }

    GuardTerms And(const GuardTerms& other) const {
        GuardTerms result;
        for (const u64 lhs : terms) {
            for (const u64 rhs : other.terms) {
                result.Add(lhs | rhs);
            }
        }
        return result;
    }

    GuardTerms Or(const GuardTerms& other) const {
        GuardTerms result = *this;
        for (const u64 term : other.terms) {
            result.Add(term);
        }
        return result;
    }

    void Add(u64 term) {
        if (std::ranges::any_of(terms, [term](u64 kept) { return (kept & term) == kept; })) {
            return;
        }
        const auto [first, last] =
            std::ranges::remove_if(terms, [term](u64 kept) { return (kept & term) == term; });
        terms.erase(first, last);
        if (terms.size() < MaxGuardTerms) {
            terms.push_back(term);
            return;
        }
        *this = Always();
    }
};

struct GuardGroup {
    GuardTerms terms;
    boost::container::small_vector<SharpLocation, 8> slots;
};

using GuardGroups = boost::container::small_vector<GuardGroup, 8>;

bool IsNormalOrZero(u32 bits) {
    const u32 exponent = (bits >> 23) & 0xFF;
    return exponent != 0xFF && (exponent != 0 || (bits & 0x7FFFFF) == 0);
}

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
    if (operand.location != UNKNOWN_LOCATION) {
        operand.shift = static_cast<u8>(operand.shift + offset);
        operand.mask = (operand.mask >> offset) & mask;
    } else {
        operand.immediate = (operand.immediate >> offset) & mask;
    }
    return operand;
}

std::optional<GuardOperand> GuardOperandOf(const IR::Value& value, u32 depth);

std::optional<GuardOperand> Masked(const IR::Inst& inst, u32 depth) {
    const std::optional<u32> lhs_mask = ImmU32(inst.Arg(0));
    const std::optional<u32> rhs_mask = ImmU32(inst.Arg(1));
    if (lhs_mask.has_value() == rhs_mask.has_value()) {
        return std::nullopt;
    }
    const std::optional<GuardOperand> operand =
        GuardOperandOf(inst.Arg(lhs_mask ? 1 : 0), depth + 1);
    if (!operand || operand->location == UNKNOWN_LOCATION) {
        return std::nullopt;
    }
    return Bits(*operand, 0, lhs_mask ? *lhs_mask : *rhs_mask);
}

std::optional<GuardOperand> Extract(const IR::Value& base, u32 offset, u32 count, u32 depth) {
    const std::optional<GuardOperand> operand = GuardOperandOf(base, depth + 1);
    if (!operand || operand->location == UNKNOWN_LOCATION || count == 0 || offset > 32 ||
        count > 32 || offset + count > 32 || operand->shift + offset >= 32) {
        return std::nullopt;
    }
    return Bits(*operand, offset, static_cast<u32>((u64{1} << count) - 1));
}

std::optional<GuardOperand> GuardOperandOf(const IR::Value& value, u32 depth) {
    if (value.IsImmediate()) {
        if (value.Type() == IR::Type::U32) {
            return GuardOperand{.immediate = value.U32()};
        }
        if (value.Type() == IR::Type::F32) {
            return GuardOperand{.immediate = std::bit_cast<u32>(value.F32())};
        }
        return std::nullopt;
    }
    if (depth >= MaxGuardDepth) {
        return std::nullopt;
    }
    const IR::Inst* const inst = value.Inst();
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::ReadConst:
        if (const SharpLocation location = SharpLocationFromSource(inst, false);
            location != UNKNOWN_LOCATION) {
            return GuardOperand{.location = location};
        }
        return std::nullopt;
    case IR::Opcode::ReadFirstLane:
    case IR::Opcode::BitCastU32F32:
    case IR::Opcode::BitCastF32U32:
        return GuardOperandOf(inst->Arg(0), depth + 1);
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
    const std::optional<GuardOperand> operand = GuardOperandOf(value, 0);
    if (!operand || operand->shift != 0 || operand->mask != ~0U) {
        return std::nullopt;
    }
    return operand;
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

class AtomBuilder {
public:
    explicit AtomBuilder(GuardAtoms& atoms_) : atoms{atoms_} {}

    GuardTerms Leaf(const IR::Value& lhs, const IR::Value& rhs, GuardCompare op, bool negate) {
        const bool is_float = op >= GuardCompare::FEq;
        const std::optional<GuardOperand> a = is_float ? FloatOperand(lhs) : GuardOperandOf(lhs, 0);
        const std::optional<GuardOperand> b = is_float ? FloatOperand(rhs) : GuardOperandOf(rhs, 0);
        if (!a || !b) {
            return GuardTerms::Always();
        }
        return Atom({.lhs = *a, .rhs = *b, .op = op, .negate = negate});
    }

private:
    GuardTerms Atom(const GuardAtom& atom) {
        if (atom.lhs.location == UNKNOWN_LOCATION && atom.rhs.location == UNKNOWN_LOCATION) {
            return GuardTerms::Always();
        }
        const auto index = static_cast<u32>(std::ranges::find(atoms, atom) - atoms.begin());
        if (index == MaxGuardAtoms) {
            return GuardTerms::Always();
        }
        if (index == atoms.size()) {
            atoms.push_back(atom);
        }
        return GuardTerms::Of(u64{1} << index);
    }

    GuardAtoms& atoms;
};

class GuardBuilder {
public:
    explicit GuardBuilder(GuardAtoms& atoms_) : atoms{atoms_} {}

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
        if (value.IsImmediate() || depth >= MaxGuardDepth) {
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
        case IR::Opcode::PackUint2x32:
            if (const IR::Inst* const unpack = Unpacked(inst.Arg(0))) {
                return Necessary(unpack->Arg(0), polarity, depth);
            }
            return GuardTerms::Always();
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
        return lhs.terms.empty() ? lhs : lhs.And(Necessary(inst.Arg(1), polarity, depth));
    }

    GuardTerms Select(const IR::Inst& inst, bool polarity, u32 depth) {
        const IR::Value cond = inst.Arg(0);
        if (const std::optional<bool> taken = Known(cond)) {
            return Necessary(inst.Arg(*taken ? 1 : 2), polarity, depth);
        }
        const auto when = [&](bool taken) {
            const GuardTerms terms = Necessary(inst.Arg(taken ? 1 : 2), polarity, depth);
            return terms.terms.empty() ? terms : Necessary(cond, taken, depth).And(terms);
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

struct SharpUse {
    const IR::Inst* user{};
    u32 operand{};
    GuardTerms terms;
    boost::container::small_vector<SharpLocation, 8> slots;
    bool is_guardable{};
    u32 parent{};
};

class SharpUses {
public:
    void Add(const IR::Inst* user, u32 operand, const SharpReference& sharp,
             const GuardTerms& terms, bool is_guardable) {
        SharpUse& use = uses.emplace_back(SharpUse{
            .user = user,
            .operand = operand,
            .terms = terms,
            .is_guardable = is_guardable,
            .parent = static_cast<u32>(uses.size()),
        });
        for (u32 i = 0; i < sharp.num_dwords; ++i) {
            const IR::Inst* const source = sharp.dwords[i].TryInst();
            if (!source) {
                use.is_guardable = false;
                continue;
            }
            AddSlot(use, SharpLocationFromSource(source, false));
        }
        if (sharp.post_op == SharpFetchPostOp::DisableAnisoIfSingleLod) {
            if (const IR::Inst* const lod_prod = sharp.post_op_data.lod_prod.TryInst()) {
                AddSlot(use, SharpLocationFromSource(lod_prod, false));
            }
        }
    }

    void AddTermsToSamplers(const GuardTerms& terms) {
        for (SharpUse& use : uses) {
            if (use.operand == 1) {
                use.terms = use.terms.Or(terms);
            }
        }
    }

    GuardGroups Build(const IR::Program& program) {
        std::unordered_map<SharpLocation, u32> slot_owner;
        for (u32 i = 0; i < uses.size(); ++i) {
            for (const SharpLocation slot : uses[i].slots) {
                if (const auto [it, is_new] = slot_owner.try_emplace(slot, i); !is_new) {
                    Join(it->second, i);
                }
            }
        }
        std::unordered_map<SharpLocation, boost::container::small_vector<const IR::Inst*, 2>>
            readers;
        for (const IR::Block* const block : program.blocks) {
            for (const IR::Inst& inst : block->Instructions()) {
                if (const std::optional<SharpLocation> slot = ReadSlot(inst)) {
                    readers[*slot].push_back(&inst);
                }
            }
        }
        std::map<u32, GuardGroup> roots;
        std::unordered_set<u32> rejected;
        for (u32 i = 0; i < uses.size(); ++i) {
            const u32 root = Find(i);
            if (!uses[i].is_guardable) {
                rejected.insert(root);
            }
            GuardGroup& group = roots[root];
            group.terms = group.terms.Or(uses[i].terms);
            for (const SharpLocation slot : uses[i].slots) {
                if (std::ranges::find(group.slots, slot) == group.slots.end()) {
                    group.slots.push_back(slot);
                }
            }
        }
        GuardGroups groups;
        for (auto& [root, group] : roots) {
            if (group.terms.IsTrivial() || rejected.contains(root) ||
                !std::ranges::all_of(group.slots, [&](SharpLocation slot) {
                    return std::ranges::all_of(readers[slot], [&](const IR::Inst* reader) {
                        std::unordered_set<const IR::Inst*> visited{reader};
                        return ReadOnlyBy(reader, root, visited);
                    });
                })) {
                continue;
            }
            std::ranges::sort(group.slots);
            groups.push_back(std::move(group));
        }
        return groups;
    }

private:
    static void AddSlot(SharpUse& use, SharpLocation slot) {
        if (slot == UNKNOWN_LOCATION) {
            use.is_guardable = false;
        } else if (std::ranges::find(use.slots, slot) == use.slots.end()) {
            use.slots.push_back(slot);
        }
    }

    static std::optional<SharpLocation> ReadSlot(const IR::Inst& inst) {
        switch (inst.GetOpcode()) {
        case IR::Opcode::GetUserData:
            return static_cast<SharpLocation>(inst.Arg(0).ScalarReg());
        case IR::Opcode::ReadConst:
            return inst.Flags<SharpLocation>();
        case IR::Opcode::ReadConstBuffer:
            if (const SharpLocation slot = SharpLocationFromSource(&inst, false);
                slot != UNKNOWN_LOCATION) {
                return slot;
            }
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }

    bool ReadOnlyBy(const IR::Inst* inst, u32 root, std::unordered_set<const IR::Inst*>& visited) {
        for (const IR::Use& use : inst->Uses()) {
            const IR::Inst* const user = use.user;
            if (IsImageInstruction(*user)) {
                const auto it = std::ranges::find_if(uses, [&](const SharpUse& sharp_use) {
                    return sharp_use.user == user && sharp_use.operand == use.operand;
                });
                if (it == uses.end() || Find(static_cast<u32>(it - uses.begin())) != root) {
                    return false;
                }
                continue;
            }
            if (user->MayHaveSideEffects() ||
                (visited.insert(user).second && !ReadOnlyBy(user, root, visited))) {
                return false;
            }
        }
        return true;
    }

    u32 Find(u32 index) {
        while (uses[index].parent != index) {
            uses[index].parent = uses[uses[index].parent].parent;
            index = uses[index].parent;
        }
        return index;
    }

    void Join(u32 a, u32 b) {
        uses[Find(b)].parent = Find(a);
    }

    boost::container::small_vector<SharpUse, 32> uses;
};

GuardGroups BuildResourceGuards(IR::Program& program, const ResourceDiscoveryList& resources,
                                GuardAtoms& atoms) {
    if (program.info.skip_resource_guards) {
        return {};
    }
    IR::ComputeDominators(program.post_order_blocks);
    GuardBuilder builder{atoms};
    GuardCollector collector{builder};
    SharpUses sharp_uses;
    boost::container::small_vector<GuardTerms, 4> lod_terms;
    for (const ResourceDiscovery& resource : resources) {
        IR::Inst* const user = resource.user;
        if (!IsImageInstruction(*user)) {
            continue;
        }
        const GuardTerms terms = collector.BlockTerms(user->GetParent());
        const bool is_written =
            user->GetOpcode() == IR::Opcode::ImageWrite || IsImageAtomicInstruction(*user);
        sharp_uses.Add(user, 0, resource.sharps[0], is_written ? GuardTerms::Always() : terms,
                       resource.sharps[0].post_op != SharpFetchPostOp::ConvertCubeTo2DArray);
        if (user->GetOpcode() == IR::Opcode::ImageSampleRaw) {
            sharp_uses.Add(user, 1, resource.sharps[1], terms, true);
        } else if (user->GetOpcode() == IR::Opcode::ImageQueryLod) {
            lod_terms.push_back(terms);
        }
    }
    for (IR::Block* const block : program.blocks) {
        block->immediate_dominator = nullptr;
    }
    for (const GuardTerms& terms : lod_terms) {
        sharp_uses.AddTermsToSamplers(terms);
    }
    return sharp_uses.Build(program);
}

void EmitGuardOperand(Xbyak::CodeGenerator& c, const Xbyak::Reg32& reg,
                      const GuardOperand& operand) {
    if (operand.location == UNKNOWN_LOCATION) {
        c.mov(reg, operand.immediate);
        return;
    }
    c.mov(reg, dword[rsi + (static_cast<u32>(operand.location) << 2)]);
    if (operand.shift != 0) {
        c.shr(reg, operand.shift);
    }
    if (operand.mask != ~0U) {
        c.and_(reg, operand.mask);
    }
}

void EmitNormalOrZeroCheck(Xbyak::CodeGenerator& c, const Xbyak::Reg32& reg,
                           const Xbyak::Label& unknown) {
    Xbyak::Label checked;
    c.mov(edx, reg);
    c.and_(edx, 0x7F800000);
    c.cmp(edx, 0x7F800000);
    c.je(unknown, Xbyak::CodeGenerator::T_NEAR);
    c.test(edx, edx);
    c.jnz(checked);
    c.test(reg, 0x7FFFFF);
    c.jnz(unknown, Xbyak::CodeGenerator::T_NEAR);
    c.L(checked);
}

void EmitCompareResult(Xbyak::CodeGenerator& c, GuardCompare op) {
    switch (op) {
    case GuardCompare::IEq:
    case GuardCompare::FEq:
        c.sete(dl);
        break;
    case GuardCompare::INe:
    case GuardCompare::FNe:
        c.setne(dl);
        break;
    case GuardCompare::SLt:
        c.setl(dl);
        break;
    case GuardCompare::SLe:
        c.setle(dl);
        break;
    case GuardCompare::ULt:
    case GuardCompare::FLt:
        c.setb(dl);
        break;
    case GuardCompare::ULe:
    case GuardCompare::FLe:
        c.setbe(dl);
        break;
    }
}

void EmitResourceGuards(Xbyak::CodeGenerator& c, const GuardAtoms& atoms,
                        const GuardGroups& groups) {
    u64 used_atoms{};
    for (const GuardGroup& group : groups) {
        for (const u64 term : group.terms.terms) {
            used_atoms |= term;
        }
    }
    if (used_atoms == 0) {
        return;
    }
    c.xor_(r8d, r8d);
    for (u32 i = 0; i < atoms.size(); ++i) {
        const GuardAtom& atom = atoms[i];
        const bool is_float = atom.op >= GuardCompare::FEq;
        const auto is_known = [is_float](const GuardOperand& operand) {
            return !is_float || operand.location != UNKNOWN_LOCATION ||
                   IsNormalOrZero(operand.immediate);
        };
        if (((used_atoms >> i) & 1) == 0 || !is_known(atom.lhs) || !is_known(atom.rhs)) {
            continue;
        }
        Xbyak::Label next;
        EmitGuardOperand(c, eax, atom.lhs);
        EmitGuardOperand(c, ecx, atom.rhs);
        if (is_float) {
            if (atom.lhs.location != UNKNOWN_LOCATION) {
                EmitNormalOrZeroCheck(c, eax, next);
            }
            if (atom.rhs.location != UNKNOWN_LOCATION) {
                EmitNormalOrZeroCheck(c, ecx, next);
            }
            c.movd(xmm0, eax);
            c.movd(xmm1, ecx);
            c.ucomiss(xmm0, xmm1);
        } else {
            c.cmp(eax, ecx);
        }
        EmitCompareResult(c, atom.op);
        c.cmp(dl, atom.negate ? 1 : 0);
        c.jne(next);
        c.bts(r8, static_cast<u8>(i));
        c.L(next);
    }
    Xbyak::Label done;
    c.test(r8, r8);
    c.jz(done, Xbyak::CodeGenerator::T_NEAR);
    for (const GuardGroup& group : groups) {
        Xbyak::Label live;
        for (const u64 term : group.terms.terms) {
            c.mov(rax, term);
            c.test(rax, r8);
            c.jz(live, Xbyak::CodeGenerator::T_NEAR);
        }
        for (const SharpLocation slot : group.slots) {
            c.mov(dword[rsi + (static_cast<u32>(slot) << 2)], 0);
        }
        c.L(live);
    }
    c.L(done);
}

static void StartSrtProgram(Info& info) {
    Xbyak::CodeGenerator& c = g_srt_codegen;

    // Register the signal handler for SRT walker, if not already registered
    if (g_srt_codegen_start == nullptr) {
        g_srt_codegen_start = c.getCurr();
        auto* signals = Core::Signals::Instance();
        // Call after the memory invalidation handler
        constexpr u32 priority = 1;
        signals->RegisterAccessViolationHandler(SrtWalkerSignalHandler, priority);
    }

    info.srt_info.walker_func = c.getCurr<PFN_SrtWalker>();
}

static void GenerateSrtProgram(Info& info, PassInfo& pass_info) {
    Xbyak::CodeGenerator& c = g_srt_codegen;

    if (pass_info.srt_roots.empty()) {
        return;
    }

    StartSrtProgram(info);
    pass_info.dst_off_dw = NUM_USER_DATA_REGS;
    ASSERT(pass_info.dst_off_dw == info.srt_info.flattened_bufsize_dw);

    for (const auto& [sgpr_base, root] : pass_info.srt_roots) {
        VisitPointer(IR::Value(static_cast<u32>(sgpr_base)), root, pass_info, c);
    }

    info.srt_info.flattened_bufsize_dw = pass_info.dst_off_dw;
}

static void FinishSrtProgram(Info& info, const GuardAtoms& atoms, const GuardGroups& groups) {
    Xbyak::CodeGenerator& c = g_srt_codegen;

    if (!info.srt_info.walker_func) {
        if (groups.empty()) {
            return;
        }
        StartSrtProgram(info);
    }

    EmitResourceGuards(c, atoms, groups);
    c.ret();
    c.ready();

    info.srt_info.walker_func_size =
        c.getCurr() - reinterpret_cast<const u8*>(info.srt_info.walker_func);

    if (EmulatorSettings.IsDumpShaders()) {
        DumpSrtProgram(info, reinterpret_cast<const u8*>(info.srt_info.walker_func),
                       info.srt_info.walker_func_size);
    }
}

static bool IsReadConstSource(const IR::Value base) {
    auto* inst = base.TryInst();
    if (!inst) {
        return false;
    }
    return inst->GetOpcode() == IR::Opcode::GetUserData ||
           inst->GetOpcode() == IR::Opcode::ReadConst ||
           inst->GetOpcode() == IR::Opcode::ReadConstBuffer;
}

void SimplifyReadConstAddressAdd(IR::Inst& inst) {
    // This handles the following pattern by combining the addition with the offset
    // s_add_u32    s12, s2, <offset>
    // s_addc_u32   s13, s3, 0
    // s_load_dword s16, s[12:13], 0x0

    if (inst.GetOpcode() != IR::Opcode::ReadConst) {
        return;
    }
    IR::Inst* addr = inst.Arg(0).Inst();
    IR::Inst* hi = addr->Arg(1).TryInst();
    IR::Inst* lo = addr->Arg(0).TryInst();
    if (!hi || !lo) {
        return;
    }

    if (lo->GetOpcode() != IR::Opcode::IAdd32 || hi->GetOpcode() != IR::Opcode::IAdd32) {
        return;
    }

    IR::Value add_offset;
    IR::Value base;
    if (base = lo->Arg(0); IsReadConstSource(base)) {
        add_offset = lo->Arg(1);
    } else if (base = lo->Arg(1); IsReadConstSource(base)) {
        add_offset = lo->Arg(0);
    } else {
        return;
    }

    IR::Inst* sel = hi->Arg(1).TryInst();
    if (!sel || sel->GetOpcode() != IR::Opcode::SelectU32) {
        return;
    }

    IR::Value sel_true = sel->Arg(1);
    IR::Value sel_false = sel->Arg(2);
    if (!sel_true.IsImmediate() || !sel_false.IsImmediate() || sel_true.U32() != 1 ||
        sel_false.U32() != 0) {
        return;
    }

    addr->SetArg(0, base);
    addr->SetArg(1, hi->Arg(0));
    for (auto [user, operand] : addr->Uses()) {
        ASSERT(user->GetOpcode() == IR::Opcode::ReadConst && operand == 0);
        IR::IREmitter ir{*user->GetParent(), IR::Block::InstructionList::s_iterator_to(*user)};
        IR::U32 offset = IR::U32{user->Arg(1)};
        IR::U32 dw_add_offset =
            add_offset.IsImmediate()
                ? ir.Imm32(add_offset.U32() >> 2u)
                : IR::U32{ir.ShiftRightLogical(IR::U32{add_offset}, ir.Imm32(2u))};
        if (offset.IsImmediate() && dw_add_offset.IsImmediate()) {
            user->SetArg(1, ir.Imm32(offset.U32() + dw_add_offset.U32()));
        } else {
            user->SetArg(1, ir.IAdd(offset, dw_add_offset));
        }
    }
}

} // Anonymous namespace

void FlattenExtendedUserdataPass(IR::Program& program, const ResourceDiscoveryList& resources) {
    auto& post_order = program.post_order_blocks;
    PassInfo pass_info;

    // traverse at end and assign offsets to duplicate readconsts, using
    // vn_to_inst as the source
    boost::container::small_vector<IR::Inst*, 32> all_readconsts, visited;
    std::queue<IR::Inst*> queue;

    for (auto it = post_order.rbegin(); it != post_order.rend(); it++) {
        IR::Block* block{*it};
        for (IR::Inst& inst : *block) {
            if (inst.GetOpcode() != IR::Opcode::ReadConst &&
                inst.GetOpcode() != IR::Opcode::ReadConstBuffer) {
                continue;
            }

            if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
                // Only flatten ReadConstBuffer if it was marked as a sharp source in the
                // resource discovery pass
                auto inst_info = inst.Flags<IR::BufferInstInfo>();
                if (!inst_info.sharp_source) {
                    continue;
                }
            }

            SimplifyReadConstAddressAdd(inst);

            all_readconsts.push_back(&inst);

            auto offset = inst.Arg(1);
            if (offset.IsImmediate()) {
                continue;
            }

            queue.push(offset.Inst());
            while (!queue.empty()) {
                IR::Inst* inst{queue.front()};
                queue.pop();

                if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
                    auto buffer_inst_info = inst->Flags<IR::BufferInstInfo>();
                    if (!buffer_inst_info.sharp_source.Value()) {
                        all_readconsts.push_back(inst);
                        auto offset = inst->Arg(1);
                        if (!offset.IsImmediate()) {
                            auto arg_inst = offset.Inst();
                            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                                visited.push_back(arg_inst);
                                queue.push(arg_inst);
                            }
                        }
                    }
                    continue;
                }
                if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                    inst->GetOpcode() == IR::Opcode::ReadConst ||
                    !IsAllowedOffsetInstruction(inst)) {
                    continue;
                }
                for (size_t arg = inst->NumArgs(); arg--;) {
                    const IR::Value arg_value = inst->Arg(arg);
                    if (arg_value.IsImmediate()) {
                        continue;
                    }
                    IR::Inst* arg_inst = arg_value.Inst();
                    if (std::ranges::find(visited, arg_inst) == visited.end()) {
                        visited.push_back(arg_inst);
                        queue.push(arg_inst);
                    }
                }
            }
        }
    }

    for (auto inst : all_readconsts) {
        if (pass_info.DeduplicateInstruction(inst) != inst) {
            // This is a duplicate of a readconst we've already visited
            continue;
        }

        IR::Inst* base = inst->Arg(0).Inst();
        if (auto* inst = base->Arg(0).TryInst();
            inst && inst->GetOpcode() == IR::Opcode::ReadFirstLane) {
            continue;
        }
        ASSERT_MSG(IsReadConstSource(base->Arg(0)), "ReadConst base low not from constant memory");
        ASSERT_MSG(IsReadConstSource(base->Arg(1)), "ReadConst base high not from constant memory");

        IR::Inst* ptr_lo = base->Arg(0).Inst();
        ptr_lo = pass_info.DeduplicateInstruction(ptr_lo);

        auto ptr_uses_kv = pass_info.pointer_uses.try_emplace(ptr_lo, PassInfo::PtrUserList{});
        PassInfo::PtrUserList& user_list = ptr_uses_kv.first->second;

        user_list[inst->Arg(1)] = inst;

        if (ptr_lo->GetOpcode() == IR::Opcode::GetUserData) {
            IR::ScalarReg ud_reg = ptr_lo->Arg(0).ScalarReg();
            pass_info.srt_roots[ud_reg] = ptr_lo;
        }
    }

    GenerateSrtProgram(program.info, pass_info);

    // Assign offsets to duplicate readconsts
    for (IR::Inst* readconst : all_readconsts) {
        ASSERT(pass_info.vn_to_inst.contains(pass_info.gvn_table.GetValueNumber(readconst)));
        IR::Inst* original = pass_info.DeduplicateInstruction(readconst);
        SetFlatbufOffset(readconst, GetFlatbufOffset(original));
    }

    GuardAtoms atoms;
    const GuardGroups groups = BuildResourceGuards(program, resources, atoms);
    FinishSrtProgram(program.info, atoms, groups);

    program.info.RefreshFlatBuf();
}

} // namespace Shader::Optimization

#else

namespace Shader {

PFN_SrtWalker RegisterWalkerCode(const u8* ptr, size_t size) {
    UNREACHABLE_MSG("RegisterWalkerCode unimplemented for target architecture.");
}

namespace Optimization {

void FlattenExtendedUserdataPass(IR::Program& program, const ResourceDiscoveryList& resources) {
    UNREACHABLE_MSG("FlattenExtendedUserdataPass unimplemented for target architecture.");
}

} // namespace Optimization

} // namespace Shader

#endif
