// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * (c) 2014-2016 Alexandro Sanchez Bach. All rights reserved.
 * Released under GPL v2 license. Read LICENSE for more details.
 * Some modifications for using with shadps4 by georgemoralis
 */

#pragma once

#include <bit>
#include <concepts>
#include "common/types.h"

namespace Common {

/**
 * Native endianness
 */
template <typename T>
using NativeEndian = T;

template <std::integral T>
class SwappedEndian {
public:
    const T& Raw() const {
        return data;
    }

    T Swap() const {
        return std::byteswap(data);
    }

    void FromRaw(const T& value) {
        data = value;
    }

    void FromSwap(const T& value) {
        data = std::byteswap(value);
    }

    operator const T() const {
        return Swap();
    }

    template <typename T1>
    explicit operator const SwappedEndian<T1>() const {
        SwappedEndian<T1> res;
        if (sizeof(T1) < sizeof(T)) {
            res.FromRaw(Raw() >> ((sizeof(T) - sizeof(T1)) * 8));
        } else if (sizeof(T1) > sizeof(T)) {
            res.FromSwap(Swap());
        } else {
            res.FromRaw(Raw());
        }
        return res;
    }

    SwappedEndian<T>& operator=(const T& right) {
        FromSwap(right);
        return *this;
    }
    SwappedEndian<T>& operator=(const SwappedEndian<T>& right) = default;

    template <typename T1>
    SwappedEndian<T>& operator+=(T1 right) {
        return *this = T(*this) + right;
    }
    template <typename T1>
    SwappedEndian<T>& operator-=(T1 right) {
        return *this = T(*this) - right;
    }
    template <typename T1>
    SwappedEndian<T>& operator*=(T1 right) {
        return *this = T(*this) * right;
    }
    template <typename T1>
    SwappedEndian<T>& operator/=(T1 right) {
        return *this = T(*this) / right;
    }
    template <typename T1>
    SwappedEndian<T>& operator%=(T1 right) {
        return *this = T(*this) % right;
    }
    template <typename T1>
    SwappedEndian<T>& operator&=(T1 right) {
        return *this = T(*this) & right;
    }
    template <typename T1>
    SwappedEndian<T>& operator|=(T1 right) {
        return *this = T(*this) | right;
    }
    template <typename T1>
    SwappedEndian<T>& operator^=(T1 right) {
        return *this = T(*this) ^ right;
    }
    template <typename T1>
    SwappedEndian<T>& operator<<=(T1 right) {
        return *this = T(*this) << right;
    }
    template <typename T1>
    SwappedEndian<T>& operator>>=(T1 right) {
        return *this = T(*this) >> right;
    }

    template <typename T1>
    SwappedEndian<T>& operator+=(const SwappedEndian<T1>& right) {
        return *this = Swap() + right.Swap();
    }
    template <typename T1>
    SwappedEndian<T>& operator-=(const SwappedEndian<T1>& right) {
        return *this = Swap() - right.Swap();
    }
    template <typename T1>
    SwappedEndian<T>& operator*=(const SwappedEndian<T1>& right) {
        return *this = Swap() * right.Swap();
    }
    template <typename T1>
    SwappedEndian<T>& operator/=(const SwappedEndian<T1>& right) {
        return *this = Swap() / right.Swap();
    }
    template <typename T1>
    SwappedEndian<T>& operator%=(const SwappedEndian<T1>& right) {
        return *this = Swap() % right.Swap();
    }
    template <typename T1>
    SwappedEndian<T>& operator&=(const SwappedEndian<T1>& right) {
        return *this = Raw() & right.Raw();
    }
    template <typename T1>
    SwappedEndian<T>& operator|=(const SwappedEndian<T1>& right) {
        return *this = Raw() | right.Raw();
    }
    template <typename T1>
    SwappedEndian<T>& operator^=(const SwappedEndian<T1>& right) {
        return *this = Raw() ^ right.Raw();
    }

    template <typename T1>
    SwappedEndian<T> operator&(const SwappedEndian<T1>& right) const {
        return SwappedEndian<T>{Raw() & right.Raw()};
    }
    template <typename T1>
    SwappedEndian<T> operator|(const SwappedEndian<T1>& right) const {
        return SwappedEndian<T>{Raw() | right.Raw()};
    }
    template <typename T1>
    SwappedEndian<T> operator^(const SwappedEndian<T1>& right) const {
        return SwappedEndian<T>{Raw() ^ right.Raw()};
    }

    template <typename T1>
    bool operator==(T1 right) const {
        return (T1)Swap() == right;
    }
    template <typename T1>
    bool operator!=(T1 right) const {
        return !(*this == right);
    }
    template <typename T1>
    bool operator>(T1 right) const {
        return (T1)Swap() > right;
    }
    template <typename T1>
    bool operator<(T1 right) const {
        return (T1)Swap() < right;
    }
    template <typename T1>
    bool operator>=(T1 right) const {
        return (T1)Swap() >= right;
    }
    template <typename T1>
    bool operator<=(T1 right) const {
        return (T1)Swap() <= right;
    }

    template <typename T1>
    bool operator==(const SwappedEndian<T1>& right) const {
        return Raw() == right.Raw();
    }
    template <typename T1>
    bool operator!=(const SwappedEndian<T1>& right) const {
        return !(*this == right);
    }
    template <typename T1>
    bool operator>(const SwappedEndian<T1>& right) const {
        return (T1)Swap() > right.Swap();
    }
    template <typename T1>
    bool operator<(const SwappedEndian<T1>& right) const {
        return (T1)Swap() < right.Swap();
    }
    template <typename T1>
    bool operator>=(const SwappedEndian<T1>& right) const {
        return (T1)Swap() >= right.Swap();
    }
    template <typename T1>
    bool operator<=(const SwappedEndian<T1>& right) const {
        return (T1)Swap() <= right.Swap();
    }

    SwappedEndian<T> operator++(int) {
        SwappedEndian<T> res = *this;
        *this += 1;
        return res;
    }
    SwappedEndian<T> operator--(int) {
        SwappedEndian<T> res = *this;
        *this -= 1;
        return res;
    }
    SwappedEndian<T>& operator++() {
        *this += 1;
        return *this;
    }
    SwappedEndian<T>& operator--() {
        *this -= 1;
        return *this;
    }

private:
    T data;
};

template <typename T>
using LittleEndian = std::conditional_t<std::endian::native == std::endian::little, NativeEndian<T>,
                                        SwappedEndian<T>>;

template <typename T>
using BigEndian =
    std::conditional_t<std::endian::native == std::endian::big, NativeEndian<T>, SwappedEndian<T>>;

} // namespace Common

using u16_be = Common::BigEndian<u16>;
using u32_be = Common::BigEndian<u32>;
using u64_be = Common::BigEndian<u64>;

using u16_le = Common::LittleEndian<u16>;
using u32_le = Common::LittleEndian<u32>;
using u64_le = Common::LittleEndian<u64>;
