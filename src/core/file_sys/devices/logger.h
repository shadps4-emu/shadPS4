// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "common/logging/log.h"
#include "common/types.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"

#define DEVICE_STUB()                                                                              \
    {                                                                                              \
        LOG_ERROR(Kernel_Fs, "(STUBBED) called");                                                  \
        return -QUASI_ENOSYS;                                                                      \
    }

namespace Core::Devices {

class Logger final : public QuasiFS::Device {
    std::string prefix;
    bool is_err;

    std::recursive_mutex mtx;
    std::vector<char> buffer;

public:
    explicit Logger(std::string prefix, bool is_err);
    ~Logger();

    s64 write(const void* buf, u64 nbytes) override;
    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override;

    s32 fsync() override;

private:
    void log(const char* buf, u64 nbytes);
    void log_flush();
};

} // namespace Core::Devices
