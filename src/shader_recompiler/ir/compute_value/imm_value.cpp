// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/hash.h"
#include "shader_recompiler/ir/compute_value/imm_value.h"

namespace Shader::IR::ComputeValue {

ImmValue::ImmValue(const IR::Value& value) noexcept {
    ASSERT(value.IsImmediate());
    switch (value.Type()) {
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
        UNREACHABLE_MSG("Invalid type {}", value.Type());
    }
}

ImmValue::ImmValue(bool value) noexcept {
    imm_values[0].imm_u1 = value;
}

ImmValue::ImmValue(u8 value) noexcept {
    imm_values[0].imm_u8 = value;
}

ImmValue::ImmValue(s8 value) noexcept {
    imm_values[0].imm_s8 = value;
}

ImmValue::ImmValue(u16 value) noexcept {
    imm_values[0].imm_u16 = value;
}

ImmValue::ImmValue(s16 value) noexcept {
    imm_values[0].imm_s16 = value;
}

ImmValue::ImmValue(u32 value) noexcept {
    imm_values[0].imm_u32 = value;
}

ImmValue::ImmValue(s32 value) noexcept {
    imm_values[0].imm_s32 = value;
}

ImmValue::ImmValue(f32 value) noexcept {
    imm_values[0].imm_f32 = value;
}

ImmValue::ImmValue(u64 value) noexcept {
    imm_values[0].imm_u64 = value;
}

ImmValue::ImmValue(s64 value) noexcept {
    imm_values[0].imm_s64 = value;
}

ImmValue::ImmValue(f64 value) noexcept {
    imm_values[0].imm_f64 = value;
}

ImmValue::ImmValue(u32 value1, u32 value2) noexcept {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
}

ImmValue::ImmValue(u32 value1, u32 value2, u32 value3) noexcept {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
    imm_values[2].imm_u32 = value3;
}

ImmValue::ImmValue(u32 value1, u32 value2, u32 value3, u32 value4) noexcept {
    imm_values[0].imm_u32 = value1;
    imm_values[1].imm_u32 = value2;
    imm_values[2].imm_u32 = value3;
    imm_values[3].imm_u32 = value4;
}

ImmValue::ImmValue(s32 value1, s32 value2) noexcept {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
}

ImmValue::ImmValue(s32 value1, s32 value2, s32 value3) noexcept {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
    imm_values[2].imm_s32 = value3;
}

ImmValue::ImmValue(s32 value1, s32 value2, s32 value3, s32 value4) noexcept {
    imm_values[0].imm_s32 = value1;
    imm_values[1].imm_s32 = value2;
    imm_values[2].imm_s32 = value3;
    imm_values[3].imm_s32 = value4;
}

ImmValue::ImmValue(f32 value1, f32 value2) noexcept {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
}

ImmValue::ImmValue(f32 value1, f32 value2, f32 value3) noexcept {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
    imm_values[2].imm_f32 = value3;
}

ImmValue::ImmValue(f32 value1, f32 value2, f32 value3, f32 value4) noexcept {
    imm_values[0].imm_f32 = value1;
    imm_values[1].imm_f32 = value2;
    imm_values[2].imm_f32 = value3;
    imm_values[3].imm_f32 = value4;
}

ImmValue::ImmValue(u64 value1, u64 value2) noexcept {
    imm_values[0].imm_u64 = value1;
    imm_values[1].imm_u64 = value2;
}

ImmValue::ImmValue(u64 value1, u64 value2, u64 value3) noexcept {
    imm_values[0].imm_u64 = value1;
    imm_values[1].imm_u64 = value2;
    imm_values[2].imm_u64 = value3;
}

ImmValue::ImmValue(u64 value1, u64 value2, u64 value3, u64 value4) noexcept {
    imm_values[0].imm_u64 = value1;
    imm_values[1].imm_u64 = value2;
    imm_values[2].imm_u64 = value3;
    imm_values[3].imm_u64 = value4;
}

ImmValue::ImmValue(s64 value1, s64 value2) noexcept {
    imm_values[0].imm_s64 = value1;
    imm_values[1].imm_s64 = value2;
}

ImmValue::ImmValue(s64 value1, s64 value2, s64 value3) noexcept {
    imm_values[0].imm_s64 = value1;
    imm_values[1].imm_s64 = value2;
    imm_values[2].imm_s64 = value3;
}

ImmValue::ImmValue(s64 value1, s64 value2, s64 value3, s64 value4) noexcept {
    imm_values[0].imm_s64 = value1;
    imm_values[1].imm_s64 = value2;
    imm_values[2].imm_s64 = value3;
    imm_values[3].imm_s64 = value4;
}

ImmValue::ImmValue(f64 value1, f64 value2) noexcept {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
}

ImmValue::ImmValue(f64 value1, f64 value2, f64 value3) noexcept {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
    imm_values[2].imm_f64 = value3;
}

ImmValue::ImmValue(f64 value1, f64 value2, f64 value3, f64 value4) noexcept {
    imm_values[0].imm_f64 = value1;
    imm_values[1].imm_f64 = value2;
    imm_values[2].imm_f64 = value3;
    imm_values[3].imm_f64 = value4;
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2) noexcept {
    imm_values[0] = value1.imm_values[0];
    imm_values[1] = value2.imm_values[0];
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2,
                   const ImmValue& value3) noexcept {
    imm_values[0] = value1.imm_values[0];
    imm_values[1] = value2.imm_values[0];
    imm_values[2] = value3.imm_values[0];
}

ImmValue::ImmValue(const ImmValue& value1, const ImmValue& value2, const ImmValue& value3,
                   const ImmValue& value4) noexcept {
    imm_values[0] = value1.imm_values[0];
    imm_values[1] = value2.imm_values[0];
    imm_values[2] = value3.imm_values[0];
    imm_values[3] = value4.imm_values[0];
}

ImmValue ImmValue::CompositeFrom2x2(const ImmValue& value1, const ImmValue& value2) noexcept {
    ImmValue result;
    result.imm_values[0] = value1.imm_values[0];
    result.imm_values[1] = value1.imm_values[1];
    result.imm_values[2] = value2.imm_values[0];
    result.imm_values[3] = value2.imm_values[1];
    return result;
}

bool ImmValue::operator==(const ImmValue& other) const noexcept {
    return imm_values[0].imm_u64 == other.imm_values[0].imm_u64 &&
           imm_values[1].imm_u64 == other.imm_values[1].imm_u64 &&
           imm_values[2].imm_u64 == other.imm_values[2].imm_u64 &&
           imm_values[3].imm_u64 == other.imm_values[3].imm_u64;
}

bool ImmValue::operator!=(const ImmValue& other) const noexcept {
    return !operator==(other);
}

ImmValue ImmValue::Extract(const ImmValue& vec, const ImmValue& index) noexcept {
    ImmValue result;
    result.imm_values[0] = vec.imm_values[index.imm_values[0].imm_u32];
    return result;
}

ImmValue ImmValue::Insert(const ImmValue& vec, const ImmValue& value,
                          const ImmValue& index) noexcept {
    ImmValue result = vec;
    result.imm_values[index.imm_values[0].imm_u32] = value.imm_values[0];
    return result;
}

template <>
ImmValue ImmValue::Convert<Type::U16, false, Type::U32, false>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u16>(in.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Convert<Type::U32, false, Type::U16, false>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u32>(in.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Convert<Type::U32, false, Type::U32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u32>(in.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Convert<Type::U32, false, Type::F32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u32>(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Convert<Type::U32, true, Type::F32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u32>(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Convert<Type::U32, true, Type::F64, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<u32>(in.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Convert<Type::F32, true, Type::U16, false>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f32>(in.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Convert<Type::F32, true, Type::U32, false>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f32>(in.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Convert<Type::F32, true, Type::U32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f32>(in.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Convert<Type::F32, true, Type::F64, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f32>(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Convert<Type::F64, true, Type::U32, false>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f64>(in.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Convert<Type::F64, true, Type::U32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f64>(in.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Convert<Type::F64, true, Type::F32, true>(const ImmValue& in) noexcept {
    return ImmValue(static_cast<f64>(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Add<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 + b.imm_values[0].imm_u8,
                    a.imm_values[1].imm_u8 + b.imm_values[1].imm_u8,
                    a.imm_values[2].imm_u8 + b.imm_values[2].imm_u8,
                    a.imm_values[3].imm_u8 + b.imm_values[3].imm_u8);
}

template <>
ImmValue ImmValue::Add<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s8 + b.imm_values[0].imm_s8,
                    a.imm_values[1].imm_s8 + b.imm_values[1].imm_s8,
                    a.imm_values[2].imm_s8 + b.imm_values[2].imm_s8,
                    a.imm_values[3].imm_s8 + b.imm_values[3].imm_s8);
}

template <>
ImmValue ImmValue::Add<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 + b.imm_values[0].imm_u16,
                    a.imm_values[1].imm_u16 + b.imm_values[1].imm_u16,
                    a.imm_values[2].imm_u16 + b.imm_values[2].imm_u16,
                    a.imm_values[3].imm_u16 + b.imm_values[3].imm_u16);
}

template <>
ImmValue ImmValue::Add<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s16 + b.imm_values[0].imm_s16,
                    a.imm_values[1].imm_s16 + b.imm_values[1].imm_s16,
                    a.imm_values[2].imm_s16 + b.imm_values[2].imm_s16,
                    a.imm_values[3].imm_s16 + b.imm_values[3].imm_s16);
}

template <>
ImmValue ImmValue::Add<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 + b.imm_values[0].imm_u32,
                    a.imm_values[1].imm_u32 + b.imm_values[1].imm_u32,
                    a.imm_values[2].imm_u32 + b.imm_values[2].imm_u32,
                    a.imm_values[3].imm_u32 + b.imm_values[3].imm_u32);
}

template <>
ImmValue ImmValue::Add<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s32 + b.imm_values[0].imm_s32,
                    a.imm_values[1].imm_s32 + b.imm_values[1].imm_s32,
                    a.imm_values[2].imm_s32 + b.imm_values[2].imm_s32,
                    a.imm_values[3].imm_s32 + b.imm_values[3].imm_s32);
}

template <>
ImmValue ImmValue::Add<Type::F32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f32 + b.imm_values[0].imm_f32,
                    a.imm_values[1].imm_f32 + b.imm_values[1].imm_f32,
                    a.imm_values[2].imm_f32 + b.imm_values[2].imm_f32,
                    a.imm_values[3].imm_f32 + b.imm_values[3].imm_f32);
}

template <>
ImmValue ImmValue::Add<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 + b.imm_values[0].imm_u64,
                    a.imm_values[1].imm_u64 + b.imm_values[1].imm_u64,
                    a.imm_values[2].imm_u64 + b.imm_values[2].imm_u64,
                    a.imm_values[3].imm_u64 + b.imm_values[3].imm_u64);
}

template <>
ImmValue ImmValue::Add<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s64 + b.imm_values[0].imm_s64,
                    a.imm_values[1].imm_s64 + b.imm_values[1].imm_s64,
                    a.imm_values[2].imm_s64 + b.imm_values[2].imm_s64,
                    a.imm_values[3].imm_s64 + b.imm_values[3].imm_s64);
}

template <>
ImmValue ImmValue::Add<Type::F64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f64 + b.imm_values[0].imm_f64,
                    a.imm_values[1].imm_f64 + b.imm_values[1].imm_f64,
                    a.imm_values[2].imm_f64 + b.imm_values[2].imm_f64,
                    a.imm_values[3].imm_f64 + b.imm_values[3].imm_f64);
}

template <>
ImmValue ImmValue::AddCarry<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    u8 result = a.imm_values[0].imm_u8 + b.imm_values[0].imm_u8;
    u8 carry = (result < a.imm_values[0].imm_u8) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    u8 result = a.imm_values[0].imm_u8 + b.imm_values[0].imm_u8;
    u8 carry = (result < a.imm_values[0].imm_u8) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    u16 result = a.imm_values[0].imm_u16 + b.imm_values[0].imm_u16;
    u16 carry = (result < a.imm_values[0].imm_u16) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    s16 result = a.imm_values[0].imm_s16 + b.imm_values[0].imm_s16;
    s16 carry = (result < a.imm_values[0].imm_s16) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    u32 result = a.imm_values[0].imm_u32 + b.imm_values[0].imm_u32;
    u32 carry = (result < a.imm_values[0].imm_u32) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    s32 result = a.imm_values[0].imm_s32 + b.imm_values[0].imm_s32;
    s32 carry = (result < a.imm_values[0].imm_s32) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    u64 result = a.imm_values[0].imm_u64 + b.imm_values[0].imm_u64;
    u64 carry = (result < a.imm_values[0].imm_u64) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::AddCarry<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    s64 result = a.imm_values[0].imm_s64 + b.imm_values[0].imm_s64;
    s64 carry = (result < a.imm_values[0].imm_s64) ? 1 : 0;
    return ImmValue(result, carry);
}

template <>
ImmValue ImmValue::Sub<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 - b.imm_values[0].imm_u8,
                    a.imm_values[1].imm_u8 - b.imm_values[1].imm_u8,
                    a.imm_values[2].imm_u8 - b.imm_values[2].imm_u8,
                    a.imm_values[3].imm_u8 - b.imm_values[3].imm_u8);
}

template <>
ImmValue ImmValue::Sub<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s8 - b.imm_values[0].imm_s8,
                    a.imm_values[1].imm_s8 - b.imm_values[1].imm_s8,
                    a.imm_values[2].imm_s8 - b.imm_values[2].imm_s8,
                    a.imm_values[3].imm_s8 - b.imm_values[3].imm_s8);
}

template <>
ImmValue ImmValue::Sub<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 - b.imm_values[0].imm_u16,
                    a.imm_values[1].imm_u16 - b.imm_values[1].imm_u16,
                    a.imm_values[2].imm_u16 - b.imm_values[2].imm_u16,
                    a.imm_values[3].imm_u16 - b.imm_values[3].imm_u16);
}

template <>
ImmValue ImmValue::Sub<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s16 - b.imm_values[0].imm_s16,
                    a.imm_values[1].imm_s16 - b.imm_values[1].imm_s16,
                    a.imm_values[2].imm_s16 - b.imm_values[2].imm_s16,
                    a.imm_values[3].imm_s16 - b.imm_values[3].imm_s16);
}

template <>
ImmValue ImmValue::Sub<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 - b.imm_values[0].imm_u32,
                    a.imm_values[1].imm_u32 - b.imm_values[1].imm_u32,
                    a.imm_values[2].imm_u32 - b.imm_values[2].imm_u32,
                    a.imm_values[3].imm_u32 - b.imm_values[3].imm_u32);
}

template <>
ImmValue ImmValue::Sub<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s32 - b.imm_values[0].imm_s32,
                    a.imm_values[1].imm_s32 - b.imm_values[1].imm_s32,
                    a.imm_values[2].imm_s32 - b.imm_values[2].imm_s32,
                    a.imm_values[3].imm_s32 - b.imm_values[3].imm_s32);
}

template <>
ImmValue ImmValue::Sub<Type::F32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f32 - b.imm_values[0].imm_f32,
                    a.imm_values[1].imm_f32 - b.imm_values[1].imm_f32,
                    a.imm_values[2].imm_f32 - b.imm_values[2].imm_f32,
                    a.imm_values[3].imm_f32 - b.imm_values[3].imm_f32);
}

template <>
ImmValue ImmValue::Sub<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 - b.imm_values[0].imm_u64,
                    a.imm_values[1].imm_u64 - b.imm_values[1].imm_u64,
                    a.imm_values[2].imm_u64 - b.imm_values[2].imm_u64,
                    a.imm_values[3].imm_u64 - b.imm_values[3].imm_u64);
}

template <>
ImmValue ImmValue::Sub<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s64 - b.imm_values[0].imm_s64,
                    a.imm_values[1].imm_s64 - b.imm_values[1].imm_s64,
                    a.imm_values[2].imm_s64 - b.imm_values[2].imm_s64,
                    a.imm_values[3].imm_s64 - b.imm_values[3].imm_s64);
}

template <>
ImmValue ImmValue::Sub<Type::F64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f64 - b.imm_values[0].imm_f64,
                    a.imm_values[1].imm_f64 - b.imm_values[1].imm_f64,
                    a.imm_values[2].imm_f64 - b.imm_values[2].imm_f64,
                    a.imm_values[3].imm_f64 - b.imm_values[3].imm_f64);
}

template <>
ImmValue ImmValue::Mul<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 * b.imm_values[0].imm_u8,
                    a.imm_values[1].imm_u8 * b.imm_values[0].imm_u8,
                    a.imm_values[2].imm_u8 * b.imm_values[0].imm_u8,
                    a.imm_values[3].imm_u8 * b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Mul<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s8 * b.imm_values[0].imm_s8,
                    a.imm_values[1].imm_s8 * b.imm_values[0].imm_s8,
                    a.imm_values[2].imm_s8 * b.imm_values[0].imm_s8,
                    a.imm_values[3].imm_s8 * b.imm_values[0].imm_s8);
}

template <>
ImmValue ImmValue::Mul<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 * b.imm_values[0].imm_u16,
                    a.imm_values[1].imm_u16 * b.imm_values[0].imm_u16,
                    a.imm_values[2].imm_u16 * b.imm_values[0].imm_u16,
                    a.imm_values[3].imm_u16 * b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Mul<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s16 * b.imm_values[0].imm_s16,
                    a.imm_values[1].imm_s16 * b.imm_values[0].imm_s16,
                    a.imm_values[2].imm_s16 * b.imm_values[0].imm_s16,
                    a.imm_values[3].imm_s16 * b.imm_values[0].imm_s16);
}

template <>
ImmValue ImmValue::Mul<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 * b.imm_values[0].imm_u32,
                    a.imm_values[1].imm_u32 * b.imm_values[0].imm_u32,
                    a.imm_values[2].imm_u32 * b.imm_values[0].imm_u32,
                    a.imm_values[3].imm_u32 * b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Mul<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s32 * b.imm_values[0].imm_s32,
                    a.imm_values[1].imm_s32 * b.imm_values[0].imm_s32,
                    a.imm_values[2].imm_s32 * b.imm_values[0].imm_s32,
                    a.imm_values[3].imm_s32 * b.imm_values[0].imm_s32);
}

template <>
ImmValue ImmValue::Mul<Type::F32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f32 * b.imm_values[0].imm_f32,
                    a.imm_values[1].imm_f32 * b.imm_values[0].imm_f32,
                    a.imm_values[2].imm_f32 * b.imm_values[0].imm_f32,
                    a.imm_values[3].imm_f32 * b.imm_values[0].imm_f32);
}

template <>
ImmValue ImmValue::Mul<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 * b.imm_values[0].imm_u64,
                    a.imm_values[1].imm_u64 * b.imm_values[0].imm_u64,
                    a.imm_values[2].imm_u64 * b.imm_values[0].imm_u64,
                    a.imm_values[3].imm_u64 * b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Mul<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s64 * b.imm_values[0].imm_s64,
                    a.imm_values[1].imm_s64 * b.imm_values[0].imm_s64,
                    a.imm_values[2].imm_s64 * b.imm_values[0].imm_s64,
                    a.imm_values[3].imm_s64 * b.imm_values[0].imm_s64);
}

template <>
ImmValue ImmValue::Mul<Type::F64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_f64 * b.imm_values[0].imm_f64,
                    a.imm_values[1].imm_f64 * b.imm_values[0].imm_f64,
                    a.imm_values[2].imm_f64 * b.imm_values[0].imm_f64,
                    a.imm_values[3].imm_f64 * b.imm_values[0].imm_f64);
}

template <>
ImmValue ImmValue::Div<Type::U8, false>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_u8 / b.imm_values[0].imm_u8,
                    a.imm_values[1].imm_u8 / b.imm_values[0].imm_u8,
                    a.imm_values[2].imm_u8 / b.imm_values[0].imm_u8,
                    a.imm_values[3].imm_u8 / b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Div<Type::U8, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_s8 / b.imm_values[0].imm_s8,
                    a.imm_values[1].imm_s8 / b.imm_values[0].imm_s8,
                    a.imm_values[2].imm_s8 / b.imm_values[0].imm_s8,
                    a.imm_values[3].imm_s8 / b.imm_values[0].imm_s8);
}

template <>
ImmValue ImmValue::Div<Type::U16, false>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_u16 / b.imm_values[0].imm_u16,
                    a.imm_values[1].imm_u16 / b.imm_values[0].imm_u16,
                    a.imm_values[2].imm_u16 / b.imm_values[0].imm_u16,
                    a.imm_values[3].imm_u16 / b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Div<Type::U16, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_s16 / b.imm_values[0].imm_s16,
                    a.imm_values[1].imm_s16 / b.imm_values[0].imm_s16,
                    a.imm_values[2].imm_s16 / b.imm_values[0].imm_s16,
                    a.imm_values[3].imm_s16 / b.imm_values[0].imm_s16);
}

template <>
ImmValue ImmValue::Div<Type::U32, false>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_u32 / b.imm_values[0].imm_u32,
                    a.imm_values[1].imm_u32 / b.imm_values[0].imm_u32,
                    a.imm_values[2].imm_u32 / b.imm_values[0].imm_u32,
                    a.imm_values[3].imm_u32 / b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Div<Type::U32, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_s32 / b.imm_values[0].imm_s32,
                    a.imm_values[1].imm_s32 / b.imm_values[0].imm_s32,
                    a.imm_values[2].imm_s32 / b.imm_values[0].imm_s32,
                    a.imm_values[3].imm_s32 / b.imm_values[0].imm_s32);
}

template <>
ImmValue ImmValue::Div<Type::F32, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_f32 / b.imm_values[0].imm_f32,
                    a.imm_values[1].imm_f32 / b.imm_values[0].imm_f32,
                    a.imm_values[2].imm_f32 / b.imm_values[0].imm_f32,
                    a.imm_values[3].imm_f32 / b.imm_values[0].imm_f32);
}

template <>
ImmValue ImmValue::Div<Type::U64, false>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_u64 / b.imm_values[0].imm_u64,
                    a.imm_values[1].imm_u64 / b.imm_values[0].imm_u64,
                    a.imm_values[2].imm_u64 / b.imm_values[0].imm_u64,
                    a.imm_values[3].imm_u64 / b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Div<Type::U64, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_s64 / b.imm_values[0].imm_s64,
                    a.imm_values[1].imm_s64 / b.imm_values[0].imm_s64,
                    a.imm_values[2].imm_s64 / b.imm_values[0].imm_s64,
                    a.imm_values[3].imm_s64 / b.imm_values[0].imm_s64);
}

template <>
ImmValue ImmValue::Div<Type::F64, true>(const ImmValue& a, const ImmValue& b) {
    return ImmValue(a.imm_values[0].imm_f64 / b.imm_values[0].imm_f64,
                    a.imm_values[1].imm_f64 / b.imm_values[0].imm_f64,
                    a.imm_values[2].imm_f64 / b.imm_values[0].imm_f64,
                    a.imm_values[3].imm_f64 / b.imm_values[0].imm_f64);
}

template <>
ImmValue ImmValue::Mod<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 % b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Mod<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s8 % b.imm_values[0].imm_s8);
}

template <>
ImmValue ImmValue::Mod<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 % b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Mod<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s16 % b.imm_values[0].imm_s16);
}

template <>
ImmValue ImmValue::Mod<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 % b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Mod<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s32 % b.imm_values[0].imm_s32);
}

template <>
ImmValue ImmValue::Mod<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 % b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Mod<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s64 % b.imm_values[0].imm_s64);
}

template <>
ImmValue ImmValue::And<Type::U1>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u1 & b.imm_values[0].imm_u1);
}

template <>
ImmValue ImmValue::And<Type::U8>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 & b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::And<Type::U16>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 & b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::And<Type::U32>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 & b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::And<Type::U64>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 & b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Or<Type::U1>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u1 | b.imm_values[0].imm_u1);
}

template <>
ImmValue ImmValue::Or<Type::U8>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 | b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Or<Type::U16>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 | b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Or<Type::U32>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 | b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Or<Type::U64>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 | b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Xor<Type::U1>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u1 ^ b.imm_values[0].imm_u1);
}

template <>
ImmValue ImmValue::Xor<Type::U8>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 ^ b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Xor<Type::U16>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 ^ b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Xor<Type::U32>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 ^ b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Xor<Type::U64>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 ^ b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::LShift<Type::U8>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 << b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::LShift<Type::U16>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 << b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::LShift<Type::U32>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 << b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::LShift<Type::U64>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 << b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::RShift<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u8 >> b.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::RShift<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s8 >> b.imm_values[0].imm_s8);
}

template <>
ImmValue ImmValue::RShift<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u16 >> b.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::RShift<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s16 >> b.imm_values[0].imm_s16);
}

template <>
ImmValue ImmValue::RShift<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u32 >> b.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::RShift<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s32 >> b.imm_values[0].imm_s32);
}

template <>
ImmValue ImmValue::RShift<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_u64 >> b.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::RShift<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(a.imm_values[0].imm_s64 >> b.imm_values[0].imm_s64);
}

template <>
ImmValue ImmValue::Not<Type::U1>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_u1);
}

template <>
ImmValue ImmValue::Not<Type::U8>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_u8);
}

template <>
ImmValue ImmValue::Not<Type::U16>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_u16);
}

template <>
ImmValue ImmValue::Not<Type::U32>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_u32);
}

template <>
ImmValue ImmValue::Not<Type::U64>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_u64);
}

template <>
ImmValue ImmValue::Neg<Type::U8>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_s8, -in.imm_values[1].imm_s8, -in.imm_values[2].imm_s8,
                    -in.imm_values[3].imm_s8);
}

template <>
ImmValue ImmValue::Neg<Type::U16>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_s16, -in.imm_values[1].imm_s16, -in.imm_values[2].imm_s16,
                    -in.imm_values[3].imm_s16);
}

template <>
ImmValue ImmValue::Neg<Type::U32>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_s32, -in.imm_values[1].imm_s32, -in.imm_values[2].imm_s32,
                    -in.imm_values[3].imm_s32);
}

template <>
ImmValue ImmValue::Neg<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_f32, -in.imm_values[1].imm_f32, -in.imm_values[2].imm_f32,
                    -in.imm_values[3].imm_f32);
}

template <>
ImmValue ImmValue::Neg<Type::U64>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_s64, -in.imm_values[1].imm_s64, -in.imm_values[2].imm_s64,
                    -in.imm_values[3].imm_s64);
}

template <>
ImmValue ImmValue::Neg<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(-in.imm_values[0].imm_f64, -in.imm_values[1].imm_f64, -in.imm_values[2].imm_f64,
                    -in.imm_values[3].imm_f64);
}

template <>
ImmValue ImmValue::Abs<Type::U8>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_s8));
}

template <>
ImmValue ImmValue::Abs<Type::U16>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_s16));
}

template <>
ImmValue ImmValue::Abs<Type::U32>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Abs<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Abs<Type::U64>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_s64));
}

template <>
ImmValue ImmValue::Abs<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::abs(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Recip<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(1.0f / in.imm_values[0].imm_f32);
}

template <>
ImmValue ImmValue::Recip<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(1.0 / in.imm_values[0].imm_f64);
}

template <>
ImmValue ImmValue::Sqrt<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::sqrt(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Sqrt<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::sqrt(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Rsqrt<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(1.0f / std::sqrt(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Rsqrt<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(1.0 / std::sqrt(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Sin<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::sin(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Sin<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::sin(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Cos<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::cos(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Cos<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::cos(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Exp2<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::exp2(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Exp2<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::exp2(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Ldexp<Type::F32>(const ImmValue& in, const ImmValue& exp) noexcept {
    return ImmValue(std::ldexp(in.imm_values[0].imm_f32, exp.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Ldexp<Type::F64>(const ImmValue& in, const ImmValue& exp) noexcept {
    return ImmValue(std::ldexp(in.imm_values[0].imm_f64, exp.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Log2<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::log2(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Log2<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::log2(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Min<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_u8, b.imm_values[0].imm_u8));
}

template <>
ImmValue ImmValue::Min<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_s8, b.imm_values[0].imm_s8));
}

template <>
ImmValue ImmValue::Min<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_u16, b.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Min<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_s16, b.imm_values[0].imm_s16));
}

template <>
ImmValue ImmValue::Min<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_u32, b.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Min<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_s32, b.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Min<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_u64, b.imm_values[0].imm_u64));
}

template <>
ImmValue ImmValue::Min<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_s64, b.imm_values[0].imm_s64));
}

template <>
ImmValue ImmValue::Min<Type::F32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Min<Type::F64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::min(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Max<Type::U8, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_u8, b.imm_values[0].imm_u8));
}

template <>
ImmValue ImmValue::Max<Type::U8, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_s8, b.imm_values[0].imm_s8));
}

template <>
ImmValue ImmValue::Max<Type::U16, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_u16, b.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Max<Type::U16, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_s16, b.imm_values[0].imm_s16));
}

template <>
ImmValue ImmValue::Max<Type::U32, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_u32, b.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Max<Type::U32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_s32, b.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Max<Type::U64, false>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_u64, b.imm_values[0].imm_u64));
}

template <>
ImmValue ImmValue::Max<Type::U64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_s64, b.imm_values[0].imm_s64));
}

template <>
ImmValue ImmValue::Max<Type::F32, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Max<Type::F64, true>(const ImmValue& a, const ImmValue& b) noexcept {
    return ImmValue(std::max(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Clamp<Type::U8, false>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_u8, min.imm_values[0].imm_u8, max.imm_values[0].imm_u8));
}

template <>
ImmValue ImmValue::Clamp<Type::U8, true>(const ImmValue& in, const ImmValue& min,
                                         const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_s8, min.imm_values[0].imm_s8, max.imm_values[0].imm_s8));
}

template <>
ImmValue ImmValue::Clamp<Type::U16, false>(const ImmValue& in, const ImmValue& min,
                                           const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_u16, min.imm_values[0].imm_u16, max.imm_values[0].imm_u16));
}

template <>
ImmValue ImmValue::Clamp<Type::U16, true>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_s16, min.imm_values[0].imm_s16, max.imm_values[0].imm_s16));
}

template <>
ImmValue ImmValue::Clamp<Type::U32, false>(const ImmValue& in, const ImmValue& min,
                                           const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_u32, min.imm_values[0].imm_u32, max.imm_values[0].imm_u32));
}

template <>
ImmValue ImmValue::Clamp<Type::U32, true>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_s32, min.imm_values[0].imm_s32, max.imm_values[0].imm_s32));
}

template <>
ImmValue ImmValue::Clamp<Type::U64, false>(const ImmValue& in, const ImmValue& min,
                                           const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_u64, min.imm_values[0].imm_u64, max.imm_values[0].imm_u64));
}

template <>
ImmValue ImmValue::Clamp<Type::U64, true>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_s64, min.imm_values[0].imm_s64, max.imm_values[0].imm_s64));
}

template <>
ImmValue ImmValue::Clamp<Type::F32, true>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_f32, min.imm_values[0].imm_f32, max.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Clamp<Type::F64, true>(const ImmValue& in, const ImmValue& min,
                                          const ImmValue& max) noexcept {
    return ImmValue(
        std::clamp(in.imm_values[0].imm_f64, min.imm_values[0].imm_f64, max.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Floor<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::floor(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Floor<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::floor(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Ceil<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::ceil(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Ceil<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::ceil(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Round<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::round(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Round<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::round(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Trunc<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(std::trunc(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Trunc<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(std::trunc(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Fract<Type::F32>(const ImmValue& in) noexcept {
    return ImmValue(in.imm_values[0].imm_f32 - std::floor(in.imm_values[0].imm_f32));
}

template <>
ImmValue ImmValue::Fract<Type::F64>(const ImmValue& in) noexcept {
    return ImmValue(in.imm_values[0].imm_f64 - std::floor(in.imm_values[0].imm_f64));
}

template <>
ImmValue ImmValue::Fma<Type::F32>(const ImmValue& a, const ImmValue& b,
                                  const ImmValue& c) noexcept {
    return ImmValue(
        std::fma(a.imm_values[0].imm_f32, b.imm_values[0].imm_f32, c.imm_values[0].imm_f32),
        std::fma(a.imm_values[1].imm_f32, b.imm_values[1].imm_f32, c.imm_values[1].imm_f32),
        std::fma(a.imm_values[2].imm_f32, b.imm_values[2].imm_f32, c.imm_values[2].imm_f32),
        std::fma(a.imm_values[3].imm_f32, b.imm_values[3].imm_f32, c.imm_values[3].imm_f32));
}

template <>
ImmValue ImmValue::Fma<Type::F64>(const ImmValue& a, const ImmValue& b,
                                  const ImmValue& c) noexcept {
    return ImmValue(
        std::fma(a.imm_values[0].imm_f64, b.imm_values[0].imm_f64, c.imm_values[0].imm_f64),
        std::fma(a.imm_values[1].imm_f64, b.imm_values[1].imm_f64, c.imm_values[1].imm_f64),
        std::fma(a.imm_values[2].imm_f64, b.imm_values[2].imm_f64, c.imm_values[2].imm_f64),
        std::fma(a.imm_values[3].imm_f64, b.imm_values[3].imm_f64, c.imm_values[3].imm_f64));
}

template <>
bool ImmValue::IsNan<Type::F32>(const ImmValue& in) noexcept {
    return std::isnan(in.imm_values[0].imm_f32) || std::isnan(in.imm_values[1].imm_f32) ||
           std::isnan(in.imm_values[2].imm_f32) || std::isnan(in.imm_values[3].imm_f32);
}

template <>
bool ImmValue::IsNan<Type::F64>(const ImmValue& in) noexcept {
    return std::isnan(in.imm_values[0].imm_f64) || std::isnan(in.imm_values[1].imm_f64) ||
           std::isnan(in.imm_values[2].imm_f64) || std::isnan(in.imm_values[3].imm_f64);
}

bool ImmValue::IsSupportedValue(const IR::Value& value) noexcept {
    if (!value.IsImmediate()) {
        return false;
    }
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

} // namespace Shader::IR::ComputeValue

namespace std {

std::size_t hash<Shader::IR::ComputeValue::ImmValue>::operator()(
    const Shader::IR::ComputeValue::ImmValue& value) const {
    using namespace Shader::IR::ComputeValue;

    u64 h = HashCombine(value.imm_values[0].imm_u64, 0UL);
    h = HashCombine(value.imm_values[1].imm_u64, h);
    h = HashCombine(value.imm_values[2].imm_u64, h);
    return HashCombine(value.imm_values[3].imm_u64, h);
}

} // namespace std