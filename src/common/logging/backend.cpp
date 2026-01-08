// SPDX-FileCopyrightText: Copyright 2014 Citra Emulator Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <filesystem>
#include <thread>

#include <fmt/format.h>

#ifdef _WIN32
#include <windows.h> // For OutputDebugStringW
#endif

#include "common/bounded_threadsafe_queue.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/io_file.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#include "common/logging/log_entry.h"
#include "common/logging/text_formatter.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "common/thread.h"

namespace Common::Log {

using namespace Common::FS;

namespace {

/**
 * Backend that writes to stderr and with color
 */
class ColorConsoleBackend {
public:
    explicit ColorConsoleBackend() = default;

    ~ColorConsoleBackend() = default;

    void Write(const Entry& entry) {
        if (enabled.load(std::memory_order_relaxed)) {
            PrintColoredMessage(entry);
        }
    }

    void Flush() {
        // stderr shouldn't be buffered
    }

    void SetEnabled(bool enabled_) {
        enabled = enabled_;
    }

private:
    std::atomic_bool enabled{true};
};

/**
 * Backend that writes to a file passed into the constructor
 */
class FileBackend {
public:
    explicit FileBackend(const std::filesystem::path& filename, bool should_append = false)
        : file{filename, should_append ? FS::FileAccessMode::Append : FS::FileAccessMode::Create,
               FS::FileType::TextFile} {}

    ~FileBackend() = default;

    void Write(const Entry& entry) {
        if (!enabled) {
            return;
        }

        bytes_written += file.WriteString(FormatLogMessage(entry).append(1, '\n'));

        // Prevent logs from exceeding a set maximum size in the event that log entries are spammed.
        const auto write_limit = 100_MB;
        const bool write_limit_exceeded = bytes_written > write_limit;
        if (entry.log_level >= Level::Error || write_limit_exceeded) {
            if (write_limit_exceeded) {
                // Stop writing after the write limit is exceeded.
                // Don't close the file so we can print a stacktrace if necessary
                enabled = false;
            }
            file.Flush();
        }
    }

    void Flush() {
        file.Flush();
    }

private:
    Common::FS::IOFile file;
    bool enabled = true;
    std::size_t bytes_written = 0;
};

#ifdef _WIN32
/**
 * Backend that writes to Visual Studio's output window
 */
class DebuggerBackend {
public:
    explicit DebuggerBackend() = default;

    ~DebuggerBackend() = default;

    void Write(const Entry& entry) {
        ::OutputDebugStringW(UTF8ToUTF16W(FormatLogMessage(entry).append(1, '\n')).c_str());
    }

    void Flush() {}

    void EnableForStacktrace() {}
};
#endif

bool initialization_in_progress_suppress_logging = true;

/**
 * Static state as a singleton.
 */
class Impl {
public:
    static Impl& Instance() {
        if (!instance) {
            throw std::runtime_error("Using Logging instance before its initialization");
        }
        return *instance;
    }

    static void Initialize(std::string_view log_file) {
        if (instance) {
            LOG_WARNING(Log, "Reinitializing logging backend");
            return;
        }
        const auto& log_dir = GetUserPath(PathType::LogDir);
        std::filesystem::create_directory(log_dir);
        Filter filter;
        filter.ParseFilterString(Config::getLogFilter());
        const auto& log_file_path = log_file.empty() ? LOG_FILE : log_file;
        instance = std::unique_ptr<Impl, decltype(&Deleter)>(
            new Impl(log_dir / log_file_path, filter), Deleter);
        initialization_in_progress_suppress_logging = false;
    }

    static void ResetInstance() {
        initialization_in_progress_suppress_logging = true;
        instance.reset();
    }

    static bool IsActive() {
        return instance != nullptr;
    }

    static void Start() {
        instance->StartBackendThread();
    }

    static void Stop() {
        instance->StopBackendThread();
    }

    static void SetAppend() {
        should_append = true;
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    void SetGlobalFilter(const Filter& f) {
        filter = f;
    }

    void SetColorConsoleBackendEnabled(bool enabled) {
        color_console_backend.SetEnabled(enabled);
    }

    void PushEntry(Class log_class, Level log_level, const char* filename, unsigned int line_num,
                   const char* function, const char* format, const fmt::format_args& args) {
        if (!filter.CheckMessage(log_class, log_level) || !Config::getLoggingEnabled()) {
            return;
        }

        const auto message = fmt::vformat(format, args);

        // Propagate important log messages to the profiler
        if (IsProfilerConnected()) {
            const auto& msg_str = fmt::format("[{}] {}", GetLogClassName(log_class), message);
            switch (log_level) {
            case Level::Warning:
                TRACE_WARN(msg_str);
                break;
            case Level::Error:
                TRACE_ERROR(msg_str);
                break;
            case Level::Critical:
                TRACE_CRIT(msg_str);
                break;
            default:
                break;
            }
        }

        using std::chrono::duration_cast;
        using std::chrono::microseconds;
        using std::chrono::steady_clock;

        const Entry entry = {
            .timestamp = duration_cast<microseconds>(steady_clock::now() - time_origin),
            .log_class = log_class,
            .log_level = log_level,
            .filename = filename,
            .line_num = line_num,
            .function = function,
            .message = std::move(message),
            .thread = Common::GetCurrentThreadName(),
        };
        if (Config::getLogType() == "async") {
            message_queue.EmplaceWait(entry);
        } else {
            ForEachBackend([&entry](auto& backend) { backend.Write(entry); });
            std::fflush(stdout);
        }
    }

private:
    Impl(const std::filesystem::path& file_backend_filename, const Filter& filter_)
        : filter{filter_}, file_backend{file_backend_filename, should_append} {}

    ~Impl() = default;

    void StartBackendThread() {
        backend_thread = std::jthread([this](std::stop_token stop_token) {
            Common::SetCurrentThreadName("shadPS4:Log");
            Entry entry;
            const auto write_logs = [this, &entry]() {
                ForEachBackend([&entry](auto& backend) { backend.Write(entry); });
            };
            while (!stop_token.stop_requested()) {
                message_queue.PopWait(entry, stop_token);
                if (entry.filename != nullptr) {
                    write_logs();
                }
            }
            // Drain the logging queue. Only writes out up to MAX_LOGS_TO_WRITE to prevent a
            // case where a system is repeatedly spamming logs even on close.
            int max_logs_to_write = filter.IsDebug() ? std::numeric_limits<s32>::max() : 100;
            while (max_logs_to_write-- && message_queue.TryPop(entry)) {
                write_logs();
            }
        });
    }

    void StopBackendThread() {
        backend_thread.request_stop();
        if (backend_thread.joinable()) {
            backend_thread.join();
        }

        ForEachBackend([](auto& backend) { backend.Flush(); });
    }

    void ForEachBackend(auto lambda) {
#ifdef _WIN32
        lambda(debugger_backend);
#endif
        lambda(color_console_backend);
        lambda(file_backend);
    }

    static void Deleter(Impl* ptr) {
        delete ptr;
    }

    static inline std::unique_ptr<Impl, decltype(&Deleter)> instance{nullptr, Deleter};
    static inline bool should_append{false};

    Filter filter;
#ifdef _WIN32
    DebuggerBackend debugger_backend{};
#endif
    ColorConsoleBackend color_console_backend{};
    FileBackend file_backend;

    MPSCQueue<Entry> message_queue{};
    std::chrono::steady_clock::time_point time_origin{std::chrono::steady_clock::now()};
    std::jthread backend_thread;
};
} // namespace

void Initialize(std::string_view log_file) {
    Impl::Initialize(log_file.empty() ? LOG_FILE : log_file);
}

bool IsActive() {
    return Impl::IsActive();
}

void Start() {
    Impl::Start();
}

void Stop() {
    Impl::Stop();
}

void Denitializer() {
    Impl::Stop();
    Impl::ResetInstance();
}

void SetGlobalFilter(const Filter& filter) {
    Impl::Instance().SetGlobalFilter(filter);
}

void SetColorConsoleBackendEnabled(bool enabled) {
    Impl::Instance().SetColorConsoleBackendEnabled(enabled);
}

void SetAppend() {
    Impl::SetAppend();
}

void FmtLogMessageImpl(Class log_class, Level log_level, const char* filename,
                       unsigned int line_num, const char* function, const char* format,
                       const fmt::format_args& args) {
    if (!initialization_in_progress_suppress_logging) [[likely]] {
        Impl::Instance().PushEntry(log_class, log_level, filename, line_num, function, format,
                                   args);
    }
}
} // namespace Common::Log
