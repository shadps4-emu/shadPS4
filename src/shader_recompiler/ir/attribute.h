// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/format.h>
#include "common/assert.h"
#include "common/types.h"

namespace Shader::IR {

enum class Attribute : u64 {
    // Export targets
    RenderTarget0 = 0,
    RenderTarget1 = 1,
    RenderTarget2 = 2,
    RenderTarget3 = 3,
    RenderTarget4 = 4,
    RenderTarget5 = 5,
    RenderTarget6 = 6,
    RenderTarget7 = 7,
    Depth = 8,
    Null = 9,
    Position0 = 12,
    Position1 = 13,
    Position2 = 14,
    Position3 = 15,
    Param0 = 32,
    Param1 = 33,
    Param2 = 34,
    Param3 = 35,
    Param4 = 36,
    Param5 = 37,
    Param6 = 38,
    Param7 = 39,
    Param8 = 40,
    Param9 = 41,
    Param10 = 42,
    Param11 = 43,
    Param12 = 44,
    Param13 = 45,
    Param14 = 46,
    Param15 = 47,
    Param16 = 48,
    Param17 = 49,
    Param18 = 50,
    Param19 = 51,
    Param20 = 52,
    Param21 = 53,
    Param22 = 54,
    Param23 = 55,
    Param24 = 56,
    Param25 = 57,
    Param26 = 58,
    Param27 = 59,
    Param28 = 60,
    Param29 = 61,
    Param30 = 62,
    Param31 = 63,
    // System values
    ClipDistance = 64,
    CullDistance = 65,
    RenderTargetId = 66,
    ViewportId = 67,
    VertexId = 68,
    PrimitiveId = 69,
    InstanceId = 70,
    IsFrontFace = 71,
    SampleIndex = 72,
    GlobalInvocationId = 73,
    WorkgroupId = 74,
    WorkgroupIndex = 75,
    LocalInvocationId = 76,
    LocalInvocationIndex = 77,
    FragCoord = 78,
    BaryCoordNoPersp = 79,
    BaryCoordNoPerspCentroid = 80,
    BaryCoordNoPerspSample = 81,
    BaryCoordSmooth = 82,
    BaryCoordSmoothCentroid = 83,
    BaryCoordSmoothSample = 84,
    BaryCoordPullModel = 85,
    InvocationId = 86, // TCS id in output patch and instanced geometry shader id
    PatchVertices = 87,
    TessellationEvaluationPointU = 88,
    TessellationEvaluationPointV = 89,
    PackedHullInvocationInfo = 90, // contains patch id within the VGT and invocation ID
    Max,
};

constexpr size_t NumAttributes = static_cast<size_t>(Attribute::Max);
constexpr size_t NumRenderTargets = 8;
constexpr size_t NumParams = 32;

constexpr bool IsPosition(Attribute attribute) noexcept {
    return attribute >= Attribute::Position0 && attribute <= Attribute::Position3;
}

constexpr bool IsTessCoord(Attribute attribute) noexcept {
    return attribute >= Attribute::TessellationEvaluationPointU &&
           attribute <= Attribute::TessellationEvaluationPointV;
}

constexpr bool IsParam(Attribute attribute) noexcept {
    return attribute >= Attribute::Param0 && attribute <= Attribute::Param31;
}

constexpr bool IsMrt(Attribute attribute) noexcept {
    return attribute >= Attribute::RenderTarget0 && attribute <= Attribute::RenderTarget7;
}

constexpr bool IsBarycentricCoord(Attribute attribute) noexcept {
    return attribute >= Attribute::BaryCoordSmooth && attribute <= Attribute::BaryCoordSmoothSample;
}

[[nodiscard]] std::string NameOf(Attribute attribute);

[[nodiscard]] constexpr Attribute operator+(Attribute attr, int num) {
    const int result{static_cast<int>(attr) + num};
    if (result > static_cast<int>(Attribute::Param31)) {
        UNREACHABLE_MSG("Overflow on register arithmetic");
    }
    if (result < static_cast<int>(Attribute::RenderTarget0)) {
        UNREACHABLE_MSG("Underflow on register arithmetic");
    }
    return static_cast<Attribute>(result);
}

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::Attribute> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Shader::IR::Attribute attribute, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", Shader::IR::NameOf(attribute));
    }
};
