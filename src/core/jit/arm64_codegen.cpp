// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <sys/mman.h>
#include "arm64_codegen.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/types.h"
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

namespace Core::Jit {

static constexpr size_t PAGE_SIZE = 4096;
static constexpr size_t ALIGNMENT = 16;

static size_t alignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static void* allocateExecutableMemory(size_t size) {
    size = alignUp(size, PAGE_SIZE);
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // On macOS ARM64:
    // 1. Allocate with PROT_READ | PROT_WRITE (no PROT_EXEC initially)
    // 2. Use pthread_jit_write_protect_np to allow writing
    // 3. After writing, use mprotect to add PROT_EXEC
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_CRITICAL(Core, "Failed to allocate executable memory: {} (errno={})", strerror(errno),
                     errno);
        return nullptr;
    }
    // Initially disable write protection so we can write code
    pthread_jit_write_protect_np(0);
    return ptr;
#else
    void* ptr =
        mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_CRITICAL(Core, "Failed to allocate executable memory: {}", strerror(errno));
        return nullptr;
    }
    return ptr;
#endif
}

Arm64CodeGenerator::Arm64CodeGenerator(size_t buffer_size, void* code_ptr)
    : buffer_size(alignUp(buffer_size, PAGE_SIZE)), owns_buffer(code_ptr == nullptr) {
    if (code_ptr) {
        code_buffer = code_ptr;
        this->code_ptr = code_ptr;
    } else {
        code_buffer = allocateExecutableMemory(buffer_size);
        this->code_ptr = code_buffer;
    }
    if (!code_buffer) {
        throw std::bad_alloc();
    }
}

Arm64CodeGenerator::~Arm64CodeGenerator() {
    if (owns_buffer && code_buffer) {
        munmap(code_buffer, buffer_size);
    }
}

void Arm64CodeGenerator::reset() {
    code_ptr = code_buffer;
    fixups.clear();
}

void Arm64CodeGenerator::setSize(size_t offset) {
    code_ptr = static_cast<u8*>(code_buffer) + offset;
}

void Arm64CodeGenerator::emit32(u32 instruction) {
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // On macOS ARM64, disable write protection before writing
    pthread_jit_write_protect_np(0);
#endif
    u8* curr = static_cast<u8*>(code_ptr);
    u8* end = static_cast<u8*>(code_buffer) + buffer_size;
    ASSERT_MSG(curr + 4 <= end, "Code buffer overflow");
    *reinterpret_cast<u32*>(curr) = instruction;
    code_ptr = curr + 4;
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // Re-enable write protection after writing
    pthread_jit_write_protect_np(1);
#endif
}

void Arm64CodeGenerator::emit64(u64 instruction) {
    emit32(static_cast<u32>(instruction));
    emit32(static_cast<u32>(instruction >> 32));
}

void* Arm64CodeGenerator::allocateCode(size_t size) {
    size = alignUp(size, ALIGNMENT);
    void* result = code_ptr;
    u8* curr = static_cast<u8*>(code_ptr);
    u8* end = static_cast<u8*>(code_buffer) + buffer_size;
    code_ptr = curr + size;
    ASSERT_MSG(static_cast<u8*>(code_ptr) <= end, "Code buffer overflow");
    return result;
}

void Arm64CodeGenerator::makeExecutable() {
    size_t size = getSize();
    size = alignUp(size, PAGE_SIZE);
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // On macOS ARM64, re-enable write protection before making executable
    pthread_jit_write_protect_np(1);
    // Flush instruction cache
    __builtin___clear_cache(static_cast<char*>(code_buffer),
                            static_cast<char*>(code_buffer) + size);
#endif
    if (mprotect(code_buffer, size, PROT_READ | PROT_EXEC) != 0) {
        LOG_CRITICAL(Core, "Failed to make code executable: {}", strerror(errno));
    }
}

// Memory operations
void Arm64CodeGenerator::ldr(int reg, void* addr) {
    movz(9, reinterpret_cast<u64>(addr) & 0xFFFF);
    movk(9, (reinterpret_cast<u64>(addr) >> 16) & 0xFFFF, 16);
    movk(9, (reinterpret_cast<u64>(addr) >> 32) & 0xFFFF, 32);
    movk(9, (reinterpret_cast<u64>(addr) >> 48) & 0xFFFF, 48);
    ldr(reg, 9, 0);
}

void Arm64CodeGenerator::ldr(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 32768 && (offset % 8 == 0)) {
        emit32(0xF9400000 | (reg << 0) | (base_reg << 5) | ((offset / 8) << 10));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        ldr(reg, 9, 0);
    }
}

void Arm64CodeGenerator::ldrh(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 8192 && (offset % 2 == 0)) {
        emit32(0x79400000 | (reg << 0) | (base_reg << 5) | ((offset / 2) << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        ldrh(reg, 9, 0);
    }
}

void Arm64CodeGenerator::ldrb(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 4096) {
        emit32(0x39400000 | (reg << 0) | (base_reg << 5) | (offset << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        ldrb(reg, 9, 0);
    }
}

void Arm64CodeGenerator::ldp(int reg1, int reg2, int base_reg, s32 offset) {
    if (offset >= -256 && offset < 256 && (offset % 8 == 0)) {
        s32 scaled_offset = offset / 8;
        u32 imm7 = (scaled_offset >= 0) ? scaled_offset : (64 + scaled_offset);
        emit32(0xA9400000 | (reg1 << 0) | (reg2 << 10) | (base_reg << 5) | (imm7 << 15));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        ldp(reg1, reg2, 9, 0);
    }
}

void Arm64CodeGenerator::str(int reg, void* addr) {
    movz(9, reinterpret_cast<u64>(addr) & 0xFFFF);
    movk(9, (reinterpret_cast<u64>(addr) >> 16) & 0xFFFF, 16);
    movk(9, (reinterpret_cast<u64>(addr) >> 32) & 0xFFFF, 32);
    movk(9, (reinterpret_cast<u64>(addr) >> 48) & 0xFFFF, 48);
    str(reg, 9, 0);
}

void Arm64CodeGenerator::str(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 32768 && (offset % 8 == 0)) {
        emit32(0xF9000000 | (reg << 0) | (base_reg << 5) | ((offset / 8) << 10));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        str(reg, 9, 0);
    }
}

void Arm64CodeGenerator::strh(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 8192 && (offset % 2 == 0)) {
        emit32(0x79000000 | (reg << 0) | (base_reg << 5) | ((offset / 2) << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        strh(reg, 9, 0);
    }
}

void Arm64CodeGenerator::strb(int reg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 4096) {
        emit32(0x39000000 | (reg << 0) | (base_reg << 5) | (offset << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        strb(reg, 9, 0);
    }
}

void Arm64CodeGenerator::stp(int reg1, int reg2, int base_reg, s32 offset) {
    if (offset >= -256 && offset < 256 && (offset % 8 == 0)) {
        s32 scaled_offset = offset / 8;
        u32 imm7 = (scaled_offset >= 0) ? scaled_offset : (64 + scaled_offset);
        emit32(0xA9000000 | (reg1 << 0) | (reg2 << 10) | (base_reg << 5) | (imm7 << 15));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        stp(reg1, reg2, 9, 0);
    }
}

// Arithmetic operations
void Arm64CodeGenerator::add(int dst, int src1, int src2) {
    emit32(0x8B000000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::add_imm(int dst, int src1, s32 imm) {
    if (imm >= 0 && imm < 4096) {
        emit32(0x91000000 | (dst << 0) | (src1 << 5) | (imm << 10));
    } else if (imm < 0 && imm > -4096) {
        sub_imm(dst, src1, -imm);
    } else {
        mov_imm(9, imm);
        add(dst, src1, 9);
    }
}

void Arm64CodeGenerator::sub(int dst, int src1, int src2) {
    emit32(0xCB000000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::sub_imm(int dst, int src1, s32 imm) {
    if (imm >= 0 && imm < 4096) {
        emit32(0xD1000000 | (dst << 0) | (src1 << 5) | (imm << 10));
    } else if (imm < 0 && imm > -4096) {
        add_imm(dst, src1, -imm);
    } else {
        mov_imm(9, imm);
        sub(dst, src1, 9);
    }
}

void Arm64CodeGenerator::mul(int dst, int src1, int src2) {
    emit32(0x9B007C00 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::sdiv(int dst, int src1, int src2) {
    emit32(0x9AC00C00 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::udiv(int dst, int src1, int src2) {
    emit32(0x9AC00800 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::and_(int dst, int src1, int src2) {
    emit32(0x8A000000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::and_(int dst, int src1, u64 imm) {
    if (imm <= 0xFFF) {
        emit32(0x92000000 | (dst << 0) | (src1 << 5) | (static_cast<u32>(imm) << 10));
    } else {
        mov_imm(9, imm);
        and_(dst, src1, 9);
    }
}

void Arm64CodeGenerator::orr(int dst, int src1, int src2) {
    emit32(0xAA000000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::orr(int dst, int src1, u64 imm) {
    if (imm <= 0xFFF) {
        emit32(0xB2000000 | (dst << 0) | (src1 << 5) | (static_cast<u32>(imm) << 10));
    } else {
        mov_imm(9, imm);
        orr(dst, src1, 9);
    }
}

void Arm64CodeGenerator::eor(int dst, int src1, int src2) {
    emit32(0xCA000000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::eor(int dst, int src1, u64 imm) {
    if (imm <= 0xFFF) {
        emit32(0xD2000000 | (dst << 0) | (src1 << 5) | (static_cast<u32>(imm) << 10));
    } else {
        mov_imm(9, imm);
        eor(dst, src1, 9);
    }
}

void Arm64CodeGenerator::mvn(int dst, int src) {
    emit32(0xAA200000 | (dst << 0) | (src << 16));
}

void Arm64CodeGenerator::lsl(int dst, int src1, int src2) {
    emit32(0x9AC02000 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::lsl(int dst, int src1, u8 shift) {
    ASSERT_MSG(shift < 64, "Shift amount must be < 64");
    emit32(0xD3400000 | (dst << 0) | (src1 << 5) | (shift << 10));
}

void Arm64CodeGenerator::lsr(int dst, int src1, int src2) {
    emit32(0x9AC02400 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::lsr(int dst, int src1, u8 shift) {
    ASSERT_MSG(shift < 64, "Shift amount must be < 64");
    emit32(0xD3500000 | (dst << 0) | (src1 << 5) | (shift << 10));
}

void Arm64CodeGenerator::asr(int dst, int src1, int src2) {
    emit32(0x9AC02800 | (dst << 0) | (src1 << 5) | (src2 << 16));
}

void Arm64CodeGenerator::asr(int dst, int src1, u8 shift) {
    ASSERT_MSG(shift < 64, "Shift amount must be < 64");
    emit32(0xD3600000 | (dst << 0) | (src1 << 5) | (shift << 10));
}

// Move operations
void Arm64CodeGenerator::mov(int dst, int src) {
    if (dst != src) {
        emit32(0xAA0003E0 | (dst << 0) | (src << 16));
    }
}

void Arm64CodeGenerator::mov_imm(int dst, s64 imm) {
    if (imm >= 0 && imm <= 0xFFFF) {
        movz(dst, static_cast<u16>(imm));
    } else if (imm >= -0x10000 && imm < 0) {
        movn(dst, static_cast<u16>(-imm - 1));
    } else {
        movz(dst, imm & 0xFFFF);
        if ((imm >> 16) & 0xFFFF) {
            movk(dst, (imm >> 16) & 0xFFFF, 16);
        }
        if ((imm >> 32) & 0xFFFF) {
            movk(dst, (imm >> 32) & 0xFFFF, 32);
        }
        if ((imm >> 48) & 0xFFFF) {
            movk(dst, (imm >> 48) & 0xFFFF, 48);
        }
    }
}

void Arm64CodeGenerator::movz(int dst, u16 imm, u8 shift) {
    ASSERT_MSG(shift % 16 == 0 && shift < 64, "Shift must be multiple of 16 and < 64");
    emit32(0xD2800000 | (dst << 0) | (imm << 5) | ((shift / 16) << 21));
}

void Arm64CodeGenerator::movk(int dst, u16 imm, u8 shift) {
    ASSERT_MSG(shift % 16 == 0 && shift < 64, "Shift must be multiple of 16 and < 64");
    emit32(0xF2800000 | (dst << 0) | (imm << 5) | ((shift / 16) << 21));
}

void Arm64CodeGenerator::movn(int dst, u16 imm, u8 shift) {
    ASSERT_MSG(shift % 16 == 0 && shift < 64, "Shift must be multiple of 16 and < 64");
    emit32(0x92800000 | (dst << 0) | (imm << 5) | ((shift / 16) << 21));
}

// Compare operations
void Arm64CodeGenerator::cmp(int reg1, int reg2) {
    emit32(0xEB000000 | (31 << 0) | (reg1 << 5) | (reg2 << 16));
}

void Arm64CodeGenerator::cmp_imm(int reg, s32 imm) {
    if (imm >= 0 && imm < 4096) {
        emit32(0xF1000000 | (31 << 0) | (reg << 5) | (imm << 10));
    } else {
        mov_imm(9, imm);
        cmp(reg, 9);
    }
}

void Arm64CodeGenerator::tst(int reg1, int reg2) {
    emit32(0xEA000000 | (31 << 0) | (reg1 << 5) | (reg2 << 16));
}

void Arm64CodeGenerator::tst(int reg, u64 imm) {
    if (imm <= 0xFFF) {
        emit32(0xF2000000 | (31 << 0) | (reg << 5) | (static_cast<u32>(imm) << 10));
    } else {
        mov(9, imm);
        tst(reg, 9);
    }
}

// Branch operations
void Arm64CodeGenerator::b(void* target) {
    s64 offset = reinterpret_cast<s64>(target) - reinterpret_cast<s64>(code_ptr);
    if (offset >= -0x8000000 && offset < 0x8000000) {
        s32 imm26 = static_cast<s32>(offset / 4);
        emit32(0x14000000 | (imm26 & 0x3FFFFFF));
    } else {
        movz(9, reinterpret_cast<u64>(target) & 0xFFFF);
        movk(9, (reinterpret_cast<u64>(target) >> 16) & 0xFFFF, 16);
        movk(9, (reinterpret_cast<u64>(target) >> 32) & 0xFFFF, 32);
        movk(9, (reinterpret_cast<u64>(target) >> 48) & 0xFFFF, 48);
        br(9);
    }
}

void Arm64CodeGenerator::b(int condition, void* target) {
    s64 offset = reinterpret_cast<s64>(target) - reinterpret_cast<s64>(code_ptr);
    if (offset >= -0x8000000 && offset < 0x8000000) {
        s32 imm19 = static_cast<s32>(offset / 4);
        emit32(0x54000000 | (condition << 0) | (imm19 << 5));
    } else {
        movz(9, reinterpret_cast<u64>(target) & 0xFFFF);
        movk(9, (reinterpret_cast<u64>(target) >> 16) & 0xFFFF, 16);
        movk(9, (reinterpret_cast<u64>(target) >> 32) & 0xFFFF, 32);
        movk(9, (reinterpret_cast<u64>(target) >> 48) & 0xFFFF, 48);
        emit32(0x54000000 | (condition << 0) | (0 << 5));
        br(9);
    }
}

void Arm64CodeGenerator::bl(void* target) {
    s64 offset = reinterpret_cast<s64>(target) - reinterpret_cast<s64>(code_ptr);
    if (offset >= -0x8000000 && offset < 0x8000000) {
        s32 imm26 = static_cast<s32>(offset / 4);
        emit32(0x94000000 | (imm26 & 0x3FFFFFF));
    } else {
        movz(9, reinterpret_cast<u64>(target) & 0xFFFF);
        movk(9, (reinterpret_cast<u64>(target) >> 16) & 0xFFFF, 16);
        movk(9, (reinterpret_cast<u64>(target) >> 32) & 0xFFFF, 32);
        movk(9, (reinterpret_cast<u64>(target) >> 48) & 0xFFFF, 48);
        blr(9);
    }
}

void Arm64CodeGenerator::br(int reg) {
    emit32(0xD61F0000 | (reg << 5));
}

void Arm64CodeGenerator::blr(int reg) {
    emit32(0xD63F0000 | (reg << 5));
}

void Arm64CodeGenerator::ret(int reg) {
    emit32(0xD65F0000 | (reg << 5));
}

// Conditional branches
void Arm64CodeGenerator::b_eq(void* target) {
    b(0, target);
}
void Arm64CodeGenerator::b_ne(void* target) {
    b(1, target);
}
void Arm64CodeGenerator::b_lt(void* target) {
    b(11, target);
}
void Arm64CodeGenerator::b_le(void* target) {
    b(13, target);
}
void Arm64CodeGenerator::b_gt(void* target) {
    b(12, target);
}
void Arm64CodeGenerator::b_ge(void* target) {
    b(10, target);
}
void Arm64CodeGenerator::b_lo(void* target) {
    b(3, target);
}
void Arm64CodeGenerator::b_ls(void* target) {
    b(9, target);
}
void Arm64CodeGenerator::b_hi(void* target) {
    b(8, target);
}
void Arm64CodeGenerator::b_hs(void* target) {
    b(2, target);
}

// Stack operations
void Arm64CodeGenerator::push(int reg) {
    sub(31, 31, 16);
    str(reg, 31, 0);
}

void Arm64CodeGenerator::push(int reg1, int reg2) {
    sub(31, 31, 16);
    stp(reg1, reg2, 31, 0);
}

void Arm64CodeGenerator::pop(int reg) {
    ldr(reg, 31, 0);
    add(31, 31, 16);
}

void Arm64CodeGenerator::pop(int reg1, int reg2) {
    ldp(reg1, reg2, 31, 0);
    add(31, 31, 16);
}

// System operations
void Arm64CodeGenerator::nop() {
    emit32(0xD503201F);
}

void Arm64CodeGenerator::brk(u16 imm) {
    emit32(0xD4200000 | (imm << 5));
}

// NEON/SIMD operations
void Arm64CodeGenerator::ldr_v(int vreg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 4096 && (offset % 16 == 0)) {
        emit32(0x3DC00000 | (vreg << 0) | (base_reg << 5) | ((offset / 16) << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        ldr_v(vreg, 9, 0);
    }
}

void Arm64CodeGenerator::str_v(int vreg, int base_reg, s32 offset) {
    if (offset >= 0 && offset < 4096 && (offset % 16 == 0)) {
        emit32(0x3D800000 | (vreg << 0) | (base_reg << 5) | ((offset / 16) << 12));
    } else {
        mov_imm(9, offset);
        add(9, base_reg, 9);
        str_v(vreg, 9, 0);
    }
}

void Arm64CodeGenerator::mov_v(int vdst, int vsrc) {
    emit32(0x4EA01C00 | (vdst << 0) | (vsrc << 5));
}

void Arm64CodeGenerator::add_v(int vdst, int vsrc1, int vsrc2) {
    emit32(0x4E208400 | (vdst << 0) | (vsrc1 << 5) | (vsrc2 << 16));
}

void Arm64CodeGenerator::sub_v(int vdst, int vsrc1, int vsrc2) {
    emit32(0x4EA08400 | (vdst << 0) | (vsrc1 << 5) | (vsrc2 << 16));
}

void Arm64CodeGenerator::mul_v(int vdst, int vsrc1, int vsrc2) {
    emit32(0x4E209C00 | (vdst << 0) | (vsrc1 << 5) | (vsrc2 << 16));
}

} // namespace Core::Jit
