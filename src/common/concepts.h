// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <concepts>
#include <iterator>
#include <type_traits>

namespace Common {

// Check if type satisfies the ContiguousContainer named requirement.
template <typename T>
concept IsContiguousContainer = std::contiguous_iterator<typename T::iterator>;

template <typename Derived, typename Base>
concept DerivedFrom = std::derived_from<Derived, Base>;

// TODO: Replace with std::convertible_to when libc++ implements it.
template <typename From, typename To>
concept ConvertibleTo = std::is_convertible_v<From, To>;

// No equivalents in the stdlib

template <typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept IsIntegral = std::is_integral_v<T>;

} // namespace Common
