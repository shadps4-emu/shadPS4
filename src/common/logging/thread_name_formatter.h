// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/formatter.h>

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
} // namespace Common::Log
