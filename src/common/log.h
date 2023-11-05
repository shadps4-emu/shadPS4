#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>

namespace Common::Log {

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define LOG_TRACE_IF(flag, ...) \
    if (flag) LOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG_IF(flag, ...) \
    if (flag) LOG_DEBUG(__VA_ARGS__)
#define LOG_INFO_IF(flag, ...) \
    if (flag) LOG_INFO(__VA_ARGS__)
#define LOG_WARN_IF(flag, ...) \
    if (flag) LOG_WARN(__VA_ARGS__)
#define LOG_ERROR_IF(flag, ...) \
    if (flag) LOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL_IF(flag, ...) \
    if (flag) LOG_CRITICAL(__VA_ARGS__)

int Init(bool use_stdout);

void SetLevel(spdlog::level::level_enum log_level);

} // namespace Common::Log
