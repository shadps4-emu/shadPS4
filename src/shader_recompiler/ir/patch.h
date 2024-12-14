// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/format.h>
#include "common/types.h"

namespace Shader::IR {

enum class Patch : u64 {
    TessellationLodLeft,
    TessellationLodTop,
    TessellationLodRight,
    TessellationLodBottom,
    TessellationLodInteriorU,
    TessellationLodInteriorV,
    Component0,
    Component1,
    Component2,
    Component3,
    Component4,
    Component5,
    Component6,
    Component7,
    Component8,
    Component9,
    Component10,
    Component11,
    Component12,
    Component13,
    Component14,
    Component15,
    Component16,
    Component17,
    Component18,
    Component19,
    Component20,
    Component21,
    Component22,
    Component23,
    Component24,
    Component25,
    Component26,
    Component27,
    Component28,
    Component29,
    Component30,
    Component31,
    Component32,
    Component33,
    Component34,
    Component35,
    Component36,
    Component37,
    Component38,
    Component39,
    Component40,
    Component41,
    Component42,
    Component43,
    Component44,
    Component45,
    Component46,
    Component47,
    Component48,
    Component49,
    Component50,
    Component51,
    Component52,
    Component53,
    Component54,
    Component55,
    Component56,
    Component57,
    Component58,
    Component59,
    Component60,
    Component61,
    Component62,
    Component63,
    Component64,
    Component65,
    Component66,
    Component67,
    Component68,
    Component69,
    Component70,
    Component71,
    Component72,
    Component73,
    Component74,
    Component75,
    Component76,
    Component77,
    Component78,
    Component79,
    Component80,
    Component81,
    Component82,
    Component83,
    Component84,
    Component85,
    Component86,
    Component87,
    Component88,
    Component89,
    Component90,
    Component91,
    Component92,
    Component93,
    Component94,
    Component95,
    Component96,
    Component97,
    Component98,
    Component99,
    Component100,
    Component101,
    Component102,
    Component103,
    Component104,
    Component105,
    Component106,
    Component107,
    Component108,
    Component109,
    Component110,
    Component111,
    Component112,
    Component113,
    Component114,
    Component115,
    Component116,
    Component117,
    Component118,
    Component119,
};
static_assert(static_cast<u64>(Patch::Component119) == 125);

constexpr bool IsGeneric(Patch patch) noexcept {
    return patch >= Patch::Component0 && patch <= Patch::Component119;
}

constexpr Patch PatchFactor(u32 index) {
    return static_cast<Patch>(index);
}

constexpr Patch PatchGeneric(u32 index) {
    return static_cast<Patch>(static_cast<u32>(Patch::Component0) + index);
}

constexpr u32 GenericPatchIndex(Patch patch) {
    return (static_cast<u32>(patch) - static_cast<u32>(Patch::Component0)) / 4;
}

constexpr u32 GenericPatchElement(Patch patch) {
    return (static_cast<u32>(patch) - static_cast<u32>(Patch::Component0)) % 4;
}

[[nodiscard]] std::string NameOf(Patch patch);

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::Patch> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Shader::IR::Patch patch, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", Shader::IR::NameOf(patch));
    }
};
