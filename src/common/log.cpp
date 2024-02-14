#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <Util/config.h>
#include "common/log.h"
#ifdef _WIN64
#include <windows.h>
#endif

namespace Common::Log {

std::vector<spdlog::sink_ptr> sinks;
constexpr bool log_file_exceptions = true;

void Flush() {
    spdlog::details::registry::instance().flush_all();
}

thread_local uint8_t TLS[1024];

uint64_t tls_access(int64_t tls_offset) {
    if (tls_offset == 0) {
        return (uint64_t)TLS;
    }
}

#ifdef _WIN64
static LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExp) noexcept {
    auto orig_rip = pExp->ContextRecord->Rip;
    while (*(uint8_t *)pExp->ContextRecord->Rip == 0x66) pExp->ContextRecord->Rip++;

    if (*(uint8_t *)pExp->ContextRecord->Rip == 0xcd) {
        int reg = *(uint8_t *)(pExp->ContextRecord->Rip + 1) - 0x80;
        int sizes = *(uint8_t *)(pExp->ContextRecord->Rip + 2);
        int pattern_size = sizes & 0xF;
        int imm_size = sizes >> 4;

        int64_t tls_offset;
        if (imm_size == 4)
            tls_offset = *(int32_t *)(pExp->ContextRecord->Rip + pattern_size);
        else
            tls_offset = *(int64_t *)(pExp->ContextRecord->Rip + pattern_size);

        (&pExp->ContextRecord->Rax)[reg] = tls_access(tls_offset); /* TLS_ACCESS */
        pExp->ContextRecord->Rip += pattern_size + imm_size;

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    pExp->ContextRecord->Rip = orig_rip;
    const u32 ec = pExp->ExceptionRecord->ExceptionCode;
    switch (ec) {
        case EXCEPTION_ACCESS_VIOLATION: {
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_ACCESS_VIOLATION ({:#x}). ", ec);
            const auto info = pExp->ExceptionRecord->ExceptionInformation;
            switch (info[0]) {
                case 0:
                    LOG_CRITICAL_IF(log_file_exceptions, "Read violation at address {:#x}.", info[1]);
                    break;
                case 1:
                    LOG_CRITICAL_IF(log_file_exceptions, "Write violation at address {:#x}.", info[1]);
                    break;
                case 8:
                    LOG_CRITICAL_IF(log_file_exceptions, "DEP violation at address {:#x}.", info[1]);
                    break;
                default:
                    break;
            }
            break;
        }
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_ARRAY_BOUNDS_EXCEEDED ({:#x}). ", ec);
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_DATATYPE_MISALIGNMENT ({:#x}). ", ec);
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_FLT_DIVIDE_BY_ZERO ({:#x}). ", ec);
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_ILLEGAL_INSTRUCTION ({:#x}). ", ec);
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_IN_PAGE_ERROR ({:#x}). ", ec);
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_INT_DIVIDE_BY_ZERO ({:#x}). ", ec);
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_PRIV_INSTRUCTION ({:#x}). ", ec);
            break;
        case EXCEPTION_STACK_OVERFLOW:
            LOG_CRITICAL_IF(log_file_exceptions, "Exception EXCEPTION_STACK_OVERFLOW ({:#x}). ", ec);
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }
    Flush();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif


int Init(bool use_stdout) {
    sinks.clear();
    if (use_stdout) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
#ifdef _WIN64
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(L"shadps4.txt", true));
#else
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("shadps4.txt", true));
#endif
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("shadps4 logger", begin(sinks), end(sinks)));
    auto f = std::make_unique<spdlog::pattern_formatter>("%^|%L|: %v%$", spdlog::pattern_time_type::local, std::string(""));  // disable eol
    spdlog::set_formatter(std::move(f));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(Config::getLogLevel()));

#ifdef _WIN64
    if (!AddVectoredExceptionHandler(0, ExceptionHandler)) {
        LOG_CRITICAL_IF(log_file_exceptions, "Failed to register an exception handler");
    }
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
        Flush();
        if (old_terminate) old_terminate();
    });

    return 0;
}

void SetLevel(spdlog::level::level_enum log_level) {
    spdlog::set_level(log_level);
}

}  // namespace Common::Log
