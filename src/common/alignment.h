// SPDX-FileCopyrightText: 2014 Jannik Vogel <email@jannikvogel.de>
// SPDX-License-Identifier: CC0-1.0

#pragma once

#include <cstddef>
#include <type_traits>

namespace Common {

template <typename T>
[[nodiscard]] constexpr T AlignUp(T value, std::size_t size) {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    auto mod{static_cast<T>(value % size)};
    value -= mod;
    return static_cast<T>(mod == T{0} ? value : value + size);
}

template <typename T>
[[nodiscard]] constexpr T AlignDown(T value, std::size_t size) {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
    return static_cast<T>(value - value % size);
}

template <typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr bool Is16KBAligned(T value) {
    return (value & 0x3FFF) == 0;
}

} // namespace Common
