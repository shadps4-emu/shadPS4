// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

#define DECLARE_ENUM_FLAG_OPERATORS(type)                                                          \
    [[nodiscard]] constexpr type operator|(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) | static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator&(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) & static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator^(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) ^ static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator<<(type a, type b) noexcept {                             \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) << static_cast<T>(b));                          \
    }                                                                                              \
    [[nodiscard]] constexpr type operator>>(type a, type b) noexcept {                             \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) >> static_cast<T>(b));                          \
    }                                                                                              \
    constexpr type& operator|=(type& a, type b) noexcept {                                         \
        a = a | b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator&=(type& a, type b) noexcept {                                         \
        a = a & b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator^=(type& a, type b) noexcept {                                         \
        a = a ^ b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator<<=(type& a, type b) noexcept {                                        \
        a = a << b;                                                                                \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator>>=(type& a, type b) noexcept {                                        \
        a = a >> b;                                                                                \
        return a;                                                                                  \
    }                                                                                              \
    [[nodiscard]] constexpr type operator~(type key) noexcept {                                    \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(~static_cast<T>(key));                                            \
    }                                                                                              \
    [[nodiscard]] constexpr bool True(type key) noexcept {                                         \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) != 0;                                                           \
    }                                                                                              \
    [[nodiscard]] constexpr bool False(type key) noexcept {                                        \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) == 0;                                                           \
    }
