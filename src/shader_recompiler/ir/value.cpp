// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <string_view>
#include "common/hash.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

Value::Value(IR::Inst* value) noexcept : type{Type::Opaque}, inst{value} {}

Value::Value(IR::ScalarReg reg) noexcept : type{Type::ScalarReg}, sreg{reg} {}

Value::Value(IR::VectorReg reg) noexcept : type{Type::VectorReg}, vreg{reg} {}

Value::Value(IR::Attribute value) noexcept : type{Type::Attribute}, attribute{value} {}

Value::Value(bool value) noexcept : type{Type::U1}, imm_u1{value} {}

Value::Value(u8 value) noexcept : type{Type::U8}, imm_u8{value} {}

Value::Value(u16 value) noexcept : type{Type::U16}, imm_u16{value} {}

Value::Value(u32 value) noexcept : type{Type::U32}, imm_u32{value} {}

Value::Value(f32 value) noexcept : type{Type::F32}, imm_f32{value} {}

Value::Value(u64 value) noexcept : type{Type::U64}, imm_u64{value} {}

Value::Value(f64 value) noexcept : type{Type::F64}, imm_f64{value} {}

Value::Value(const char* value) noexcept : type{Type::StringLiteral}, string_literal{value} {}

IR::Type Value::Type() const noexcept {
    if (IsPhi()) {
        // The type of a phi node is stored in its flags
        return inst->Flags<IR::Type>();
    }
    if (IsIdentity()) {
        return inst->Arg(0).Type();
    }
    if (type == Type::Opaque) {
        return inst->Type();
    }
    return type;
}

bool Value::operator==(const Value& other) const {
    if (type != other.type) {
        return false;
    }
    switch (type) {
    case Type::Void:
        return true;
    case Type::Opaque:
        return inst == other.inst;
    case Type::ScalarReg:
        return sreg == other.sreg;
    case Type::VectorReg:
        return vreg == other.vreg;
    case Type::Attribute:
        return attribute == other.attribute;
    case Type::U1:
        return imm_u1 == other.imm_u1;
    case Type::U8:
        return imm_u8 == other.imm_u8;
    case Type::U16:
    case Type::F16:
        return imm_u16 == other.imm_u16;
    case Type::U32:
    case Type::F32:
        return imm_u32 == other.imm_u32;
    case Type::U64:
    case Type::F64:
        return imm_u64 == other.imm_u64;
    case Type::StringLiteral:
        return std::string_view(string_literal) == other.string_literal;
    case Type::U32x2:
    case Type::U32x3:
    case Type::U32x4:
    case Type::F16x2:
    case Type::F16x3:
    case Type::F16x4:
    case Type::F32x2:
    case Type::F32x3:
    case Type::F32x4:
    case Type::F64x2:
    case Type::F64x3:
    case Type::F64x4:
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid type {}", type);
}

bool Value::operator!=(const Value& other) const {
    return !operator==(other);
}

} // namespace Shader::IR

namespace std {
std::size_t hash<Shader::IR::Value>::operator()(const Shader::IR::Value& v) const {
    using namespace Shader::IR;

    u64 h = HashCombine(static_cast<u64>(v.type), 0ULL);

    switch (v.type) {
    case Type::Void:
        return h;
    case Type::Opaque:
        return reinterpret_cast<u64>(v.InstRecursive());
    case Type::ScalarReg:
        return HashCombine(static_cast<u64>(v.sreg), h);
    case Type::VectorReg:
        return HashCombine(static_cast<u64>(v.vreg), h);
    case Type::Attribute:
        return HashCombine(static_cast<u64>(v.attribute), h);
    case Type::U1:
        return HashCombine(static_cast<u64>(v.attribute), h);
    case Type::U8:
        return HashCombine(static_cast<u64>(v.imm_u8), h);
    case Type::U16:
    case Type::F16:
        return HashCombine(static_cast<u64>(v.imm_u16), h);
    case Type::U32:
    case Type::F32:
        return HashCombine(static_cast<u64>(v.imm_u32), h);
    case Type::U64:
    case Type::F64:
        return HashCombine(static_cast<u64>(v.imm_u64), h);
    case Type::U32x2:
    case Type::U32x3:
    case Type::U32x4:
    case Type::F16x2:
    case Type::F16x3:
    case Type::F16x4:
    case Type::F32x2:
    case Type::F32x3:
    case Type::F32x4:
    case Type::F64x2:
    case Type::F64x3:
    case Type::F64x4:
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid type {}", v.type);
}
} // namespace std
