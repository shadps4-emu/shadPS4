// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <fmt/format.h>
#include "common/types.h"
#include "shader_recompiler/ir/type.h"

namespace Shader::IR {

enum class Opcode {
#define OPCODE(name, ...) name,
#include "opcodes.inc"
#undef OPCODE
};

namespace Detail {
struct OpcodeMeta {
    std::string_view name;
    Type type;
    std::array<Type, 5> arg_types;
};

// using enum Type;
constexpr Type Void{Type::Void};
constexpr Type Opaque{Type::Opaque};
constexpr Type ScalarReg{Type::ScalarReg};
constexpr Type VectorReg{Type::VectorReg};
constexpr Type Attribute{Type::Attribute};
constexpr Type SystemValue{Type::SystemValue};
constexpr Type U1{Type::U1};
constexpr Type U8{Type::U8};
constexpr Type U16{Type::U16};
constexpr Type U32{Type::U32};
constexpr Type U64{Type::U64};
constexpr Type F16{Type::F16};
constexpr Type F32{Type::F32};
constexpr Type F64{Type::F64};
constexpr Type U32x2{Type::U32x2};
constexpr Type U32x3{Type::U32x3};
constexpr Type U32x4{Type::U32x4};
constexpr Type F16x2{Type::F16x2};
constexpr Type F16x3{Type::F16x3};
constexpr Type F16x4{Type::F16x4};
constexpr Type F32x2{Type::F32x2};
constexpr Type F32x3{Type::F32x3};
constexpr Type F32x4{Type::F32x4};
constexpr Type F64x2{Type::F64x2};
constexpr Type F64x3{Type::F64x3};
constexpr Type F64x4{Type::F64x4};
constexpr Type StringLiteral{Type::StringLiteral};

constexpr OpcodeMeta META_TABLE[]{
#define OPCODE(name_token, type_token, ...)                                                        \
    {                                                                                              \
        .name{#name_token},                                                                        \
        .type = type_token,                                                                        \
        .arg_types{__VA_ARGS__},                                                                   \
    },
#include "opcodes.inc"
#undef OPCODE
};
constexpr size_t CalculateNumArgsOf(Opcode op) {
    const auto& arg_types{META_TABLE[static_cast<size_t>(op)].arg_types};
    return static_cast<size_t>(
        std::distance(arg_types.begin(), std::ranges::find(arg_types, Type::Void)));
}

constexpr u8 NUM_ARGS[]{
#define OPCODE(name_token, type_token, ...) static_cast<u8>(CalculateNumArgsOf(Opcode::name_token)),
#include "opcodes.inc"
#undef OPCODE
};
} // namespace Detail

/// Get return type of an opcode
[[nodiscard]] inline Type TypeOf(Opcode op) noexcept {
    return Detail::META_TABLE[static_cast<size_t>(op)].type;
}

/// Get the number of arguments an opcode accepts
[[nodiscard]] constexpr inline size_t NumArgsOf(Opcode op) noexcept {
    return static_cast<size_t>(Detail::NUM_ARGS[static_cast<size_t>(op)]);
}

/// Get the required type of an argument of an opcode
[[nodiscard]] inline Type ArgTypeOf(Opcode op, size_t arg_index) noexcept {
    return Detail::META_TABLE[static_cast<size_t>(op)].arg_types[arg_index];
}

/// Get the name of an opcode
[[nodiscard]] std::string_view NameOf(Opcode op);

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::Opcode> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const Shader::IR::Opcode op, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", Shader::IR::NameOf(op));
    }
};
