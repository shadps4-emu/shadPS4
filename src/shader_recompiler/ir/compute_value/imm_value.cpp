// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/hash.h"
#include "shader_recompiler/ir/compute_value/imm_value.h"

namespace Shader::IR {

ImmValue::ImmValue(const IR::Value& value) noexcept {
    type = value.Type();
    switch (type) {
    case Type::U1:
        imm_values[0].imm_u1 = value.U1();
        break;
    case Type::U8:
        imm_values[0].imm_u8 = value.U8();
        break;
    case Type::U16:
        imm_values[0].imm_u16 = value.U16();
        break;
    case Type::U32:
        imm_values[0].imm_u32 = value.U32();
        break;
    case Type::F32:
        imm_values[0].imm_f32 = value.F32();
        break;
    case Type::U64:
        imm_values[0].imm_u64 = value.U64();
        break;
    case Type::F64:
        imm_values[0].imm_f64 = value.F64();
        break;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue::ImmValue(bool value) noexcept : type{Type::U1}, is_signed{false} {
    imm_values[0].imm_u1 = value;
}

ImmValue::ImmValue(u8 value) noexcept : type{Type::U8}, is_signed{false} {
    imm_values[0].imm_u8 = value;
}

ImmValue::ImmValue(s8 value) noexcept : type{Type::U8}, is_signed{true} {
    imm_values[0].imm_s8 = value;
}

ImmValue::ImmValue(u16 value) noexcept : type{Type::U16}, is_signed{false} {
    imm_values[0].imm_u16 = value;
}

ImmValue::ImmValue(s16 value) noexcept : type{Type::U16}, is_signed{true} {
    imm_values[0].imm_s16 = value;
}

ImmValue::ImmValue(u32 value) noexcept : type{Type::U32}, is_signed{false} {
    imm_values[0].imm_u32 = value;
}

ImmValue::ImmValue(s32 value) noexcept : type{Type::U32}, is_signed{true} {
    imm_values[0].imm_s32 = value;
}

ImmValue::ImmValue(f32 value) noexcept : type{Type::F32}, is_signed{true} {
    imm_values[0].imm_f32 = value;
}

ImmValue::ImmValue(u64 value) noexcept : type{Type::U64}, is_signed{false} {
    imm_values[0].imm_u64 = value;
}

ImmValue::ImmValue(s64 value) noexcept : type{Type::U64}, is_signed{true} {
    imm_values[0].imm_s64 = value;
}

ImmValue::ImmValue(f64 value) noexcept : type{Type::F64}, is_signed{true} {
    imm_values[0].imm_f64 = value;
}

ImmValue::ImmValue(u32 value1, u32 value2) noexcept : type{Type::U32x2}, is_signed{false} {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
}

ImmValue::ImmValue(u32 value1, u32 value2, u32 value3) noexcept
    : type{Type::U32x3}, is_signed{false} {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
    imm_values[2].imm_u32 = value3;
}

ImmValue::ImmValue(u32 value1, u32 value2, u32 value3, u32 value4) noexcept
    : type{Type::U32x4}, is_signed{false} {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
    imm_values[2].imm_u32 = value3;
    imm_values[3].imm_u32 = value4;
}

ImmValue::ImmValue(s32 value1, s32 value2) noexcept : type{Type::U32x2}, is_signed{true} {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
}

ImmValue::ImmValue(s32 value1, s32 value2, s32 value3) noexcept
    : type{Type::U32x3}, is_signed{true} {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
    imm_values[2].imm_s32 = value3;
}

ImmValue::ImmValue(s32 value1, s32 value2, s32 value3, s32 value4) noexcept
    : type{Type::U32x4}, is_signed{true} {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
    imm_values[2].imm_s32 = value3;
    imm_values[3].imm_s32 = value4;
}

ImmValue::ImmValue(f32 value1, f32 value2) noexcept : type{Type::F32x2}, is_signed{true} {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
}

ImmValue::ImmValue(f32 value1, f32 value2, f32 value3) noexcept
    : type{Type::F32x3}, is_signed{true} {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
    imm_values[2].imm_f32 = value3;
}

ImmValue::ImmValue(f32 value1, f32 value2, f32 value3, f32 value4) noexcept
    : type{Type::F32x4}, is_signed{true} {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
    imm_values[2].imm_f32 = value3;
    imm_values[3].imm_f32 = value4;
}

ImmValue::ImmValue(f64 value1, f64 value2) noexcept : type{Type::F64x2}, is_signed{true} {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
}

ImmValue::ImmValue(f64 value1, f64 value2, f64 value3) noexcept
    : type{Type::F64x3}, is_signed{true} {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
    imm_values[2].imm_f64 = value3;
}

ImmValue::ImmValue(f64 value1, f64 value2, f64 value3, f64 value4) noexcept
    : type{Type::F64x4}, is_signed{true} {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
    imm_values[2].imm_f64 = value3;
    imm_values[3].imm_f64 = value4;
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2) noexcept
    : type{value1.type}, is_signed{value1.is_signed} {
    ASSERT(value1.type == value2.type && value1.is_signed == value2.is_signed);
    switch (value1.Dimensions()) {
    case 1:
        imm_values[0] = value1.imm_values[0];
        imm_values[1] = value2.imm_values[0];
        break;
    case 2:
        imm_values[0] = value1.imm_values[0];
        imm_values[1] = value1.imm_values[1];
        imm_values[2] = value2.imm_values[0];
        imm_values[3] = value2.imm_values[1];
        break;
    default:
        UNREACHABLE_MSG("Invalid dimensions {}", value1.Dimensions());
    }
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2, const ImmValue& value3) noexcept
    : type{value1.type}, is_signed{value1.is_signed} {
    ASSERT(value1.type == value2.type && value1.type == value3.type && value1.is_signed == value2.is_signed &&
           value1.is_signed == value3.is_signed && value1.Dimensions() == 1);
    imm_values[0] = value1.imm_values[0];
    imm_values[1] = value2.imm_values[0];
    imm_values[2] = value3.imm_values[0];
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2, const ImmValue& value3, const ImmValue& value4) noexcept
    : type{value1.type}, is_signed{value1.is_signed} {
    ASSERT(value1.type == value2.type && value1.type == value3.type && value1.type == value4.type && value1.is_signed == value2.is_signed &&
           value1.is_signed == value3.is_signed && value1.is_signed == value4.is_signed && value1.Dimensions() == 1);
    imm_values[0] = value1.imm_values[0];
    imm_values[1] = value2.imm_values[0];
    imm_values[2] = value3.imm_values[0];
    imm_values[3] = value4.imm_values[0];
}

IR::Type ImmValue::BaseType() const noexcept {
    switch (type) {
    case Type::U1:
        return Type::U1;
    case Type::U8:
        return Type::U8;
    case Type::U16:
        return Type::U16;
    case Type::U32:
    case Type::U32x2:
    case Type::U32x3:
    case Type::U32x4:
        return Type::U32;
    case Type::U64:
        return Type::U64;
    case Type::F32:
    case Type::F32x2:
    case Type::F32x3:
    case Type::F32x4:
        return Type::F32;
    case Type::F64:
    case Type::F64x2:
    case Type::F64x3:
    case Type::F64x4:
        return Type::F64;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

u32 ImmValue::Dimensions() const noexcept {
    switch (type) {
    case Type::U1:
    case Type::U8:
    case Type::U16:
    case Type::U32:
    case Type::U64:
    case Type::F32:
    case Type::F64:
        return 1;
    case Type::U32x2:
    case Type::F32x2:
    case Type::F64x2:
        return 2;
    case Type::U32x3:
    case Type::F32x3:
    case Type::F64x3:
        return 3;
    case Type::U32x4:
    case Type::F32x4:
    case Type::F64x4:
        return 4;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

bool ImmValue::IsSigned() const noexcept {
    return is_signed;
}

void ImmValue::SetSigned(bool signed_) noexcept {
    is_signed = signed_;
}

void ImmValue::SameSignAs(const ImmValue& other) noexcept {
    SetSigned(other.IsSigned());
}

ImmValue ImmValue::Convert(IR::Type new_type, bool new_signed) const noexcept {
    switch (new_type) {
    case Type::U16: {
        switch (type) {
        case Type::U32:
            return ImmValue(static_cast<u16>(imm_values[0].imm_u32));
        default:
            break;
        }
        break;
    }
    case Type::U32: {
        if (new_signed) {
            switch (type) {
                case Type::F32:
                    return ImmValue(static_cast<s32>(imm_values[0].imm_f32));
                case Type::F64:
                    return ImmValue(static_cast<s32>(imm_values[0].imm_f64));
                default:
                    break;
            }
        } else {
            switch (type) {
            case Type::U16:
                return ImmValue(static_cast<u32>(imm_values[0].imm_u16));
            case Type::U32:
                if (is_signed) {
                    return ImmValue(static_cast<u32>(imm_values[0].imm_s32));
                }
                break;
            case Type::F32:
                return ImmValue(static_cast<u32>(imm_values[0].imm_f32));
            default:
                break;
            }
        }
    }
    case Type::F32: {
        switch (type) {
        case Type::U16:
            return ImmValue(static_cast<f32>(imm_values[0].imm_u16));
        case Type::U32:
            if (is_signed) {
                return ImmValue(static_cast<f32>(imm_values[0].imm_s32));
            } else {
                return ImmValue(static_cast<f32>(imm_values[0].imm_u32));
            }
        case Type::F64:
            return ImmValue(static_cast<f32>(imm_values[0].imm_f64));
        default:
            break;
        }
        break;
    }
    case Type::F64: {
        switch (type) {
        case Type::F32:
            return ImmValue(static_cast<f64>(imm_values[0].imm_f32));
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid conversion from {} {} to {} {}", is_signed ? "signed" : "unsigned",
                    type, new_signed ? "signed" : "unsigned", new_type);
}

ImmValue ImmValue::Bitcast(IR::Type new_type, bool new_signed) const noexcept {
    ImmValue result;
    result.type = new_type;
    result.is_signed = new_signed;
    result.imm_values = imm_values;
    ASSERT(Dimensions() == result.Dimensions());
    return result;
}

ImmValue ImmValue::Extract(const ImmValue& index) const noexcept {
    ASSERT(index.type == Type::U32 && !index.is_signed && index.imm_values[0].imm_u32 < Dimensions());
    ImmValue result;
    result.type = BaseType();
    result.is_signed = IsSigned();
    result.imm_values[0] = imm_values[index.imm_values[0].imm_u32];
    return result;
}

ImmValue ImmValue::Insert(const ImmValue& value, const ImmValue& index) const noexcept {
    ASSERT(index.type == Type::U32 && !index.is_signed && index.imm_values[0].imm_u32 < Dimensions());
    ASSERT(value.type == BaseType() && value.IsSigned() == IsSigned());
    ImmValue result = *this;
    result.imm_values[index.imm_values[0].imm_u32] = value.imm_values[0];
    return result;
}

bool ImmValue::operator==(const ImmValue& other) const noexcept {
    if (type != other.type) {
        return false;
    }
    switch (type) {
    case Type::U1:
        return imm_values[0].imm_u1 == other.imm_values[0].imm_u1;
    case Type::U8:
        return imm_values[0].imm_u8 == other.imm_values[0].imm_u8;
    case Type::U16:
        return imm_values[0].imm_u16 == other.imm_values[0].imm_u16;
    case Type::U32:
    case Type::F32:
        return imm_values[0].imm_u32 == other.imm_values[0].imm_u32;
    case Type::U64:
    case Type::F64:
        return imm_values[0].imm_u64 == other.imm_values[0].imm_u64;
    case Type::U32x2:
    case Type::F32x2:
    case Type::F64x2:
        return imm_values[0].imm_u32 == other.imm_values[0].imm_u32 &&
               imm_values[1].imm_u32 == other.imm_values[1].imm_u32;
    case Type::U32x3:
    case Type::F32x3:
    case Type::F64x3:
        return imm_values[0].imm_u32 == other.imm_values[0].imm_u32 &&
               imm_values[1].imm_u32 == other.imm_values[1].imm_u32 &&
               imm_values[2].imm_u32 == other.imm_values[2].imm_u32;
    case Type::U32x4:
    case Type::F32x4:
    case Type::F64x4:
        return imm_values[0].imm_u32 == other.imm_values[0].imm_u32 &&
               imm_values[1].imm_u32 == other.imm_values[1].imm_u32 &&
               imm_values[2].imm_u32 == other.imm_values[2].imm_u32 &&
               imm_values[3].imm_u32 == other.imm_values[3].imm_u32;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

bool ImmValue::operator!=(const ImmValue& other) const noexcept {
    return !operator==(other);
}

bool ImmValue::operator<(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed ? imm_values[0].imm_s8 < other.imm_values[0].imm_s8
                                            : imm_values[0].imm_u8 < other.imm_values[0].imm_u8;
    case Type::U16:
        return is_signed && other.is_signed ? imm_values[0].imm_s16 < other.imm_values[0].imm_s16
                                            : imm_values[0].imm_u16 < other.imm_values[0].imm_u16;
    case Type::U32:
        return is_signed && other.is_signed ? imm_values[0].imm_s32 < other.imm_values[0].imm_s32
                                            : imm_values[0].imm_u32 < other.imm_values[0].imm_u32;
    case Type::F32:
        return imm_values[0].imm_f32 < other.imm_values[0].imm_f32;
    case Type::U64:
        return is_signed && other.is_signed ? imm_values[0].imm_s64 < other.imm_values[0].imm_s64
                                            : imm_values[0].imm_u64 < other.imm_values[0].imm_u64;
    case Type::F64:
        return imm_values[0].imm_f64 < other.imm_values[0].imm_f64;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

bool ImmValue::operator>(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed ? imm_values[0].imm_s8 > other.imm_values[0].imm_s8
                                            : imm_values[0].imm_u8 > other.imm_values[0].imm_u8;
    case Type::U16:
        return is_signed && other.is_signed ? imm_values[0].imm_s16 > other.imm_values[0].imm_s16
                                            : imm_values[0].imm_u16 > other.imm_values[0].imm_u16;
    case Type::U32:
        return is_signed && other.is_signed ? imm_values[0].imm_s32 > other.imm_values[0].imm_s32
                                            : imm_values[0].imm_u32 > other.imm_values[0].imm_u32;
    case Type::F32:
        return imm_values[0].imm_f32 > other.imm_values[0].imm_f32;
    case Type::U64:
        return is_signed && other.is_signed ? imm_values[0].imm_s64 > other.imm_values[0].imm_s64
                                            : imm_values[0].imm_u64 > other.imm_values[0].imm_u64;
    case Type::F64:
        return imm_values[0].imm_f64 > other.imm_values[0].imm_f64;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

bool ImmValue::operator<=(const ImmValue& other) const noexcept {
    return !operator>(other);
}

bool ImmValue::operator>=(const ImmValue& other) const noexcept {
    return !operator<(other);
}

ImmValue ImmValue::operator+(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 + other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 + other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 + other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 + other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 + other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 + other.imm_values[0].imm_u32);
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32 + other.imm_values[0].imm_f32);
    case Type::U32x2:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 + other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 + other.imm_values[1].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 + other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 + other.imm_values[1].imm_u32);
    case Type::F32x2:
        return ImmValue(imm_values[0].imm_f32 + other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 + other.imm_values[1].imm_f32);
    case Type::U32x3:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 + other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 + other.imm_values[1].imm_s32,
                              imm_values[2].imm_s32 + other.imm_values[2].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 + other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 + other.imm_values[1].imm_u32,
                              imm_values[2].imm_u32 + other.imm_values[2].imm_u32);
    case Type::F32x3:
        return ImmValue(imm_values[0].imm_f32 + other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 + other.imm_values[1].imm_f32,
                        imm_values[2].imm_f32 + other.imm_values[2].imm_f32);
    case Type::U32x4:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 + other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 + other.imm_values[1].imm_s32,
                              imm_values[2].imm_s32 + other.imm_values[2].imm_s32,
                              imm_values[3].imm_s32 + other.imm_values[3].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 + other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 + other.imm_values[1].imm_u32,
                              imm_values[2].imm_u32 + other.imm_values[2].imm_u32,
                              imm_values[3].imm_u32 + other.imm_values[3].imm_u32);
    case Type::F32x4:
        return ImmValue(imm_values[0].imm_f32 + other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 + other.imm_values[1].imm_f32,
                        imm_values[2].imm_f32 + other.imm_values[2].imm_f32,
                        imm_values[3].imm_f32 + other.imm_values[3].imm_f32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 + other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 + other.imm_values[0].imm_u64);
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64 + other.imm_values[0].imm_f64);
    case Type::F64x2:
        return ImmValue(imm_values[0].imm_f64 + other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 + other.imm_values[1].imm_f64);
    case Type::F64x3:
        return ImmValue(imm_values[0].imm_f64 + other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 + other.imm_values[1].imm_f64,
                        imm_values[2].imm_f64 + other.imm_values[2].imm_f64);
    case Type::F64x4:
        return ImmValue(imm_values[0].imm_f64 + other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 + other.imm_values[1].imm_f64,
                        imm_values[2].imm_f64 + other.imm_values[2].imm_f64,
                        imm_values[3].imm_f64 + other.imm_values[3].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator-(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 - other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 - other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 - other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 - other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 - other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 - other.imm_values[0].imm_u32);
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32 - other.imm_values[0].imm_f32);
    case Type::U32x2:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 - other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 - other.imm_values[1].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 - other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 - other.imm_values[1].imm_u32);
    case Type::F32x2:
        return ImmValue(imm_values[0].imm_f32 - other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 - other.imm_values[1].imm_f32);
    case Type::U32x3:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 - other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 - other.imm_values[1].imm_s32,
                              imm_values[2].imm_s32 - other.imm_values[2].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 - other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 - other.imm_values[1].imm_u32,
                              imm_values[2].imm_u32 - other.imm_values[2].imm_u32);
    case Type::F32x3:
        return ImmValue(imm_values[0].imm_f32 - other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 - other.imm_values[1].imm_f32,
                        imm_values[2].imm_f32 - other.imm_values[2].imm_f32);
    case Type::U32x4:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 - other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 - other.imm_values[1].imm_s32,
                              imm_values[2].imm_s32 - other.imm_values[2].imm_s32,
                              imm_values[3].imm_s32 - other.imm_values[3].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 - other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 - other.imm_values[1].imm_u32,
                              imm_values[2].imm_u32 - other.imm_values[2].imm_u32,
                              imm_values[3].imm_u32 - other.imm_values[3].imm_u32);
    case Type::F32x4:
        return ImmValue(imm_values[0].imm_f32 - other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 - other.imm_values[1].imm_f32,
                        imm_values[2].imm_f32 - other.imm_values[2].imm_f32,
                        imm_values[3].imm_f32 - other.imm_values[3].imm_f32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 - other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 - other.imm_values[0].imm_u64);
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64 - other.imm_values[0].imm_f64);
    case Type::F64x2:
        return ImmValue(imm_values[0].imm_f64 - other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 - other.imm_values[1].imm_f64);
    case Type::F64x3:
        return ImmValue(imm_values[0].imm_f64 - other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 - other.imm_values[1].imm_f64,
                        imm_values[2].imm_f64 - other.imm_values[2].imm_f64);
    case Type::F64x4:
        return ImmValue(imm_values[0].imm_f64 - other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 - other.imm_values[1].imm_f64,
                        imm_values[2].imm_f64 - other.imm_values[2].imm_f64,
                        imm_values[3].imm_f64 - other.imm_values[3].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator*(const ImmValue& other) const noexcept {
    ASSERT(BaseType() == other.BaseType());
    const ImmValue* vector;
    const ImmValue* scalar;
    if (Dimensions() == 1) {
        scalar = this;
        vector = &other;
    } else if (other.Dimensions() == 1) {
        scalar = &other;
        vector = this;
    } else {
        UNREACHABLE_MSG("Unspecified behavior for vector * vector multiplication");
    }
    switch (vector->type) {
    case Type::U8:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s8 * vector->imm_values[0].imm_s8)
                   : ImmValue(scalar->imm_values[0].imm_u8 * vector->imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s16 * vector->imm_values[0].imm_s16)
                   : ImmValue(scalar->imm_values[0].imm_u16 * vector->imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s32 * vector->imm_values[0].imm_s32)
                   : ImmValue(scalar->imm_values[0].imm_u32 * vector->imm_values[0].imm_u32);
    case Type::F32:
        return ImmValue(scalar->imm_values[0].imm_f32 * vector->imm_values[0].imm_f32);
    case Type::U32x2:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s32 * vector->imm_values[0].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[1].imm_s32)
                   : ImmValue(scalar->imm_values[0].imm_u32 * vector->imm_values[0].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[1].imm_u32);
    case Type::F32x2:
        return ImmValue(scalar->imm_values[0].imm_f32 * vector->imm_values[0].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[1].imm_f32);
    case Type::U32x3:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s32 * vector->imm_values[0].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[1].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[2].imm_s32)
                   : ImmValue(scalar->imm_values[0].imm_u32 * vector->imm_values[0].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[1].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[2].imm_u32);
    case Type::F32x3:
        return ImmValue(scalar->imm_values[0].imm_f32 * vector->imm_values[0].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[1].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[2].imm_f32);
    case Type::U32x4:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s32 * vector->imm_values[0].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[1].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[2].imm_s32,
                              scalar->imm_values[0].imm_s32 * vector->imm_values[3].imm_s32)
                   : ImmValue(scalar->imm_values[0].imm_u32 * vector->imm_values[0].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[1].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[2].imm_u32,
                              scalar->imm_values[0].imm_u32 * vector->imm_values[3].imm_u32);
    case Type::F32x4:
        return ImmValue(scalar->imm_values[0].imm_f32 * vector->imm_values[0].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[1].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[2].imm_f32,
                        scalar->imm_values[0].imm_f32 * vector->imm_values[3].imm_f32);
    case Type::U64:
        return is_signed && scalar->is_signed
                   ? ImmValue(scalar->imm_values[0].imm_s64 * vector->imm_values[0].imm_s64)
                   : ImmValue(scalar->imm_values[0].imm_u64 * vector->imm_values[0].imm_u64);
    case Type::F64:
        return ImmValue(scalar->imm_values[0].imm_f64 * vector->imm_values[0].imm_f64);
    case Type::F64x2:
        return ImmValue(scalar->imm_values[0].imm_f64 * vector->imm_values[0].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[1].imm_f64);
    case Type::F64x3:
        return ImmValue(scalar->imm_values[0].imm_f64 * vector->imm_values[0].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[1].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[2].imm_f64);
    case Type::F64x4:
        return ImmValue(scalar->imm_values[0].imm_f64 * vector->imm_values[0].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[1].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[2].imm_f64,
                        scalar->imm_values[0].imm_f64 * vector->imm_values[3].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", vector->type);
    }
}

ImmValue ImmValue::operator/(const ImmValue& other) const {
    ASSERT(BaseType() == other.BaseType() && other.Dimensions() == 1);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 / other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 / other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 / other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 / other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 / other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 / other.imm_values[0].imm_u32);
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32 / other.imm_values[0].imm_f32);
    case Type::U32x2:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 / other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 / other.imm_values[0].imm_u32);
    case Type::F32x2:
        return ImmValue(imm_values[0].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 / other.imm_values[0].imm_f32);
    case Type::U32x3:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[2].imm_s32 / other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[2].imm_u32 / other.imm_values[0].imm_u32);
    case Type::F32x3:
        return ImmValue(imm_values[0].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[2].imm_f32 / other.imm_values[0].imm_f32);
    case Type::U32x4:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[1].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[2].imm_s32 / other.imm_values[0].imm_s32,
                              imm_values[3].imm_s32 / other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[1].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[2].imm_u32 / other.imm_values[0].imm_u32,
                              imm_values[3].imm_u32 / other.imm_values[0].imm_u32);
    case Type::F32x4:
        return ImmValue(imm_values[0].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[1].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[2].imm_f32 / other.imm_values[0].imm_f32,
                        imm_values[3].imm_f32 / other.imm_values[0].imm_f32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 / other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 / other.imm_values[0].imm_u64);
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64 / other.imm_values[0].imm_f64);
    case Type::F64x2:
        return ImmValue(imm_values[0].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 / other.imm_values[0].imm_f64);
    case Type::F64x3:
        return ImmValue(imm_values[0].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[2].imm_f64 / other.imm_values[0].imm_f64);
    case Type::F64x4:
        return ImmValue(imm_values[0].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[1].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[2].imm_f64 / other.imm_values[0].imm_f64,
                        imm_values[3].imm_f64 / other.imm_values[0].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator%(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 % other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 % other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 % other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 % other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 % other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 % other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 % other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 % other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator&(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U1:
        return ImmValue(imm_values[0].imm_u1 & other.imm_values[0].imm_u1);
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 & other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 & other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 & other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 & other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 & other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 & other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 & other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 & other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator|(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U1:
        return ImmValue(imm_values[0].imm_u1 | other.imm_values[0].imm_u1);
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 | other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 | other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 | other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 | other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 | other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 | other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 | other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 | other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator^(const ImmValue& other) const noexcept {
    ASSERT(type == other.type);
    switch (type) {
    case Type::U1:
        return ImmValue(imm_values[0].imm_u1 ^ other.imm_values[0].imm_u1);
    case Type::U8:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s8 ^ other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 ^ other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s16 ^ other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 ^ other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s32 ^ other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 ^ other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed && other.is_signed
                   ? ImmValue(imm_values[0].imm_s64 ^ other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 ^ other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator<<(const ImmValue& other) const noexcept {
    ASSERT(other.type == Type::U32 && other.Dimensions() == 1);
    switch (type) {
    case Type::U1:
        return ImmValue(imm_values[0].imm_u1 << other.imm_values[0].imm_u1);
    case Type::U8:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s8 << other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 << other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s16 << other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 << other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s32 << other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 << other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s64 << other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 << other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator>>(const ImmValue& other) const noexcept {
    ASSERT(other.type == Type::U32 && other.Dimensions() == 1);
    switch (type) {
    case Type::U1:
        return ImmValue(imm_values[0].imm_u1 >> other.imm_values[0].imm_u1);
    case Type::U8:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s8 >> other.imm_values[0].imm_s8)
                   : ImmValue(imm_values[0].imm_u8 >> other.imm_values[0].imm_u8);
    case Type::U16:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s16 >> other.imm_values[0].imm_s16)
                   : ImmValue(imm_values[0].imm_u16 >> other.imm_values[0].imm_u16);
    case Type::U32:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s32 >> other.imm_values[0].imm_s32)
                   : ImmValue(imm_values[0].imm_u32 >> other.imm_values[0].imm_u32);
    case Type::U64:
        return is_signed
                   ? ImmValue(imm_values[0].imm_s64 >> other.imm_values[0].imm_s64)
                   : ImmValue(imm_values[0].imm_u64 >> other.imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator~() const noexcept {
    switch (type) {
    case Type::U1:
        return ImmValue(~imm_values[0].imm_u1);
    case Type::U8:
        return is_signed ? ImmValue(imm_values[0].imm_s8) : ImmValue(imm_values[0].imm_u8);
    case Type::U16:
        return is_signed ? ImmValue(imm_values[0].imm_s16) : ImmValue(imm_values[0].imm_u16);
    case Type::U32:
        return is_signed ? ImmValue(imm_values[0].imm_s32) : ImmValue(imm_values[0].imm_u32);
    case Type::U64:
        return is_signed ? ImmValue(imm_values[0].imm_s64) : ImmValue(imm_values[0].imm_u64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator++(int) noexcept {
    switch (type) {
    case Type::U8:
        return is_signed ? ImmValue(imm_values[0].imm_s8++) : ImmValue(imm_values[0].imm_u8++);
    case Type::U16:
        return is_signed ? ImmValue(imm_values[0].imm_s16++) : ImmValue(imm_values[0].imm_u16++);
    case Type::U32:
        return is_signed ? ImmValue(imm_values[0].imm_s32++) : ImmValue(imm_values[0].imm_u32++);
    case Type::U64:
        return is_signed ? ImmValue(imm_values[0].imm_s64++) : ImmValue(imm_values[0].imm_u64++);
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32++);
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64++);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator--(int) noexcept {
    switch (type) {
    case Type::U8:
        return is_signed ? ImmValue(imm_values[0].imm_s8--) : ImmValue(imm_values[0].imm_u8--);
    case Type::U16:
        return is_signed ? ImmValue(imm_values[0].imm_s16--) : ImmValue(imm_values[0].imm_u16--);
    case Type::U32:
        return is_signed ? ImmValue(imm_values[0].imm_s32--) : ImmValue(imm_values[0].imm_u32--);
    case Type::U64:
        return is_signed ? ImmValue(imm_values[0].imm_s64--) : ImmValue(imm_values[0].imm_u64--);
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32--);
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64--);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue& ImmValue::operator++() noexcept {
    switch (type) {
    case Type::U8:
        if (is_signed) {
            imm_values[0].imm_s8++;
        } else {
            imm_values[0].imm_u8++;
        }
        break;
    case Type::U16:
        if (is_signed) {
            imm_values[0].imm_s16++;
        } else {
            imm_values[0].imm_u16++;
        }
        break;
    case Type::U32:
        if (is_signed) {
            imm_values[0].imm_s32++;
        } else {
            imm_values[0].imm_u32++;
        }
        break;
    case Type::U64:
        if (is_signed) {
            imm_values[0].imm_s64++;
        } else {
            imm_values[0].imm_u64++;
        }
        break;
    case Type::F32:
        imm_values[0].imm_f32++;
        break;
    case Type::F64:
        imm_values[0].imm_f64++;
        break;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
    return *this;
}

ImmValue& ImmValue::operator--() noexcept {
    switch (type) {
    case Type::U8:
        if (is_signed) {
            imm_values[0].imm_s8--;
        } else {
            imm_values[0].imm_u8--;
        }
        break;
    case Type::U16:
        if (is_signed) {
            imm_values[0].imm_s16--;
        } else {
            imm_values[0].imm_u16--;
        }
        break;
    case Type::U32:
        if (is_signed) {
            imm_values[0].imm_s32--;
        } else {
            imm_values[0].imm_u32--;
        }
        break;
    case Type::U64:
        if (is_signed) {
            imm_values[0].imm_s64--;
        } else {
            imm_values[0].imm_u64--;
        }
        break;
    case Type::F32:
        imm_values[0].imm_f32--;
        break;
    case Type::F64:
        imm_values[0].imm_f64--;
        break;
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
    return *this;
}

ImmValue ImmValue::operator-() const noexcept {
    switch (type) {
    case Type::U8:
        return is_signed ? ImmValue(-imm_values[0].imm_s8) : ImmValue(-imm_values[0].imm_u8);
    case Type::U16:
        return is_signed ? ImmValue(-imm_values[0].imm_s16) : ImmValue(-imm_values[0].imm_u16);
    case Type::U32:
        return is_signed ? ImmValue(-imm_values[0].imm_s32) : ImmValue(-imm_values[0].imm_u32);
    case Type::U32x2:
        return is_signed ? ImmValue(-imm_values[0].imm_s32, -imm_values[1].imm_s32)
                         : ImmValue(-imm_values[0].imm_u32, -imm_values[1].imm_u32);
    case Type::U32x3:
        return is_signed ? ImmValue(-imm_values[0].imm_s32, -imm_values[1].imm_s32,
                                    -imm_values[2].imm_s32)
                         : ImmValue(-imm_values[0].imm_u32, -imm_values[1].imm_u32,
                                    -imm_values[2].imm_u32);
    case Type::U32x4:
        return is_signed ? ImmValue(-imm_values[0].imm_s32, -imm_values[1].imm_s32,
                                    -imm_values[2].imm_s32, -imm_values[3].imm_s32)
                         : ImmValue(-imm_values[0].imm_u32, -imm_values[1].imm_u32,
                                    -imm_values[2].imm_u32, -imm_values[3].imm_u32);
    case Type::U64:
        return is_signed ? ImmValue(-imm_values[0].imm_s64) : ImmValue(-imm_values[0].imm_u64);
    case Type::F32:
        return ImmValue(-imm_values[0].imm_f32);
    case Type::F32x2:
        return ImmValue(-imm_values[0].imm_f32, -imm_values[1].imm_f32);
    case Type::F32x3:
        return ImmValue(-imm_values[0].imm_f32, -imm_values[1].imm_f32, -imm_values[2].imm_f32);
    case Type::F32x4:
        return ImmValue(-imm_values[0].imm_f32, -imm_values[1].imm_f32, -imm_values[2].imm_f32,
                        -imm_values[3].imm_f32);
    case Type::F64:
        return ImmValue(-imm_values[0].imm_f64);
    case Type::F64x2:
        return ImmValue(-imm_values[0].imm_f64, -imm_values[1].imm_f64);
    case Type::F64x3:
        return ImmValue(-imm_values[0].imm_f64, -imm_values[1].imm_f64, -imm_values[2].imm_f64);
    case Type::F64x4:
        return ImmValue(-imm_values[0].imm_f64, -imm_values[1].imm_f64, -imm_values[2].imm_f64,
                        -imm_values[3].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::operator+() const noexcept {
    return *this;
}

// this is not the best way

ImmValue& ImmValue::operator+=(const ImmValue& other) noexcept {
    ImmValue result = *this + other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator-=(const ImmValue& other) noexcept {
    ImmValue result = *this - other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator*=(const ImmValue& other) noexcept {
    ImmValue result = *this * other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator/=(const ImmValue& other) {
    ImmValue result = *this / other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator%=(const ImmValue& other) noexcept {
    ImmValue result = *this % other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator&=(const ImmValue& other) noexcept {
    ImmValue result = *this & other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator|=(const ImmValue& other) noexcept {
    ImmValue result = *this | other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator^=(const ImmValue& other) noexcept {
    ImmValue result = *this ^ other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator<<=(const ImmValue& other) noexcept {
    ImmValue result = *this << other;
    *this = result;
    return *this;
}

ImmValue& ImmValue::operator>>=(const ImmValue& other) noexcept {
    ImmValue result = *this >> other;
    *this = result;
    return *this;
}

ImmValue ImmValue::abs() const noexcept {
    switch (type) {
    case Type::U8:
        return is_signed ? ImmValue(std::abs(imm_values[0].imm_s8))
                         : ImmValue(imm_values[0].imm_u8);
    case Type::U16:
        return is_signed ? ImmValue(std::abs(imm_values[0].imm_s16))
                         : ImmValue(imm_values[0].imm_u16);
    case Type::U32:
        return is_signed ? ImmValue(std::abs(imm_values[0].imm_s32))
                         : ImmValue(imm_values[0].imm_u32);
    case Type::U64:
        return is_signed ? ImmValue(std::abs(imm_values[0].imm_s64))
                         : ImmValue(imm_values[0].imm_u64);
    case Type::F32:
        return ImmValue(std::abs(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::abs(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::recip() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(1.0f / imm_values[0].imm_f32);
    case Type::F64:
        return ImmValue(1.0 / imm_values[0].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::sqrt() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::sqrt(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::sqrt(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::rsqrt() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(1.0f / std::sqrt(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(1.0 / std::sqrt(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::sin() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::sin(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::sin(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::cos() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::cos(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::cos(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::exp2() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::exp2(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::exp2(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::ldexp(const ImmValue& exp) const noexcept {
    ASSERT(type == exp.type);
    switch (type) {
    case Type::F32:
        return ImmValue(std::ldexp(imm_values[0].imm_f32, exp.imm_values[0].imm_s32));
    case Type::F64:
        return ImmValue(std::ldexp(imm_values[0].imm_f64, exp.imm_values[0].imm_s32));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::log2() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::log2(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::log2(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::clamp(const ImmValue& min, const ImmValue& max) const noexcept {
    ASSERT(type == min.type && min.type == max.type);
    switch (type) {
    case Type::U8:
        return is_signed && min.is_signed && max.is_signed
                   ? ImmValue(std::clamp(imm_values[0].imm_s8, min.imm_values[0].imm_s8,
                                         max.imm_values[0].imm_s8))
                   : ImmValue(std::clamp(imm_values[0].imm_u8, min.imm_values[0].imm_u8,
                                         max.imm_values[0].imm_u8));
    case Type::U16:
        return is_signed && min.is_signed && max.is_signed
                   ? ImmValue(std::clamp(imm_values[0].imm_s16, min.imm_values[0].imm_s16,
                                         max.imm_values[0].imm_s16))
                   : ImmValue(std::clamp(imm_values[0].imm_u16, min.imm_values[0].imm_u16,
                                         max.imm_values[0].imm_u16));
    case Type::U32:
        return is_signed && min.is_signed && max.is_signed
                   ? ImmValue(std::clamp(imm_values[0].imm_s32, min.imm_values[0].imm_s32,
                                         max.imm_values[0].imm_s32))
                   : ImmValue(std::clamp(imm_values[0].imm_u32, min.imm_values[0].imm_u32,
                                         max.imm_values[0].imm_u32));
    case Type::U64:
        return is_signed && min.is_signed && max.is_signed
                   ? ImmValue(std::clamp(imm_values[0].imm_s64, min.imm_values[0].imm_s64,
                                         max.imm_values[0].imm_s64))
                   : ImmValue(std::clamp(imm_values[0].imm_u64, min.imm_values[0].imm_u64,
                                         max.imm_values[0].imm_u64));
    case Type::F32:
        return ImmValue(std::clamp(imm_values[0].imm_f32, min.imm_values[0].imm_f32,
                                   max.imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::clamp(imm_values[0].imm_f64, min.imm_values[0].imm_f64,
                                   max.imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::floor() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::floor(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::floor(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::ceil() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::ceil(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::ceil(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::round() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::round(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::round(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::trunc() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(std::trunc(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(std::trunc(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::fract() const noexcept {
    switch (type) {
    case Type::F32:
        return ImmValue(imm_values[0].imm_f32 - std::floor(imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(imm_values[0].imm_f64 - std::floor(imm_values[0].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

bool ImmValue::isnan() const noexcept {
    switch (type) {
    case Type::F32:
        return std::isnan(imm_values[0].imm_f32);
    case Type::F64:
        return std::isnan(imm_values[0].imm_f64);
    case Type::F32x2:
        return std::isnan(imm_values[0].imm_f32) || std::isnan(imm_values[1].imm_f32);
    case Type::F64x2:
        return std::isnan(imm_values[0].imm_f64) || std::isnan(imm_values[1].imm_f64);
    case Type::F32x3:
        return std::isnan(imm_values[0].imm_f32) || std::isnan(imm_values[1].imm_f32) ||
               std::isnan(imm_values[2].imm_f32);
    case Type::F64x3:
        return std::isnan(imm_values[0].imm_f64) || std::isnan(imm_values[1].imm_f64) ||
               std::isnan(imm_values[2].imm_f64);
    case Type::F32x4:
        return std::isnan(imm_values[0].imm_f32) || std::isnan(imm_values[1].imm_f32) ||
               std::isnan(imm_values[2].imm_f32) || std::isnan(imm_values[3].imm_f32);
    case Type::F64x4:
        return std::isnan(imm_values[0].imm_f64) || std::isnan(imm_values[1].imm_f64) ||
               std::isnan(imm_values[2].imm_f64) || std::isnan(imm_values[3].imm_f64);
    default:
        UNREACHABLE_MSG("Invalid type {}", type);
    }
}

ImmValue ImmValue::fma(const ImmValue& a, const ImmValue& b, const ImmValue& c) noexcept {
    ASSERT(a.type == b.type && b.type == c.type);
    switch (a.type) {
    case Type::F32:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32, c.imm_values[0].imm_f32));
    case Type::F64:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64, c.imm_values[0].imm_f64));
    case Type::F32x2:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32, c.imm_values[0].imm_f32),
            std::fma(a.imm_values[1].imm_f32, b.imm_values[1].imm_f32, c.imm_values[1].imm_f32));
    case Type::F64x2:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64, c.imm_values[0].imm_f64),
            std::fma(a.imm_values[1].imm_f64, b.imm_values[1].imm_f64, c.imm_values[1].imm_f64));
    case Type::F32x3:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32, c.imm_values[0].imm_f32),
            std::fma(a.imm_values[1].imm_f32, b.imm_values[1].imm_f32, c.imm_values[1].imm_f32),
            std::fma(a.imm_values[2].imm_f32, b.imm_values[2].imm_f32, c.imm_values[2].imm_f32));
    case Type::F64x3:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64, c.imm_values[0].imm_f64),
            std::fma(a.imm_values[1].imm_f64, b.imm_values[1].imm_f64, c.imm_values[1].imm_f64),
            std::fma(a.imm_values[2].imm_f64, b.imm_values[2].imm_f64, c.imm_values[2].imm_f64));
    case Type::F32x4:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32, c.imm_values[0].imm_f32),
            std::fma(a.imm_values[1].imm_f32, b.imm_values[1].imm_f32, c.imm_values[1].imm_f32),
            std::fma(a.imm_values[2].imm_f32, b.imm_values[2].imm_f32, c.imm_values[2].imm_f32),
            std::fma(a.imm_values[3].imm_f32, b.imm_values[3].imm_f32, c.imm_values[3].imm_f32));
    case Type::F64x4:
        return ImmValue(
            std::fma(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64, c.imm_values[0].imm_f64),
            std::fma(a.imm_values[1].imm_f64, b.imm_values[1].imm_f64, c.imm_values[1].imm_f64),
            std::fma(a.imm_values[2].imm_f64, b.imm_values[2].imm_f64, c.imm_values[2].imm_f64),
            std::fma(a.imm_values[3].imm_f64, b.imm_values[3].imm_f64, c.imm_values[3].imm_f64));
    default:
        UNREACHABLE_MSG("Invalid type {}", a.type);
    }
}

bool ImmValue::IsSupportedValue(const IR::Value& value) noexcept {
    switch (value.Type()) {
    case IR::Type::U1:
    case IR::Type::U8:
    case IR::Type::U16:
    case IR::Type::U32:
    case IR::Type::U64:
    case IR::Type::F32:
    case IR::Type::F64:
        return true;
    default:
        return false;
    }
}

} // namespace Shader::IR

namespace std {

std::size_t hash<Shader::IR::ImmValue>::operator()(const Shader::IR::ImmValue& value) const {
    using namespace Shader::IR;

    u64 h = HashCombine(static_cast<u64>(value.Type()), 0ULL);

    switch (value.Type()) {
    case Type::U1:
        return HashCombine(static_cast<u64>(value.imm_values[0].imm_u1), h);
    case Type::U8:
        return HashCombine(static_cast<u64>(value.imm_values[0].imm_u8), h);
    case Type::U16:
        return HashCombine(static_cast<u64>(value.imm_values[0].imm_u16), h);
    case Type::U32:
    case Type::F32:
        return HashCombine(static_cast<u64>(value.imm_values[0].imm_u32), h);
    case Type::U64:
    case Type::F64:
        return HashCombine(static_cast<u64>(value.imm_values[0].imm_u64), h);
    case Type::U32x2:
    case Type::F32x2:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_u32), h);
        return HashCombine(static_cast<u64>(value.imm_values[1].imm_u32), h);
    case Type::F64x2:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_f64), h);
        return HashCombine(static_cast<u64>(value.imm_values[1].imm_f64), h);
    case Type::U32x3:
    case Type::F32x3:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_u32), h);
        h = HashCombine(static_cast<u64>(value.imm_values[1].imm_u32), h);
        return HashCombine(static_cast<u64>(value.imm_values[2].imm_u32), h);
    case Type::F64x3:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_f64), h);
        h = HashCombine(static_cast<u64>(value.imm_values[1].imm_f64), h);
        return HashCombine(static_cast<u64>(value.imm_values[2].imm_f64), h);
    case Type::U32x4:
    case Type::F32x4:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_u32), h);
        h = HashCombine(static_cast<u64>(value.imm_values[1].imm_u32), h);
        h = HashCombine(static_cast<u64>(value.imm_values[2].imm_u32), h);
        return HashCombine(static_cast<u64>(value.imm_values[3].imm_u32), h);
    case Type::F64x4:
        h = HashCombine(static_cast<u64>(value.imm_values[0].imm_f64), h);
        h = HashCombine(static_cast<u64>(value.imm_values[1].imm_f64), h);
        h = HashCombine(static_cast<u64>(value.imm_values[2].imm_f64), h);
        return HashCombine(static_cast<u64>(value.imm_values[3].imm_f64), h);
    default:
        UNREACHABLE_MSG("Invalid type {}", value.Type());
    }
}

} // namespace std
