// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <fmt/format.h>
#include "common/types.h"
#include "video_core/amdgpu/types.h"

namespace AmdGpu {

[[nodiscard]] constexpr bool IsInteger(NumberFormat nfmt) {
    return nfmt == AmdGpu::NumberFormat::Sint || nfmt == AmdGpu::NumberFormat::Uint;
}

[[nodiscard]] std::string_view NameOf(DataFormat fmt);
[[nodiscard]] std::string_view NameOf(NumberFormat fmt);

int NumComponents(DataFormat format);
int NumBits(DataFormat format);
u32 ComponentBits(DataFormat format, u32 comp);
s32 ComponentOffset(DataFormat format, u32 comp);

} // namespace AmdGpu

template <>
struct fmt::formatter<AmdGpu::DataFormat> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::DataFormat fmt, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(fmt));
    }
};

template <>
struct fmt::formatter<AmdGpu::NumberFormat> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::NumberFormat fmt, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(fmt));
    }
};
