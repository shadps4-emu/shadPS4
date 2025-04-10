// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_utils.h"

using namespace Xbyak;
using namespace Xbyak::util;

namespace Shader::Backend::X64 {

bool IsFloatingType(const IR::Value& value) {
    // We store F16 on general purpose registers since we don't do
    // arithmetic on them
    IR::Type type = value.Type();
    return type == IR::Type::F32 || type == IR::Type::F64;
}

size_t GetRegBytesOfType(const IR::Value& value) {
    switch (value.Type()) {
    case IR::Type::U1:
    case IR::Type::U8:
        return 1;
    case IR::Type::U16:
    case IR::Type::F16:
    case IR::Type::F16x2:
    case IR::Type::F16x3:
    case IR::Type::F16x4:
        return 2;
    case IR::Type::U32:
    case IR::Type::U32x2:
    case IR::Type::U32x3:
    case IR::Type::U32x4:
    case IR::Type::F32:
    case IR::Type::F32x2:
    case IR::Type::F32x3:
    case IR::Type::F32x4:
    case IR::Type::ScalarReg:
    case IR::Type::VectorReg:
        return 4;
    case IR::Type::U64:
    case IR::Type::F64:
    case IR::Type::F64x2:
    case IR::Type::F64x3:
    case IR::Type::F64x4:
    case IR::Type::Attribute:
    case IR::Type::Patch:
        return 8;
    default:
        break;
    }
    UNREACHABLE_MSG("Unsupported type {}", IR::NameOf(value.Type()));
    return 0;
}

u8 GetNumComponentsOfType(const IR::Value& value) {
    switch (value.Type()) {
    case IR::Type::U1:
    case IR::Type::U8:
    case IR::Type::U16:
    case IR::Type::F16:
    case IR::Type::U32:
    case IR::Type::F32:
    case IR::Type::U64:
    case IR::Type::F64:
    case IR::Type::ScalarReg:
    case IR::Type::VectorReg:
    case IR::Type::Attribute:
    case IR::Type::Patch:
        return 1;
    case IR::Type::U32x2:
    case IR::Type::F32x2:
    case IR::Type::F16x2:
    case IR::Type::F64x2:
        return 2;
    case IR::Type::U32x3:
    case IR::Type::F32x3:
    case IR::Type::F16x3:
    case IR::Type::F64x3:
        return 3;
    case IR::Type::U32x4:
    case IR::Type::F32x4:
    case IR::Type::F16x4:
    case IR::Type::F64x4:
        return 4;
    default:
        break;
    }
    UNREACHABLE_MSG("Unsupported type {}", IR::NameOf(value.Type()));
    return 0;
}

Reg ResizeRegToType(const Reg& reg, const IR::Value& value) {
    ASSERT(reg.getKind() == Operand::Kind::REG);
    switch (GetRegBytesOfType(value)) {
    case 1:
        return reg.cvt8();
    case 2:
        return reg.cvt16();
    case 4:
        return reg.cvt32();
    case 8:
        return reg.cvt64();
    default:
        break;
    }
    UNREACHABLE_MSG("Unsupported type {}", IR::NameOf(value.Type()));
    return reg;
}

void MovFloat(EmitContext& ctx, const OperandHolder& dst, const OperandHolder& src) {
    CodeGenerator& c = ctx.Code();
    if (src.Op() == dst.Op()) {
        return;
    }
    if (src.IsMem() && dst.IsMem()) {
        Reg tmp = ctx.TempGPReg(false).cvt32();
        c.mov(tmp, src.Mem());
        c.mov(dst.Mem(), tmp);
    } else if (src.IsMem() && dst.IsXmm()) {
        c.movss(dst.Xmm(), src.Mem());
    } else if (src.IsXmm() && dst.IsMem()) {
        c.movss(dst.Mem(), src.Xmm());
    } else if (src.IsXmm() && dst.IsXmm()) {
        c.movaps(dst.Xmm(), src.Xmm());
    } else {
        UNREACHABLE_MSG("Unsupported mov float {} {}", src.Op().toString(), dst.Op().toString());
    }
}

void MovDouble(EmitContext& ctx, const OperandHolder& dst, const OperandHolder& src) {
    CodeGenerator& c = ctx.Code();
    if (src.Op() == dst.Op()) {
        return;
    }
    if (src.IsMem() && dst.IsMem()) {
        const Reg64& tmp = ctx.TempGPReg(false);
        c.mov(tmp, src.Mem());
        c.mov(dst.Mem(), tmp);
    } else if (src.IsMem() && dst.IsXmm()) {
        c.movsd(dst.Xmm(), src.Mem());
    } else if (src.IsXmm() && dst.IsMem()) {
        c.movsd(dst.Mem(), src.Xmm());
    } else if (src.IsXmm() && dst.IsXmm()) {
        c.movapd(dst.Xmm(), src.Xmm());
    } else {
        UNREACHABLE_MSG("Unsupported mov double {} {}", src.Op().toString(), dst.Op().toString());
    }
}

void MovGP(EmitContext& ctx, const OperandHolder& dst, const OperandHolder& src) {
    CodeGenerator& c = ctx.Code();
    if (src.Op() == dst.Op()) {
        return;
    }
    const bool is_mem2mem = src.IsMem() && dst.IsMem();
    const u32 src_bit = src.Op().getBit();
    const u32 dst_bit = dst.Op().getBit();
    OperandHolder tmp = is_mem2mem ? ctx.TempGPReg(false).changeBit(dst_bit) : dst;
    if (src_bit < dst_bit) {
        if (!tmp.IsMem() && !src.Op().isBit(32)) {
            c.movzx(tmp.Reg(), src.Op());
        } else if (tmp.IsMem()) {
            Address addr = tmp.Mem();
            c.mov(addr, 0);
            addr.setBit(dst_bit);
            c.mov(addr, src.Reg());
        } else {
            c.mov(tmp.Reg().cvt32(), src.Op());
        }
    } else if (src_bit > dst_bit) {
        OperandHolder src_tmp = src;
        src_tmp.Op().setBit(dst_bit);
        c.mov(tmp.Op(), src_tmp.Op());
    } else {
        c.mov(tmp.Op(), src.Op());
    }
    if (is_mem2mem) {
        c.mov(dst.Op(), tmp.Op());
    }
}

void MovValue(EmitContext& ctx, const Operands& dst, const IR::Value& src) {
    if (!src.IsImmediate()) {
        IR::Inst* src_inst = src.InstRecursive();
        const Operands& src_op = ctx.Def(src_inst);
        if (IsFloatingType(src)) {
            switch (GetRegBytesOfType(src)) {
            case 32:
                for (size_t i = 0; i < src_op.size(); i++) {
                    MovFloat(ctx, dst[i], src_op[i]);
                }
                break;
            case 64:
                for (size_t i = 0; i < src_op.size(); i++) {
                    MovDouble(ctx, dst[i], src_op[i]);
                }
                break;
            default:
                UNREACHABLE_MSG("Unsupported type {}", IR::NameOf(src.Type()));
                break;
            }
        } else {
            for (size_t i = 0; i < src_op.size(); i++) {
                MovGP(ctx, dst[i], src_op[i]);
            }
        }
    } else {
        CodeGenerator& c = ctx.Code();
        const bool is_mem = dst[0].IsMem();
        Reg64& tmp = ctx.TempGPReg(false);
        switch (src.Type()) {
        case IR::Type::U1:
            c.mov(is_mem ? tmp.cvt8() : dst[0].Reg(), src.U1());
            break;
        case IR::Type::U8:
            c.mov(is_mem ? tmp.cvt8() : dst[0].Reg(), src.U8());
            break;
        case IR::Type::U16:
            c.mov(is_mem ? tmp.cvt16() : dst[0].Reg(), src.U16());
            break;
        case IR::Type::U32:
            c.mov(is_mem ? tmp.cvt32() : dst[0].Reg(), src.U32());
            break;
        case IR::Type::F32:
            c.mov(tmp.cvt32(), static_cast<u32>(src.F32()));
            if (!is_mem) {
                c.movd(dst[0].Xmm(), tmp.cvt32());
                return;
            }
            break;
        case IR::Type::U64:
            c.mov(is_mem ? tmp : dst[0].Reg(), src.U64());
            break;
        case IR::Type::F64:
            c.mov(tmp, static_cast<u64>(src.F64()));
            if (!is_mem) {
                c.movq(dst[0].Xmm(), tmp);
                return;
            }
            break;
        case IR::Type::ScalarReg:
            c.mov(is_mem ? tmp.cvt32() : dst[0].Reg(), static_cast<u32>(src.ScalarReg()));
            break;
        case IR::Type::VectorReg:
            c.mov(is_mem ? tmp.cvt32() : dst[0].Reg(), static_cast<u32>(src.VectorReg()));
            break;
        case IR::Type::Attribute:
            c.mov(is_mem ? tmp : dst[0].Reg(), std::bit_cast<u64>(src.Attribute()));
            break;
        case IR::Type::Patch:
            c.mov(is_mem ? tmp : dst[0].Reg(), std::bit_cast<u64>(src.Patch()));
            break;
        default:
            UNREACHABLE_MSG("Unsupported type {}", IR::NameOf(src.Type()));
            break;
        }
        if (is_mem) {
            c.mov(dst[0].Mem(), tmp);
        }
    }
}

void EmitInlineF16ToF32(EmitContext& ctx, const Operand& dest, const Operand& src) {
    CodeGenerator& c = ctx.Code();
    Label nonzero_exp, zero_mantissa, norm_loop, norm_done, normal, done;
    Reg sign = ctx.TempGPReg().cvt32();
    Reg exponent = ctx.TempGPReg().cvt32();
    Reg mantissa = ctx.TempGPReg().cvt32();

    c.movzx(mantissa, src);

    // Extract sign, exponent, and mantissa
    c.mov(sign, mantissa);
    c.and_(sign, 0x8000);
    c.shl(sign, 16);
    c.mov(exponent, mantissa);
    c.and_(exponent, 0x7C00);
    c.shr(exponent, 10);
    c.and_(mantissa, 0x03FF);

    // Check for zero exponent and mantissa
    c.test(exponent, exponent);
    c.jnz(nonzero_exp);
    c.test(mantissa, mantissa);
    c.jz(zero_mantissa);

    // Nromalize subnormal number
    c.mov(exponent, 1);
    c.L(norm_loop);
    c.test(mantissa, 0x400);
    c.jnz(norm_done);
    c.shl(mantissa, 1);
    c.dec(exponent);
    c.jmp(norm_loop);
    c.L(norm_done);
    c.and_(mantissa, 0x03FF);
    c.jmp(normal);

    // Zero mantissa
    c.L(zero_mantissa);
    c.and_(mantissa, sign);
    c.jmp(done);

    // Non-zero exponent
    c.L(nonzero_exp);
    c.cmp(exponent, 0x1F);
    c.jne(normal);

    // Infinite or NaN
    c.shl(mantissa, 13);
    c.or_(mantissa, sign);
    c.or_(mantissa, 0x7F800000);
    c.jmp(done);

    // Normal number
    c.L(normal);
    c.add(exponent, 112);
    c.shl(exponent, 23);
    c.shl(mantissa, 13);
    c.or_(mantissa, sign);
    c.or_(mantissa, exponent);

    c.L(done);
    if (dest.isMEM()) {
        c.mov(dest, mantissa);
    } else {
        c.movd(dest.getReg().cvt128(), mantissa);
    }

    ctx.PopTempGPReg();
    ctx.PopTempGPReg();
    ctx.PopTempGPReg();
}

void EmitInlineF32ToF16(EmitContext& ctx, const Operand& dest, const Operand& src) {
    CodeGenerator& c = ctx.Code();
    Label zero_exp, underflow, overflow, done;
    Reg sign = ctx.TempGPReg().cvt32();
    Reg exponent = ctx.TempGPReg().cvt32();
    Reg mantissa = dest.isMEM() ? ctx.TempGPReg().cvt32() : dest.getReg().cvt32();

    if (src.isMEM()) {
        c.mov(mantissa, src);
    } else {
        c.movd(mantissa, src.getReg().cvt128());
    }

    // Extract sign, exponent, and mantissa
    c.mov(exponent, mantissa);
    c.mov(sign, mantissa);
    c.and_(exponent, 0x7F800000);
    c.and_(mantissa, 0x007FFFFF);
    c.shr(exponent, 23);
    c.shl(mantissa, 3);
    c.shr(sign, 16);
    c.and_(sign, 0x8000);

    // Subnormal numbers will be zero
    c.test(exponent, exponent);
    c.jz(zero_exp);

    // Check for overflow and underflow
    c.sub(exponent, 112);
    c.cmp(exponent, 0);
    c.jle(underflow);
    c.cmp(exponent, 0x1F);
    c.jge(overflow);

    // Normal number
    c.shl(exponent, 10);
    c.shr(mantissa, 13);
    c.or_(mantissa, exponent);
    c.or_(mantissa, sign);
    c.jmp(done);

    // Undeflow
    c.L(underflow);
    c.xor_(mantissa, mantissa);
    c.jmp(done);

    // Overflow
    c.L(overflow);
    c.mov(mantissa, 0x7C00);
    c.or_(mantissa, sign);
    c.jmp(done);

    // Zero value
    c.L(zero_exp);
    c.and_(mantissa, sign);

    c.L(done);
    if (dest.isMEM()) {
        c.mov(dest, mantissa);
    } else {
        c.and_(mantissa, 0xFFFF);
    }

    ctx.PopTempGPReg();
    ctx.PopTempGPReg();
    ctx.PopTempGPReg();
}

} // namespace Shader::Backend::X64