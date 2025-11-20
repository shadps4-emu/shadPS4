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

    static QuasiFS::dev_ptr Create(std::string prefix, bool is_err) {
        return std::make_shared<Logger>(prefix, is_err);
    }

    s64 write(const void* buf, u64 nbytes) override;
    s32 fsync() override;

    // clang-format off
    s64 read(void* buf, u64 count) override { DEVICE_STUB(); };
    s32 ioctl(u64 cmd, Common::VaCtx* args) override { DEVICE_STUB(); }
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override { DEVICE_STUB(); }
    s32 ftruncate(s64 length) override { DEVICE_STUB(); }
    s64 getdents(void* buf, u32 nbytes, s64 offset, s64* basep) override { DEVICE_STUB(); }
    // clang-format on

private:
    void log(const char* buf, u64 nbytes);
    void log_flush();
};

} // namespace Core::Devices
