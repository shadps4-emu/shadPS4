// SPDX-FileCopyrightText: Copyright 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>
#include <fmt/format.h>

// Adapted from https://github.com/fmtlib/fmt/issues/2704
// a generic formatter for enum classes
#if FMT_VERSION >= 80100
template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_enum_v<T>, char>>
    : formatter<std::underlying_type_t<T>> {
    template <typename FormatContext>
    auto format(const T& value, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::formatter<std::underlying_type_t<T>>::format(
            static_cast<std::underlying_type_t<T>>(value), ctx);
    }
};
#endif

namespace fmt {
template <typename T = std::string_view>
struct UTF {
    T data;

    explicit UTF(const std::u8string_view view) {
        data = view.empty() ? T{} : T{(const char*)&view.front(), (const char*)&view.back() + 1};
    }

    explicit UTF(const std::u8string& str) : UTF(std::u8string_view{str}) {}
};
} // namespace fmt

template <>
struct fmt::formatter<fmt::UTF<std::string_view>, char> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const UTF<std::string_view>& wrapper, FormatContext& ctx) const {
        return formatter<std::string_view>::format(wrapper.data, ctx);
    }
};