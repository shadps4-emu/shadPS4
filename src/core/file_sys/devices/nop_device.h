// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "base_device.h"

namespace Core::Devices {

class NopDevice final : BaseDevice {
    u32 handle;

public:
    explicit NopDevice(u32 handle) : handle(handle) {}

    ~NopDevice() override = default;

    int ioctl(u64 cmd, Common::VaCtx* args) override {
        return 0;
    }

    s64 write(const void* buf, size_t nbytes) override {
        return 0;
    }

    size_t readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override {
        return 0;
    }

    size_t writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) override {
        return 0;
    }

    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) override {
        return 0;
    }

    s64 lseek(s64 offset, int whence) override {
        return 0;
    }

    s64 read(void* buf, size_t nbytes) override {
        return 0;
    }

    int fstat(Libraries::Kernel::OrbisKernelStat* sb) override {
        return 0;
    }

    s32 fsync() override {
        return 0;
    }

    int ftruncate(s64 length) override {
        return 0;
    }

    int getdents(void* buf, u32 nbytes, s64* basep) override {
        return 0;
    }

    s64 pwrite(const void* buf, size_t nbytes, u64 offset) override {
        return 0;
    }
};

} // namespace Core::Devices
