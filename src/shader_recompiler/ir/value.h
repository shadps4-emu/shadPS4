// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bit>
#include <cstring>
#include <type_traits>
#include <utility>
#include <boost/container/list.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/intrusive/list.hpp>

#include "common/assert.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/opcodes.h"
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
    explicit Value(IR::Inst* value) noexcept;
    explicit Value(const IR::Inst* value) noexcept;
    explicit Value(IR::ScalarReg reg) noexcept;
    explicit Value(IR::VectorReg reg) noexcept;
    explicit Value(IR::Attribute value) noexcept;
    explicit Value(IR::Patch patch) noexcept;
    explicit Value(bool value) noexcept;
    explicit Value(u8 value) noexcept;
    explicit Value(u16 value) noexcept;
    explicit Value(u32 value) noexcept;
    explicit Value(f32 value) noexcept;
    explicit Value(u64 value) noexcept;
    explicit Value(f64 value) noexcept;
    explicit Value(const char* value) noexcept;

    [[nodiscard]] bool IsIdentity() const noexcept;
    [[nodiscard]] bool IsPhi() const noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] bool IsImmediate() const noexcept;
    [[nodiscard]] IR::Type Type() const noexcept;

    [[nodiscard]] IR::Inst* Inst() const;
    [[nodiscard]] IR::Inst* InstRecursive() const;
    [[nodiscard]] IR::Inst* TryInstRecursive() const;
    [[nodiscard]] IR::Value Resolve() const;
    [[nodiscard]] IR::ScalarReg ScalarReg() const;
    [[nodiscard]] IR::VectorReg VectorReg() const;
    [[nodiscard]] IR::Attribute Attribute() const;
    [[nodiscard]] IR::Patch Patch() const;
    [[nodiscard]] bool U1() const;
    [[nodiscard]] u8 U8() const;
    [[nodiscard]] u16 U16() const;
    [[nodiscard]] u32 U32() const;
    [[nodiscard]] f32 F32() const;
    [[nodiscard]] u64 U64() const;
    [[nodiscard]] f64 F64() const;
    [[nodiscard]] const char* StringLiteral() const;

    [[nodiscard]] bool operator==(const Value& other) const;
    [[nodiscard]] bool operator!=(const Value& other) const;

private:
    IR::Type type{};
    union {
        IR::Inst* inst{};
        IR::ScalarReg sreg;
        IR::VectorReg vreg;
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
    explicit(false) TypedValue(const TypedValue<other_type>& value) : Value(value) {}

    explicit TypedValue(const Value& value) : Value(value) {
        if ((value.Type() & type_) == IR::Type::Void) {
            UNREACHABLE_MSG("Incompatible types {} and {}", type_, value.Type());
        }
    }

    explicit TypedValue(IR::Inst* inst_) : TypedValue(Value(inst_)) {}
};

struct Use {
    Inst* user;
    u32 operand;

    Use() = default;
    Use(Inst* user_, u32 operand_) : user(user_), operand(operand_) {}
    Use(const Use&) = default;
    bool operator==(const Use&) const noexcept = default;
};

class Inst : public boost::intrusive::list_base_hook<> {
public:
    explicit Inst(IR::Opcode op_, u32 flags_) noexcept;
    explicit Inst(const Inst& base);
    ~Inst();

    Inst& operator=(const Inst&) = delete;

    Inst& operator=(Inst&&) = delete;
    Inst(Inst&&) = delete;

    IR::Block* GetParent() const {
        ASSERT(parent);
        return parent;
    }
    void SetParent(IR::Block* block) {
        parent = block;
    }

    /// Get the number of uses this instruction has.
    [[nodiscard]] int UseCount() const noexcept {
        return uses.size();
    }

    /// Determines whether this instruction has uses or not.
    [[nodiscard]] bool HasUses() const noexcept {
        return uses.size() > 0;
    }

    /// Get the opcode this microinstruction represents.
    [[nodiscard]] IR::Opcode GetOpcode() const noexcept {
        return op;
    }

    /// Determines whether or not this instruction may have side effects.
    [[nodiscard]] bool MayHaveSideEffects() const noexcept;

    /// Determines if all arguments of this instruction are immediates.
    [[nodiscard]] bool AreAllArgsImmediates() const;

    /// Get the type this instruction returns.
    [[nodiscard]] IR::Type Type() const;

    /// Get the number of arguments this instruction has.
    [[nodiscard]] size_t NumArgs() const {
        return op == IR::Opcode::Phi ? phi_args.size() : NumArgsOf(op);
    }

    /// Get the value of a given argument index.
    [[nodiscard]] Value Arg(size_t index) const noexcept {
        if (op == IR::Opcode::Phi) {
            return phi_args[index].second;
        } else {
            return args[index];
        }
    }

    /// Set the value of a given argument index.
    void SetArg(size_t index, Value value);

    /// Get a pointer to the block of a phi argument.
    [[nodiscard]] Block* PhiBlock(size_t index) const;
    /// Add phi operand to a phi instruction.
    void AddPhiOperand(Block* predecessor, const Value& value);

    void Invalidate();
    void ClearArgs();

    void ReplaceUsesWithAndRemove(Value replacement) {
        ReplaceUsesWith(replacement, false);
    }

    void ReplaceUsesWith(Value replacement) {
        ReplaceUsesWith(replacement, true);
    }

    void ReplaceOpcode(IR::Opcode opcode);

    template <typename FlagsType>
        requires(sizeof(FlagsType) <= sizeof(u32) && std::is_trivially_copyable_v<FlagsType>)
    [[nodiscard]] FlagsType Flags() const noexcept {
        FlagsType ret;
        std::memcpy(reinterpret_cast<char*>(&ret), &flags, sizeof(ret));
        return ret;
    }

    template <typename FlagsType>
        requires(sizeof(FlagsType) <= sizeof(u32) && std::is_trivially_copyable_v<FlagsType>)
    void SetFlags(FlagsType value) noexcept {
        std::memcpy(&flags, &value, sizeof(value));
    }

    /// Intrusively store the host definition of this instruction.
    template <typename DefinitionType>
    void SetDefinition(DefinitionType def) {
        definition = std::bit_cast<u32>(def);
    }

    /// Return the intrusively stored host definition of this instruction.
    template <typename DefinitionType>
    [[nodiscard]] DefinitionType Definition() const noexcept {
        return std::bit_cast<DefinitionType>(definition);
    }

    const auto Uses() const {
        return uses;
    }

private:
    struct NonTriviallyDummy {
        NonTriviallyDummy() noexcept {}
    };

    void Use(Inst* used, u32 operand);
    void UndoUse(Inst* used, u32 operand);
    void ReplaceUsesWith(Value replacement, bool preserve);

    IR::Opcode op{};
    u32 flags{};
    u32 definition{};
    IR::Block* parent{};
    union {
        NonTriviallyDummy dummy{};
        boost::container::small_vector<std::pair<Block*, Value>, 2> phi_args;
        std::array<Value, 6> args;
    };

    boost::container::list<IR::Use> uses;
};
static_assert(sizeof(Inst) <= 160, "Inst size unintentionally increased");

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

inline bool Value::IsIdentity() const noexcept {
    return type == Type::Opaque && inst->GetOpcode() == Opcode::Identity;
}

inline bool Value::IsPhi() const noexcept {
    return type == Type::Opaque && inst->GetOpcode() == Opcode::Phi;
}

inline bool Value::IsEmpty() const noexcept {
    return type == Type::Void;
}

inline bool Value::IsImmediate() const noexcept {
    IR::Type current_type{type};
    const IR::Inst* current_inst{inst};
    while (current_type == Type::Opaque && current_inst->GetOpcode() == Opcode::Identity) {
        const Value& arg{current_inst->Arg(0)};
        current_type = arg.type;
        current_inst = arg.inst;
    }
    return current_type != Type::Opaque;
}

inline IR::Inst* Value::Inst() const {
    DEBUG_ASSERT(type == Type::Opaque);
    return inst;
}

inline IR::Inst* Value::InstRecursive() const {
    DEBUG_ASSERT(type == Type::Opaque);
    if (IsIdentity()) {
        return inst->Arg(0).InstRecursive();
    }
    return inst;
}

inline IR::Inst* Value::TryInstRecursive() const {
    if (IsIdentity()) {
        return inst->Arg(0).TryInstRecursive();
    }
    return type == Type::Opaque ? inst : nullptr;
}

inline IR::Value Value::Resolve() const {
    if (IsIdentity()) {
        return inst->Arg(0).Resolve();
    }
    return *this;
}

inline IR::ScalarReg Value::ScalarReg() const {
    DEBUG_ASSERT(type == Type::ScalarReg);
    return sreg;
}

inline IR::VectorReg Value::VectorReg() const {
    DEBUG_ASSERT(type == Type::VectorReg);
    return vreg;
}

inline IR::Attribute Value::Attribute() const {
    DEBUG_ASSERT(type == Type::Attribute);
    return attribute;
}

inline IR::Patch Value::Patch() const {
    DEBUG_ASSERT(type == Type::Patch);
    return patch;
}

inline bool Value::U1() const {
    if (IsIdentity()) {
        return inst->Arg(0).U1();
    }
    DEBUG_ASSERT(type == Type::U1);
    return imm_u1;
}

inline u8 Value::U8() const {
    if (IsIdentity()) {
        return inst->Arg(0).U8();
    }
    DEBUG_ASSERT(type == Type::U8);
    return imm_u8;
}

inline u16 Value::U16() const {
    if (IsIdentity()) {
        return inst->Arg(0).U16();
    }
    DEBUG_ASSERT(type == Type::U16);
    return imm_u16;
}

inline u32 Value::U32() const {
    if (IsIdentity()) {
        return inst->Arg(0).U32();
    }
    DEBUG_ASSERT(type == Type::U32);
    return imm_u32;
}

inline f32 Value::F32() const {
    if (IsIdentity()) {
        return inst->Arg(0).F32();
    }
    DEBUG_ASSERT(type == Type::F32);
    return imm_f32;
}

inline u64 Value::U64() const {
    if (IsIdentity()) {
        return inst->Arg(0).U64();
    }
    DEBUG_ASSERT(type == Type::U64);
    return imm_u64;
}

inline f64 Value::F64() const {
    if (IsIdentity()) {
        return inst->Arg(0).F64();
    }
    DEBUG_ASSERT(type == Type::F64);
    return imm_f64;
}

inline const char* Value::StringLiteral() const {
    if (IsIdentity()) {
        return inst->Arg(0).StringLiteral();
    }
    DEBUG_ASSERT(type == Type::StringLiteral);
    return string_literal;
}

[[nodiscard]] inline bool IsPhi(const Inst& inst) {
    return inst.GetOpcode() == Opcode::Phi;
}

} // namespace Shader::IR

namespace std {
template <>
struct hash<Shader::IR::Value> {
    std::size_t operator()(const Shader::IR::Value& v) const;
};
} // namespace std
