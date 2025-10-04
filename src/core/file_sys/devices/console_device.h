// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <memory>
#include "core/file_sys/devices/base_device.h"

namespace Core::Devices {

class ConsoleDevice final : public BaseDevice {
    u32 handle;

public:
    static std::shared_ptr<BaseDevice> Create(u32 handle, const char*, s32, u16);
    explicit ConsoleDevice(u32 handle) : handle(handle) {}

    ~ConsoleDevice() override = default;

    s32 ioctl(u64 cmd, Common::VaCtx* args) override;
    s64 write(const void* buf, u64 nbytes) override;
    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override;
    s64 lseek(s64 offset, s32 whence) override;
    s64 read(void* buf, u64 nbytes) override;
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override;
    s32 fsync() override;
    s32 ftruncate(s64 length) override;
    s64 getdents(void* buf, u32 nbytes, s64* basep) override;
    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override;
};

} // namespace Core::Devices