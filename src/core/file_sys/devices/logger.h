// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base_device.h"

#include <mutex>
#include <string>
#include <vector>

namespace Core::Devices {

class Logger final : BaseDevice {
    std::string prefix;
    bool is_err;

    std::recursive_mutex mtx;
    std::vector<char> buffer;

public:
    explicit Logger(std::string prefix, bool is_err);

    ~Logger() override;

    s64 write(const void* buf, size_t nbytes) override;
    size_t writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override;
    s64 pwrite(const void* buf, size_t nbytes, u64 offset) override;

    s32 fsync() override;

private:
    void log(const char* buf, size_t nbytes);
    void log_flush();
};

} // namespace Core::Devices
