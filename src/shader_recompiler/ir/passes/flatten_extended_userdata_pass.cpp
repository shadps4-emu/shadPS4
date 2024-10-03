
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/srt.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"
#include "src/common/arch.h"
#include "src/common/decoder.h"

using namespace Xbyak::util;

// TODO make sure no problems with identity and Insts being used in maps

// TODO refactor, copied from signals.cpp
static void DumpSrtProgram(const Shader::Info& info, const u8* code, size_t codesize) {
#ifdef ARCH_X86_64
    using namespace Common::FS;

    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename =
        fmt::format("{}_{:#018x}_{}.srtprogram.txt", info.stage, info.pgm_hash, info.perm_idx);
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

namespace {
class SrtCodegen : public Xbyak::CodeGenerator {
public:
    SrtCodegen() : CodeGenerator(1_MB) {}
};

using namespace Shader;
struct PassInfo {
    // map offset to inst
    using PtrUserList = boost::container::map<u32, Shader::IR::Inst*>;

    // keys are GetUserData or ReadConst instructions that are used as pointers
    std::unordered_map<IR::Inst*, PtrUserList> pointer_uses;
    // GetUserData instructions corresponding to sgpr_base of SRT roots
    boost::container::map<IR::ScalarReg, IR::Inst*> srt_roots;

    u32 dst_off_dw;

    PtrUserList* GetUsesAsPointer(IR::Inst* inst) {
        auto it = pointer_uses.find(inst);
        if (it != pointer_uses.end()) {
            return &it->second;
        }
        return nullptr;
    }
};
} // namespace

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

static void VisitPointer(u32 off_dw, IR::Inst* subtree, PassInfo& pass_info,
                         Xbyak::CodeGenerator& c) {
    PushPtr(c, off_dw);
    PassInfo::PtrUserList* use_list = pass_info.GetUsesAsPointer(subtree);
    ASSERT(use_list);

    // First copy all the src data from this tree level
    // That way, all data that was contiguous in the guest SRT is also contiguous in the
    // flattened buffer.
    // TODO src and dst are contiguous. Optimize with wider loads/stores
    // TODO if this subtree is dynamically indexed, don't compact it (keep it sparse)
    for (auto [src_off_dw, use] : *use_list) {
        c.mov(r10d, ptr[rdi + (src_off_dw << 2)]);
        c.mov(ptr[rsi + (pass_info.dst_off_dw << 2)], r10d);

        use->SetFlags<u32>(pass_info.dst_off_dw);
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

static void GenerateSrtProgram(Info& info, PassInfo& pass_info) {
    Xbyak::CodeGenerator& c = *Common::Singleton<SrtCodegen>::Instance();

    if (info.srt_info.srt_reservations.empty() && pass_info.srt_roots.empty()) {
        return;
    }

    pass_info.dst_off_dw = NumUserDataRegs;

    // Special case for V# step rate buffers in fetch shader
    for (auto i = 0; i < info.srt_info.srt_reservations.size(); i++) {
        PersistentSrtInfo::SrtSharpReservation res = info.srt_info.srt_reservations[i];
        // get pointer to V#
        c.mov(r10d, ptr[rdi + (res.sgpr_base << 2)]);

        u32 src_off = res.dword_offset << 2;

        for (auto j = 0; j < res.num_dwords; j++) {
            c.mov(r11d, ptr[r10d + src_off]);
            c.mov(ptr[rsi + (pass_info.dst_off_dw << 2)], r11d);

            src_off += 4;
            ++pass_info.dst_off_dw;
        }
    }

    ASSERT(pass_info.dst_off_dw == info.srt_info.flattened_bufsize_dw);

    for (const auto& [sgpr_base, root] : pass_info.srt_roots) {
        VisitPointer(static_cast<u32>(sgpr_base), root, pass_info, c);
    }

    c.ret();
    c.ready();

    size_t codesize = c.getSize();
    info.srt_info.walker = SmallCodeArray(c.getCode(), codesize);

    if (Config::dumpShaders()) {
        DumpSrtProgram(info, c.getCode(), codesize);
    }

    c.reset();

    info.srt_info.flattened_bufsize_dw = pass_info.dst_off_dw;
}

}; // namespace

void FlattenExtendedUserdataPass(IR::Program& program) {
    Shader::Info& info = program.info;
    PassInfo srt_info;

    // HoistConstantReads should have put all SRT reads into the entry block
    IR::Block* entry_bb = *program.blocks.begin();

    for (IR::Inst& inst : *entry_bb) {
        if (inst.GetOpcode() == IR::Opcode::ReadConst) {
            if (!GetReadConstOff(&inst).IsImmediate()) {
                continue;
            }

            IR::Inst* ptr_composite = inst.Arg(0).InstRecursive();

            const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
                if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                    inst->GetOpcode() == IR::Opcode::ReadConst) {
                    return inst;
                }
                return std::nullopt;
            };
            auto base0 = IR::BreadthFirstSearch(ptr_composite->Arg(0), pred);
            auto base1 = IR::BreadthFirstSearch(ptr_composite->Arg(1), pred);
            ASSERT_MSG(base0 && base1 && "ReadConst not from constant memory");

            // TODO this probably requires some template magic to fix. BFS needs non-const variant
            // Needs to be non-const to change flags
            IR::Inst* ptr_lo = const_cast<IR::Inst*>(base0.value());

            auto it = srt_info.pointer_uses.try_emplace(ptr_lo, PassInfo::PtrUserList{});
            PassInfo::PtrUserList& user_list = it.first->second;

            user_list[GetReadConstOff(&inst).U32()] = &inst;

            if (ptr_lo->GetOpcode() == IR::Opcode::GetUserData) {
                IR::ScalarReg ud_reg = GetUserDataSgprBase(ptr_lo);
                srt_info.srt_roots[ud_reg] = ptr_lo;
            }
        }
    }

    GenerateSrtProgram(info, srt_info);
}

} // namespace Shader::Optimization