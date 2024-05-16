// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

namespace detail {
template <class F>
class ScopeGuard {
private:
    F f;
    bool active;

public:
    constexpr ScopeGuard(F f_) : f(std::move(f_)), active(true) {}
    constexpr ~ScopeGuard() {
        if (active) {
            f();
        }
    }
    constexpr void Cancel() {
        active = false;
    }

    constexpr ScopeGuard(ScopeGuard&& rhs) : f(std::move(rhs.f)), active(rhs.active) {
        rhs.Cancel();
    }

    ScopeGuard& operator=(ScopeGuard&& rhs) = delete;
};

template <class F>
constexpr ScopeGuard<F> MakeScopeGuard(F f) {
    return ScopeGuard<F>(std::move(f));
}

enum class ScopeGuardOnExit {};

template <typename F>
constexpr ScopeGuard<F> operator+(ScopeGuardOnExit, F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

} // namespace detail

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __LINE__)
#endif

/**
 * This macro is similar to SCOPE_EXIT, except the object is caller managed. This is intended to be
 * used when the caller might want to cancel the ScopeExit.
 */
#define SCOPE_GUARD detail::ScopeGuardOnExit() + [&]()

/**
 * This macro allows you to conveniently specify a block of code that will run on scope exit. Handy
 * for doing ad-hoc clean-up tasks in a function with multiple returns.
 *
 * Example usage:
 * \code
 * const int saved_val = g_foo;
 * g_foo = 55;
 * SCOPE_EXIT{ g_foo = saved_val; };
 *
 * if (Bar()) {
 *     return 0;
 * } else {
 *     return 20;
 * }
 * \endcode
 */
#define SCOPE_EXIT auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_) = SCOPE_GUARD
