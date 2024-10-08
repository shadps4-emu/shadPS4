// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <fmt/format.h>
#include "common/enum.h"

namespace Shader::IR {

enum class Type {
    Void = 0,
    Opaque = 1 << 0,
    ScalarReg = 1 << 1,
    VectorReg = 1 << 2,
    Attribute = 1 << 3,
    SystemValue = 1 << 4,
    U1 = 1 << 5,
    U8 = 1 << 6,
    U16 = 1 << 7,
    U32 = 1 << 8,
    U64 = 1 << 9,
    F16 = 1 << 10,
    F32 = 1 << 11,
    F64 = 1 << 12,
    U32x2 = 1 << 13,
    U32x3 = 1 << 14,
    U32x4 = 1 << 15,
    F16x2 = 1 << 16,
    F16x3 = 1 << 17,
    F16x4 = 1 << 18,
    F32x2 = 1 << 19,
    F32x3 = 1 << 20,
    F32x4 = 1 << 21,
    F64x2 = 1 << 22,
    F64x3 = 1 << 23,
    F64x4 = 1 << 24,
    StringLiteral = 1 << 25,
};
DECLARE_ENUM_FLAG_OPERATORS(Type)

[[nodiscard]] std::string NameOf(Type type);

[[nodiscard]] bool AreTypesCompatible(Type lhs, Type rhs) noexcept;

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::Type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(Shader::IR::Type type, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", Shader::IR::NameOf(type));
    }
};
