// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <memory>
#include "base_device.h"

namespace Core::Devices {

class DeciTty6Device final : BaseDevice {
    u32 handle;

public:
    static std::shared_ptr<BaseDevice> Create(u32 handle, const char*, int, u16);
    explicit DeciTty6Device(u32 handle) : handle(handle) {}

    ~DeciTty6Device() override = default;

    int ioctl(u64 cmd, Common::VaCtx* args) override;
    s64 write(const void* buf, size_t nbytes) override;
    size_t readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override;
    size_t writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override;
    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) override;
    s64 lseek(s64 offset, int whence) override;
    s64 read(void* buf, size_t nbytes) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* sb) override;
    s32 fsync() override;
    int ftruncate(s64 length) override;
    int getdents(void* buf, u32 nbytes, s64* basep) override;
    s64 pwrite(const void* buf, size_t nbytes, u64 offset) override;
};

} // namespace Core::Devices