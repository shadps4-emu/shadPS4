// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/formatter.h>
#include <spdlog/sinks/base_sink.h>

namespace Common::Log {
static constexpr unsigned long long UNLIMITED_SIZE = 0;

/*
 * Trivial file sink with two files as target : main and session
 */
class LogFileSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit LogFileSink(const spdlog::filename_t& main_filename, bool truncate = false,
                         unsigned long long size_limit = UNLIMITED_SIZE)
        : _size_limit{size_limit} {
        main_file_helper_.open(main_filename, truncate);
    }

    spdlog::details::file_helper main_file_helper_;
    spdlog::details::file_helper session_file_helper_;

    unsigned long long _size_limit;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        if (session_file_helper_.filename().empty()) {
            // always log in the main file, it shouldnt be too big
            main_file_helper_.write(formatted);
        } else if (_current_size < _size_limit || msg.log_level == spdlog::level::critical) {
            session_file_helper_.write(formatted);
            _current_size += formatted.size();
        }
    }

    void flush_() override {
        main_file_helper_.flush();
        session_file_helper_.flush();
    }

    unsigned long long _current_size = 0;
};
} // namespace Common::Log
