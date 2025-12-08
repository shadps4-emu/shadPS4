// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/types.h"

namespace Core::Jit {

class Arm64CodeGenerator {
public:
    explicit Arm64CodeGenerator(size_t buffer_size = 64_KB, void* code_ptr = nullptr);
    ~Arm64CodeGenerator();

    Arm64CodeGenerator(const Arm64CodeGenerator&) = delete;
    Arm64CodeGenerator& operator=(const Arm64CodeGenerator&) = delete;

    void* getCode() const {
        return code_buffer;
    }
    void* getCurr() const {
        return code_ptr;
    }
    size_t getSize() const {
        return static_cast<u8*>(code_ptr) - static_cast<u8*>(code_buffer);
    }

    void reset();
    void setSize(size_t offset);

    // Memory operations
    void ldr(int reg, void* addr);
    void ldr(int reg, int base_reg, s32 offset = 0);
    void ldrh(int reg, int base_reg, s32 offset = 0);
    void ldrb(int reg, int base_reg, s32 offset = 0);
    void ldp(int reg1, int reg2, int base_reg, s32 offset = 0);
    void str(int reg, void* addr);
    void str(int reg, int base_reg, s32 offset = 0);
    void strh(int reg, int base_reg, s32 offset = 0);
    void strb(int reg, int base_reg, s32 offset = 0);
    void stp(int reg1, int reg2, int base_reg, s32 offset = 0);

    // Arithmetic operations
    void add(int dst, int src1, int src2);
    void add_imm(int dst, int src1, s32 imm);
    void sub(int dst, int src1, int src2);
    void sub_imm(int dst, int src1, s32 imm);
    void mul(int dst, int src1, int src2);
    void sdiv(int dst, int src1, int src2);
    void udiv(int dst, int src1, int src2);
    void and_(int dst, int src1, int src2);
    void and_(int dst, int src1, u64 imm);
    void orr(int dst, int src1, int src2);
    void orr(int dst, int src1, u64 imm);
    void eor(int dst, int src1, int src2);
    void eor(int dst, int src1, u64 imm);
    void mvn(int dst, int src);
    void lsl(int dst, int src1, int src2);
    void lsl(int dst, int src1, u8 shift);
    void lsr(int dst, int src1, int src2);
    void lsr(int dst, int src1, u8 shift);
    void asr(int dst, int src1, int src2);
    void asr(int dst, int src1, u8 shift);

    // Move operations
    void mov(int dst, int src);
    void mov_imm(int dst, s64 imm);
    void movz(int dst, u16 imm, u8 shift = 0);
    void movk(int dst, u16 imm, u8 shift = 0);
    void movn(int dst, u16 imm, u8 shift = 0);

    // Compare operations
    void cmp(int reg1, int reg2);
    void cmp_imm(int reg, s32 imm);
    void tst(int reg1, int reg2);
    void tst(int reg, u64 imm);

    // Branch operations
    void b(void* target);
    void b(int condition, void* target);
    void bl(void* target);
    void br(int reg);
    void blr(int reg);
    void ret(int reg = 30); // X30 is LR by default

    // Conditional branches
    void b_eq(void* target);
    void b_ne(void* target);
    void b_lt(void* target);
    void b_le(void* target);
    void b_gt(void* target);
    void b_ge(void* target);
    void b_lo(void* target); // unsigned lower
    void b_ls(void* target); // unsigned lower or same
    void b_hi(void* target); // unsigned higher
    void b_hs(void* target); // unsigned higher or same

    // Stack operations
    void push(int reg);
    void push(int reg1, int reg2);
    void pop(int reg);
    void pop(int reg1, int reg2);

    // System operations
    void nop();
    void brk(u16 imm = 0);

    // NEON/SIMD operations (for XMM registers)
    void ldr_v(int vreg, int base_reg, s32 offset = 0);
    void str_v(int vreg, int base_reg, s32 offset = 0);
    void mov_v(int vdst, int vsrc);
    void add_v(int vdst, int vsrc1, int vsrc2);
    void sub_v(int vdst, int vsrc1, int vsrc2);
    void mul_v(int vdst, int vsrc1, int vsrc2);

    void makeExecutable();

private:
    void emit32(u32 instruction);
    void emit64(u64 instruction);
    void* allocateCode(size_t size);

    void* code_buffer;
    void* code_ptr;
    size_t buffer_size;
    bool owns_buffer;
    std::vector<std::pair<void*, void*>> fixups; // (fixup_location, target_address)
};

} // namespace Core::Jit
