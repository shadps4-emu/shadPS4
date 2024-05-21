// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>
#include "common/types.h"

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

namespace Common {

template <typename T>
class Flags {
public:
    using IntType = std::underlying_type_t<T>;

    Flags() {}

    Flags(IntType t) : m_bits(t) {}

    template <typename... Tx>
    Flags(T f, Tx... fx) {
        this->set(f, fx...);
    }

    template <typename... Tx>
    void set(Tx... fx) {
        m_bits |= bits(fx...);
    }

    void set(Flags flags) {
        m_bits |= flags.m_bits;
    }

    template <typename... Tx>
    void clr(Tx... fx) {
        m_bits &= ~bits(fx...);
    }

    void clr(Flags flags) {
        m_bits &= ~flags.m_bits;
    }

    template <typename... Tx>
    bool any(Tx... fx) const {
        return (m_bits & bits(fx...)) != 0;
    }

    template <typename... Tx>
    bool all(Tx... fx) const {
        const IntType mask = bits(fx...);
        return (m_bits & mask) == mask;
    }

    bool test(T f) const {
        return this->any(f);
    }

    bool isClear() const {
        return m_bits == 0;
    }

    void clrAll() {
        m_bits = 0;
    }

    u32 raw() const {
        return m_bits;
    }

    Flags operator&(const Flags& other) const {
        return Flags(m_bits & other.m_bits);
    }

    Flags operator|(const Flags& other) const {
        return Flags(m_bits | other.m_bits);
    }

    Flags operator^(const Flags& other) const {
        return Flags(m_bits ^ other.m_bits);
    }

    bool operator==(const Flags& other) const {
        return m_bits == other.m_bits;
    }

    bool operator!=(const Flags& other) const {
        return m_bits != other.m_bits;
    }

private:
    IntType m_bits = 0;

    static IntType bit(T f) {
        return IntType(1) << static_cast<IntType>(f);
    }

    template <typename... Tx>
    static IntType bits(T f, Tx... fx) {
        return bit(f) | bits(fx...);
    }

    static IntType bits() {
        return 0;
    }
};

} // namespace Common
