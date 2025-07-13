// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <tuple>

namespace Common {

template <class Func, class Enable = void>
struct FuncTraits;

// Function type
template <class ReturnType_, class... Args>
struct FuncTraits<ReturnType_(Args...), void> {
    using ReturnType = ReturnType_;
    static constexpr size_t NUM_ARGS = sizeof...(Args);

    template <size_t I>
    using ArgType = std::tuple_element_t<I, std::tuple<Args...>>;
};

// Function pointer
template <class ReturnType_, class... Args>
struct FuncTraits<ReturnType_ (*)(Args...), void> : FuncTraits<ReturnType_(Args...)> {};

// Member function pointer
template <class ClassType, class ReturnType_, class... Args>
struct FuncTraits<ReturnType_ (ClassType::*)(Args...), void> : FuncTraits<ReturnType_(Args...)> {};

template <class ClassType, class ReturnType_, class... Args>
struct FuncTraits<ReturnType_ (ClassType::*)(Args...) const, void>
    : FuncTraits<ReturnType_(Args...)> {};

// Catch-all for callables
template <class Func>
struct FuncTraits<Func, std::void_t<decltype(&std::remove_reference_t<Func>::operator())>>
    : FuncTraits<decltype(&std::remove_reference_t<Func>::operator())> {};


// For lambdas: for compat (may be removed)
template <typename Func>
struct LambdaTraits : LambdaTraits<decltype(&std::remove_reference_t<Func>::operator())> {};

template <typename ReturnType, typename LambdaType, typename... Args>
struct LambdaTraits<ReturnType (LambdaType::*)(Args...) const> {
    template <size_t I>
    using ArgType = std::tuple_element_t<I, std::tuple<Args...>>;

    static constexpr size_t NUM_ARGS{sizeof...(Args)};
};

} // namespace Common
