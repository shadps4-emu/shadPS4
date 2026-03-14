// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <vector>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/cfg/argv.h>
#include <spdlog/cfg/env.h>
#include <spdlog/details/fmt_helper.h>
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

#include "common/config.h"
#include "common/logging/classes.h"
#include "common/path_util.h"
#include "common/thread.h"
#include "common/types.h"

namespace Common::Log {
static constexpr unsigned long long UNLIMITED_SIZE = 0;

struct thread_name_formatter : spdlog::formatter {
    ~thread_name_formatter() override = default;

    thread_name_formatter(unsigned long long size_limit) : _size_limit(size_limit) {}

    void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
        if (_size_limit != UNLIMITED_SIZE && _current_size >= _size_limit) {
            return;
        }

        msg.color_range_start = dest.size();

        dest.push_back('[');
        spdlog::details::fmt_helper::append_string_view(msg.logger_name, dest);
        dest.push_back(']');
        dest.push_back(' ');
        dest.push_back('<');
        spdlog::details::fmt_helper::append_string_view(spdlog::level::to_string_view(msg.level),
                                                        dest);
        dest.push_back('>');
        dest.push_back(' ');
        dest.push_back('(');
        spdlog::details::fmt_helper::append_string_view(GetCurrentThreadName(), dest);
        dest.push_back(')');
        dest.push_back(' ');
        spdlog::details::fmt_helper::append_string_view(
            std::filesystem::path(msg.source.filename).filename().string(), dest);
        dest.push_back(':');
        spdlog::details::fmt_helper::append_int(msg.source.line, dest);
        dest.push_back(' ');
        spdlog::details::fmt_helper::append_string_view(msg.source.funcname, dest);
        dest.push_back(':');
        dest.push_back(' ');
        spdlog::details::fmt_helper::append_string_view(msg.payload, dest);
        spdlog::details::fmt_helper::append_string_view(spdlog::details::os::default_eol, dest);

        msg.color_range_end = dest.size();

        _current_size += dest.size();
    }

    std::unique_ptr<formatter> clone() const override {
        return std::make_unique<thread_name_formatter>(_size_limit);
    }

    const unsigned long long _size_limit;
    unsigned long long _current_size = 0;
};

static constexpr auto POSITON_CONSOLE_LOG = 0;
static constexpr auto POSITON_SHAD_LOG = 1;
static constexpr auto POSITON_GAME_LOG = 2;

static constexpr auto POSITON_DUPLICATE_SINK = 0;

extern std::shared_ptr<spdlog_stdout> g_console_sink;
extern std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_shad_file_sink;
extern std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_game_file_sink;
extern bool g_should_append;

static void Shutdown() {
    g_game_file_sink.reset();
    g_shad_file_sink.reset();
    g_console_sink.reset();
    spdlog::shutdown();
}

static void Setup(int argc, char* argv[]) {
    if (Config::getLoggingEnabled()) {
        spdlog::cfg::load_env_levels();
        spdlog::cfg::helpers::load_levels(Config::getLogFilter());
        spdlog::cfg::load_argv_levels(argc, argv);
    } else {
        spdlog::cfg::helpers::load_levels("off");
    }

    if (!Config::isLogSync()) {
        spdlog::init_thread_pool(8192, 1);
    }

    std::at_quick_exit(Shutdown);

    spdlog::set_formatter(std::make_unique<thread_name_formatter>(UNLIMITED_SIZE));

#ifdef _WIN32
    if (Config::getLogType() == "wincolor") {
        g_console_sink =
            std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(spdlog::color_mode::always);
    } else {
        g_console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    }
#else
    g_console_sink = std::make_shared<spdlog_stdout>(spdlog::color_mode::always);
#endif

    g_shad_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (GetUserPath(Common::FS::PathType::LogDir) / "shad_log.txt").string());
    g_shad_file_sink->set_formatter(
        std::make_unique<thread_name_formatter>(Config::getLogSizeLimit()));

    std::shared_ptr<spdlog::sinks::dup_filter_sink_mt> dup_filter;

    if (Config::getLogSkipDuplicate()) {
        dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(
            std::chrono::milliseconds(Config::getMaxSkipDuration()));
        dup_filter->set_sinks(
            {g_console_sink, g_shad_file_sink,
             std::make_shared<
                 spdlog::sinks::null_sink_mt>() /*for POSITON_GAME_LOG id + ".log" */});
    }

    for (const auto& name : Common::Log::ALL_LOG_CLASSES) {
        if (Config::getLogSkipDuplicate()) {
            spdlog::initialize_logger(Config::isLogSync()
                                          ? std::make_shared<spdlog::logger>(name, dup_filter)
                                          : std::make_shared<spdlog::async_logger>(
                                                name, dup_filter, spdlog::thread_pool()));
        } else {
            spdlog::initialize_logger(
                Config::isLogSync()
                    ? std::make_shared<spdlog::logger>(
                          name,
                          std::initializer_list<spdlog::sink_ptr>{
                              g_console_sink, g_shad_file_sink,
                              std::make_shared<
                                  spdlog::sinks::
                                      null_sink_mt>() /*for POSITON_GAME_LOG id + ".log" */})
                    : std::make_shared<spdlog::async_logger>(
                          name,
                          std::initializer_list<spdlog::sink_ptr>{
                              g_console_sink, g_shad_file_sink,
                              std::make_shared<
                                  spdlog::sinks::
                                      null_sink_mt>() /*for POSITON_GAME_LOG id + ".log" */},
                          spdlog::thread_pool()));
        }
    }
}

static void Redirect(const std::string& name) {
    g_game_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (GetUserPath(Common::FS::PathType::LogDir) / name).string(), !g_should_append);
    g_game_file_sink->set_formatter(
        std::make_unique<Common::Log::thread_name_formatter>(Config::getLogSizeLimit()));

    for (const auto& name : Common::Log::ALL_LOG_CLASSES) {
        auto l = spdlog::get(name);
        auto& sinks = Config::getLogSkipDuplicate()
                          ? std::dynamic_pointer_cast<spdlog::sinks::dup_filter_sink_mt>(
                                l->sinks()[POSITON_DUPLICATE_SINK])
                                ->sinks()
                          : l->sinks();

        if (sinks.size() == 3) {
            sinks[POSITON_GAME_LOG] = g_game_file_sink;
        }
    }

    g_shad_file_sink->set_level(spdlog::level::off);
}

static void StopRedirection() {
    g_game_file_sink->set_level(spdlog::level::off);
    g_shad_file_sink->set_level(spdlog::level::trace);
}

static void Truncate() {
    if (g_game_file_sink != nullptr) {
        g_game_file_sink->truncate();
    }
    if (g_shad_file_sink != nullptr) {
        g_shad_file_sink->truncate();
    }
}
} // namespace Common::Log

// Define the fmt lib macros
#define LOG_GENERIC(log_class, log_level, ...)                                                     \
    SPDLOG_LOGGER_CALL(spdlog::get(log_class), log_level, __VA_ARGS__)

#ifdef _DEBUG
#define LOG_TRACE(log_class, ...)                                                                  \
    SPDLOG_LOGGER_TRACE(spdlog::get(Common::Log::log_class), __VA_ARGS__)
#else
#define LOG_TRACE(log_class, ...) (void(0))
#endif

#define LOG_DEBUG(log_class, ...)                                                                  \
    SPDLOG_LOGGER_DEBUG(spdlog::get(Common::Log::log_class), __VA_ARGS__)
#define LOG_INFO(log_class, ...)                                                                   \
    SPDLOG_LOGGER_INFO(spdlog::get(Common::Log::log_class), __VA_ARGS__)
#define LOG_WARNING(log_class, ...)                                                                \
    SPDLOG_LOGGER_WARN(spdlog::get(Common::Log::log_class), __VA_ARGS__)
#define LOG_ERROR(log_class, ...)                                                                  \
    SPDLOG_LOGGER_ERROR(spdlog::get(Common::Log::log_class), __VA_ARGS__)
#define LOG_CRITICAL(log_class, ...)                                                               \
    SPDLOG_LOGGER_CRITICAL(spdlog::get(Common::Log::log_class), __VA_ARGS__)
