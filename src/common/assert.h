// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"

// Sometimes we want to try to continue even after hitting an assert.
// However touching this file yields a global recompilation as this header is included almost
// everywhere. So let's just move the handling of the failed assert to a single cpp file.

void assert_fail_impl();
[[noreturn]] void unreachable_impl();

#ifdef _MSC_VER
#define SHAD_NO_INLINE __declspec(noinline)
#else
#define SHAD_NO_INLINE __attribute__((cold, noinline))
#endif

namespace Common::Detail {

template <typename... Args>
SHAD_NO_INLINE void AssertFail(const char* file, int line, const char* func,
                               fmt::format_string<Args...> format, Args... args) {
    Common::Log::VLog(Common::Log::Class::Debug, Common::Log::Level::Critical, file, line, func,
                      format, fmt::make_format_args(args...));
    assert_fail_impl();
}

template <typename... Args>
[[noreturn]] SHAD_NO_INLINE void UnreachableFail(const char* file, int line, const char* func,
                                                 fmt::format_string<Args...> format, Args... args) {
    Common::Log::VLog(Common::Log::Class::Debug, Common::Log::Level::Critical, file, line, func,
                      format, fmt::make_format_args(args...));
    unreachable_impl();
}

} // namespace Common::Detail

#define ASSERT(_a_)                                                                                \
    do {                                                                                           \
        if (!(_a_)) [[unlikely]] {                                                                 \
            [&, shad_func_ = __func__]() SHAD_NO_INLINE {                                          \
                Common::Detail::AssertFail(__FILE__, __LINE__, __func__, "Assertion Failed!\n");   \
            }();                                                                                   \
        }                                                                                          \
    } while (false)

#define ASSERT_MSG(_a_, ...)                                                                       \
    do {                                                                                           \
        if (!(_a_)) [[unlikely]] {                                                                 \
            Common::Detail::AssertFail(__FILE__, __LINE__, __func__,                               \
                                       "Assertion Failed!\n" __VA_ARGS__);                         \
        }                                                                                          \
    } while (0)

#define UNREACHABLE()                                                                              \
    Common::Detail::UnreachableFail(__FILE__, __LINE__, __func__, "Unreachable code!\n")

#define UNREACHABLE_MSG(...)                                                                       \
    Common::Detail::UnreachableFail(__FILE__, __LINE__, __func__, "Unreachable code!\n" __VA_ARGS__)

#ifdef _DEBUG
#define DEBUG_ASSERT(_a_) ASSERT(_a_)
#define DEBUG_ASSERT_MSG(_a_, ...) ASSERT_MSG(_a_, __VA_ARGS__)
#else // not debug
#define DEBUG_ASSERT(_a_)                                                                          \
    do {                                                                                           \
    } while (0)
#define DEBUG_ASSERT_MSG(_a_, _desc_, ...)                                                         \
    do {                                                                                           \
    } while (0)
#endif

#define UNIMPLEMENTED() ASSERT_MSG(false, "Unimplemented code!")
#define UNIMPLEMENTED_MSG(...) ASSERT_MSG(false, __VA_ARGS__)

#define UNIMPLEMENTED_IF(cond) ASSERT_MSG(!(cond), "Unimplemented code!")
#define UNIMPLEMENTED_IF_MSG(cond, ...) ASSERT_MSG(!(cond), __VA_ARGS__)
