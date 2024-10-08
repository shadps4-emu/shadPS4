// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <string>
#include "shader_recompiler/ir/type.h"

namespace Shader::IR {

std::string NameOf(Type type) {
    static constexpr std::array names{
        "Opaque", "Label", "Reg",   "Pred",  "Attribute", "U1",    "U8",    "U16",          "U32",
        "U64",    "F16",   "F32",   "F64",   "U32x2",     "U32x3", "U32x4", "F16x2",        "F16x3",
        "F16x4",  "F32x2", "F32x3", "F32x4", "F64x2",     "F64x3", "F64x4", "StringLiteral"};
    const size_t bits{static_cast<size_t>(type)};
    if (bits == 0) {
        return "Void";
    }
    std::string result;
    for (size_t i = 0; i < names.size(); i++) {
        if ((bits & (size_t{1} << i)) != 0) {
            if (!result.empty()) {
                result += '|';
            }
            result += names[i];
        }
    }
    return result;
}

bool AreTypesCompatible(Type lhs, Type rhs) noexcept {
    return lhs == rhs || lhs == Type::Opaque || rhs == Type::Opaque;
}

} // namespace Shader::IR
