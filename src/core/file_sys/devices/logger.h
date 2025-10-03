// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/file_sys/devices/base_device.h"

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

    s64 write(const void* buf, u64 nbytes) override;
    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override;

    s32 fsync() override;

private:
    void log(const char* buf, u64 nbytes);
    void log_flush();
};

} // namespace Core::Devices
