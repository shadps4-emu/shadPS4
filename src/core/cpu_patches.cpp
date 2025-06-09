// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <Zydis/Zydis.h>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/decoder.h"
#include "common/signal_context.h"
#include "common/types.h"
#include "core/signals.h"
#include "core/tls.h"
#include "cpu_patches.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

using namespace Xbyak::util;

namespace Core {

static Xbyak::Reg ZydisToXbyakRegister(const ZydisRegister reg) {
    if (reg >= ZYDIS_REGISTER_EAX && reg <= ZYDIS_REGISTER_R15D) {
        return Xbyak::Reg32(reg - ZYDIS_REGISTER_EAX + Xbyak::Operand::EAX);
    }
    if (reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15) {
        return Xbyak::Reg64(reg - ZYDIS_REGISTER_RAX + Xbyak::Operand::RAX);
    }
    if (reg >= ZYDIS_REGISTER_XMM0 && reg <= ZYDIS_REGISTER_XMM31) {
        return Xbyak::Xmm(reg - ZYDIS_REGISTER_XMM0 + xmm0.getIdx());
    }
    if (reg >= ZYDIS_REGISTER_YMM0 && reg <= ZYDIS_REGISTER_YMM31) {
        return Xbyak::Ymm(reg - ZYDIS_REGISTER_YMM0 + ymm0.getIdx());
    }
    UNREACHABLE_MSG("Unsupported register: {}", static_cast<u32>(reg));
}

static Xbyak::Reg ZydisToXbyakRegisterOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_REGISTER,
               "Expected register operand, got type: {}", static_cast<u32>(operand.type));

    return ZydisToXbyakRegister(operand.reg.value);
}

static Xbyak::Address ZydisToXbyakMemoryOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand, got type: {}",
               static_cast<u32>(operand.type));

    if (operand.mem.base == ZYDIS_REGISTER_RIP) {
        return ptr[rip + operand.mem.disp.value];
    }

    Xbyak::RegExp expression{};
    if (operand.mem.base != ZYDIS_REGISTER_NONE) {
        expression = expression + ZydisToXbyakRegister(operand.mem.base);
    }
    if (operand.mem.index != ZYDIS_REGISTER_NONE) {
        if (operand.mem.scale != 0) {
            expression = expression + ZydisToXbyakRegister(operand.mem.index) * operand.mem.scale;
        } else {
            expression = expression + ZydisToXbyakRegister(operand.mem.index);
        }
    }
    if (operand.mem.disp.size != 0 && operand.mem.disp.value != 0) {
        expression = expression + operand.mem.disp.value;
    }

    return ptr[expression];
}

static bool FilterTcbAccess(const ZydisDecodedOperand* operands) {
    const auto& dst_op = operands[0];
    const auto& src_op = operands[1];

    // Patch only 'mov (64-bit register), fs:[0]'
    return src_op.type == ZYDIS_OPERAND_TYPE_MEMORY && src_op.mem.segment == ZYDIS_REGISTER_FS &&
           src_op.mem.base == ZYDIS_REGISTER_NONE && src_op.mem.index == ZYDIS_REGISTER_NONE &&
           src_op.mem.disp.value == 0 && dst_op.reg.value >= ZYDIS_REGISTER_RAX &&
           dst_op.reg.value <= ZYDIS_REGISTER_R15;
}

static void GenerateTcbAccess(void* /* address */, const ZydisDecodedOperand* operands,
                              Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

#if defined(_WIN32)
    // The following logic is based on the Kernel32.dll asm of TlsGetValue
    static constexpr u32 TlsSlotsOffset = 0x1480;
    static constexpr u32 TlsExpansionSlotsOffset = 0x1780;
    static constexpr u32 TlsMinimumAvailable = 64;

    const auto slot = GetTcbKey();

    // Load the pointer to the table of TLS slots.
    c.putSeg(gs);
    if (slot < TlsMinimumAvailable) {
        // Load the pointer to TLS slots.
        c.mov(dst, ptr[reinterpret_cast<void*>(TlsSlotsOffset + slot * sizeof(LPVOID))]);
    } else {
        const u32 tls_index = slot - TlsMinimumAvailable;

        // Load the pointer to the table of TLS expansion slots.
        c.mov(dst, ptr[reinterpret_cast<void*>(TlsExpansionSlotsOffset)]);
        // Load the pointer to our buffer.
        c.mov(dst, qword[dst + tls_index * sizeof(LPVOID)]);
    }
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs read with gs read.
    c.putSeg(gs);
    c.mov(dst, src);
#endif
}

static bool FilterNoSSE4a(const ZydisDecodedOperand*) {
    Cpu cpu;
    return !cpu.has(Cpu::tSSE4a);
}

static void GenerateEXTRQ(void* /* address */, const ZydisDecodedOperand* operands,
                          Xbyak::CodeGenerator& c) {
    bool immediateForm = operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                         operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;

    ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER, "operand 0 must be a register");

    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

    ASSERT_MSG(dst.isXMM(), "operand 0 must be an XMM register");

    Xbyak::Xmm xmm_dst = *reinterpret_cast<const Xbyak::Xmm*>(&dst);

    if (immediateForm) {
        u8 length = operands[1].imm.value.u & 0x3F;
        u8 index = operands[2].imm.value.u & 0x3F;

        LOG_DEBUG(Core, "Patching immediate form EXTRQ, length: {}, index: {}", length, index);

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;

        // Set rsp to before red zone and save scratch registers
        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);

        u64 mask;
        if (length == 0) {
            length = 64; // for the check below
            mask = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask = (1ULL << length) - 1;
        }

        ASSERT_MSG(length + index <= 64, "length + index must be less than or equal to 64.");

        // Get lower qword from xmm register
        c.vmovq(scratch1, xmm_dst);

        if (index != 0) {
            c.shr(scratch1, index);
        }

        // We need to move mask to a register because we can't use all the possible
        // immediate values with `and reg, imm32`
        c.mov(scratch2, mask);
        c.and_(scratch1, scratch2);

        // Writeback to xmm register, extrq instruction says top 64-bits are undefined so we don't
        // care to preserve them
        c.vmovq(xmm_dst, scratch1);

        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    } else {
        ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                       operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                       operands[0].reg.value >= ZYDIS_REGISTER_XMM0 &&
                       operands[0].reg.value <= ZYDIS_REGISTER_XMM15 &&
                       operands[1].reg.value >= ZYDIS_REGISTER_XMM0 &&
                       operands[1].reg.value <= ZYDIS_REGISTER_XMM15,
                   "Unexpected operand types for EXTRQ instruction");

        const auto src = ZydisToXbyakRegisterOperand(operands[1]);

        ASSERT_MSG(src.isXMM(), "operand 1 must be an XMM register");

        Xbyak::Xmm xmm_src = *reinterpret_cast<const Xbyak::Xmm*>(&src);

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 mask = rdx;

        Xbyak::Label length_zero, end;

        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(mask);

        // Construct the mask out of the length that resides in bottom 6 bits of source xmm
        c.vmovq(scratch1, xmm_src);
        c.mov(scratch2, scratch1);
        c.and_(scratch2, 0x3F);
        c.jz(length_zero);

        // mask = (1ULL << length) - 1
        c.mov(mask, 1);
        c.shl(mask, cl);
        c.dec(mask);
        c.jmp(end);

        c.L(length_zero);
        c.mov(mask, 0xFFFF'FFFF'FFFF'FFFF);

        c.L(end);

        // Get the shift amount and store it in scratch2
        c.shr(scratch1, 8);
        c.and_(scratch1, 0x3F);
        c.mov(scratch2, scratch1); // cl now contains the shift amount

        c.vmovq(scratch1, xmm_dst);
        c.shr(scratch1, cl);
        c.and_(scratch1, mask);
        c.vmovq(xmm_dst, scratch1);

        c.pop(mask);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    }
}

static void GenerateINSERTQ(void* /* address */, const ZydisDecodedOperand* operands,
                            Xbyak::CodeGenerator& c) {
    bool immediateForm = operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                         operands[3].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;

    ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER,
               "operands 0 and 1 must be registers.");

    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakRegisterOperand(operands[1]);

    ASSERT_MSG(dst.isXMM() && src.isXMM(), "operands 0 and 1 must be xmm registers.");

    Xbyak::Xmm xmm_dst = *reinterpret_cast<const Xbyak::Xmm*>(&dst);
    Xbyak::Xmm xmm_src = *reinterpret_cast<const Xbyak::Xmm*>(&src);

    if (immediateForm) {
        u8 length = operands[2].imm.value.u & 0x3F;
        u8 index = operands[3].imm.value.u & 0x3F;

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 mask = rdx;

        // Set rsp to before red zone and save scratch registers
        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(mask);

        u64 mask_value;
        if (length == 0) {
            length = 64; // for the check below
            mask_value = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask_value = (1ULL << length) - 1;
        }

        ASSERT_MSG(length + index <= 64, "length + index must be less than or equal to 64.");

        c.vmovq(scratch1, xmm_src);
        c.vmovq(scratch2, xmm_dst);
        c.mov(mask, mask_value);

        // src &= mask
        c.and_(scratch1, mask);

        // src <<= index
        c.shl(scratch1, index);

        // dst &= ~(mask << index)
        mask_value = ~(mask_value << index);
        c.mov(mask, mask_value);
        c.and_(scratch2, mask);

        // dst |= src
        c.or_(scratch2, scratch1);

        // Insert scratch2 into low 64 bits of dst, upper 64 bits are unaffected
        c.vpinsrq(xmm_dst, xmm_dst, scratch2, 0);

        c.pop(mask);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    } else {
        ASSERT_MSG(operands[2].type == ZYDIS_OPERAND_TYPE_UNUSED &&
                       operands[3].type == ZYDIS_OPERAND_TYPE_UNUSED,
                   "operands 2 and 3 must be unused for register form.");

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 index = rdx;
        const Xbyak::Reg64 mask = rbx;

        Xbyak::Label length_zero, end;

        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(index);
        c.push(mask);

        // Get upper 64 bits of src and copy it to mask and index
        c.vpextrq(index, xmm_src, 1);
        c.mov(mask, index);

        // When length is 0, set it to 64
        c.and_(mask, 0x3F); // mask now holds the length
        c.jz(length_zero);  // Check if length is 0 and set mask to all 1s if it is

        // Create a mask out of the length
        c.mov(cl, mask.cvt8());
        c.mov(mask, 1);
        c.shl(mask, cl);
        c.dec(mask);
        c.jmp(end);

        c.L(length_zero);
        c.mov(mask, 0xFFFF'FFFF'FFFF'FFFF);

        c.L(end);
        // Get index to insert at
        c.shr(index, 8);
        c.and_(index, 0x3F);

        // src &= mask
        c.vmovq(scratch1, xmm_src);
        c.and_(scratch1, mask);

        // mask = ~(mask << index)
        c.mov(cl, index.cvt8());
        c.shl(mask, cl);
        c.not_(mask);

        // src <<= index
        c.shl(scratch1, cl);

        // dst = (dst & mask) | src
        c.vmovq(scratch2, xmm_dst);
        c.and_(scratch2, mask);
        c.or_(scratch2, scratch1);

        // Upper 64 bits are undefined in insertq
        c.vmovq(xmm_dst, scratch2);

        c.pop(mask);
        c.pop(index);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    }
}

static void ReplaceMOVNT(void* address, u8 rep_prefix) {
    // Find the opcode byte
    // There can be any amount of prefixes but the instruction can't be more than 15 bytes
    // And we know for sure this is a MOVNTSS/MOVNTSD
    bool found = false;
    bool rep_prefix_found = false;
    int index = 0;
    u8* ptr = reinterpret_cast<u8*>(address);
    for (int i = 0; i < 15; i++) {
        if (ptr[i] == rep_prefix) {
            rep_prefix_found = true;
        } else if (ptr[i] == 0x2B) {
            index = i;
            found = true;
            break;
        }
    }

    // Some sanity checks
    ASSERT(found);
    ASSERT(index >= 2);
    ASSERT(ptr[index - 1] == 0x0F);
    ASSERT(rep_prefix_found);

    // This turns the MOVNTSS/MOVNTSD to a MOVSS/MOVSD m, xmm
    ptr[index] = 0x11;
}

static void ReplaceMOVNTSS(void* address, const ZydisDecodedOperand*, Xbyak::CodeGenerator&) {
    ReplaceMOVNT(address, 0xF3);
}

static void ReplaceMOVNTSD(void* address, const ZydisDecodedOperand*, Xbyak::CodeGenerator&) {
    ReplaceMOVNT(address, 0xF2);
}

using PatchFilter = bool (*)(const ZydisDecodedOperand*);
using InstructionGenerator = void (*)(void*, const ZydisDecodedOperand*, Xbyak::CodeGenerator&);
struct PatchInfo {
    /// Filter for more granular patch conditions past just the instruction mnemonic.
    PatchFilter filter;

    /// Generator for the patch/trampoline.
    InstructionGenerator generator;

    /// Whether to use a trampoline for this patch.
    bool trampoline;
};

static const std::unordered_map<ZydisMnemonic, PatchInfo> Patches = {
    // SSE4a
    {ZYDIS_MNEMONIC_EXTRQ, {FilterNoSSE4a, GenerateEXTRQ, true}},
    {ZYDIS_MNEMONIC_INSERTQ, {FilterNoSSE4a, GenerateINSERTQ, true}},
    {ZYDIS_MNEMONIC_MOVNTSS, {FilterNoSSE4a, ReplaceMOVNTSS, false}},
    {ZYDIS_MNEMONIC_MOVNTSD, {FilterNoSSE4a, ReplaceMOVNTSD, false}},

#if defined(_WIN32)
    // Windows needs a trampoline.
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, true}},
#elif !defined(__APPLE__)
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, false}},
#endif
};

static std::once_flag init_flag;

struct PatchModule {
    /// Mutex controlling access to module code regions.
    std::mutex mutex{};

    /// Start of the module.
    u8* start;

    /// End of the module.
    u8* end;

    /// Tracker for patched code locations.
    std::set<u8*> patched;

    /// Code generator for patching the module.
    Xbyak::CodeGenerator patch_gen;

    /// Code generator for writing trampoline patches.
    Xbyak::CodeGenerator trampoline_gen;

    PatchModule(u8* module_ptr, const u64 module_size, u8* trampoline_ptr,
                const u64 trampoline_size)
        : start(module_ptr), end(module_ptr + module_size), patch_gen(module_size, module_ptr),
          trampoline_gen(trampoline_size, trampoline_ptr) {}
};
static std::map<u64, PatchModule> modules;

static PatchModule* GetModule(const void* ptr) {
    auto upper_bound = modules.upper_bound(reinterpret_cast<u64>(ptr));
    if (upper_bound == modules.begin()) {
        return nullptr;
    }
    return &(std::prev(upper_bound)->second);
}

/// Returns a boolean indicating whether the instruction was patched, and the offset to advance past
/// whatever is at the current code pointer.
static std::pair<bool, u64> TryPatch(u8* code, PatchModule* module) {
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    const auto status = Common::Decoder::Instance()->decodeInstruction(instruction, operands, code,
                                                                       module->end - code);
    if (!ZYAN_SUCCESS(status)) {
        return std::make_pair(false, 1);
    }

    if (Patches.contains(instruction.mnemonic)) {
        const auto& patch_info = Patches.at(instruction.mnemonic);
        bool needs_trampoline = patch_info.trampoline;
        if (patch_info.filter(operands)) {
            auto& patch_gen = module->patch_gen;

            if (needs_trampoline && instruction.length < 5) {
                // Trampoline is needed but instruction is too short to patch.
                // Return false and length to signal to AOT compilation that this instruction
                // should be skipped and handled at runtime.
                return std::make_pair(false, instruction.length);
            }

            // Reset state and move to current code position.
            patch_gen.reset();
            patch_gen.setSize(code - patch_gen.getCode());

            if (needs_trampoline) {
                auto& trampoline_gen = module->trampoline_gen;
                const auto trampoline_ptr = trampoline_gen.getCurr();

                patch_info.generator(code, operands, trampoline_gen);

                // Return to the following instruction at the end of the trampoline.
                trampoline_gen.jmp(code + instruction.length);

                // Replace instruction with near jump to the trampoline.
                patch_gen.jmp(trampoline_ptr, Xbyak::CodeGenerator::LabelType::T_NEAR);
            } else {
                patch_info.generator(code, operands, patch_gen);
            }

            const auto patch_size = patch_gen.getCurr() - code;
            if (patch_size > 0) {
                ASSERT_MSG(instruction.length >= patch_size,
                           "Instruction {} with length {} is too short to replace at: {}",
                           ZydisMnemonicGetString(instruction.mnemonic), instruction.length,
                           fmt::ptr(code));

                // Fill remaining space with nops.
                patch_gen.nop(instruction.length - patch_size);

                module->patched.insert(code);
                LOG_DEBUG(Core, "Patched instruction '{}' at: {}",
                          ZydisMnemonicGetString(instruction.mnemonic), fmt::ptr(code));
                return std::make_pair(true, instruction.length);
            }
        }
    }

    return std::make_pair(false, instruction.length);
}

#if defined(ARCH_X86_64)

static bool Is4ByteExtrqOrInsertq(void* code_address) {
    u8* bytes = (u8*)code_address;
    if (bytes[0] == 0x66 && bytes[1] == 0x0F && bytes[2] == 0x79) {
        return true; // extrq
    } else if (bytes[0] == 0xF2 && bytes[1] == 0x0F && bytes[2] == 0x79) {
        return true; // insertq
    } else {
        return false;
    }
}

static bool TryExecuteIllegalInstruction(void* ctx, void* code_address) {
    // We need to decode the instruction to find out what it is. Normally we'd use a fully fleshed
    // out decoder like Zydis, however Zydis does a bunch of stuff that impact performance that we
    // don't care about. We can get information about the instruction a lot faster by writing a mini
    // decoder here, since we know it is definitely an extrq or an insertq. If for some reason we
    // need to interpret more instructions in the future (I don't see why we would), we can revert
    // to using Zydis.
    ZydisMnemonic mnemonic;
    u8* bytes = (u8*)code_address;
    if (bytes[0] == 0x66) {
        mnemonic = ZYDIS_MNEMONIC_EXTRQ;
    } else if (bytes[0] == 0xF2) {
        mnemonic = ZYDIS_MNEMONIC_INSERTQ;
    } else {
        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        const auto status =
            Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);
        LOG_ERROR(Core, "Unhandled illegal instruction at code address {}: {}",
                  fmt::ptr(code_address),
                  ZYAN_SUCCESS(status) ? ZydisMnemonicGetString(instruction.mnemonic)
                                       : "Failed to decode");
        return false;
    }

    ASSERT(bytes[1] == 0x0F && bytes[2] == 0x79);

    // Note: It's guaranteed that there's no REX prefix in these instructions checked by
    // Is4ByteExtrqOrInsertq
    u8 modrm = bytes[3];
    u8 rm = modrm & 0b111;
    u8 reg = (modrm >> 3) & 0b111;
    u8 mod = (modrm >> 6) & 0b11;

    ASSERT(mod == 0b11); // Any instruction we interpret here uses reg/reg addressing only

    int dstIndex = reg;
    int srcIndex = rm;

    switch (mnemonic) {
    case ZYDIS_MNEMONIC_EXTRQ: {
        const auto dst = Common::GetXmmPointer(ctx, dstIndex);
        const auto src = Common::GetXmmPointer(ctx, srcIndex);

        u64 lowQWordSrc;
        memcpy(&lowQWordSrc, src, sizeof(lowQWordSrc));

        u64 lowQWordDst;
        memcpy(&lowQWordDst, dst, sizeof(lowQWordDst));

        u64 length = lowQWordSrc & 0x3F;
        u64 mask;
        if (length == 0) {
            length = 64; // for the check below
            mask = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask = (1ULL << length) - 1;
        }

        u64 index = (lowQWordSrc >> 8) & 0x3F;
        if (length + index > 64) {
            // Undefined behavior if length + index is bigger than 64 according to the spec,
            // we'll warn and continue execution.
            LOG_TRACE(Core,
                      "extrq at {} with length {} and index {} is bigger than 64, "
                      "undefined behavior",
                      fmt::ptr(code_address), length, index);
        }

        lowQWordDst >>= index;
        lowQWordDst &= mask;

        memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

        Common::IncrementRip(ctx, 4);

        return true;
    }
    case ZYDIS_MNEMONIC_INSERTQ: {
        const auto dst = Common::GetXmmPointer(ctx, dstIndex);
        const auto src = Common::GetXmmPointer(ctx, srcIndex);

        u64 lowQWordSrc, highQWordSrc;
        memcpy(&lowQWordSrc, src, sizeof(lowQWordSrc));
        memcpy(&highQWordSrc, (u8*)src + 8, sizeof(highQWordSrc));

        u64 lowQWordDst;
        memcpy(&lowQWordDst, dst, sizeof(lowQWordDst));

        u64 length = highQWordSrc & 0x3F;
        u64 mask;
        if (length == 0) {
            length = 64; // for the check below
            mask = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask = (1ULL << length) - 1;
        }

        u64 index = (highQWordSrc >> 8) & 0x3F;
        if (length + index > 64) {
            // Undefined behavior if length + index is bigger than 64 according to the spec,
            // we'll warn and continue execution.
            LOG_TRACE(Core,
                      "insertq at {} with length {} and index {} is bigger than 64, "
                      "undefined behavior",
                      fmt::ptr(code_address), length, index);
        }

        lowQWordSrc &= mask;
        lowQWordDst &= ~(mask << index);
        lowQWordDst |= lowQWordSrc << index;

        memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

        Common::IncrementRip(ctx, 4);

        return true;
    }
    default: {
        UNREACHABLE();
    }
    }

    UNREACHABLE();
}
#elif defined(ARCH_ARM64)
// These functions shouldn't be needed for ARM as it will use a JIT so there's no need to patch
// instructions.
static bool TryExecuteIllegalInstruction(void*, void*) {
    return false;
}
#else
#error "Unsupported architecture"
#endif

static bool TryPatchJit(void* code_address) {
    auto* code = static_cast<u8*>(code_address);
    auto* module = GetModule(code);
    if (module == nullptr) {
        return false;
    }

    std::unique_lock lock{module->mutex};

    // Return early if already patched, in case multiple threads signaled at the same time.
    if (std::ranges::find(module->patched, code) != module->patched.end()) {
        return true;
    }

    return TryPatch(code, module).first;
}

static void TryPatchAot(void* code_address, u64 code_size) {
    auto* code = static_cast<u8*>(code_address);
    auto* module = GetModule(code);
    if (module == nullptr) {
        return;
    }

    std::unique_lock lock{module->mutex};

    const auto* end = code + code_size;
    while (code < end) {
        code += TryPatch(code, module).second;
    }
}

static bool PatchesAccessViolationHandler(void* context, void* /* fault_address */) {
    return TryPatchJit(Common::GetRip(context));
}

static bool PatchesIllegalInstructionHandler(void* context) {
    void* code_address = Common::GetRip(context);
    if (Is4ByteExtrqOrInsertq(code_address)) {
        // The instruction is not big enough for a relative jump, don't try to patch it and pass it
        // to our illegal instruction interpreter directly
        return TryExecuteIllegalInstruction(context, code_address);
    } else {
        if (!TryPatchJit(code_address)) {
            ZydisDecodedInstruction instruction;
            ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
            const auto status =
                Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);
            LOG_ERROR(Core, "Failed to patch address {:x} -- mnemonic: {}", (u64)code_address,
                      ZYAN_SUCCESS(status) ? ZydisMnemonicGetString(instruction.mnemonic)
                                           : "Failed to decode");
        }
    }

    return true;
}

static void PatchesInit() {
    if (!Patches.empty()) {
        auto* signals = Signals::Instance();
        // Should be called last.
        constexpr auto priority = std::numeric_limits<u32>::max();
        signals->RegisterAccessViolationHandler(PatchesAccessViolationHandler, priority);
        signals->RegisterIllegalInstructionHandler(PatchesIllegalInstructionHandler, priority);
    }
}

void RegisterPatchModule(void* module_ptr, u64 module_size, void* trampoline_area_ptr,
                         u64 trampoline_area_size) {
    std::call_once(init_flag, PatchesInit);

    const auto module_addr = reinterpret_cast<u64>(module_ptr);
    modules.emplace(std::piecewise_construct, std::forward_as_tuple(module_addr),
                    std::forward_as_tuple(static_cast<u8*>(module_ptr), module_size,
                                          static_cast<u8*>(trampoline_area_ptr),
                                          trampoline_area_size));
}

void PrePatchInstructions(u64 segment_addr, u64 segment_size) {
#if !defined(_WIN32) && !defined(__APPLE__)
    // Linux and others have an FS segment pointing to valid memory, so continue to do full
    // ahead-of-time patching for now until a better solution is worked out.
    if (!Patches.empty()) {
        TryPatchAot(reinterpret_cast<void*>(segment_addr), segment_size);
    }
#endif
}

} // namespace Core
