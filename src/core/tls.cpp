// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xbyak/xbyak.h>
#include "common/assert.h"
#include "common/types.h"
#include "core/tls.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Core {

struct TLSPattern {
    u8 pattern[5];
    u8 pattern_size;
    u8 imm_size;
    u8 target_reg;
};

constexpr static TLSPattern TlsPatterns[] = {
    // 64 48 A1 | 00 00 00 00 00 00 00 00 # mov rax, qword ptr fs:[64b imm]
    {{0x64, 0x48, 0xA1}, 3, 8, 0},
    // 64 48 8B 04 25 | 00 00 00 00 # mov rax,qword ptr fs:[0]
    {{0x64, 0x48, 0x8B, 0x4, 0x25}, 5, 4, 0},   // rax
    {{0x64, 0x48, 0x8B, 0xC, 0x25}, 5, 4, 1},   // rcx
    {{0x64, 0x48, 0x8B, 0x14, 0x25}, 5, 4, 2},  // rdx
    {{0x64, 0x48, 0x8B, 0x1C, 0x25}, 5, 4, 3},  // rbx
    {{0x64, 0x48, 0x8B, 0x24, 0x25}, 5, 4, 4},  // rsp
    {{0x64, 0x48, 0x8B, 0x2C, 0x25}, 5, 4, 5},  // rbp
    {{0x64, 0x48, 0x8B, 0x34, 0x25}, 5, 4, 6},  // rsi
    {{0x64, 0x48, 0x8B, 0x3C, 0x25}, 5, 4, 7},  // rdi
    {{0x64, 0x4C, 0x8B, 0x4, 0x25}, 5, 4, 8},   // r8
    {{0x64, 0x4C, 0x8B, 0xC, 0x25}, 5, 4, 9},   // r9
    {{0x64, 0x4C, 0x8B, 0x14, 0x25}, 5, 4, 10}, // r10
    {{0x64, 0x4C, 0x8B, 0x1C, 0x25}, 5, 4, 11}, // r11
    {{0x64, 0x4C, 0x8B, 0x24, 0x25}, 5, 4, 12}, // r12
    {{0x64, 0x4C, 0x8B, 0x2C, 0x25}, 5, 4, 13}, // r13
    {{0x64, 0x4C, 0x8B, 0x34, 0x25}, 5, 4, 14}, // r14
    {{0x64, 0x4C, 0x8B, 0x3C, 0x25}, 5, 4, 15}, // r15
};

#ifdef _WIN32
static DWORD slot = 0;

void SetTcbBase(void* image_address) {
    const BOOL result = TlsSetValue(slot, image_address);
    ASSERT(result != 0);
}

Tcb* GetTcbBase() {
    return reinterpret_cast<Tcb*>(TlsGetValue(slot));
}

static void AllocTcbKey() {
    slot = TlsAlloc();
}

static void PatchFsAccess(u8* code, const TLSPattern& tls_pattern, Xbyak::CodeGenerator& c) {
    using namespace Xbyak::util;
    const auto total_size = tls_pattern.pattern_size + tls_pattern.imm_size;

    // Replace mov instruction with near jump to the trampoline.
    static constexpr u32 NearJmpSize = 5;
    auto patch = Xbyak::CodeGenerator(total_size, code);
    patch.jmp(c.getCurr(), Xbyak::CodeGenerator::LabelType::T_NEAR);
    patch.nop(total_size - NearJmpSize);

    // Write the trampoline.
    // The following logic is based on the wine implementation of TlsGetValue
    // https://github.com/wine-mirror/wine/blob/a27b9551/dlls/kernelbase/thread.c#L719
    static constexpr u32 TlsSlotsOffset = 0x1480;
    static constexpr u32 TlsExpansionSlotsOffset = 0x1780;
    static constexpr u32 TlsMinimumAvailable = 64;
    const u32 teb_offset = slot < TlsMinimumAvailable ? TlsSlotsOffset : TlsExpansionSlotsOffset;
    const u32 tls_index = slot < TlsMinimumAvailable ? slot : slot - TlsMinimumAvailable;

    const auto target_reg = Xbyak::Reg64(tls_pattern.target_reg);
    c.mov(target_reg, teb_offset);
    c.putSeg(gs);
    c.mov(target_reg, ptr[target_reg]); // Load the pointer to the table of tls slots.
    c.mov(target_reg,
          qword[target_reg + tls_index * sizeof(LPVOID)]); // Load the pointer to our buffer.
    c.jmp(code + total_size); // Return to the instruction right after the mov.
}

#else

static pthread_key_t slot = 0;

void SetTcbBase(void* image_address) {
    ASSERT(pthread_setspecific(slot, image_address) == 0);
}

Tcb* GetTcbBase() {
    return reinterpret_cast<Tcb*>(pthread_getspecific(slot));
}

static void AllocTcbKey() {
    ASSERT(pthread_key_create(&slot, nullptr) == 0);
}

static void PatchFsAccess(u8* code, const TLSPattern& tls_pattern, Xbyak::CodeGenerator& c) {
    using namespace Xbyak::util;
    const auto total_size = tls_pattern.pattern_size + tls_pattern.imm_size;

    // Replace mov instruction with near jump to the trampoline.
    static constexpr u32 NearJmpSize = 5;
    auto patch = Xbyak::CodeGenerator(total_size, code);
    patch.jmp(c.getCurr(), Xbyak::CodeGenerator::LabelType::T_NEAR);
    patch.nop(total_size - NearJmpSize);

    // Write the trampoline.
    // The following logic is based on the glibc implementation of pthread_getspecific
    // https://github.com/bminor/glibc/blob/29807a27/nptl/pthread_getspecific.c#L23
    static constexpr u32 PthreadKeySecondLevelSize = 32;
    static constexpr u32 PthreadSpecificOffset = 0x510;
    static constexpr u32 PthreadKeyDataSize = 16;
    ASSERT(slot >= PthreadKeySecondLevelSize);

    const u32 idx1st = slot / PthreadKeySecondLevelSize;
    const u32 idx2nd = slot % PthreadKeySecondLevelSize;
    const auto target_reg = Xbyak::Reg64(tls_pattern.target_reg);
    c.putSeg(fs);
    c.mov(target_reg,
          qword[PthreadSpecificOffset + idx1st * 8]); // Load first level specific array.
    c.mov(target_reg, qword[target_reg + idx2nd * 16 +
                            8]); // Load data member of pthread_key_data our slot specifies.
    c.jmp(code + total_size);    // Return to the instruction right after the mov.
}

#endif

void PatchTLS(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c) {
    u8* code = reinterpret_cast<u8*>(segment_addr);
    auto remaining_size = segment_size;

    // Sometimes loads from the FS segment are prefixed with useless operand size prefix bytes like:
    // |66 66 66| 64 48 8b 04 25 00 # mov rax, qword ptr fs:[0x0]
    // These are probably ignored by the processor but when patching the instruction to a jump
    // they cause issues. So look for them and patch them to nop to avoid problems.
    static constexpr std::array<u8, 3> BadPrefix = {0x66, 0x66, 0x66};

    while (remaining_size) {
        for (const auto& tls_pattern : TlsPatterns) {
            const auto total_size = tls_pattern.pattern_size + tls_pattern.imm_size;
            if (remaining_size < total_size) {
                continue;
            }
            if (std::memcmp(code, tls_pattern.pattern, tls_pattern.pattern_size) != 0) {
                continue;
            }
            u64 offset = 0;
            if (tls_pattern.imm_size == 4) {
                std::memcpy(&offset, code + tls_pattern.pattern_size, sizeof(u32));
                LOG_INFO(Core_Linker, "PATTERN32 FOUND at {}, reg: {} offset: {:#x}",
                         fmt::ptr(code), tls_pattern.target_reg, offset);
            } else {
                std::memcpy(&offset, code + tls_pattern.pattern_size, sizeof(u64));
                LOG_INFO(Core_Linker, "PATTERN64 FOUND at {}, reg: {} offset: {:#x}",
                         fmt::ptr(code), tls_pattern.target_reg, offset);
                continue;
            }
            ASSERT(offset == 0);

            // Allocate slot in the process if not done already.
            if (slot == 0) {
                AllocTcbKey();
            }

            // Replace bogus instruction prefix with nops if it exists.
            if (std::memcmp(code - BadPrefix.size(), BadPrefix.data(), sizeof(BadPrefix)) == 0) {
                auto patch = Xbyak::CodeGenerator(BadPrefix.size(), code - BadPrefix.size());
                patch.nop(BadPrefix.size());
            }

            // Patch access to FS register to a trampoline.
            PatchFsAccess(code, tls_pattern, c);

            // Move ahead in module.
            code += total_size - 1;
            remaining_size -= total_size - 1;
            break;
        }
        code++;
        remaining_size--;
    }
}

} // namespace Core
