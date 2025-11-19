// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/format.h>
#include "common/types.h"

namespace Shader::IR {

enum class Condition : u32 {
    False,
    True,
    Scc0,
    Scc1,
    Vccz,
    Vccnz,
    Execz,
    Execnz,
};

constexpr std::string_view NameOf(Condition condition) {
    switch (condition) {
    case Condition::False:
        return "False";
    case Condition::True:
        return "True";
    case Condition::Scc0:
        return "Scc0";
    case Condition::Scc1:
        return "Scc1";
    case Condition::Vccz:
        return "Vccz";
    case Condition::Vccnz:
        return "Vccnz";
    case Condition::Execz:
        return "Execz";
    case Condition::Execnz:
        return "Execnz";
    default:
        UNREACHABLE();
    }
}

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::Condition> : formatter<std::string_view> {
    auto format(const Shader::IR::Condition cond, format_context& ctx) const {
        return formatter<string_view>::format(NameOf(cond), ctx);
    }
};
