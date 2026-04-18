// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <unordered_map>
#include <vector>
#include <spdlog/details/fmt_helper.h>
#include <spdlog/sinks/async_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
using spdlog_stdout = spdlog::sinks::sink;
#else
using spdlog_stdout = spdlog::sinks::stdout_color_sink_mt;
#endif
#include <spdlog/spdlog.h>

#include "common/logging/classes.h"
#include "common/path_util.h"

namespace Common::Log {
extern bool g_should_append;
extern std::unordered_map<std::string_view, std::shared_ptr<spdlog::logger>> ALL_LOGGERS;

void Setup(std::string_view log_filename);

void Shutdown();

void Redirect(const std::string& name);
} // namespace Common::Log

// Define the fmt lib macros
#define LOG_GENERIC(log_class, log_level, ...)                                                     \
    SPDLOG_LOGGER_CALL(Common::Log::ALL_LOGGERS[log_class], log_level, __VA_ARGS__)

#ifdef _DEBUG
#define LOG_TRACE(log_class, ...)                                                                  \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::trace, __VA_ARGS__)
#else
#define LOG_TRACE(log_class, ...) (void(0))
#endif

#define LOG_DEBUG(log_class, ...)                                                                  \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(log_class, ...)                                                                   \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::info, __VA_ARGS__)
#define LOG_WARNING(log_class, ...)                                                                \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(log_class, ...)                                                                  \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::err, __VA_ARGS__)
#define LOG_CRITICAL(log_class, ...)                                                               \
    LOG_GENERIC(Common::Log::Class::log_class, spdlog::level::critical, __VA_ARGS__)
