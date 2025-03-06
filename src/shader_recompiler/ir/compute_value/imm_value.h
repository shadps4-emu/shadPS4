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

namespace Shader::IR {

// Live IR::Value but can only hold immediate values. Additionally, can hold vectors of values.
// Has arithmetic operations defined for it. Usefull for computing a value at shader compile time.

class ImmValue {
public:
    ImmValue() noexcept = default;
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
    ImmValue(f64 value1, f64 value2) noexcept;
    ImmValue(f64 value1, f64 value2, f64 value3) noexcept;
    ImmValue(f64 value1, f64 value2, f64 value3, f64 value4) noexcept;

    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] IR::Type Type() const noexcept;
    [[nodiscard]] IR::Type BaseType() const noexcept;
    [[nodiscard]] u32 Dimensions() const noexcept;

    [[nodiscard]] bool IsSigned() const noexcept;
    void SetSigned(bool signed_) noexcept;
    void SameSignAs(const ImmValue& other) noexcept;

    [[nodiscard]] bool U1() const;
    [[nodiscard]] u8 U8() const;
    [[nodiscard]] s8 S8() const;
    [[nodiscard]] u16 U16() const;
    [[nodiscard]] s16 S16() const;
    [[nodiscard]] u32 U32() const;
    [[nodiscard]] s32 S32() const;
    [[nodiscard]] f32 F32() const;
    [[nodiscard]] u64 U64() const;
    [[nodiscard]] s64 S64() const;
    [[nodiscard]] f64 F64() const;

    [[nodiscard]] std::tuple<u32, u32> U32x2() const;
    [[nodiscard]] std::tuple<u32, u32, u32> U32x3() const;
    [[nodiscard]] std::tuple<u32, u32, u32, u32> U32x4() const;
    [[nodiscard]] std::tuple<s32, s32> S32x2() const;
    [[nodiscard]] std::tuple<s32, s32, s32> S32x3() const;
    [[nodiscard]] std::tuple<s32, s32, s32, s32> S32x4() const;
    [[nodiscard]] std::tuple<f32, f32> F32x2() const;
    [[nodiscard]] std::tuple<f32, f32, f32> F32x3() const;
    [[nodiscard]] std::tuple<f32, f32, f32, f32> F32x4() const;
    [[nodiscard]] std::tuple<f64, f64> F64x2() const;
    [[nodiscard]] std::tuple<f64, f64, f64> F64x3() const;
    [[nodiscard]] std::tuple<f64, f64, f64, f64> F64x4() const;

    [[nodiscard]] bool operator==(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator!=(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator<(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator>(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator<=(const ImmValue& other) const noexcept;
    [[nodiscard]] bool operator>=(const ImmValue& other) const noexcept;

    [[nodiscard]] ImmValue operator+(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator-(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator*(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator/(const ImmValue& other) const;
    [[nodiscard]] ImmValue operator%(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator&(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator|(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator^(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator<<(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator>>(const ImmValue& other) const noexcept;
    [[nodiscard]] ImmValue operator~() const noexcept;

    [[nodiscard]] ImmValue operator++(int) noexcept;
    [[nodiscard]] ImmValue operator--(int) noexcept;

    ImmValue& operator++() noexcept;
    ImmValue& operator--() noexcept;

    [[nodiscard]] ImmValue operator-() const noexcept;
    [[nodiscard]] ImmValue operator+() const noexcept;

    ImmValue& operator+=(const ImmValue& other) noexcept;
    ImmValue& operator-=(const ImmValue& other) noexcept;
    ImmValue& operator*=(const ImmValue& other) noexcept;
    ImmValue& operator/=(const ImmValue& other);
    ImmValue& operator%=(const ImmValue& other) noexcept;
    ImmValue& operator&=(const ImmValue& other) noexcept;
    ImmValue& operator|=(const ImmValue& other) noexcept;
    ImmValue& operator^=(const ImmValue& other) noexcept;
    ImmValue& operator<<=(const ImmValue& other) noexcept;
    ImmValue& operator>>=(const ImmValue& other) noexcept;

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

    IR::Type type{};
    bool is_signed{};
    std::array<Value, 4> imm_values;

    friend class std::hash<ImmValue>;
};
static_assert(std::is_trivially_copyable_v<ImmValue>);

template <IR::Type type_, bool is_signed_>
class TypedImmValue : public ImmValue {
public:
    inline static constexpr IR::Type static_type = type_;
    inline static constexpr bool static_is_signed = is_signed_;

    TypedImmValue() = default;

    template <IR::Type other_type, bool other_signed>
        requires((other_type & type_) != IR::Type::Void && other_signed == is_signed_)
    explicit(false) TypedImmValue(const TypedImmValue<other_type, other_signed>& other)
        : ImmValue(other) {}

    explicit TypedImmValue(const ImmValue& value) : ImmValue(value) {
        if ((value.Type() & type_) == IR::Type::Void && value.IsSigned() == is_signed_) {
            throw InvalidArgument("Incompatible types {} {} and {} {}",
                                  is_signed_ ? "signed" : "unsigned", type_, value.Type(),
                                  value.IsSigned() ? "signed" : "unsigned");
        }
    }
};

using ImmU1 = TypedImmValue<Type::U1, false>;
using ImmU8 = TypedImmValue<Type::U8, false>;
using ImmS8 = TypedImmValue<Type::U8, true>;
using ImmU16 = TypedImmValue<Type::U16, false>;
using ImmS16 = TypedImmValue<Type::U16, true>;
using ImmU32 = TypedImmValue<Type::U32, false>;
using ImmS32 = TypedImmValue<Type::U32, true>;
using ImmF32 = TypedImmValue<Type::F32, true>;
using ImmU64 = TypedImmValue<Type::U64, false>;
using ImmS64 = TypedImmValue<Type::U64, true>;
using ImmF64 = TypedImmValue<Type::F64, true>;
using ImmS32F32 = TypedImmValue<Type::U32 | Type::F32, true>;
using ImmS64F64 = TypedImmValue<Type::U64 | Type::F64, true>;
using ImmU32U64 = TypedImmValue<Type::U32 | Type::U64, false>;
using ImmS32S64 = TypedImmValue<Type::U32 | Type::U64, true>;
using ImmU16U32U64 = TypedImmValue<Type::U16 | Type::U32 | Type::U64, false>;
using ImmS16S32S64 = TypedImmValue<Type::U16 | Type::U32 | Type::U64, true>;
using ImmF32F64 = TypedImmValue<Type::F32 | Type::F64, true>;
using ImmUAny = TypedImmValue<Type::U1 | Type::U8 | Type::U16 | Type::U32 | Type::U64, false>;
using ImmSAny = TypedImmValue<Type::U8 | Type::U16 | Type::U32 | Type::U64, true>;
using ImmU32x2 = TypedImmValue<Type::U32x2, false>;
using ImmU32x3 = TypedImmValue<Type::U32x3, false>;
using ImmU32x4 = TypedImmValue<Type::U32x4, false>;
using ImmS32x2 = TypedImmValue<Type::U32x2, true>;
using ImmS32x3 = TypedImmValue<Type::U32x3, true>;
using ImmS32x4 = TypedImmValue<Type::U32x4, true>;
using ImmF32x2 = TypedImmValue<Type::F32x2, true>;
using ImmF32x3 = TypedImmValue<Type::F32x3, true>;
using ImmF32x4 = TypedImmValue<Type::F32x4, true>;
using ImmF64x2 = TypedImmValue<Type::F64x2, true>;
using ImmF64x3 = TypedImmValue<Type::F64x3, true>;
using ImmF64x4 = TypedImmValue<Type::F64x4, true>;
using ImmS32F32x2 = TypedImmValue<Type::U32x2 | Type::F32x2, true>;
using ImmS32F32x3 = TypedImmValue<Type::U32x3 | Type::F32x3, true>;
using ImmS32F32x4 = TypedImmValue<Type::U32x4 | Type::F32x4, true>;
using ImmF32F64x2 = TypedImmValue<Type::F32x2 | Type::F64x2, true>;
using ImmF32F64x3 = TypedImmValue<Type::F32x3 | Type::F64x3, true>;
using ImmF32F64x4 = TypedImmValue<Type::F32x4 | Type::F64x4, true>;
using ImmU32xAny = TypedImmValue<Type::U32 | Type::U32x2 | Type::U32x3 | Type::U32x4, false>;
using ImmS32xAny = TypedImmValue<Type::U32 | Type::U32x2 | Type::U32x3 | Type::U32x4, true>;
using ImmF32xAny = TypedImmValue<Type::F32 | Type::F32x2 | Type::F32x3 | Type::F32x4, true>;
using ImmF64xAny = TypedImmValue<Type::F64 | Type::F64x2 | Type::F64x3 | Type::F64x4, true>;
using ImmS32F32xAny = TypedImmValue<ImmS32F32::static_type | ImmS32F32x2::static_type |
                                        ImmS32F32x3::static_type | ImmS32F32x4::static_type,
                                    true>;
using ImmF32F64xAny = TypedImmValue<ImmF32F64::static_type | ImmF32F64x2::static_type |
                                        ImmF32F64x3::static_type | ImmF32F64x4::static_type,
                                    true>;

inline bool ImmValue::IsEmpty() const noexcept {
    return type == Type::Void;
}

inline IR::Type ImmValue::Type() const noexcept {
    return type;
}

inline bool ImmValue::U1() const {
    ASSERT(type == Type::U1 && !is_signed);
    return imm_values[0].imm_u1;
}

inline u8 ImmValue::U8() const {
    ASSERT(type == Type::U8 && !is_signed);
    return imm_values[0].imm_u8;
}

inline s8 ImmValue::S8() const {
    ASSERT(type == Type::U8 && is_signed);
    return imm_values[0].imm_s8;
}

inline u16 ImmValue::U16() const {
    ASSERT(type == Type::U16 && !is_signed);
    return imm_values[0].imm_u16;
}

inline s16 ImmValue::S16() const {
    ASSERT(type == Type::U16 && is_signed);
    return imm_values[0].imm_s16;
}

inline u32 ImmValue::U32() const {
    ASSERT(type == Type::U32 && !is_signed);
    return imm_values[0].imm_u32;
}

inline s32 ImmValue::S32() const {
    ASSERT(type == Type::U32 && is_signed);
    return imm_values[0].imm_s32;
}

inline f32 ImmValue::F32() const {
    ASSERT(type == Type::F32 && is_signed);
    return imm_values[0].imm_f32;
}

inline u64 ImmValue::U64() const {
    ASSERT(type == Type::U64 && !is_signed);
    return imm_values[0].imm_u64;
}

inline s64 ImmValue::S64() const {
    ASSERT(type == Type::U64 && is_signed);
    return imm_values[0].imm_s64;
}

inline f64 ImmValue::F64() const {
    ASSERT(type == Type::F64 && is_signed);
    return imm_values[0].imm_f64;
}

inline std::tuple<u32, u32> ImmValue::U32x2() const {
    ASSERT(type == Type::U32x2 && !is_signed);
    return {imm_values[0].imm_u32, imm_values[1].imm_u32};
}

inline std::tuple<u32, u32, u32> ImmValue::U32x3() const {
    ASSERT(type == Type::U32x3 && !is_signed);
    return {imm_values[0].imm_u32, imm_values[1].imm_u32, imm_values[2].imm_u32};
}

inline std::tuple<u32, u32, u32, u32> ImmValue::U32x4() const {
    ASSERT(type == Type::U32x4 && !is_signed);
    return {imm_values[0].imm_u32, imm_values[1].imm_u32, imm_values[2].imm_u32,
            imm_values[3].imm_u32};
}

inline std::tuple<s32, s32> ImmValue::S32x2() const {
    ASSERT(type == Type::U32x2 && is_signed);
    return {imm_values[0].imm_s32, imm_values[1].imm_s32};
}

inline std::tuple<s32, s32, s32> ImmValue::S32x3() const {
    ASSERT(type == Type::U32x3 && is_signed);
    return {imm_values[0].imm_s32, imm_values[1].imm_s32, imm_values[2].imm_s32};
}

inline std::tuple<s32, s32, s32, s32> ImmValue::S32x4() const {
    ASSERT(type == Type::U32x4 && is_signed);
    return {imm_values[0].imm_s32, imm_values[1].imm_s32, imm_values[2].imm_s32,
            imm_values[3].imm_s32};
}

inline std::tuple<f32, f32> ImmValue::F32x2() const {
    ASSERT(type == Type::F32x2 && is_signed);
    return {imm_values[0].imm_f32, imm_values[1].imm_f32};
}

inline std::tuple<f32, f32, f32> ImmValue::F32x3() const {
    ASSERT(type == Type::F32x3 && is_signed);
    return {imm_values[0].imm_f32, imm_values[1].imm_f32, imm_values[2].imm_f32};
}

inline std::tuple<f32, f32, f32, f32> ImmValue::F32x4() const {
    ASSERT(type == Type::F32x4 && is_signed);
    return {imm_values[0].imm_f32, imm_values[1].imm_f32, imm_values[2].imm_f32,
            imm_values[3].imm_f32};
}

inline std::tuple<f64, f64> ImmValue::F64x2() const {
    ASSERT(type == Type::F64x2 && is_signed);
    return {imm_values[0].imm_f64, imm_values[1].imm_f64};
}

inline std::tuple<f64, f64, f64> ImmValue::F64x3() const {
    ASSERT(type == Type::F64x3 && is_signed);
    return {imm_values[0].imm_f64, imm_values[1].imm_f64, imm_values[2].imm_f64};
}

inline std::tuple<f64, f64, f64, f64> ImmValue::F64x4() const {
    ASSERT(type == Type::F64x4 && is_signed);
    return {imm_values[0].imm_f64, imm_values[1].imm_f64, imm_values[2].imm_f64,
            imm_values[3].imm_f64};
}

} // namespace Shader::IR

namespace std {
template <>
struct hash<Shader::IR::ImmValue> {
    std::size_t operator()(const Shader::IR::ImmValue& value) const;
};
} // namespace std