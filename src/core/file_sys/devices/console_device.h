// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"
#include "common/types.h"

#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"

#define DEVICE_STUB()                                                                              \
    {                                                                                              \
        LOG_ERROR(Kernel_Fs, "(STUBBED) called");                                                  \
        return -QUASI_ENOSYS;                                                                      \
    }

namespace Core::Devices {

class ConsoleDevice final : public QuasiFS::Device {

public:
    ConsoleDevice();
    ~ConsoleDevice();

    // clang-format off
    s64 read(void* buf, u64 count) override { DEVICE_STUB(); };
    s64 write(const void* buf, u64 count) override { DEVICE_STUB(); };
    s32 ioctl(u64 cmd, Common::VaCtx* args) override { DEVICE_STUB(); };
    s64 pread(void* buf, size_t count, s64 offset) override { DEVICE_STUB(); };
    s64 pwrite(const void* buf, size_t count, s64 offset) override { DEVICE_STUB(); };
    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override { DEVICE_STUB(); };
    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override { DEVICE_STUB(); };
    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, s64 offset) override { DEVICE_STUB(); };
    s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, s64 offset) override { DEVICE_STUB(); };
    s64 lseek(s64 offset, int whence) override { DEVICE_STUB(); };
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override { DEVICE_STUB(); };
    s32 fsync() override { DEVICE_STUB(); };
    s32 ftruncate(s64 length) override { DEVICE_STUB(); };
    s64 getdents(void* buf, u32 nbytes, s64* basep) override { DEVICE_STUB(); };
    // clang-format on
};

} // namespace Core::Devices