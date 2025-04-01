// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_utils.h"

using namespace Xbyak;
using namespace Xbyak::util;

namespace Shader::Backend::X64 {

bool IsFloatingType(IR::Type type) {
    // We store F16 on general purpose registers since we don't do
    // arithmetic on them
    return type == IR::Type::F32 || type == IR::Type::F64;
}

bool IsConditionalOpcode(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::FPOrdEqual32:
    case IR::Opcode::FPOrdEqual64:
    case IR::Opcode::FPUnordEqual32:
    case IR::Opcode::FPUnordEqual64:
    case IR::Opcode::FPOrdNotEqual32:
    case IR::Opcode::FPOrdNotEqual64:
    case IR::Opcode::FPUnordNotEqual32:
    case IR::Opcode::FPUnordNotEqual64:
    case IR::Opcode::FPOrdLessThan32:
    case IR::Opcode::FPOrdLessThan64:
    case IR::Opcode::FPUnordLessThan32:
    case IR::Opcode::FPUnordLessThan64:
    case IR::Opcode::FPOrdGreaterThan32:
    case IR::Opcode::FPOrdGreaterThan64:
    case IR::Opcode::FPUnordGreaterThan32:
    case IR::Opcode::FPUnordGreaterThan64:
    case IR::Opcode::FPOrdLessThanEqual32:
    case IR::Opcode::FPOrdLessThanEqual64:
    case IR::Opcode::FPUnordLessThanEqual32:
    case IR::Opcode::FPUnordLessThanEqual64:
    case IR::Opcode::FPOrdGreaterThanEqual32:
    case IR::Opcode::FPOrdGreaterThanEqual64:
    case IR::Opcode::FPUnordGreaterThanEqual32:
    case IR::Opcode::FPUnordGreaterThanEqual64:
    case IR::Opcode::FPIsNan32:
    case IR::Opcode::FPIsNan64:
    case IR::Opcode::FPIsInf32:
    case IR::Opcode::FPIsInf64:
    case IR::Opcode::FPCmpClass32:
    case IR::Opcode::SLessThan32:
    case IR::Opcode::SLessThan64:
    case IR::Opcode::ULessThan32:
    case IR::Opcode::ULessThan64:
    case IR::Opcode::IEqual32:
    case IR::Opcode::IEqual64:
    case IR::Opcode::SLessThanEqual:
    case IR::Opcode::ULessThanEqual:
    case IR::Opcode::SGreaterThan:
    case IR::Opcode::UGreaterThan:
    case IR::Opcode::INotEqual32:
    case IR::Opcode::INotEqual64:
    case IR::Opcode::SGreaterThanEqual:
    case IR::Opcode::UGreaterThanEqual:
        return true;
    default:
        return false;
    }
}

size_t GetRegBytesOfType(IR::Type type) {
    switch (type) {
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
    UNREACHABLE_MSG("Unsupported type %s", IR::NameOf(type));
    return 0;
}

u8 GetNumComponentsOfType(IR::Type type) {
    switch (type) {
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
    UNREACHABLE_MSG("Unsupported type %s", IR::NameOf(type));
    return 0;
}

Reg ResizeRegToType(const Reg& reg, IR::Type type) {
    ASSERT(reg.getKind() == Operand::Kind::REG);
    switch (GetRegBytesOfType(type)) {
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
    UNREACHABLE_MSG("Unsupported type %s", IR::NameOf(type));
    return reg;
}

void MovFloat(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src) {
    CodeGenerator& c = ctx.Code();
    if (src.isMEM() && dst.isMEM()) {
        Reg tmp = ctx.TempGPReg(false).cvt32();
        c.mov(tmp, src);
        c.mov(dst, tmp);
    } else if (src.isMEM() && dst.isXMM()) {
        c.movss(dst.getReg().cvt128(), src.getAddress());
    } else if (src.isXMM() && dst.isMEM()) {
        c.movss(dst.getAddress(), src.getReg().cvt128());
    } else if (src.isXMM() && dst.isXMM()) {
        c.movaps(dst.getReg().cvt128(), src.getReg().cvt128());
    } else {
        UNREACHABLE_MSG("Unsupported mov float %s %s", src.toString(), dst.toString());
    }
}

void MovDouble(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src) {
    CodeGenerator& c = ctx.Code();
    if (src.isMEM() && dst.isMEM()) {
        const Reg64& tmp = ctx.TempGPReg(false);
        c.mov(tmp, src);
        c.mov(dst, tmp);
    } else if (src.isMEM() && dst.isXMM()) {
        c.movsd(dst.getReg().cvt128(), src.getAddress());
    } else if (src.isXMM() && dst.isMEM()) {
        c.movsd(dst.getAddress(), src.getReg().cvt128());
    } else if (src.isXMM() && dst.isXMM()) {
        c.movapd(dst.getReg().cvt128(), src.getReg().cvt128());
    } else {
        UNREACHABLE_MSG("Unsupported mov double %s %s", src.toString(), dst.toString());
    }
}

void MovGP(EmitContext& ctx, const Xbyak::Operand& dst, const Xbyak::Operand& src) {
    CodeGenerator& c = ctx.Code();
    if (src.isMEM() && dst.isMEM()) {
        const Reg64& tmp = ctx.TempGPReg(false);
        c.mov(tmp, src);
        c.mov(dst, tmp);
    } else {
        c.mov(dst, src);
    }
}

void MovValue(EmitContext& ctx, const Operands& dst, const IR::Value& src) {
    if (!src.IsImmediate()) {
        const Operands& src_op = ctx.Def(src);
        if (IsFloatingType(src.Type())) {
            switch (GetRegBytesOfType(src.Type())) {
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
                UNREACHABLE_MSG("Unsupported type %s", IR::NameOf(src.Type()));
                break;
            }
        } else {
            for (size_t i = 0; i < src_op.size(); i++) {
                MovGP(ctx, dst[i], src_op[i]);
            }
        }
    } else {
        CodeGenerator& c = ctx.Code();
        const bool is_mem = dst[0].isMEM();
        Reg64& tmp = ctx.TempGPReg(false);
        switch (src.Type()) {
        case IR::Type::U1:
            c.mov(is_mem ? tmp.cvt8() : dst[0], src.U1());
            break;
        case IR::Type::U8:
            c.mov(is_mem ? tmp.cvt8() : dst[0], src.U8());
            break;
        case IR::Type::U16:
            c.mov(is_mem ? tmp.cvt16() : dst[0], src.U16());
            break;
        case IR::Type::U32:
            c.mov(is_mem ? tmp.cvt32() : dst[0], src.U32());
            break;
        case IR::Type::F32:
            c.mov(tmp.cvt32(), std::bit_cast<u32>(src.F32()));
            if (!is_mem) {
                c.movd(dst[0].getReg().cvt128(), tmp.cvt32());
                return;
            }
            break;
        case IR::Type::U64:
            c.mov(is_mem ? tmp : dst[0], src.U64());
            break;
        case IR::Type::F64:
            c.mov(tmp, std::bit_cast<u64>(src.F64()));
            if (!is_mem) {
                c.movq(dst[0].getReg().cvt128(), tmp);
                return;
            }
            break;
        case IR::Type::ScalarReg:
            c.mov(is_mem ? tmp.cvt32() : dst[0], std::bit_cast<u32>(src.ScalarReg()));
            break;
        case IR::Type::VectorReg:
            c.mov(is_mem ? tmp.cvt32() : dst[0], std::bit_cast<u32>(src.VectorReg()));
            break;
        case IR::Type::Attribute:
            c.mov(is_mem ? tmp : dst[0], std::bit_cast<u64>(src.Attribute()));
            break;
        case IR::Type::Patch:
            c.mov(is_mem ? tmp : dst[0], std::bit_cast<u64>(src.Patch()));
            break;
        default:
            UNREACHABLE_MSG("Unsupported type %s", IR::NameOf(src.Type()));
            break;
        }
        if (is_mem) {
            c.mov(dst[0], tmp);
        }
    }
}

} // namespace Shader::Backend::X64