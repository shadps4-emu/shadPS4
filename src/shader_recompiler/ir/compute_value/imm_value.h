// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <tuple>
#include <type_traits>
#include "common/assert.h"
#include "shader_recompiler/exception.h"
#include "shader_recompiler/ir/type.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR::ComputeValue {

// Holds an immediate value and provides helper functions to do arithmetic operations on it.

class ImmValue {
public:
    ImmValue() noexcept = default;
    ImmValue(const ImmValue& value) noexcept = default;
    explicit ImmValue(const IR::Value& value) noexcept;
    explicit ImmValue(bool value) noexcept;
    explicit ImmValue(u8 value) noexcept;
    explicit ImmValue(s8 value) noexcept;
    explicit ImmValue(u16 value) noexcept;
    explicit ImmValue(s16 value) noexcept;
    explicit ImmValue(u32 value) noexcept;
    explicit ImmValue(s32 value) noexcept;
    explicit ImmValue(f32 value) noexcept;
    explicit ImmValue(u64 value) noexcept;
    explicit ImmValue(s64 value) noexcept;
    explicit ImmValue(f64 value) noexcept;
    ImmValue(u32 value1, u32 value2) noexcept;
    ImmValue(u32 value1, u32 value2, u32 value3) noexcept;
    ImmValue(u32 value1, u32 value2, u32 value3, u32 value4) noexcept;
    ImmValue(s32 value1, s32 value2) noexcept;
    ImmValue(s32 value1, s32 value2, s32 value3) noexcept;
    ImmValue(s32 value1, s32 value2, s32 value3, s32 value4) noexcept;
    ImmValue(f32 value1, f32 value2) noexcept;
    ImmValue(f32 value1, f32 value2, f32 value3) noexcept;
    ImmValue(f32 value1, f32 value2, f32 value3, f32 value4) noexcept;
    ImmValue(u64 value1, u64 value2) noexcept;
    ImmValue(u64 value1, u64 value2, u64 value3) noexcept;
    ImmValue(u64 value1, u64 value2, u64 value3, u64 value4) noexcept;
    ImmValue(s64 value1, s64 value2) noexcept;
    ImmValue(s64 value1, s64 value2, s64 value3) noexcept;
    ImmValue(s64 value1, s64 value2, s64 value3, s64 value4) noexcept;
    ImmValue(f64 value1, f64 value2) noexcept;
    ImmValue(f64 value1, f64 value2, f64 value3) noexcept;
    ImmValue(f64 value1, f64 value2, f64 value3, f64 value4) noexcept;
    ImmValue(const ImmValue& value1, const ImmValue& value2) noexcept;
    ImmValue(const ImmValue& value1, const ImmValue& value2, const ImmValue& value3) noexcept;
    ImmValue(const ImmValue& value1, const ImmValue& value2, const ImmValue& value3,
             const ImmValue& value4) noexcept;

    [[nodiscard]] static ImmValue CompositeFrom2x2(const ImmValue& value1,
                                                   const ImmValue& value2) noexcept;

    [[nodiscard]] bool U1() const noexcept;
    [[nodiscard]] u8 U8() const noexcept;
    [[nodiscard]] s8 S8() const noexcept;
    [[nodiscard]] u16 U16() const noexcept;
    [[nodiscard]] s16 S16() const noexcept;
    [[nodiscard]] u32 U32() const noexcept;
    [[nodiscard]] s32 S32() const noexcept;
    [[nodiscard]] f32 F32() const noexcept;
    [[nodiscard]] u64 U64() const noexcept;
    [[nodiscard]] s64 S64() const noexcept;
    [[nodiscard]] f64 F64() const noexcept;

    [[nodiscard]] std::tuple<u32, u32> U32x2() const noexcept;
    [[nodiscard]] std::tuple<u32, u32, u32> U32x3() const noexcept;
    [[nodiscard]] std::tuple<u32, u32, u32, u32> U32x4() const noexcept;
    [[nodiscard]] std::tuple<s32, s32> S32x2() const noexcept;
    [[nodiscard]] std::tuple<s32, s32, s32> S32x3() const noexcept;
    [[nodiscard]] std::tuple<s32, s32, s32, s32> S32x4() const noexcept;
    [[nodiscard]] std::tuple<f32, f32> F32x2() const noexcept;
    [[nodiscard]] std::tuple<f32, f32, f32> F32x3() const noexcept;
    [[nodiscard]] std::tuple<f32, f32, f32, f32> F32x4() const noexcept;
    [[nodiscard]] std::tuple<f64, f64> F64x2() const noexcept;
    [[nodiscard]] std::tuple<f64, f64, f64> F64x3() const noexcept;
    [[nodiscard]] std::tuple<f64, f64, f64, f64> F64x4() const noexcept;

    ImmValue& operator=(const ImmValue& value) noexcept = default;

    [[nodiscard]] bool operator==(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator!=(const ImmValue& other) const noexcept;

    [[nodiscard]] static ImmValue Extract(const ImmValue& vec, const ImmValue& index) noexcept;
    [[nodiscard]] static ImmValue Insert(const ImmValue& vec, const ImmValue& value,
                                         const ImmValue& index) noexcept;

    template <IR::Type NewType, bool NewSigned, IR::Type OldType, bool OldSigned>
    [[nodiscard]] static ImmValue Convert(const ImmValue& in) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Add(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Sub(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Mul(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Div(const ImmValue& a, const ImmValue& b);

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Mod(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue And(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Or(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Xor(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue LShift(const ImmValue& a, const ImmValue& shift) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue RShift(const ImmValue& a, const ImmValue& shift) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Not(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Neg(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Abs(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Recip(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Sqrt(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Rsqrt(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Sin(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Cos(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Exp2(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Ldexp(const ImmValue& in, const ImmValue& exp) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Log2(const ImmValue& in) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Min(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Max(const ImmValue& a, const ImmValue& b) noexcept;

    template <IR::Type Type, bool IsSigned>
    [[nodiscard]] static ImmValue Clamp(const ImmValue& in, const ImmValue& min,
                                        const ImmValue& max) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Floor(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Ceil(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Round(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Trunc(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Fract(const ImmValue& in) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static ImmValue Fma(const ImmValue& a, const ImmValue& b,
                                      const ImmValue& c) noexcept;

    template <IR::Type Type>
    [[nodiscard]] static bool IsNan(const ImmValue& in) noexcept;

    [[nodiscard]] static bool IsSupportedValue(const IR::Value& value) noexcept;

private:
    union Value {
        bool imm_u1;
        u8 imm_u8;
        s8 imm_s8;
        u16 imm_u16;
        s16 imm_s16;
        u32 imm_u32;
        s32 imm_s32;
        f32 imm_f32;
        u64 imm_u64;
        s64 imm_s64;
        f64 imm_f64;
    };

    std::array<Value, 4> imm_values;

    friend class std::hash<ImmValue>;
};
static_assert(std::is_trivially_copyable_v<ImmValue>);

inline bool ImmValue::U1() const noexcept {
    return imm_values[0].imm_u1;
}

inline u8 ImmValue::U8() const noexcept {
    return imm_values[0].imm_u8;
}

inline s8 ImmValue::S8() const noexcept {
    return imm_values[0].imm_s8;
}

inline u16 ImmValue::U16() const noexcept {
    return imm_values[0].imm_u16;
}

inline s16 ImmValue::S16() const noexcept {
    return imm_values[0].imm_s16;
}

inline u32 ImmValue::U32() const noexcept {
    return imm_values[0].imm_u32;
}

inline s32 ImmValue::S32() const noexcept {
    return imm_values[0].imm_s32;
}

inline f32 ImmValue::F32() const noexcept {
    return imm_values[0].imm_f32;
}

inline u64 ImmValue::U64() const noexcept {
    return imm_values[0].imm_u64;
}

inline s64 ImmValue::S64() const noexcept {
    return imm_values[0].imm_s64;
}

inline f64 ImmValue::F64() const noexcept {
    return imm_values[0].imm_f64;
}

inline std::tuple<u32, u32> ImmValue::U32x2() const noexcept {
    return {imm_values[0].imm_u32, imm_values[1].imm_u32};
}

inline std::tuple<u32, u32, u32> ImmValue::U32x3() const noexcept {
    return {imm_values[0].imm_u32, imm_values[1].imm_u32, imm_values[2].imm_u32};
}

inline std::tuple<u32, u32, u32, u32> ImmValue::U32x4() const noexcept {
    return {imm_values[0].imm_u32, imm_values[1].imm_u32, imm_values[2].imm_u32,
            imm_values[3].imm_u32};
}

inline std::tuple<s32, s32> ImmValue::S32x2() const noexcept {
    return {imm_values[0].imm_s32, imm_values[1].imm_s32};
}

inline std::tuple<s32, s32, s32> ImmValue::S32x3() const noexcept {
    return {imm_values[0].imm_s32, imm_values[1].imm_s32, imm_values[2].imm_s32};
}

inline std::tuple<s32, s32, s32, s32> ImmValue::S32x4() const noexcept {
    return {imm_values[0].imm_s32, imm_values[1].imm_s32, imm_values[2].imm_s32,
            imm_values[3].imm_s32};
}

inline std::tuple<f32, f32> ImmValue::F32x2() const noexcept {
    return {imm_values[0].imm_f32, imm_values[1].imm_f32};
}

inline std::tuple<f32, f32, f32> ImmValue::F32x3() const noexcept {
    return {imm_values[0].imm_f32, imm_values[1].imm_f32, imm_values[2].imm_f32};
}

inline std::tuple<f32, f32, f32, f32> ImmValue::F32x4() const noexcept {
    return {imm_values[0].imm_f32, imm_values[1].imm_f32, imm_values[2].imm_f32,
            imm_values[3].imm_f32};
}

inline std::tuple<f64, f64> ImmValue::F64x2() const noexcept {
    return {imm_values[0].imm_f64, imm_values[1].imm_f64};
}

inline std::tuple<f64, f64, f64> ImmValue::F64x3() const noexcept {
    return {imm_values[0].imm_f64, imm_values[1].imm_f64, imm_values[2].imm_f64};
}

inline std::tuple<f64, f64, f64, f64> ImmValue::F64x4() const noexcept {
    return {imm_values[0].imm_f64, imm_values[1].imm_f64, imm_values[2].imm_f64,
            imm_values[3].imm_f64};
}

} // namespace Shader::IR::ComputeValue

namespace std {
template <>
struct hash<Shader::IR::ComputeValue::ImmValue> {
    std::size_t operator()(const Shader::IR::ComputeValue::ImmValue& value) const;
};
} // namespace std