// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <fmt/format.h>
#include "common/types.h"

namespace AmdGpu {

enum class FpRoundMode : u32 {
    NearestEven = 0,
    PlusInf = 1,
    MinInf = 2,
    ToZero = 3,
};

enum class FpDenormMode : u32 {
    InOutFlush = 0,
    InAllowOutFlush = 1,
    InFlushOutAllow = 2,
    InOutAllow = 3,
};

enum class TessellationType : u32 {
    Isoline = 0,
    Triangle = 1,
    Quad = 2,
};

constexpr std::string_view NameOf(TessellationType type) {
    switch (type) {
    case TessellationType::Isoline:
        return "Isoline";
    case TessellationType::Triangle:
        return "Triangle";
    case TessellationType::Quad:
        return "Quad";
    default:
        return "Unknown";
    }
}

enum class TessellationPartitioning : u32 {
    Integer = 0,
    Pow2 = 1,
    FracOdd = 2,
    FracEven = 3,
};

constexpr std::string_view NameOf(TessellationPartitioning partitioning) {
    switch (partitioning) {
    case TessellationPartitioning::Integer:
        return "Integer";
    case TessellationPartitioning::Pow2:
        return "Pow2";
    case TessellationPartitioning::FracOdd:
        return "FracOdd";
    case TessellationPartitioning::FracEven:
        return "FracEven";
    default:
        return "Unknown";
    }
}

enum class TessellationTopology : u32 {
    Point = 0,
    Line = 1,
    TriangleCw = 2,
    TriangleCcw = 3,
};

constexpr std::string_view NameOf(TessellationTopology topology) {
    switch (topology) {
    case TessellationTopology::Point:
        return "Point";
    case TessellationTopology::Line:
        return "Line";
    case TessellationTopology::TriangleCw:
        return "TriangleCw";
    case TessellationTopology::TriangleCcw:
        return "TriangleCcw";
    default:
        return "Unknown";
    }
}

// See `VGT_PRIMITIVE_TYPE` description in [Radeon Sea Islands 3D/Compute Register Reference Guide]
enum class PrimitiveType : u32 {
    None = 0,
    PointList = 1,
    LineList = 2,
    LineStrip = 3,
    TriangleList = 4,
    TriangleFan = 5,
    TriangleStrip = 6,
    PatchPrimitive = 9,
    AdjLineList = 10,
    AdjLineStrip = 11,
    AdjTriangleList = 12,
    AdjTriangleStrip = 13,
    RectList = 17,
    LineLoop = 18,
    QuadList = 19,
    QuadStrip = 20,
    Polygon = 21,
};

enum class GsOutputPrimitiveType : u32 {
    PointList = 0,
    LineStrip = 1,
    TriangleStrip = 2,
};

} // namespace AmdGpu

template <>
struct fmt::formatter<AmdGpu::TessellationType> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::TessellationType type, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(type));
    }
};

template <>
struct fmt::formatter<AmdGpu::TessellationPartitioning> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::TessellationPartitioning type, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(type));
    }
};

template <>
struct fmt::formatter<AmdGpu::TessellationTopology> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::TessellationTopology type, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(type));
    }
};
