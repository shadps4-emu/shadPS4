// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <fmt/base.h>

#include "common/logging/classes.h"

namespace Common::Log {

enum class Level : std::uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off,
};
extern std::array<Level, NUM_LOG_CLASSES> g_class_levels;

void Setup(std::string_view shadps4_filename);
void Switch(std::string_view game_filename, bool append_log);
void Shutdown();
void Flush();
void Terminate();
void UpdateSinks();
void UpdateLogLevels(std::string_view log_filter);
void UpdateLogFlushLevel(std::string_view log_flush_level);

[[nodiscard]] inline bool ShouldLog(Class log_class, Level level) {
    return level >= g_class_levels[static_cast<std::size_t>(log_class)];
}

void VLog(Class log_class, Level level, const char* file, int line, const char* func,
          fmt::string_view format, fmt::format_args args);

template <typename... Args>
void Log(Class log_class, Level level, const char* file, int line, const char* func,
         fmt::format_string<Args...> format, Args&&... args) {
    VLog(log_class, level, file, line, func, format, fmt::make_format_args(args...));
}

} // namespace Common::Log

// Define the fmt lib macros
#define LOG_GENERIC_AT(log_class, log_level, func, ...)                                            \
    do {                                                                                           \
        if (Common::Log::ShouldLog(Common::Log::Class::log_class, log_level)) {                    \
            Common::Log::Log(Common::Log::Class::log_class, log_level, __FILE__, __LINE__, func,   \
                             __VA_ARGS__);                                                         \
        }                                                                                          \
    } while (false)

#define LOG_GENERIC(log_class, log_level, ...)                                                     \
    LOG_GENERIC_AT(log_class, log_level, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define LOG_TRACE(log_class, ...) (void(0))
#else
#define LOG_TRACE(log_class, ...) LOG_GENERIC(log_class, Common::Log::Level::Trace, __VA_ARGS__)
#endif

#define LOG_DEBUG(log_class, ...) LOG_GENERIC(log_class, Common::Log::Level::Debug, __VA_ARGS__)
#define LOG_INFO(log_class, ...) LOG_GENERIC(log_class, Common::Log::Level::Info, __VA_ARGS__)
#define LOG_WARNING(log_class, ...) LOG_GENERIC(log_class, Common::Log::Level::Warning, __VA_ARGS__)
#define LOG_ERROR(log_class, ...) LOG_GENERIC(log_class, Common::Log::Level::Error, __VA_ARGS__)
#define LOG_CRITICAL(log_class, ...)                                                               \
    LOG_GENERIC(log_class, Common::Log::Level::Critical, __VA_ARGS__)
