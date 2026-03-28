// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "log.h"
#include "common/config.h"
#include "core/emulator_settings.h"
#include "common/types.h"
#include "shadps4_app.h"

#include <cstdlib>
#include <string>

namespace Common::Log {
std::shared_ptr<spdlog_stdout> g_console_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_shad_file_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_game_file_sink;
bool g_should_append = false;

void Setup(int argc, char* argv[]) {
    if (EmulatorSettings.IsLogEnable()) {
        spdlog::cfg::load_env_levels();
        spdlog::cfg::helpers::load_levels(EmulatorSettings.GetLogFilter());
        spdlog::cfg::load_argv_levels(argc, argv);
    } else {
        spdlog::cfg::helpers::load_levels("off");
    }

    if (!EmulatorSettings.IsLogSync()) {
        spdlog::init_thread_pool(8192, 1);
    }

    std::atexit(Shutdown);
    std::at_quick_exit(Shutdown);

    spdlog::set_formatter(std::make_unique<thread_name_formatter>(UNLIMITED_SIZE));

#ifdef _WIN32
    if (EmulatorSettings.GetLogType() == "wincolor") {
        g_console_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
    } else {
        g_console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    }
#else
    g_console_sink = std::make_shared<spdlog_stdout>();
#endif

    g_shad_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (GetUserPath(Common::FS::PathType::LogDir) / "shad_log.txt").string());
    g_shad_file_sink->set_formatter(
        std::make_unique<thread_name_formatter>(EmulatorSettings.GetLogSizeLimit()));

    std::shared_ptr<spdlog::sinks::dup_filter_sink_mt> dup_filter;

    if (EmulatorSettings.IsLogSkipDuplicate()) {
        dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(
            std::chrono::milliseconds(EmulatorSettings.GetLogMaxSkipDuration()));
        dup_filter->set_sinks(
            {g_console_sink, g_shad_file_sink,
             std::make_shared<
                 spdlog::sinks::null_sink_mt>() /*for POSITON_GAME_LOG id + ".log" */});
    }

    for (const auto& name : Common::Log::ALL_LOG_CLASSES) {
        if (EmulatorSettings.IsLogSkipDuplicate()) {
            spdlog::initialize_logger(EmulatorSettings.IsLogSync()
                                          ? std::make_shared<spdlog::logger>(name, dup_filter)
                                          : std::make_shared<spdlog::async_logger>(
                                                name, dup_filter, spdlog::thread_pool()));
        } else {
            spdlog::initialize_logger(
                EmulatorSettings.IsLogSync()
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

void Redirect(const std::string& name) {
    g_game_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (GetUserPath(Common::FS::PathType::LogDir) / name).string(), !g_should_append);
    g_game_file_sink->set_formatter(
        std::make_unique<Common::Log::thread_name_formatter>(EmulatorSettings.GetLogSizeLimit()));

    for (const auto& name : Common::Log::ALL_LOG_CLASSES) {
        auto l = spdlog::get(name);
        auto& sinks = EmulatorSettings.IsLogSkipDuplicate()
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
} // namespace Common::Log
