// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <string>
#include <fmt/std.h>

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/emulator_settings.h"
#ifdef _WIN32
#include <Windows.h>
#endif

// return codes above 'standard'
// https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
enum class ShadPs4ReturnCode : u32 {
    TERMINATE_WITHOUT_EXCEPTION = 20'000,
    TERMINATE_WITH_EXCEPTION = 20'001,
    TERMINATE_WITH_UNKNOWN_EXCEPTION = 20'002,
};

namespace Common::Log {
bool g_should_append = false;

static std::shared_ptr<spdlog_stdout> g_console_sink;
static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_shad_file_sink;

std::unordered_map<std::string_view, std::shared_ptr<spdlog::logger>> ALL_LOGGERS{
    {LogClass::Common, nullptr},
    {LogClass::Common_Filesystem, nullptr},
    {LogClass::Common_Memory, nullptr},
    {LogClass::Config, nullptr},
    {LogClass::Core, nullptr},
    {LogClass::Core_Devices, nullptr},
    {LogClass::Core_Linker, nullptr},
    {LogClass::Debug, nullptr},
    {LogClass::Frontend, nullptr},
    {LogClass::IPC, nullptr},
    {LogClass::ImGui, nullptr},
    {LogClass::Input, nullptr},
    {LogClass::Kernel, nullptr},
    {LogClass::Kernel_Event, nullptr},
    {LogClass::Kernel_Fs, nullptr},
    {LogClass::Kernel_Pthread, nullptr},
    {LogClass::Kernel_Sce, nullptr},
    {LogClass::Kernel_Vmm, nullptr},
    {LogClass::KeyManager, nullptr},
    {LogClass::Lib, nullptr},
    {LogClass::Lib_Ajm, nullptr},
    {LogClass::Lib_AppContent, nullptr},
    {LogClass::Lib_Audio3d, nullptr},
    {LogClass::Lib_AudioIn, nullptr},
    {LogClass::Lib_AudioOut, nullptr},
    {LogClass::Lib_AvPlayer, nullptr},
    {LogClass::Lib_Camera, nullptr},
    {LogClass::Lib_CommonDlg, nullptr},
    {LogClass::Lib_CompanionHttpd, nullptr},
    {LogClass::Lib_CompanionUtil, nullptr},
    {LogClass::Lib_DiscMap, nullptr},
    {LogClass::Lib_ErrorDialog, nullptr},
    {LogClass::Lib_Fiber, nullptr},
    {LogClass::Lib_Font, nullptr},
    {LogClass::Lib_FontFt, nullptr},
    {LogClass::Lib_GameLiveStreaming, nullptr},
    {LogClass::Lib_GnmDriver, nullptr},
    {LogClass::Lib_Hmd, nullptr},
    {LogClass::Lib_HmdSetupDialog, nullptr},
    {LogClass::Lib_Http, nullptr},
    {LogClass::Lib_Http2, nullptr},
    {LogClass::Lib_Ime, nullptr},
    {LogClass::Lib_ImeDialog, nullptr},
    {LogClass::Lib_Jpeg, nullptr},
    {LogClass::Lib_Kernel, nullptr},
    {LogClass::Lib_LibcInternal, nullptr},
    {LogClass::Lib_Mouse, nullptr},
    {LogClass::Lib_Move, nullptr},
    {LogClass::Lib_MsgDlg, nullptr},
    {LogClass::Lib_Net, nullptr},
    {LogClass::Lib_NetCtl, nullptr},
    {LogClass::Lib_Ngs2, nullptr},
    {LogClass::Lib_NpAuth, nullptr},
    {LogClass::Lib_NpCommerce, nullptr},
    {LogClass::Lib_NpCommon, nullptr},
    {LogClass::Lib_NpManager, nullptr},
    {LogClass::Lib_NpMatching2, nullptr},
    {LogClass::Lib_NpSignaling, nullptr},
    {LogClass::Lib_NpPartner, nullptr},
    {LogClass::Lib_NpParty, nullptr},
    {LogClass::Lib_NpProfileDialog, nullptr},
    {LogClass::Lib_NpScore, nullptr},
    {LogClass::Lib_NpSnsFacebookDialog, nullptr},
    {LogClass::Lib_NpTrophy, nullptr},
    {LogClass::Lib_NpTus, nullptr},
    {LogClass::Lib_NpWebApi, nullptr},
    {LogClass::Lib_NpWebApi2, nullptr},
    {LogClass::Lib_Pad, nullptr},
    {LogClass::Lib_PlayGo, nullptr},
    {LogClass::Lib_PlayGoDialog, nullptr},
    {LogClass::Lib_Png, nullptr},
    {LogClass::Lib_Random, nullptr},
    {LogClass::Lib_RazorCpu, nullptr},
    {LogClass::Lib_Remoteplay, nullptr},
    {LogClass::Lib_Rtc, nullptr},
    {LogClass::Lib_Rudp, nullptr},
    {LogClass::Lib_SaveData, nullptr},
    {LogClass::Lib_SaveDataDialog, nullptr},
    {LogClass::Lib_Screenshot, nullptr},
    {LogClass::Lib_SharePlay, nullptr},
    {LogClass::Lib_SigninDialog, nullptr},
    {LogClass::Lib_Ssl, nullptr},
    {LogClass::Lib_Ssl2, nullptr},
    {LogClass::Lib_SysModule, nullptr},
    {LogClass::Lib_SystemGesture, nullptr},
    {LogClass::Lib_SystemService, nullptr},
    {LogClass::Lib_Usbd, nullptr},
    {LogClass::Lib_UserService, nullptr},
    {LogClass::Lib_Vdec2, nullptr},
    {LogClass::Lib_VideoOut, nullptr},
    {LogClass::Lib_Videodec, nullptr},
    {LogClass::Lib_Voice, nullptr},
    {LogClass::Lib_VrTracker, nullptr},
    {LogClass::Lib_WebBrowserDialog, nullptr},
    {LogClass::Lib_Zlib, nullptr},
    {LogClass::Loader, nullptr},
    {LogClass::Log, nullptr},
    {LogClass::Render, nullptr},
    {LogClass::Render_Recompiler, nullptr},
    {LogClass::Render_Vulkan, nullptr},
    {LogClass::Tty, nullptr},
};

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

void Setup(std::string_view log_filename) {
    static bool already_registered = false;

    if (!already_registered) {
        already_registered = true;
        std::atexit(Shutdown);
        std::at_quick_exit(Flush);
        std::set_terminate(Terminate);
    }

#ifdef _WIN32
    if (Config::GetLogType() == "wincolor") {
        g_console_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
    } else {
        g_console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    }

#else
    g_console_sink = UpdateColorLevels(std::make_shared<spdlog_stdout>());
#endif

    g_console_sink->set_formatter(std::make_unique<thread_name_formatter>(UNLIMITED_SIZE));

    g_shad_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (GetUserPath(Common::FS::PathType::LogDir) / log_filename).string(), !g_should_append);
    g_shad_file_sink->set_formatter(
        std::make_unique<thread_name_formatter>(Config::GetLogSizeLimit()));

    std::initializer_list<spdlog::sink_ptr> sinks{g_console_sink, g_shad_file_sink};

    std::initializer_list<spdlog::sink_ptr> async_sink{std::make_shared<spdlog::sinks::async_sink>(
        spdlog::sinks::async_sink::config{.sinks = sinks})};

    std::initializer_list<spdlog::sink_ptr> dup_filter{
        std::make_shared<spdlog::sinks::dup_filter_sink_mt>(
            std::chrono::milliseconds(Config::GetLogMaxSkipDuration()),
            Config::IsLogSync() ? sinks : async_sink)};

    spdlog::level default_log_level = spdlog::level::info;
    std::unordered_map<std::string, spdlog::level> log_level_per_class;

    if (Config::IsLogEnable()) {
        for (const auto class_level : std::views::split(Config::GetLogFilter(), ',')) {
            const auto class_level_pair =
                std::views::split(class_level, '=') | std::ranges::to<std::vector<std::string>>();

            if (class_level_pair.size() == 1) {
                default_log_level = spdlog::level_from_str(class_level_pair.front() |
                                                           std::ranges::to<std::string>());
            } else {
                log_level_per_class[class_level_pair.front() | std::ranges::to<std::string>()] =
                    spdlog::level_from_str(class_level_pair.back() |
                                           std::ranges::to<std::string>());
            }
        }
    }

    for (auto& [name, logger] : ALL_LOGGERS) {
        logger = std::make_shared<spdlog::logger>(
            std::string(name),
            Config::IsLogSkipDuplicate() ? dup_filter : (Config::IsLogSync() ? sinks : async_sink));

        if (Config::IsLogEnable()) {
            const auto level_it = log_level_per_class.find(std::string(name));

            logger->set_level(level_it != log_level_per_class.end() ? level_it->second
                                                                    : default_log_level);
        } else {
            logger->set_level(spdlog::level::off);
        }
    }
}

void Shutdown() {
    for (auto& logger : ALL_LOGGERS | std::views::values) {
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

void Terminate() {
    try {
        if (std::exception_ptr eptr{std::current_exception()}) {
            std::rethrow_exception(eptr);
        }

        LOG_CRITICAL(Debug, "Exiting without exception");

        std::quick_exit(std::to_underlying(ShadPs4ReturnCode::TERMINATE_WITHOUT_EXCEPTION));
    } catch (const std::exception& exception) {
        LOG_CRITICAL(Debug, "Exception: {}", exception);

        std::quick_exit(std::to_underlying(ShadPs4ReturnCode::TERMINATE_WITH_EXCEPTION));
    } catch (...) {
        LOG_CRITICAL(Debug, "Unknown exception caught");

        std::quick_exit(std::to_underlying(ShadPs4ReturnCode::TERMINATE_WITH_UNKNOWN_EXCEPTION));
    }
}
} // namespace Common::Log
