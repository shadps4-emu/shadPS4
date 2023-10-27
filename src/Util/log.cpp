#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <vector>
#include <Util/config.h>

#ifdef _WIN64
#include <Windows.h>
#endif

#include "log.h"

namespace logging {
std::vector<spdlog::sink_ptr> sinks;
constexpr bool log_file_exceptions = true;  // disable it to disable logging

void flush() { spdlog::details::registry::instance().flush_all(); }

#ifdef _WIN64

static LONG WINAPI exception_handler(PEXCEPTION_POINTERS pExp) noexcept {
    const u32 ec = pExp->ExceptionRecord->ExceptionCode;
    switch (ec) {
        case EXCEPTION_ACCESS_VIOLATION:
            LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_ACCESS_VIOLATION ({}). ", log_hex(ec));
            switch (pExp->ExceptionRecord->ExceptionInformation[0]) {
                case 0: LOG_CRITICAL_IF(log_file_exceptions,"Read violation at address {}.", log_hex(pExp->ExceptionRecord->ExceptionInformation[1])); break;
                case 1: LOG_CRITICAL_IF(log_file_exceptions,"Write violation at address {}.", log_hex(pExp->ExceptionRecord->ExceptionInformation[1])); break;
                case 8:LOG_CRITICAL_IF(log_file_exceptions,"DEP violation at address {}.", log_hex(pExp->ExceptionRecord->ExceptionInformation[1])); break;
                default: break;
            }
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_ARRAY_BOUNDS_EXCEEDED ({}). ", log_hex(ec)); break;
        case EXCEPTION_DATATYPE_MISALIGNMENT: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_DATATYPE_MISALIGNMENT ({}). ", log_hex(ec)); break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_FLT_DIVIDE_BY_ZERO ({}). ", log_hex(ec)); break;
        case EXCEPTION_ILLEGAL_INSTRUCTION: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_ILLEGAL_INSTRUCTION ({}). ", log_hex(ec)); break;
        case EXCEPTION_IN_PAGE_ERROR: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_IN_PAGE_ERROR ({}). ", log_hex(ec)); break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_INT_DIVIDE_BY_ZERO ({}). ", log_hex(ec)); break;
        case EXCEPTION_PRIV_INSTRUCTION: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_PRIV_INSTRUCTION ({}). ", log_hex(ec)); break;
        case EXCEPTION_STACK_OVERFLOW: LOG_CRITICAL_IF(log_file_exceptions,"Exception EXCEPTION_STACK_OVERFLOW ({}). ", log_hex(ec)); break;
        default: return EXCEPTION_CONTINUE_SEARCH;
    }
    flush();
    return EXCEPTION_CONTINUE_SEARCH;
}

void register_log_exception_handler() {
    if (!AddVectoredExceptionHandler(0, exception_handler)) {
        LOG_CRITICAL_IF(log_file_exceptions,"Failed to register an exception handler");
    }
}
#endif


int init(bool use_stdout) {
    sinks.clear();   // clear existing sinks
    if (use_stdout)  // if we use stdout window then init it as well
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(L"shadps4.txt", true));
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("shadps4 logger", begin(sinks), end(sinks)));
    auto f = std::make_unique<spdlog::pattern_formatter>("%^|%L|: %v%$", spdlog::pattern_time_type::local, std::string(""));  // disable eol
    spdlog::set_formatter(std::move(f));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(Config::getLogLevel()));
    spdlog::level::level_enum t = spdlog::get_level();

    #ifdef _WIN64
    register_log_exception_handler();
    #endif
   static std::terminate_handler old_terminate = nullptr;
    old_terminate = std::set_terminate([]() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception &e) {
            LOG_CRITICAL_IF(log_file_exceptions, "Unhandled C++ exception. {}", e.what());
        } catch (...) {
            LOG_CRITICAL_IF(log_file_exceptions, "Unhandled C++ exception. UNKNOWN");
        }
        flush();
        if (old_terminate) old_terminate();
    });

    return 0;  // all ok
}

void set_level(spdlog::level::level_enum log_level) { spdlog::set_level(log_level); }

}  // namespace logging