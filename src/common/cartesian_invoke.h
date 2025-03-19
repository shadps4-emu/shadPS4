// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tuple>

namespace Common {
namespace Detail {

template <typename Func, typename OutputIt, std::size_t N, std::size_t Level, typename... ArgLists>
void CartesianInvokeImpl(Func func, OutputIt out_it,
                         std::tuple<typename ArgLists::const_iterator...>& arglists_its,
                         const std::tuple<const ArgLists&...>& arglists_tuple) {
    if constexpr (Level == N) {
        auto get_tuple = [&]<std::size_t... I>(std::index_sequence<I...>) {
            return std::forward_as_tuple(*std::get<I>(arglists_its)...);
        };
        *out_it++ = std::move(std::apply(func, get_tuple(std::make_index_sequence<N>{})));
        return;
    } else {
        const auto& arglist = std::get<Level>(arglists_tuple);
        for (auto it = arglist.begin(); it != arglist.end(); ++it) {
            std::get<Level>(arglists_its) = it;
            CartesianInvokeImpl<Func, OutputIt, N, Level + 1, ArgLists...>(
                func, out_it, arglists_its, arglists_tuple);
        }
    }
}

} // namespace Detail

template <typename Func, typename OutputIt, typename... ArgLists>
void CartesianInvoke(Func func, OutputIt out_it, const ArgLists&... arg_lists) {
    constexpr std::size_t N = sizeof...(ArgLists);
    const std::tuple<const ArgLists&...> arglists_tuple = std::forward_as_tuple(arg_lists...);

    std::tuple<typename ArgLists::const_iterator...> arglists_it;
    Detail::CartesianInvokeImpl<Func, OutputIt, N, 0, ArgLists...>(func, out_it, arglists_it,
                                                                   arglists_tuple);
}

} // namespace Common
