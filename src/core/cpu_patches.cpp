// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <bitset>
#include <climits>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>
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
#if defined(__FreeBSD__)
#include <machine/npx.h>
#endif
#include <pthread.h>
#include <sys/ucontext.h>
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
           src_op.mem.disp.value < sizeof(Core::Tcb) && dst_op.reg.value >= ZYDIS_REGISTER_RAX &&
           dst_op.reg.value <= ZYDIS_REGISTER_R15;
}

#if defined(_WIN32)
static void RetrieveTcbPointer(Xbyak::Reg dst, Xbyak::CodeGenerator& c, ZyanI64 offset) {
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
    if (offset > 0) {
        // TCB starts with a pointer to self. Dereference this to get the correct data.
        c.mov(dst, qword[dst + offset]);
    }
}
#endif

static void GenerateTcbAccess(void* /* address */, const ZydisDecodedOperand* operands,
                              Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

#if defined(_WIN32)
    RetrieveTcbPointer(dst, c, operands[1].mem.disp.value);
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs read with gs read.
    c.putSeg(gs);
    c.mov(dst, src);
#endif
}

static void GenerateTcbCompare(void* /* address */, const ZydisDecodedOperand* operands,
                               Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

#if defined(_WIN32)
    // Prepare a scratch register. Use rax where possible, rbx if rax is in use.
    const Xbyak::Reg64 scratch = operands[0].reg.value == ZYDIS_REGISTER_RAX ? rbx : rax;

    // Set rsp to before red zone and save scratch register
    c.lea(rsp, ptr[rsp - 128]);
    c.pushfq();
    c.push(scratch);

    // Retrieve value from TCB and store it in the scratch register
    RetrieveTcbPointer(scratch, c, operands[1].mem.disp.value);

    // Perform compare op
    c.cmp(dst, scratch);

    // Restore registers
    c.pop(scratch);
    c.popfq();
    c.lea(rsp, ptr[rsp + 128]);
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs compare with gs compare.
    c.putSeg(gs);
    c.cmp(dst, src);
#endif
}

static void GenerateTcbExclusiveOr(void* /* address */, const ZydisDecodedOperand* operands,
                                   Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

#if defined(_WIN32)
    // Prepare a scratch register. Use rax where possible, rbx if rax is in use.
    const Xbyak::Reg64 scratch = operands[0].reg.value == ZYDIS_REGISTER_RAX ? rbx : rax;

    // Set rsp to before red zone and save scratch register
    c.lea(rsp, ptr[rsp - 128]);
    c.pushfq();
    c.push(scratch);

    // Retrieve value from TCB and store it in the scratch register
    RetrieveTcbPointer(scratch, c, operands[1].mem.disp.value);

    // Perform xor
    c.xor_(dst, scratch);

    // Restore registers
    c.pop(scratch);
    c.popfq();
    c.lea(rsp, ptr[rsp + 128]);
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs xor with gs xor.
    c.putSeg(gs);
    c.xor_(dst, src);
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

        if (length + index > 64) {
            mask = 0xFFFF'FFFF'FFFF'FFFF;
        }

        // Get lower qword from xmm register
        c.vmovq(scratch1, xmm_dst);

        if (index != 0) {
            c.shr(scratch1, index);
        }

        // We need to move mask to a register because we can't use all the possible
        // immediate values with `and reg, imm32`
        c.mov(scratch2, mask);
        c.and_(scratch1, scratch2);

        // Writeback to xmm register, extrq instruction says top 64-bits are undefined but zeroed on
        // AMD CPUs
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

        if (length + index > 64) {
            mask_value = 0xFFFF'FFFF'FFFF'FFFF;
        }

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

        // Insert scratch2 into low 64 bits of dst, upper 64 bits are undefined but zeroed on AMD
        // CPUs
        c.vmovq(xmm_dst, scratch2);

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

        // Upper 64 bits are undefined in insertq but AMD CPUs zero them
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

constexpr size_t NearJumpSize = 5;

#if defined(_WIN32)
static const bool need_tcb_trampoline = true;
#else
static const bool need_tcb_trampoline = false;
#endif

static const std::unordered_map<ZydisMnemonic, std::vector<PatchInfo>> Patches = {
    // SSE4a
    {ZYDIS_MNEMONIC_EXTRQ, {{FilterNoSSE4a, GenerateEXTRQ, true}}},
    {ZYDIS_MNEMONIC_INSERTQ, {{FilterNoSSE4a, GenerateINSERTQ, true}}},
    {ZYDIS_MNEMONIC_MOVNTSS, {{FilterNoSSE4a, ReplaceMOVNTSS, false}}},
    {ZYDIS_MNEMONIC_MOVNTSD, {{FilterNoSSE4a, ReplaceMOVNTSD, false}}},

#if !defined(__APPLE__)
    // FS segment patches
    // For most of these, Windows needs a trampoline while other platforms do not.
    {ZYDIS_MNEMONIC_XOR, {{FilterTcbAccess, GenerateTcbExclusiveOr, need_tcb_trampoline}}},
    {ZYDIS_MNEMONIC_MOV, {{FilterTcbAccess, GenerateTcbAccess, need_tcb_trampoline}}},
    {ZYDIS_MNEMONIC_CMP, {{FilterTcbAccess, GenerateTcbCompare, need_tcb_trampoline}}}
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

    /// Prevents repeated generation attempts after the fixed trampoline area is exhausted.
    bool trampoline_exhausted{};

    PatchModule(u8* module_ptr, const u64 module_size, u8* trampoline_ptr,
                const u64 trampoline_size)
        : start(module_ptr), end(module_ptr + module_size), patch_gen(module_size, module_ptr),
          trampoline_gen(trampoline_size, trampoline_ptr) {}
};
static std::map<u64, PatchModule> modules;

static bool HandleTrampolineError(PatchModule* module, const Xbyak::Error& error) {
    if (static_cast<int>(error) != Xbyak::ERR_CODE_IS_TOO_BIG) {
        return false;
    }
    if (!module->trampoline_exhausted) {
        LOG_WARNING(Core, "Patch trampoline space exhausted for module at {}",
                    fmt::ptr(module->start));
        module->trampoline_exhausted = true;
    }
    return true;
}

static PatchModule* GetModule(const void* ptr) {
    auto upper_bound = modules.upper_bound(reinterpret_cast<u64>(ptr));
    if (upper_bound == modules.begin()) {
        return nullptr;
    }
    return &(std::prev(upper_bound)->second);
}

// Windows static guest red-zone protection
static PatchModule* GetContainingModule(const void* ptr) {
    auto* module = GetModule(ptr);
    const auto* address = static_cast<const u8*>(ptr);
    return module != nullptr && address < module->end ? module : nullptr;
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
        const auto& patches = Patches.at(instruction.mnemonic);
        for (const auto& patch_info : patches) {
            bool needs_trampoline = patch_info.trampoline;
            if (patch_info.filter(operands)) {
                auto& patch_gen = module->patch_gen;

                if (needs_trampoline && instruction.length < NearJumpSize) {
                    // Trampoline is needed but instruction is too short to patch.
                    // Return false and length to signal to AOT compilation that this instruction
                    // should be skipped and handled at runtime.
                    return std::make_pair(false, instruction.length);
                }

                if (needs_trampoline && module->trampoline_exhausted) {
                    return std::make_pair(false, instruction.length);
                }

                // Reset state and move to current code position.
                patch_gen.reset();
                patch_gen.setSize(code - patch_gen.getCode());

                if (needs_trampoline) {
                    auto& trampoline_gen = module->trampoline_gen;
                    const size_t trampoline_offset = trampoline_gen.getSize();
                    const auto trampoline_ptr = trampoline_gen.getCurr();
                    try {
                        patch_info.generator(code, operands, trampoline_gen);

                        // Return to the following instruction at the end of the trampoline.
                        trampoline_gen.jmp(code + instruction.length);
                    } catch (const Xbyak::Error& error) {
                        trampoline_gen.setSize(trampoline_offset);
                        if (HandleTrampolineError(module, error)) {
                            return std::make_pair(false, instruction.length);
                        }
                        throw;
                    }

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
    }

    return std::make_pair(false, instruction.length);
}

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

static void* GetXmmPointer(void* ctx, u8 index) {
#if defined(_WIN32)
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((EXCEPTION_POINTERS*)ctx)->ContextRecord->Xmm##index.Low)
#elif defined(__APPLE__)
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((ucontext_t*)ctx)->uc_mcontext->__fs.__fpu_xmm##index);
#elif defined(__FreeBSD__)
    // In mc_fpstate
    // See <machine/npx.h> for the internals of mc_fpstate[].
#define CASE(index)                                                                                \
    case index: {                                                                                  \
        auto& mctx = ((ucontext_t*)ctx)->uc_mcontext;                                              \
        ASSERT(mctx.mc_fpformat == _MC_FPFMT_XMM);                                                 \
        auto* s_fpu = (struct savefpu*)(&mctx.mc_fpstate[0]);                                      \
        return (void*)(&(s_fpu->sv_xmm[0]));                                                       \
    }
#else
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((ucontext_t*)ctx)->uc_mcontext.fpregs->_xmm[index].element[0])
#endif
    switch (index) {
        CASE(0);
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(10);
        CASE(11);
        CASE(12);
        CASE(13);
        CASE(14);
        CASE(15);
    default: {
        UNREACHABLE_MSG("Invalid XMM register index: {}", index);
        return nullptr;
    }
    }
#undef CASE
}

static void IncrementRip(void* ctx, u64 length) {
#if defined(_WIN32)
    ((EXCEPTION_POINTERS*)ctx)->ContextRecord->Rip += length;
#elif defined(__APPLE__)
    ((ucontext_t*)ctx)->uc_mcontext->__ss.__rip += length;
#elif defined(__FreeBSD__)
    ((ucontext_t*)ctx)->uc_mcontext.mc_rip += length;
#else
    ((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP] += length;
#endif
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
        const auto dst = GetXmmPointer(ctx, dstIndex);
        const auto src = GetXmmPointer(ctx, srcIndex);

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

        memset((u8*)dst + sizeof(u64), 0, sizeof(u64));
        memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

        IncrementRip(ctx, 4);

        return true;
    }
    case ZYDIS_MNEMONIC_INSERTQ: {
        const auto dst = GetXmmPointer(ctx, dstIndex);
        const auto src = GetXmmPointer(ctx, srcIndex);

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

        memset((u8*)dst + sizeof(u64), 0, sizeof(u64));
        memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

        IncrementRip(ctx, 4);

        return true;
    }
    default: {
        UNREACHABLE();
    }
    }

    UNREACHABLE();
}

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

// ============================================================================
// Windows static guest red-zone protection
// ============================================================================

namespace WindowsGuestRedZoneProtection {
namespace {

std::atomic active_mode{WindowsGuestRedZoneProtectionMode::Disabled};

} // namespace

void SetActiveMode(WindowsGuestRedZoneProtectionMode mode) noexcept {
    active_mode.store(mode, std::memory_order_release);
}

WindowsGuestRedZoneProtectionMode GetActiveMode() noexcept {
    return active_mode.load(std::memory_order_acquire);
}

bool IsStaticPatchingEnabled() noexcept {
    return GetActiveMode() == WindowsGuestRedZoneProtectionMode::StaticPatching;
}

} // namespace WindowsGuestRedZoneProtection

#if defined(_WIN32)

namespace {

constexpr size_t GuestRedZoneSize = 128;
constexpr size_t ShortJumpSize = 2;
using RedZoneMask = std::bitset<GuestRedZoneSize>;

struct DecodedCodeInstruction {
    uintptr_t address{};
    ZydisDecodedInstruction instruction{};
    std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
    RedZoneMask red_zone_use{};
    RedZoneMask red_zone_def{};
    RedZoneMask red_zone_live{};
    bool accesses_memory{};
    bool uses_stack_pointer{};
    bool has_red_zone_operand{};
    bool has_unmodeled_red_zone_operand{};
    bool changes_stack_pointer{};
    bool replaces_stack_pointer{};
    std::optional<s64> stack_pointer_delta;
};

struct DecodedFunction {
    std::map<uintptr_t, DecodedCodeInstruction> instructions;
    std::set<uintptr_t> branch_targets;
    bool uses_red_zone{};
    bool has_indirect_branch{};
    bool requires_conservative_red_zone_tracking{};
};

struct InstructionRewrite {
    const PatchInfo* cpu_patch{};
    bool protect_red_zone{};
    bool protected_indirect_call{};
};

bool IsStackPointerRegister(ZydisRegister reg) {
    return reg != ZYDIS_REGISTER_NONE &&
           ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, reg) == ZYDIS_REGISTER_RSP;
}

bool IsControlFlowTerminator(const ZydisDecodedInstruction& instruction) {
    return instruction.meta.category == ZYDIS_CATEGORY_UNCOND_BR ||
           instruction.meta.category == ZYDIS_CATEGORY_RET ||
           instruction.meta.category == ZYDIS_CATEGORY_INTERRUPT ||
           instruction.meta.category == ZYDIS_CATEGORY_SYSRET ||
           instruction.mnemonic == ZYDIS_MNEMONIC_UD2;
}

uintptr_t GetRelativeTarget(const DecodedCodeInstruction& decoded) {
    for (u8 index = 0; index < decoded.instruction.operand_count_visible; ++index) {
        const auto& operand = decoded.operands[index];
        if (operand.type != ZYDIS_OPERAND_TYPE_IMMEDIATE || !operand.imm.is_relative) {
            continue;
        }

        ZyanU64 target{};
        if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded.instruction, &operand, decoded.address,
                                                  &target))) {
            return target;
        }
    }
    return 0;
}

DecodedCodeInstruction DecodeCodeInstruction(uintptr_t address, uintptr_t end) {
    DecodedCodeInstruction decoded{.address = address};
    const auto status = Common::Decoder::Instance()->decodeInstruction(
        decoded.instruction, decoded.operands.data(), reinterpret_cast<void*>(address),
        end - address);
    if (!ZYAN_SUCCESS(status)) {
        decoded.instruction.length = 0;
        return decoded;
    }

    for (u8 index = 0; index < decoded.instruction.operand_count; ++index) {
        const auto& operand = decoded.operands[index];
        if (operand.type == ZYDIS_OPERAND_TYPE_REGISTER) {
            decoded.uses_stack_pointer |= IsStackPointerRegister(operand.reg.value);
            if (IsStackPointerRegister(operand.reg.value) &&
                (operand.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE) != 0 &&
                decoded.instruction.meta.category != ZYDIS_CATEGORY_CALL &&
                decoded.instruction.meta.category != ZYDIS_CATEGORY_RET) {
                decoded.changes_stack_pointer = true;
            }
            continue;
        }
        if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY) {
            continue;
        }

        const bool stack_relative =
            IsStackPointerRegister(operand.mem.base) || IsStackPointerRegister(operand.mem.index);
        decoded.uses_stack_pointer |= stack_relative;
        constexpr ZydisOperandActions MemoryAccessMask =
            ZYDIS_OPERAND_ACTION_MASK_READ | ZYDIS_OPERAND_ACTION_MASK_WRITE;
        if (decoded.instruction.mnemonic != ZYDIS_MNEMONIC_LEA &&
            decoded.instruction.mnemonic != ZYDIS_MNEMONIC_NOP && !stack_relative &&
            (operand.actions & MemoryAccessMask) != 0) {
            decoded.accesses_memory = true;
        }
    }

    if (decoded.changes_stack_pointer) {
        const auto& operands = decoded.operands;
        if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
            operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            IsStackPointerRegister(operands[0].reg.value) &&
            operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY) {
            decoded.replaces_stack_pointer = true;
        } else if ((decoded.instruction.mnemonic == ZYDIS_MNEMONIC_ADD ||
                    decoded.instruction.mnemonic == ZYDIS_MNEMONIC_SUB) &&
                   operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   IsStackPointerRegister(operands[0].reg.value) &&
                   operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
            const s64 immediate = operands[1].imm.is_signed
                                      ? operands[1].imm.value.s
                                      : static_cast<s64>(operands[1].imm.value.u);
            decoded.stack_pointer_delta =
                decoded.instruction.mnemonic == ZYDIS_MNEMONIC_ADD ? immediate : -immediate;
        } else if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_LEA &&
                   operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   IsStackPointerRegister(operands[0].reg.value) &&
                   operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   IsStackPointerRegister(operands[1].mem.base) &&
                   operands[1].mem.index == ZYDIS_REGISTER_NONE) {
            decoded.stack_pointer_delta = operands[1].mem.disp.value;
        } else if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_PUSH ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_PUSHF ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_PUSHFD ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_PUSHFQ) {
            decoded.stack_pointer_delta = -static_cast<s64>(sizeof(u64));
        } else if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_POP ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_POPF ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_POPFD ||
                   decoded.instruction.mnemonic == ZYDIS_MNEMONIC_POPFQ) {
            decoded.stack_pointer_delta = sizeof(u64);
        }
    }

    for (u8 index = 0; index < decoded.instruction.operand_count_visible; ++index) {
        const auto& operand = decoded.operands[index];
        if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY ||
            !IsStackPointerRegister(operand.mem.base) || operand.mem.disp.size == 0 ||
            operand.mem.disp.value >= 0) {
            continue;
        }

        decoded.has_red_zone_operand = true;
        if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_LEA) {
            if (decoded.stack_pointer_delta.has_value()) {
                decoded.has_red_zone_operand = false;
                continue;
            }
            decoded.has_unmodeled_red_zone_operand = true;
            continue;
        }

        const s64 access_start = operand.mem.disp.value;
        const s64 access_size = std::max<s64>(operand.size / 8, 1);
        const s64 range_start = std::max(access_start, -static_cast<s64>(GuestRedZoneSize));
        const s64 range_end = std::min(access_start + access_size, 0LL);
        for (s64 offset = range_start; offset < range_end; ++offset) {
            const size_t bit = static_cast<size_t>(offset + static_cast<s64>(GuestRedZoneSize));
            if ((operand.actions & ZYDIS_OPERAND_ACTION_MASK_READ) != 0) {
                decoded.red_zone_use.set(bit);
            }
            if ((operand.actions & ZYDIS_OPERAND_ACTION_WRITE) != 0) {
                decoded.red_zone_def.set(bit);
            }
        }
    }
    return decoded;
}

bool IsSameRegister(ZydisRegister lhs, ZydisRegister rhs) {
    return lhs != ZYDIS_REGISTER_NONE && rhs != ZYDIS_REGISTER_NONE &&
           ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, lhs) ==
               ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, rhs);
}

bool WritesRegister(const DecodedCodeInstruction& decoded, ZydisRegister reg) {
    return std::ranges::any_of(std::span{decoded.operands}.first(decoded.instruction.operand_count),
                               [reg](const ZydisDecodedOperand& operand) {
                                   return operand.type == ZYDIS_OPERAND_TYPE_REGISTER &&
                                          (operand.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE) !=
                                              0 &&
                                          IsSameRegister(operand.reg.value, reg);
                               });
}

std::optional<std::vector<uintptr_t>> ResolveBoundedJumpTable(
    const DecodedFunction& function, uintptr_t branch_address, uintptr_t function_start,
    uintptr_t function_end, uintptr_t segment_start, uintptr_t segment_end) {
    const auto branch = function.instructions.find(branch_address);
    if (branch == function.instructions.end() ||
        branch->second.instruction.mnemonic != ZYDIS_MNEMONIC_JMP ||
        branch->second.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER) {
        return std::nullopt;
    }
    const ZydisRegister target_reg = branch->second.operands[0].reg.value;

    const auto previous_contiguous = [&function](auto instruction) {
        if (instruction == function.instructions.begin()) {
            return function.instructions.end();
        }
        const auto previous = std::prev(instruction);
        return previous->first + previous->second.instruction.length == instruction->first
                   ? previous
                   : function.instructions.end();
    };

    constexpr size_t MaxInterveningInstructions = 4;
    auto add = function.instructions.end();
    auto pattern_cursor = branch;
    for (size_t count = 0; count <= MaxInterveningInstructions; ++count) {
        const auto candidate = previous_contiguous(pattern_cursor);
        if (candidate == function.instructions.end()) {
            break;
        }
        const auto& decoded = candidate->second;
        if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_ADD &&
            decoded.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            IsSameRegister(decoded.operands[0].reg.value, target_reg) &&
            decoded.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER) {
            add = candidate;
            break;
        }
        if (WritesRegister(decoded, target_reg) || IsControlFlowTerminator(decoded.instruction)) {
            return std::nullopt;
        }
        pattern_cursor = candidate;
    }
    if (add == function.instructions.end()) {
        return std::nullopt;
    }
    const ZydisRegister table_reg = add->second.operands[1].reg.value;

    const auto load = previous_contiguous(add);
    if (load == function.instructions.end() ||
        load->second.instruction.mnemonic != ZYDIS_MNEMONIC_MOVSXD ||
        load->second.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
        !IsSameRegister(load->second.operands[0].reg.value, target_reg) ||
        load->second.operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
        !IsSameRegister(load->second.operands[1].mem.base, table_reg) ||
        load->second.operands[1].mem.index == ZYDIS_REGISTER_NONE ||
        load->second.operands[1].mem.scale != sizeof(s32)) {
        return std::nullopt;
    }
    const ZydisRegister index_reg = load->second.operands[1].mem.index;

    constexpr size_t MaxPatternInstructions = 64;
    std::optional<size_t> table_size;
    std::optional<uintptr_t> guarded_path_start;
    auto cursor = load;
    for (size_t count = 0; count < MaxPatternInstructions; ++count) {
        cursor = previous_contiguous(cursor);
        if (cursor == function.instructions.end()) {
            break;
        }
        const auto& decoded = cursor->second;
        if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_CMP &&
            decoded.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            IsSameRegister(decoded.operands[0].reg.value, index_reg) &&
            decoded.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
            const uintptr_t next_address = decoded.address + decoded.instruction.length;
            const auto bounds_branch = function.instructions.find(next_address);
            if (bounds_branch == function.instructions.end()) {
                return std::nullopt;
            }
            const s64 bound = decoded.operands[1].imm.is_signed
                                  ? decoded.operands[1].imm.value.s
                                  : static_cast<s64>(decoded.operands[1].imm.value.u);
            if (bound < 0) {
                return std::nullopt;
            }
            if (bounds_branch->second.instruction.mnemonic == ZYDIS_MNEMONIC_JNBE) {
                table_size = static_cast<size_t>(bound) + 1;
            } else if (bounds_branch->second.instruction.mnemonic == ZYDIS_MNEMONIC_JNB) {
                table_size = static_cast<size_t>(bound);
            } else {
                return std::nullopt;
            }
            guarded_path_start = next_address + bounds_branch->second.instruction.length;
            break;
        }
        if (WritesRegister(decoded, index_reg)) {
            return std::nullopt;
        }
    }
    if (!table_size) {
        return std::nullopt;
    }
    if (std::ranges::any_of(function.branch_targets, [&](uintptr_t target) {
            return target >= *guarded_path_start && target <= branch_address;
        })) {
        return std::nullopt;
    }

    const auto decode_table_address =
        [table_reg](const DecodedCodeInstruction& decoded) -> std::optional<uintptr_t> {
        if (decoded.instruction.mnemonic != ZYDIS_MNEMONIC_LEA ||
            decoded.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            !IsSameRegister(decoded.operands[0].reg.value, table_reg) ||
            decoded.operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY) {
            return std::nullopt;
        }
        ZyanU64 absolute_address{};
        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded.instruction, &decoded.operands[1],
                                                   decoded.address, &absolute_address))) {
            return std::nullopt;
        }
        return absolute_address;
    };

    std::set<uintptr_t> table_candidates;
    cursor = load;
    for (size_t count = 0; count < MaxPatternInstructions; ++count) {
        cursor = previous_contiguous(cursor);
        if (cursor == function.instructions.end()) {
            break;
        }
        if (!WritesRegister(cursor->second, table_reg)) {
            continue;
        }
        if (const auto address = decode_table_address(cursor->second)) {
            table_candidates.insert(*address);
        }
        break;
    }
    if (table_candidates.empty()) {
        for (const auto& [address, decoded] : function.instructions) {
            if (address >= branch_address) {
                break;
            }
            if (const auto table_address = decode_table_address(decoded)) {
                table_candidates.insert(*table_address);
            }
        }
    }
    constexpr size_t MaxJumpTableEntries = 4096;
    if (*table_size == 0 || *table_size > MaxJumpTableEntries) {
        return std::nullopt;
    }

    std::optional<std::vector<uintptr_t>> resolved_targets;
    for (const uintptr_t table_address : table_candidates) {
        if (table_address < segment_start || table_address > segment_end ||
            *table_size > (segment_end - table_address) / sizeof(s32)) {
            continue;
        }

        std::vector<uintptr_t> targets;
        targets.reserve(*table_size);
        bool valid = true;
        for (size_t index = 0; index < *table_size; ++index) {
            s32 offset;
            std::memcpy(&offset,
                        reinterpret_cast<const void*>(table_address + index * sizeof(offset)),
                        sizeof(offset));
            const s64 target = static_cast<s64>(table_address) + offset;
            if (target < static_cast<s64>(function_start) ||
                target >= static_cast<s64>(function_end)) {
                valid = false;
                break;
            }
            targets.push_back(static_cast<uintptr_t>(target));
        }
        if (!valid) {
            continue;
        }
        std::ranges::sort(targets);
        targets.erase(std::ranges::unique(targets).begin(), targets.end());
        if (resolved_targets) {
            return std::nullopt;
        }
        resolved_targets = std::move(targets);
    }
    return resolved_targets;
}

DecodedFunction DecodeFunction(uintptr_t function_start, uintptr_t function_end,
                               uintptr_t segment_start, uintptr_t segment_end) {
    DecodedFunction function;
    std::vector<uintptr_t> blocks{function_start};
    std::unordered_set<uintptr_t> visited;
    std::set<uintptr_t> indirect_branches;
    std::set<uintptr_t> resolved_indirect_branches;

    while (true) {
        while (!blocks.empty()) {
            uintptr_t address = blocks.back();
            blocks.pop_back();

            while (address >= function_start && address < function_end &&
                   !visited.contains(address)) {
                visited.insert(address);
                auto decoded = DecodeCodeInstruction(address, function_end);
                if (decoded.instruction.length == 0) {
                    break;
                }

                function.uses_red_zone |= decoded.has_red_zone_operand;
                function.requires_conservative_red_zone_tracking |=
                    decoded.has_unmodeled_red_zone_operand ||
                    (decoded.changes_stack_pointer && !decoded.stack_pointer_delta.has_value());

                const uintptr_t next_address = address + decoded.instruction.length;
                const uintptr_t branch_target = GetRelativeTarget(decoded);
                if (branch_target >= function_start && branch_target < function_end) {
                    function.branch_targets.insert(branch_target);
                }
                function.instructions.emplace(address, decoded);

                if (decoded.instruction.meta.category == ZYDIS_CATEGORY_COND_BR) {
                    if (branch_target >= function_start && branch_target < function_end) {
                        blocks.push_back(branch_target);
                    }
                } else if (IsControlFlowTerminator(decoded.instruction)) {
                    if (decoded.instruction.meta.category == ZYDIS_CATEGORY_UNCOND_BR &&
                        branch_target >= function_start && branch_target < function_end) {
                        blocks.push_back(branch_target);
                    } else if (decoded.instruction.meta.category == ZYDIS_CATEGORY_UNCOND_BR &&
                               branch_target == 0) {
                        indirect_branches.insert(address);
                    }
                    break;
                }
                address = next_address;
            }
        }

        bool discovered_block = false;
        for (const uintptr_t branch_address : indirect_branches) {
            if (resolved_indirect_branches.contains(branch_address)) {
                continue;
            }
            const auto targets = ResolveBoundedJumpTable(function, branch_address, function_start,
                                                         function_end, segment_start, segment_end);
            if (!targets) {
                continue;
            }
            resolved_indirect_branches.insert(branch_address);
            for (const uintptr_t target : *targets) {
                function.branch_targets.insert(target);
                if (!visited.contains(target)) {
                    blocks.push_back(target);
                    discovered_block = true;
                }
            }
        }
        if (!discovered_block) {
            break;
        }
    }
    function.has_indirect_branch = indirect_branches.size() != resolved_indirect_branches.size();
    return function;
}

RedZoneMask TranslateRedZoneMask(const RedZoneMask& mask, s64 stack_pointer_delta) {
    RedZoneMask translated;
    for (size_t bit = 0; bit < GuestRedZoneSize; ++bit) {
        if (!mask.test(bit)) {
            continue;
        }
        const s64 after_offset = static_cast<s64>(bit) - static_cast<s64>(GuestRedZoneSize);
        const s64 before_offset = after_offset + stack_pointer_delta;
        if (before_offset >= -static_cast<s64>(GuestRedZoneSize) && before_offset < 0) {
            translated.set(static_cast<size_t>(before_offset + static_cast<s64>(GuestRedZoneSize)));
        }
    }
    return translated;
}

void AnalyzeRedZoneLiveness(DecodedFunction& function) {
    if (!function.uses_red_zone) {
        return;
    }

    if (function.has_indirect_branch || function.requires_conservative_red_zone_tracking ||
        std::ranges::any_of(function.branch_targets, [&function](uintptr_t target) {
            return !function.instructions.contains(target);
        })) {
        for (auto& [_, decoded] : function.instructions) {
            decoded.red_zone_live.set();
        }
        return;
    }

    std::vector<DecodedCodeInstruction*> instructions;
    instructions.reserve(function.instructions.size());
    std::map<uintptr_t, size_t> indices;
    for (auto& [address, decoded] : function.instructions) {
        indices.emplace(address, instructions.size());
        instructions.push_back(&decoded);
    }

    std::vector<RedZoneMask> live_in(instructions.size());
    bool changed;
    do {
        changed = false;
        for (size_t reverse_index = instructions.size(); reverse_index-- > 0;) {
            const auto& decoded = *instructions[reverse_index];
            RedZoneMask live_out;

            const auto add_successor = [&](uintptr_t address) {
                if (const auto successor = indices.find(address); successor != indices.end()) {
                    live_out |= live_in[successor->second];
                }
            };

            const uintptr_t next_address = decoded.address + decoded.instruction.length;
            const uintptr_t branch_target = GetRelativeTarget(decoded);
            if (decoded.instruction.meta.category == ZYDIS_CATEGORY_COND_BR) {
                add_successor(next_address);
                add_successor(branch_target);
            } else if (decoded.instruction.meta.category == ZYDIS_CATEGORY_UNCOND_BR) {
                add_successor(branch_target);
            } else if (!IsControlFlowTerminator(decoded.instruction)) {
                add_successor(next_address);
            }

            const RedZoneMask translated_live_out =
                decoded.stack_pointer_delta.has_value()
                    ? TranslateRedZoneMask(live_out, *decoded.stack_pointer_delta)
                    : live_out;
            const RedZoneMask new_live_in =
                decoded.red_zone_use | (translated_live_out & ~decoded.red_zone_def);
            if (new_live_in != live_in[reverse_index]) {
                live_in[reverse_index] = new_live_in;
                changed = true;
            }
        }
    } while (changed);

    for (size_t index = 0; index < instructions.size(); ++index) {
        instructions[index]->red_zone_live = live_in[index];
    }
}

bool EncodeRelocatedInstruction(const DecodedCodeInstruction& decoded,
                                Xbyak::CodeGenerator& generator) {
    ZydisEncoderRequest request;
    if (!ZYAN_SUCCESS(ZydisEncoderDecodedInstructionToEncoderRequest(
            &decoded.instruction, decoded.operands.data(),
            decoded.instruction.operand_count_visible, &request))) {
        return false;
    }

    for (u8 index = 0; index < decoded.instruction.operand_count_visible; ++index) {
        const auto& operand = decoded.operands[index];
        if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operand.imm.is_relative) {
            ZyanU64 absolute_address{};
            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded.instruction, &operand,
                                                       decoded.address, &absolute_address))) {
                return false;
            }
            request.operands[index].imm.u = absolute_address;
        } else if (operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   (operand.mem.base == ZYDIS_REGISTER_RIP ||
                    operand.mem.base == ZYDIS_REGISTER_EIP)) {
            ZyanU64 absolute_address{};
            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded.instruction, &operand,
                                                       decoded.address, &absolute_address))) {
                return false;
            }
            request.operands[index].mem.displacement = static_cast<ZyanI64>(absolute_address);
        }
    }

    std::array<u8, ZYDIS_MAX_INSTRUCTION_LENGTH> encoded{};
    ZyanUSize encoded_size = encoded.size();
    if (!ZYAN_SUCCESS(ZydisEncoderEncodeInstructionAbsolute(
            &request, encoded.data(), &encoded_size,
            reinterpret_cast<ZyanU64>(generator.getCurr())))) {
        return false;
    }
    generator.db(encoded.data(), encoded_size);
    return true;
}

bool GenerateProtectedIndirectCall(const DecodedCodeInstruction& decoded,
                                   Xbyak::CodeGenerator& generator) {
    ASSERT(decoded.instruction.meta.category == ZYDIS_CATEGORY_CALL);
    const auto& target = decoded.operands[0];
    ASSERT(target.type == ZYDIS_OPERAND_TYPE_MEMORY && target.size == sizeof(uintptr_t) * CHAR_BIT);

    ZydisEncoderRequest request{};
    request.machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    request.mnemonic = ZYDIS_MNEMONIC_MOV;
    request.operand_count = 2;
    request.operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
    request.operands[0].reg.value = ZYDIS_REGISTER_R11;

    ZydisEncoderRequest original_request{};
    if (!ZYAN_SUCCESS(ZydisEncoderDecodedInstructionToEncoderRequest(
            &decoded.instruction, decoded.operands.data(),
            decoded.instruction.operand_count_visible, &original_request))) {
        return false;
    }
    request.prefixes = original_request.prefixes & ZYDIS_ATTRIB_HAS_SEGMENT;
    request.address_size_hint = original_request.address_size_hint;
    request.operands[1] = original_request.operands[0];
    request.operands[1].mem.size = sizeof(uintptr_t);

    if (target.mem.base == ZYDIS_REGISTER_RIP || target.mem.base == ZYDIS_REGISTER_EIP) {
        ZyanU64 absolute_address{};
        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&decoded.instruction, &target, decoded.address,
                                                   &absolute_address))) {
            return false;
        }
        request.operands[1].mem.displacement = static_cast<ZyanI64>(absolute_address);
    }

    generator.lea(rsp, ptr[rsp - GuestRedZoneSize]);
    generator.push(r11);

    std::array<u8, ZYDIS_MAX_INSTRUCTION_LENGTH> encoded{};
    ZyanUSize encoded_size = encoded.size();
    if (!ZYAN_SUCCESS(ZydisEncoderEncodeInstructionAbsolute(
            &request, encoded.data(), &encoded_size,
            reinterpret_cast<ZyanU64>(generator.getCurr())))) {
        return false;
    }
    generator.db(encoded.data(), encoded_size);

    generator.xchg(r11, ptr[rsp]);
    generator.lea(rsp, ptr[rsp + GuestRedZoneSize + sizeof(uintptr_t)]);
    generator.call(ptr[rsp - GuestRedZoneSize - sizeof(uintptr_t)]);
    return true;
}

bool IsInPlaceMemoryPatch(ZydisMnemonic mnemonic) {
    return mnemonic == ZYDIS_MNEMONIC_MOVNTSS || mnemonic == ZYDIS_MNEMONIC_MOVNTSD;
}

const PatchInfo* FindMatchingPatch(const DecodedCodeInstruction& decoded) {
    const auto patches = Patches.find(decoded.instruction.mnemonic);
    if (patches == Patches.end()) {
        return nullptr;
    }
    const auto patch =
        std::ranges::find_if(patches->second, [&decoded](const PatchInfo& candidate) {
            return candidate.filter(decoded.operands.data());
        });
    return patch != patches->second.end() ? &*patch : nullptr;
}

} // namespace

RedZonePatchResult PatchRedZoneMemoryInstructions(u64 segment_addr, u64 segment_size,
                                                  std::span<const uintptr_t> function_starts) {
    RedZonePatchResult result{};
    auto* module = GetContainingModule(reinterpret_cast<void*>(segment_addr));
    if (module == nullptr || function_starts.empty()) {
        return result;
    }

    const uintptr_t segment_end = segment_addr + segment_size;
    std::vector<uintptr_t> starts;
    starts.reserve(function_starts.size());
    for (const uintptr_t start : function_starts) {
        if (start >= segment_addr && start < segment_end) {
            starts.push_back(start);
        }
    }
    std::ranges::sort(starts);
    const auto unique_end = std::ranges::unique(starts).begin();
    starts.erase(unique_end, starts.end());

    std::unique_lock lock{module->mutex};
    for (size_t function_index = 0; function_index < starts.size(); ++function_index) {
        const uintptr_t function_start = starts[function_index];
        const uintptr_t function_end =
            function_index + 1 < starts.size() ? starts[function_index + 1] : segment_end;
        if (function_end <= function_start) {
            continue;
        }

        ++result.function_count;
        auto function = DecodeFunction(function_start, function_end, segment_addr, segment_end);
        AnalyzeRedZoneLiveness(function);
        result.instruction_count += function.instructions.size();

        std::map<uintptr_t, InstructionRewrite> rewrite_sites;

        // Apply existing instruction substitutions ahead of time now that only verified
        // instruction boundaries are being visited. Short trampoline patches are combined with
        // the relocation pass below.
        for (auto& [address, decoded] : function.instructions) {
            const PatchInfo* matching_patch = FindMatchingPatch(decoded);
            const auto [patched, _] = TryPatch(reinterpret_cast<u8*>(address), module);
            if (IsInPlaceMemoryPatch(decoded.instruction.mnemonic)) {
                const RedZoneMask red_zone_live = decoded.red_zone_live;
                decoded = DecodeCodeInstruction(address, function_end);
                decoded.red_zone_live = red_zone_live;
            } else if (patched) {
                decoded = DecodeCodeInstruction(address, function_end);
                decoded.accesses_memory = false;
            } else if (matching_patch != nullptr && matching_patch->trampoline) {
                rewrite_sites.emplace(address, InstructionRewrite{.cpu_patch = matching_patch});
                ++result.cpu_patch_instruction_count;
            }
        }

        if (function.uses_red_zone) {
            ++result.red_zone_function_count;
            result.indirect_red_zone_function_count += function.has_indirect_branch;
            for (const auto& [address, decoded] : function.instructions) {
                if (!decoded.accesses_memory || !decoded.red_zone_live.any() ||
                    rewrite_sites.contains(address)) {
                    continue;
                }
                ++result.memory_instruction_count;
                if (decoded.instruction.length < NearJumpSize) {
                    ++result.short_memory_instruction_count;
                }
                if (decoded.instruction.meta.category == ZYDIS_CATEGORY_CALL &&
                    decoded.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    decoded.operands[0].size == sizeof(uintptr_t) * CHAR_BIT &&
                    !IsStackPointerRegister(decoded.operands[0].mem.base) &&
                    !IsStackPointerRegister(decoded.operands[0].mem.index)) {
                    rewrite_sites[address] = {
                        .protect_red_zone = true,
                        .protected_indirect_call = true,
                    };
                    continue;
                }
                if (decoded.uses_stack_pointer && !decoded.replaces_stack_pointer) {
                    ++result.stack_dependent_memory_instruction_count;
                    continue;
                }
                if (decoded.instruction.meta.category == ZYDIS_CATEGORY_CALL ||
                    IsControlFlowTerminator(decoded.instruction)) {
                    ++result.control_flow_memory_instruction_count;
                    continue;
                }
                rewrite_sites[address].protect_red_zone = true;
            }
        }
        if (rewrite_sites.empty()) {
            continue;
        }

        struct RelocationSpan {
            std::vector<const DecodedCodeInstruction*> instructions;
            uintptr_t patch_start{};
            uintptr_t continuation{};
            size_t patch_size{};
        };

        const auto emit_span = [&](const RelocationSpan& span) -> std::optional<size_t> {
            const size_t trampoline_offset = module->trampoline_gen.getSize();
            if (module->trampoline_exhausted) {
                return std::nullopt;
            }
            try {
                for (const auto* decoded : span.instructions) {
                    const auto rewrite = rewrite_sites.find(decoded->address);
                    const bool protected_indirect_call =
                        rewrite != rewrite_sites.end() && rewrite->second.protected_indirect_call;
                    const bool protect_red_zone = rewrite != rewrite_sites.end() &&
                                                  rewrite->second.protect_red_zone &&
                                                  !protected_indirect_call;
                    if (protect_red_zone) {
                        module->trampoline_gen.lea(rsp, ptr[rsp - GuestRedZoneSize]);
                    }
                    if (protected_indirect_call) {
                        if (!GenerateProtectedIndirectCall(*decoded, module->trampoline_gen)) {
                            module->trampoline_gen.setSize(trampoline_offset);
                            return std::nullopt;
                        }
                    } else if (rewrite != rewrite_sites.end() &&
                               rewrite->second.cpu_patch != nullptr) {
                        rewrite->second.cpu_patch->generator(
                            reinterpret_cast<void*>(decoded->address), decoded->operands.data(),
                            module->trampoline_gen);
                    } else if (!EncodeRelocatedInstruction(*decoded, module->trampoline_gen)) {
                        module->trampoline_gen.setSize(trampoline_offset);
                        return std::nullopt;
                    }
                    if (protect_red_zone && !decoded->replaces_stack_pointer) {
                        module->trampoline_gen.lea(rsp, ptr[rsp + GuestRedZoneSize]);
                    }
                }
                module->trampoline_gen.jmp(reinterpret_cast<void*>(span.continuation));
            } catch (const Xbyak::Error& error) {
                module->trampoline_gen.setSize(trampoline_offset);
                if (HandleTrampolineError(module, error)) {
                    return std::nullopt;
                }
                throw;
            }
            return trampoline_offset;
        };

        const auto record_rewrites = [&](const RelocationSpan& span) {
            for (const auto* decoded : span.instructions) {
                const auto rewrite = rewrite_sites.find(decoded->address);
                if (rewrite == rewrite_sites.end()) {
                    continue;
                }
                if (rewrite->second.protect_red_zone) {
                    ++result.patched_memory_instruction_count;
                }
                if (rewrite->second.cpu_patch != nullptr) {
                    ++result.patched_cpu_patch_instruction_count;
                }
                module->patched.insert(reinterpret_cast<u8*>(decoded->address));
            }
        };

        std::vector<std::pair<uintptr_t, uintptr_t>> patched_spans;
        std::vector<uintptr_t> relay_slots;
        std::vector<uintptr_t> short_relay_slots;
        std::vector<uintptr_t> unresolved_sites;
        uintptr_t covered_until{};
        for (auto site_it = rewrite_sites.begin(); site_it != rewrite_sites.end(); ++site_it) {
            const uintptr_t site = site_it->first;
            if (site < covered_until) {
                continue;
            }

            const auto collect_forward_span = [&]() -> std::optional<RelocationSpan> {
                RelocationSpan span{.patch_start = site, .continuation = site};
                while (span.patch_size < NearJumpSize) {
                    const auto decoded_it = function.instructions.find(span.continuation);
                    if (decoded_it == function.instructions.end() ||
                        (span.continuation != site &&
                         function.branch_targets.contains(span.continuation))) {
                        return std::nullopt;
                    }

                    const auto& decoded = decoded_it->second;
                    span.instructions.push_back(&decoded);
                    span.patch_size += decoded.instruction.length;
                    span.continuation += decoded.instruction.length;
                    if (span.patch_size < NearJumpSize &&
                        IsControlFlowTerminator(decoded.instruction)) {
                        return std::nullopt;
                    }
                }
                return span;
            };

            const auto collect_backward_span = [&]() -> std::optional<RelocationSpan> {
                const auto site_instruction = function.instructions.find(site);
                ASSERT(site_instruction != function.instructions.end());
                RelocationSpan span{
                    .instructions = {&site_instruction->second},
                    .patch_start = site,
                    .continuation = site + site_instruction->second.instruction.length,
                    .patch_size = site_instruction->second.instruction.length,
                };

                while (span.patch_size < NearJumpSize) {
                    const auto previous_end = function.instructions.lower_bound(span.patch_start);
                    if (previous_end == function.instructions.begin() ||
                        function.branch_targets.contains(span.patch_start)) {
                        return std::nullopt;
                    }

                    const auto previous = std::prev(previous_end);
                    const auto& decoded = previous->second;
                    if (previous->first + decoded.instruction.length != span.patch_start ||
                        previous->first < covered_until ||
                        IsControlFlowTerminator(decoded.instruction)) {
                        return std::nullopt;
                    }

                    span.instructions.insert(span.instructions.begin(), &decoded);
                    span.patch_start = previous->first;
                    span.patch_size += decoded.instruction.length;
                }
                return span;
            };

            std::optional<RelocationSpan> selected_span;
            std::optional<size_t> trampoline_offset;
            const auto& site_instruction = function.instructions.at(site);
            const bool can_relocate_neighbors = !function.has_indirect_branch;
            if (can_relocate_neighbors || site_instruction.instruction.length >= NearJumpSize) {
                auto forward_span = collect_forward_span();
                if (forward_span) {
                    trampoline_offset = emit_span(*forward_span);
                    if (trampoline_offset) {
                        selected_span = std::move(forward_span);
                    }
                }
            }
            if (!selected_span && can_relocate_neighbors) {
                if (auto backward_span = collect_backward_span()) {
                    trampoline_offset = emit_span(*backward_span);
                    if (trampoline_offset) {
                        selected_span = std::move(backward_span);
                    }
                }
            }
            if (!selected_span) {
                unresolved_sites.push_back(site);
                continue;
            }

            const auto& span = *selected_span;
            const auto* trampoline = module->trampoline_gen.getCode() + *trampoline_offset;

            auto& patch_gen = module->patch_gen;
            patch_gen.reset();
            patch_gen.setSize(span.patch_start - reinterpret_cast<uintptr_t>(patch_gen.getCode()));
            patch_gen.jmp(trampoline, Xbyak::CodeGenerator::LabelType::T_NEAR);
            patch_gen.nop(span.patch_size - NearJumpSize);

            record_rewrites(span);
            patched_spans.emplace_back(span.patch_start, span.continuation);
            if (span.patch_size >= NearJumpSize * 2) {
                relay_slots.push_back(span.patch_start + NearJumpSize);
            }
            if (span.patch_size >= NearJumpSize + ShortJumpSize) {
                short_relay_slots.push_back(span.patch_start + NearJumpSize);
            }
            covered_until = span.continuation;
        }

        const auto record_unsupported = [&](uintptr_t site) {
            const auto& rewrite = rewrite_sites.at(site);
            if (rewrite.cpu_patch != nullptr) {
                ++result.unsupported_cpu_patch_instruction_count;
            }
            if (rewrite.protect_red_zone) {
                ++result.unrelocatable_memory_instruction_count;
            }
        };
        const auto overlaps_patched_span = [&patched_spans](uintptr_t start, uintptr_t end) {
            return std::ranges::any_of(patched_spans, [start, end](const auto& patched) {
                return start < patched.second && patched.first < end;
            });
        };

        for (const uintptr_t site : unresolved_sites) {
            if (module->patched.contains(reinterpret_cast<u8*>(site))) {
                continue;
            }

            const auto site_instruction = function.instructions.find(site);
            ASSERT(site_instruction != function.instructions.end());
            if (function.has_indirect_branch ||
                site_instruction->second.instruction.length < ShortJumpSize) {
                record_unsupported(site);
                continue;
            }

            const RelocationSpan site_span{
                .instructions = {&site_instruction->second},
                .patch_start = site,
                .continuation = site + site_instruction->second.instruction.length,
                .patch_size = site_instruction->second.instruction.length,
            };
            const size_t trampoline_start = module->trampoline_gen.getSize();
            const auto site_trampoline_offset = emit_span(site_span);
            if (!site_trampoline_offset) {
                record_unsupported(site);
                continue;
            }

            constexpr s64 ShortJumpMin = std::numeric_limits<s8>::min();
            constexpr s64 ShortJumpMax = std::numeric_limits<s8>::max();
            const auto relay_slot = std::ranges::find_if(relay_slots, [site](uintptr_t address) {
                const s64 displacement =
                    static_cast<s64>(address) - static_cast<s64>(site + ShortJumpSize);
                return displacement >= ShortJumpMin && displacement <= ShortJumpMax;
            });
            if (relay_slot != relay_slots.end()) {
                const auto* site_trampoline =
                    module->trampoline_gen.getCode() + *site_trampoline_offset;

                auto& patch_gen = module->patch_gen;
                patch_gen.reset();
                patch_gen.setSize(*relay_slot - reinterpret_cast<uintptr_t>(patch_gen.getCode()));
                patch_gen.jmp(site_trampoline, Xbyak::CodeGenerator::LabelType::T_NEAR);

                patch_gen.reset();
                patch_gen.setSize(site - reinterpret_cast<uintptr_t>(patch_gen.getCode()));
                patch_gen.jmp(reinterpret_cast<void*>(*relay_slot),
                              Xbyak::CodeGenerator::LabelType::T_SHORT);
                patch_gen.nop(site_span.patch_size - ShortJumpSize);

                record_rewrites(site_span);
                patched_spans.emplace_back(site_span.patch_start, site_span.continuation);
                std::erase(short_relay_slots, *relay_slot);
                relay_slots.erase(relay_slot);
                continue;
            }

            const auto find_host = [&](uintptr_t short_jump_address) {
                std::pair<std::optional<RelocationSpan>, std::optional<size_t>> result;
                const s64 minimum_host = static_cast<s64>(short_jump_address) +
                                         static_cast<s64>(ShortJumpSize) -
                                         static_cast<s64>(NearJumpSize) + ShortJumpMin;
                const s64 maximum_host = static_cast<s64>(short_jump_address) +
                                         static_cast<s64>(ShortJumpSize) -
                                         static_cast<s64>(NearJumpSize) + ShortJumpMax;

                auto candidate = function.instructions.lower_bound(
                    static_cast<uintptr_t>(std::max<s64>(minimum_host, 0)));
                for (; candidate != function.instructions.end() &&
                       static_cast<s64>(candidate->first) <= maximum_host;
                     ++candidate) {
                    RelocationSpan span{.patch_start = candidate->first,
                                        .continuation = candidate->first};
                    while (span.patch_size < NearJumpSize * 2) {
                        const auto instruction = function.instructions.find(span.continuation);
                        if (instruction == function.instructions.end() ||
                            (span.continuation != span.patch_start &&
                             function.branch_targets.contains(span.continuation))) {
                            span.instructions.clear();
                            break;
                        }
                        span.instructions.push_back(&instruction->second);
                        span.patch_size += instruction->second.instruction.length;
                        span.continuation += instruction->second.instruction.length;
                        if (span.patch_size < NearJumpSize * 2 &&
                            IsControlFlowTerminator(instruction->second.instruction)) {
                            span.instructions.clear();
                            break;
                        }
                    }
                    if (span.instructions.empty() ||
                        !(span.continuation <= site ||
                          span.patch_start >= site_span.continuation) ||
                        overlaps_patched_span(span.patch_start, span.continuation)) {
                        continue;
                    }

                    const uintptr_t relay_address = span.patch_start + NearJumpSize;
                    const s64 displacement = static_cast<s64>(relay_address) -
                                             static_cast<s64>(short_jump_address + ShortJumpSize);
                    if (displacement < ShortJumpMin || displacement > ShortJumpMax) {
                        continue;
                    }

                    const auto offset = emit_span(span);
                    if (!offset) {
                        continue;
                    }
                    result.first = std::move(span);
                    result.second = offset;
                    break;
                }
                return result;
            };

            auto [host_span, host_trampoline_offset] = find_host(site);
            std::optional<uintptr_t> final_relay_slot;
            uintptr_t final_short_jump = site;
            std::map<uintptr_t, uintptr_t> relay_parent{{site, site}};
            std::vector<uintptr_t> relay_queue{site};
            for (size_t queue_index = 0;
                 !host_span && !final_relay_slot && queue_index < relay_queue.size();
                 ++queue_index) {
                const uintptr_t current = relay_queue[queue_index];
                if (current != site) {
                    if (std::ranges::find(relay_slots, current) != relay_slots.end()) {
                        final_relay_slot = current;
                        final_short_jump = relay_parent.at(current);
                        break;
                    }
                    const auto near_slot =
                        std::ranges::find_if(relay_slots, [current](uintptr_t address) {
                            const s64 displacement = static_cast<s64>(address) -
                                                     static_cast<s64>(current + ShortJumpSize);
                            return address != current && displacement >= ShortJumpMin &&
                                   displacement <= ShortJumpMax;
                        });
                    if (near_slot != relay_slots.end()) {
                        final_relay_slot = *near_slot;
                        final_short_jump = current;
                        break;
                    }

                    auto bridge_host = find_host(current);
                    if (bridge_host.first) {
                        host_span = std::move(bridge_host.first);
                        host_trampoline_offset = bridge_host.second;
                        final_short_jump = current;
                        break;
                    }
                }

                for (const uintptr_t slot : short_relay_slots) {
                    if (relay_parent.contains(slot)) {
                        continue;
                    }
                    const s64 displacement =
                        static_cast<s64>(slot) - static_cast<s64>(current + ShortJumpSize);
                    if (displacement < ShortJumpMin || displacement > ShortJumpMax) {
                        continue;
                    }
                    relay_parent.emplace(slot, current);
                    relay_queue.push_back(slot);
                }
            }

            if (!host_span && !final_relay_slot) {
                module->trampoline_gen.setSize(trampoline_start);
                record_unsupported(site);
                continue;
            }

            const auto* site_trampoline =
                module->trampoline_gen.getCode() + *site_trampoline_offset;
            auto& patch_gen = module->patch_gen;
            uintptr_t relay_address{};
            if (final_relay_slot) {
                relay_address = *final_relay_slot;
                patch_gen.reset();
                patch_gen.setSize(relay_address - reinterpret_cast<uintptr_t>(patch_gen.getCode()));
                patch_gen.jmp(site_trampoline, Xbyak::CodeGenerator::LabelType::T_NEAR);
                std::erase(relay_slots, relay_address);
                std::erase(short_relay_slots, relay_address);
            } else {
                ASSERT(host_span && host_trampoline_offset);
                const auto* host_trampoline =
                    module->trampoline_gen.getCode() + *host_trampoline_offset;
                relay_address = host_span->patch_start + NearJumpSize;

                patch_gen.reset();
                patch_gen.setSize(host_span->patch_start -
                                  reinterpret_cast<uintptr_t>(patch_gen.getCode()));
                patch_gen.jmp(host_trampoline, Xbyak::CodeGenerator::LabelType::T_NEAR);
                patch_gen.jmp(site_trampoline, Xbyak::CodeGenerator::LabelType::T_NEAR);
                patch_gen.nop(host_span->patch_size - NearJumpSize * 2);
            }

            uintptr_t jump_target = relay_address;
            while (final_short_jump != site) {
                patch_gen.reset();
                patch_gen.setSize(final_short_jump -
                                  reinterpret_cast<uintptr_t>(patch_gen.getCode()));
                patch_gen.jmp(reinterpret_cast<void*>(jump_target),
                              Xbyak::CodeGenerator::LabelType::T_SHORT);
                jump_target = final_short_jump;
                const auto parent = relay_parent.find(final_short_jump);
                ASSERT(parent != relay_parent.end());
                final_short_jump = parent->second;
                std::erase(relay_slots, jump_target);
                std::erase(short_relay_slots, jump_target);
            }

            patch_gen.reset();
            patch_gen.setSize(site - reinterpret_cast<uintptr_t>(patch_gen.getCode()));
            patch_gen.jmp(reinterpret_cast<void*>(jump_target),
                          Xbyak::CodeGenerator::LabelType::T_SHORT);
            patch_gen.nop(site_span.patch_size - ShortJumpSize);

            if (host_span) {
                record_rewrites(*host_span);
                patched_spans.emplace_back(host_span->patch_start, host_span->continuation);
                if (host_span->patch_size >= NearJumpSize * 3) {
                    relay_slots.push_back(host_span->patch_start + NearJumpSize * 2);
                }
                if (host_span->patch_size >= NearJumpSize * 2 + ShortJumpSize) {
                    short_relay_slots.push_back(host_span->patch_start + NearJumpSize * 2);
                }
            }
            record_rewrites(site_span);
            patched_spans.emplace_back(site_span.patch_start, site_span.continuation);
        }
    }
    return result;
}

#else

RedZonePatchResult PatchRedZoneMemoryInstructions(u64, u64, std::span<const uintptr_t>) {
    return {};
}

#endif

// ============================================================================
// End Windows static guest red-zone protection
// ============================================================================

static bool PatchesAccessViolationHandler(void* context, void* /* fault_address */) {
    return TryPatchJit(Common::GetRip(context));
}

static bool PatchesIllegalInstructionHandler(void* context) {
    void* code_address = Common::GetRip(context);
#if defined(_WIN32)
    // Windows static guest red-zone protection
    const bool inspect_short_cpu_patch =
        !WindowsGuestRedZoneProtection::IsStaticPatchingEnabled() ||
        GetContainingModule(code_address) != nullptr;
#else
    constexpr bool inspect_short_cpu_patch = true;
#endif
    if (inspect_short_cpu_patch && // Windows static guest red-zone protection
        Is4ByteExtrqOrInsertq(code_address)) {
        // The instruction is not big enough for a relative jump, don't try to patch it and pass it
        // to our illegal instruction interpreter directly
        return TryExecuteIllegalInstruction(context, code_address);
    } else {
        if (!TryPatchJit(code_address)) {
            ZydisDecodedInstruction instruction;
            ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
            const auto status =
                Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);
            if (ZYAN_SUCCESS(status)) {
                const auto disassembled = Common::Decoder::Instance()->disassembleInst(
                    instruction, operands, std::bit_cast<u64>(code_address));
                LOG_ERROR(Core, "Failed to patch address {:x} -- mnemonic: {}, instruction: {}",
                          reinterpret_cast<u64>(code_address),
                          ZydisMnemonicGetString(instruction.mnemonic), disassembled);
            } else {
                LOG_ERROR(Core, "Failed to patch address {:x} -- mnemonic: (failed to decode)",
                          reinterpret_cast<u64>(code_address));
            }
            return false;
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
