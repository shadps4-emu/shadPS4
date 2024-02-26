// SPDX-FileCopyrightText: Copyright 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "common/logging/formatter.h"
#include "common/logging/types.h"

namespace Common::Log {

constexpr const char* TrimSourcePath(std::string_view source) {
    const auto rfind = [source](const std::string_view match) {
        return source.rfind(match) == source.npos ? 0 : (source.rfind(match) + match.size());
    };
    auto idx = std::max({rfind("/"), rfind("\\")});
    return source.data() + idx;
}

/// Logs a message to the global logger, using fmt
void FmtLogMessageImpl(Class log_class, Level log_level, const char* filename,
                       unsigned int line_num, const char* function, const char* format,
                       const fmt::format_args& args);

template <typename... Args>
void FmtLogMessage(Class log_class, Level log_level, const char* filename, unsigned int line_num,
                   const char* function, const char* format, const Args&... args) {
    FmtLogMessageImpl(log_class, log_level, filename, line_num, function, format,
                      fmt::make_format_args(args...));
}

} // namespace Common::Log

// Define the fmt lib macros
#define LOG_GENERIC(log_class, log_level, ...)                                                     \
    Common::Log::FmtLogMessage(log_class, log_level, Common::Log::TrimSourcePath(__FILE__),        \
                               __LINE__, __func__, __VA_ARGS__)

#ifdef _DEBUG
#define LOG_TRACE(log_class, ...)                                                                  \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Trace,           \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
#else
#define LOG_TRACE(log_class, fmt, ...) (void(0))
#endif

#define LOG_DEBUG(log_class, ...)                                                                  \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Debug,           \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
#define LOG_INFO(log_class, ...)                                                                   \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Info,            \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
#define LOG_WARNING(log_class, ...)                                                                \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Warning,         \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
#define LOG_ERROR(log_class, ...)                                                                  \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Error,           \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
#define LOG_CRITICAL(log_class, ...)                                                               \
    Common::Log::FmtLogMessage(Common::Log::Class::log_class, Common::Log::Level::Critical,        \
                               Common::Log::TrimSourcePath(__FILE__), __LINE__, __func__,          \
                               __VA_ARGS__)
