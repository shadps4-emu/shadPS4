// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <utility>

#include "common/assert.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/patch.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/type.h"

namespace Shader::IR {

class Block;
class Inst;

struct AssociatedInsts;

class Value {
public:
    Value() noexcept = default;
    explicit Value(IR::Inst* value) noexcept : type{Type::Opaque}, inst{value} {}
    explicit Value(IR::ScalarReg reg) noexcept : type{Type::ScalarReg}, sreg{reg} {}
    explicit Value(IR::VectorReg reg) noexcept : type{Type::VectorReg}, vreg{reg} {}
    explicit Value(IR::VirtualReg reg_) noexcept : type{Type::VirtualReg}, reg{reg_} {}
    explicit Value(IR::Attribute value) noexcept : type{Type::Attribute}, attribute{value} {}
    explicit Value(IR::Patch patch) noexcept : type{Type::Patch}, patch{patch} {}
    explicit Value(bool value) noexcept : type{Type::U1}, imm_u1{value} {}
    explicit Value(u8 value) noexcept : type{Type::U8}, imm_u8{value} {}
    explicit Value(u16 value) noexcept : type{Type::U16}, imm_u16{value} {}
    explicit Value(u32 value) noexcept : type{Type::U32}, imm_u32{value} {}
    explicit Value(f32 value) noexcept : type{Type::F32}, imm_f32{value} {}
    explicit Value(u64 value) noexcept : type{Type::U64}, imm_u64{value} {}
    explicit Value(f64 value) noexcept : type{Type::F64}, imm_f64{value} {}
    explicit Value(const char* value) noexcept : type{Type::StringLiteral}, string_literal{value} {}

    bool IsPhi() const noexcept;
    IR::Type Type() const noexcept;

    inline bool IsEmpty() const noexcept {
        return type == Type::Void;
    }

    inline bool IsImmediate() const noexcept {
        return type != Type::Opaque;
    }

    inline IR::Inst* Inst() const {
        DEBUG_ASSERT(type == Type::Opaque);
        return inst;
    }

    inline IR::Inst* TryInst() const {
        return type == Type::Opaque ? inst : nullptr;
    }

    inline IR::ScalarReg ScalarReg() const {
        DEBUG_ASSERT(type == Type::ScalarReg);
        return sreg;
    }

    inline IR::VectorReg VectorReg() const {
        DEBUG_ASSERT(type == Type::VectorReg);
        return vreg;
    }

    inline IR::VirtualReg VirtualReg() const {
        DEBUG_ASSERT(type == Type::VirtualReg);
        return reg;
    }

    inline IR::Attribute Attribute() const {
        DEBUG_ASSERT(type == Type::Attribute);
        return attribute;
    }

    inline IR::Patch Patch() const {
        DEBUG_ASSERT(type == Type::Patch);
        return patch;
    }

    inline bool U1() const {
        DEBUG_ASSERT(type == Type::U1);
        return imm_u1;
    }

    inline u8 U8() const {
        DEBUG_ASSERT(type == Type::U8);
        return imm_u8;
    }

    inline u16 U16() const {
        DEBUG_ASSERT(type == Type::U16);
        return imm_u16;
    }

    inline u32 U32() const {
        DEBUG_ASSERT(type == Type::U32);
        return imm_u32;
    }

    inline f32 F32() const {
        DEBUG_ASSERT(type == Type::F32);
        return imm_f32;
    }

    inline u64 U64() const {
        DEBUG_ASSERT(type == Type::U64);
        return imm_u64;
    }

    inline f64 F64() const {
        DEBUG_ASSERT(type == Type::F64);
        return imm_f64;
    }

    inline const char* StringLiteral() const {
        DEBUG_ASSERT(type == Type::StringLiteral);
        return string_literal;
    }

    [[nodiscard]] bool operator==(const Value& other) const;
    [[nodiscard]] bool operator!=(const Value& other) const;

private:
    IR::Type type{};
    union {
        IR::Inst* inst{};
        IR::ScalarReg sreg;
        IR::VectorReg vreg;
        IR::VirtualReg reg;
        IR::Attribute attribute;
        IR::Patch patch;
        bool imm_u1;
        u8 imm_u8;
        u16 imm_u16;
        u32 imm_u32;
        f32 imm_f32;
        u64 imm_u64;
        f64 imm_f64;
        const char* string_literal;
    };

    friend class std::hash<Value>;
};
static_assert(static_cast<u32>(IR::Type::Void) == 0, "memset relies on IR::Type being zero");
static_assert(std::is_trivially_copyable_v<Value>);

template <IR::Type type_>
class TypedValue : public Value {
public:
    TypedValue() = default;

    template <IR::Type other_type>
        requires((other_type & type_) != IR::Type::Void)
    TypedValue(const TypedValue<other_type>& value) : Value(value) {}

    explicit TypedValue(const Value& value) : Value(value) {
        ASSERT_MSG((value.Type() & type_) != IR::Type::Void, "Incompatible types {} and {}", type_,
                   value.Type());
    }

    explicit TypedValue(IR::Inst* inst_) : TypedValue(Value(inst_)) {}
};

using U1 = TypedValue<Type::U1>;
using U8 = TypedValue<Type::U8>;
using U16 = TypedValue<Type::U16>;
using U32 = TypedValue<Type::U32>;
using U64 = TypedValue<Type::U64>;
using F16 = TypedValue<Type::F16>;
using F32 = TypedValue<Type::F32>;
using F64 = TypedValue<Type::F64>;
using U32F32 = TypedValue<Type::U32 | Type::F32>;
using U64F64 = TypedValue<Type::U64 | Type::F64>;
using U32U64 = TypedValue<Type::U32 | Type::U64>;
using U16U32U64 = TypedValue<Type::U16 | Type::U32 | Type::U64>;
using U8U16U32U64 = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64>;
using F32F64 = TypedValue<Type::F32 | Type::F64>;
using F16F32F64 = TypedValue<Type::F16 | Type::F32 | Type::F64>;
using UAny = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64>;

} // namespace Shader::IR

namespace std {
template <>
struct hash<Shader::IR::Value> {
    std::size_t operator()(const Shader::IR::Value& v) const;
};
} // namespace std
