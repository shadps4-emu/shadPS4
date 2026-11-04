// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <iostream>
#include <string>
#include <fmt/std.h>
#include <spdlog/sinks/async_sink.h>
#include <spdlog/sinks/dup_filter_sink.h>

#include <spdlog/details/fmt_helper.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
using spdlog_stdout = spdlog::sinks::sink;
#else
using spdlog_stdout = spdlog::sinks::stdout_color_sink_mt;
#endif

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/logging/log_file_sink.h"
#include "common/path_util.h"
#include "common/thread.h"
#include "common/types.h"
#include "core/emulator_settings.h"

namespace Common::Log {

static std::shared_ptr<spdlog_stdout> g_console_sink;
static std::shared_ptr<LogFileSink> g_shad_file_sink;
static std::array<std::unique_ptr<spdlog::logger>, NUM_LOG_CLASSES> ALL_LOGGERS{};

std::array<Level, NUM_LOG_CLASSES> g_class_levels{};

static spdlog::level ToSpdlog(Level l) {
    return static_cast<spdlog::level>(l);
}

static Level FromSpdlog(spdlog::level l) {
    return static_cast<Level>(l);
}

[[nodiscard]] static constexpr std::string_view NameOf(spdlog::level lvl) noexcept {
    static constexpr std::array level_string_views{"Trace", "Debug",    "Info", "Warning",
                                                   "Error", "Critical", "Off"};
    return level_string_views[level_to_number(lvl)];
}

void VLog(Class log_class, Level level, const char* file, int line, const char* func,
          fmt::string_view format, fmt::format_args args) {
    const auto& logger = ALL_LOGGERS[static_cast<size_t>(log_class)];
    if (!logger) {
        return;
    }
    fmt::memory_buffer msg;
    fmt::vformat_to(fmt::appender(msg), format, args);
    const std::string_view fn = std::string_view(func) == "operator()" ? "lambda" : func;
    logger->log(ToSpdlog(level), "[{}] <{}> ({}) {}:{} {}: {}", NameOf(log_class),
                NameOf(ToSpdlog(level)), Common::GetCurrentThreadName(),
                spdlog::source_loc::basename(file), line, fn,
                std::string_view(msg.data(), msg.size()));
}

template <typename T>
static auto UpdateColorLevels(T sink) {
#ifdef _WIN32
    using LogColor = std::uint16_t;

    const auto Grey = FOREGROUND_INTENSITY;
    const auto Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE;
    const auto Bright_gray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const auto Bright_yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const auto Bright_red = FOREGROUND_RED | FOREGROUND_INTENSITY;
    const auto Bright_magenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#else
    using LogColor = std::string_view;

#define ESC "\x1b"
    const auto Grey = ESC "[1;30m";
    const auto Cyan = ESC "[0;36m";
    const auto Bright_gray = ESC "[0;37m";
    const auto Bright_yellow = ESC "[1;33m";
    const auto Bright_red = ESC "[1;31m";
    const auto Bright_magenta = ESC "[1;35m";
#undef ESC
#endif

    const std::unordered_map<spdlog::level, LogColor> colors{
        {spdlog::level::trace, Grey},       {spdlog::level::debug, Cyan},
        {spdlog::level::info, Bright_gray}, {spdlog::level::warn, Bright_yellow},
        {spdlog::level::err, Bright_red},   {spdlog::level::critical, Bright_magenta}};

    for (const auto& [level, color] : colors) {
        sink->set_color(level, color);
    }

    return sink;
}

void Setup(std::string_view shadps4_filename) {
    static std::once_flag already_registered;

    std::call_once(already_registered, []() {
        std::atexit(Shutdown);
        std::at_quick_exit(Flush);
    });

    for (u32 i = 0; i < ALL_LOGGERS.size(); ++i) {
        const auto log_class = static_cast<Class>(i);
        auto& logger = ALL_LOGGERS[i];
        logger = std::make_unique<spdlog::logger>(std::string(NameOf(log_class)));
        logger->set_level(spdlog::level::trace);
    }

    // Setup console

#ifdef _WIN32
    if (EmulatorSettings.GetLogType() == "wincolor") {
        g_console_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
    } else {
        g_console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    }

#else
    g_console_sink = UpdateColorLevels(std::make_shared<spdlog_stdout>(spdlog::color_mode::always));
#endif

    g_console_sink->set_pattern("%^%v%$");

    // Setup file

    g_shad_file_sink = std::make_shared<LogFileSink>(
        (GetUserPath(Common::FS::PathType::LogDir) / shadps4_filename).string(), false,
        EmulatorSettings.GetLogSizeLimit());
    g_shad_file_sink->set_pattern("%^%v%$");

    UpdateSinks();
}

void Switch(std::string_view game_filename, bool append_log) {
    UpdateSinks();
    UpdateLogLevels(EmulatorSettings.GetLogFilter());
    UpdateLogFlushLevel(EmulatorSettings.GetLogFlushLevel());

    g_shad_file_sink->_size_limit = EmulatorSettings.GetLogSizeLimit();
    g_shad_file_sink->session_file_helper_.open(
        (GetUserPath(Common::FS::PathType::LogDir) / game_filename).string(),
        !(append_log || EmulatorSettings.IsLogAppend()));
}

void Shutdown() {
    for (auto& logger : ALL_LOGGERS) {
        logger.reset();
    }

    g_shad_file_sink.reset();
    g_console_sink.reset();
}

void Flush() {
    if (g_shad_file_sink != nullptr) {
        g_shad_file_sink->flush();
    }

    if (g_console_sink != nullptr) {
        g_console_sink->flush();
    }
}

void UpdateSinks() {
    std::initializer_list<spdlog::sink_ptr> sinks{g_console_sink, g_shad_file_sink};

    std::initializer_list<spdlog::sink_ptr> async_sink{std::make_shared<spdlog::sinks::async_sink>(
        spdlog::sinks::async_sink::config{.sinks = sinks})};

    std::initializer_list<spdlog::sink_ptr> dup_filter{
        std::make_shared<spdlog::sinks::dup_filter_sink_mt>(
            std::chrono::milliseconds(EmulatorSettings.GetLogMaxSkipDuration()),
            EmulatorSettings.IsLogSync() ? sinks : async_sink)};

    for (auto& logger : ALL_LOGGERS) {
        logger->sinks() = EmulatorSettings.IsLogSkipDuplicate()
                              ? dup_filter
                              : (EmulatorSettings.IsLogSync() ? sinks : async_sink);
    }
}

void UpdateLogLevels(std::string_view log_filter) {
    spdlog::level default_log_level = spdlog::level::info;
    std::unordered_map<std::string, spdlog::level> log_level_per_class;

    if (EmulatorSettings.IsLogEnable()) {
        for (const auto class_level : std::views::split(log_filter, ' ')) {
            const auto class_level_pair =
                std::views::split(class_level, ':') | std::ranges::to<std::vector<std::string>>();

            if (class_level_pair.size() != 2) {
                LOG_ERROR(Config, "bad log filter provided");
                continue;
            }

            if (class_level_pair.front()[0] == '*') {
                default_log_level = spdlog::level_from_str(class_level_pair.back() |
                                                           std::ranges::to<std::string>());
            } else {
                log_level_per_class[class_level_pair.front() | std::ranges::to<std::string>()] =
                    spdlog::level_from_str(class_level_pair.back() |
                                           std::ranges::to<std::string>());
            }
        }
    }

    for (u32 i = 0; i < ALL_LOGGERS.size(); ++i) {
        const auto log_class = static_cast<Class>(i);
        auto& logger = ALL_LOGGERS[i];
        if (EmulatorSettings.IsLogEnable()) {
            const auto level_it = log_level_per_class.find(std::string(NameOf(log_class)));
            const auto log_level =
                level_it != log_level_per_class.end() ? level_it->second : default_log_level;
            logger->set_level(log_level);
            g_class_levels[i] = FromSpdlog(log_level);
        } else {
            logger->set_level(spdlog::level::off);
            g_class_levels[i] = Level::Off;
        }
    }
}

void UpdateLogFlushLevel(std::string_view log_flush_level) {
    if (!log_flush_level.empty()) {
        for (auto& logger : ALL_LOGGERS) {
            logger->flush_on(spdlog::level_from_str(log_flush_level.data()));
        }
    }
}

} // namespace Common::Log
