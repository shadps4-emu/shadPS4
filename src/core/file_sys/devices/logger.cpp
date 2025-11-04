// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/file_sys/devices/logger.h"
#include "core/libraries/kernel/file_system.h"

namespace Core::Devices {

Logger::Logger(std::string prefix, bool is_err) : prefix(std::move(prefix)), is_err(is_err) {}

Logger::~Logger() = default;

s64 Logger::write(const void* buf, u64 nbytes) {
    log(static_cast<const char*>(buf), nbytes);
    return nbytes;
}

s64 Logger::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    u64 total_written = 0;
    for (int i = 0; i < iovcnt; i++) {
        log(static_cast<const char*>(iov[i].iov_base), iov[i].iov_len);
        total_written += iov[i].iov_len;
    }
    return total_written;
}

s64 Logger::pwrite(const void* buf, u64 nbytes, s64 offset) {
    log(static_cast<const char*>(buf), nbytes);
    return nbytes;
}

s32 Logger::fsync() {
    log_flush();
    return 0;
}

void Logger::log(const char* buf, u64 nbytes) {
    std::scoped_lock lock{mtx};
    const char* end = buf + nbytes;
    for (const char* it = buf; it < end; ++it) {
        char c = *it;
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            log_flush();
            continue;
        }
        buffer.push_back(c);
    }
}

void Logger::log_flush() {
    std::scoped_lock lock{mtx};
    if (buffer.empty()) {
        return;
    }
    if (is_err) {
        LOG_ERROR(Tty, "[{}] {}", prefix, std::string_view{buffer});
    } else {
        LOG_INFO(Tty, "[{}] {}", prefix, std::string_view{buffer});
    }
    buffer.clear();
}

} // namespace Core::Devices