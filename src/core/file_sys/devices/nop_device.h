// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "core/file_sys/devices/base_device.h"

namespace Core::Devices {

class NopDevice final : BaseDevice {
    u32 handle;

public:
    explicit NopDevice(u32 handle) : handle(handle) {}

    ~NopDevice() override = default;

    s32 ioctl(u64 cmd, Common::VaCtx* args) override {
        return 0;
    }

    s64 write(const void* buf, u64 nbytes) override {
        return 0;
    }

    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return 0;
    }

    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return 0;
    }

    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return 0;
    }

    s64 lseek(s64 offset, s32 whence) override {
        return 0;
    }

    s64 read(void* buf, u64 nbytes) override {
        return 0;
    }

    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override {
        return 0;
    }

    s32 fsync() override {
        return 0;
    }

    s32 ftruncate(s64 length) override {
        return 0;
    }

    s64 getdents(void* buf, u32 nbytes, s64* basep) override {
        return 0;
    }

    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override {
        return 0;
    }
};

} // namespace Core::Devices
