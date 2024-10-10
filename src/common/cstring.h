// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

#include "assert.h"

namespace Common {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-undefined-compare"

/**
 * @brief A null-terminated string with a fixed maximum length
 *        This class is not meant to be used as a general-purpose string class
 *        It is meant to be used as `char[N]` where memory layout is fixed
 * @tparam N Maximum length of the string
 * @tparam T Type of character
 */
template <size_t N, typename T = char>
class CString {
    T data[N]{};

public:
    class Iterator;

    CString() = default;

    template <size_t M>
    explicit CString(const CString<M>& other)
        requires(M <= N)
    {
        if (this == nullptr) {
            return;
        }
        std::ranges::copy(other.begin(), other.end(), data);
    }

    void FromString(const std::basic_string_view<T>& str) {
        if (this == nullptr) {
            return;
        }
        size_t p = str.copy(data, N - 1);
        data[p] = '\0';
    }

    void Zero() {
        if (this == nullptr) {
            return;
        }
        std::ranges::fill(data, 0);
    }

    explicit(false) operator std::basic_string_view<T>() const {
        if (this == nullptr) {
            return {};
        }
        return std::basic_string_view<T>{data};
    }

    explicit operator std::basic_string<T>() const {
        if (this == nullptr) {
            return {};
        }
        return std::basic_string<T>{data};
    }

    std::basic_string<T> to_string() const {
        if (this == nullptr) {
            return {};
        }
        return std::basic_string<T>{data};
    }

    std::basic_string_view<T> to_view() const {
        if (this == nullptr) {
            return {};
        }
        return std::basic_string_view<T>{data};
    }

    T* begin() {
        if (this == nullptr) {
            return nullptr;
        }
        return data;
    }

    const T* begin() const {
        if (this == nullptr) {
            return nullptr;
        }
        return data;
    }

    T* end() {
        if (this == nullptr) {
            return nullptr;
        }
        return data + N;
    }

    const T* end() const {
        if (this == nullptr) {
            return nullptr;
        }
        return data + N;
    }

    constexpr std::size_t capacity() const {
        return N;
    }

    std::size_t size() const {
        return std::char_traits<T>::length(data);
    }

    T& operator[](size_t idx) {
        return data[idx];
    }

    const T& operator[](size_t idx) const {
        return data[idx];
    }

    class Iterator {
        T* ptr;
        T* end;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        Iterator() = default;
        explicit Iterator(T* ptr) : ptr(ptr), end(ptr + N) {}

        Iterator& operator++() {
            ++ptr;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++ptr;
            return tmp;
        }

        operator T*() {
            ASSERT_MSG(ptr >= end, "CString iterator out of bounds");
            return ptr;
        }
    };
};

static_assert(sizeof(CString<13>) == sizeof(char[13])); // Ensure size still matches a simple array
static_assert(std::weakly_incrementable<CString<13>::Iterator>);

template <size_t N>
using CWString = CString<N, wchar_t>;

template <size_t N>
using CU16String = CString<N, char16_t>;

#pragma clang diagnostic pop

} // namespace Common