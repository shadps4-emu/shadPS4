// SPDX-FileCopyrightText: Copyright 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>
#include <string_view>

#include "common/logging/formatter.h"
#include "common/logging/types.h"

namespace Common::Log {

constexpr const char* TrimSourcePath(std::string_view source) {
    const auto rfind = [source](const std::string_view match) {
        return source.rfind(match) == source.npos ? 0 : (source.rfind(match) + match.size());
    };
    auto idx = std::max({rfind("/"), rfind("\\")});
    return source.data() + idx;
}

} // namespace Common::Log

// Define the fmt lib macros
#define LOG_GENERIC(log_class, log_level, ...)                                                     \
    spdlog::get(#log_class)->log(log_level, __VA_ARGS__)

#ifdef _DEBUG
#define LOG_TRACE(log_class, ...)                                                                  \
    spdlog::get(#log_class)->trace(__VA_ARGS__)
#else
#define LOG_TRACE(log_class, fmt, ...) (void(0))
#endif

#define LOG_DEBUG(log_class, ...)                                                                  \
    spdlog::get(#log_class)->debug(__VA_ARGS__)
#define LOG_INFO(log_class, ...)                                                                   \
    spdlog::get(#log_class)->info(__VA_ARGS__)
#define LOG_WARNING(log_class, ...)                                                                \
    spdlog::get(#log_class)->warn(__VA_ARGS__)
#define LOG_ERROR(log_class, ...)                                                                  \
    spdlog::get(#log_class)->error(__VA_ARGS__)
#define LOG_CRITICAL(log_class, ...)                                                               \
    spdlog::get(#log_class)->critical(__VA_ARGS__)
