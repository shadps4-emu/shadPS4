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

class NullDevice final : public QuasiFS::Device {

public:
    NullDevice();
    ~NullDevice();

    static QuasiFS::dev_ptr Create() {
        return std::make_shared<NullDevice>();
    }

    s64 read(void* buf, u64 count) override;
    s64 write(const void* buf, u64 count) override;

    // clang-format off
    s32 ioctl(u64 cmd, Common::VaCtx* args) override { DEVICE_STUB(); }
    s64 lseek(s64 current, s64 offset, s32 whence) override { DEVICE_STUB(); }
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override { DEVICE_STUB(); }
    s32 fsync() override { DEVICE_STUB(); }
    s32 ftruncate(s64 length) override { DEVICE_STUB(); }
    s64 getdents(void* buf, u32 nbytes, s64 offset, s64* basep) override { DEVICE_STUB(); }
    // clang-format on
};

} // namespace Core::Devices