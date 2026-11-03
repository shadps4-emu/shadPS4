// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include <memory>
#include "core/file_sys/devices/base_device.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core::Devices {

class NopDevice final : public BaseDevice {
    u32 handle;

public:
    static std::shared_ptr<BaseDevice> Create(u32 handle, const char*, s32, u16) {
        return std::make_shared<NopDevice>(handle);
    }

    explicit NopDevice(u32 handle) : handle(handle) {}

    ~NopDevice() override = default;

    s32 ioctl(u64 cmd, Common::VaCtx* args) override {
        return 0;
    }

    s64 write(const void* buf, u64 nbytes) override {
        return static_cast<s64>(nbytes);
    }

    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return 0;
    }

    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        if (!iov || iovcnt <= 0) {
            return 0;
        }
        s64 total = 0;
        for (s32 i = 0; i < iovcnt; ++i) {
            total += static_cast<s64>(iov[i].iov_len);
        }
        return total;
    }

    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return 0;
    }

    s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return writev(iov, iovcnt);
    }

    s64 lseek(s64 offset, s32 whence) override {
        return 0;
    }

    s64 read(void* buf, u64 nbytes) override {
        return 0;
    }

    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override {
        if (sb) {
            std::memset(sb, 0, sizeof(Libraries::Kernel::OrbisKernelStat));
            sb->st_mode = 0000666u | 0020000u; // S_IFCHR
            sb->st_nlink = 1;
        }
        return 0;
    }

    s32 fsync() override {
        return 0;
    }

    s32 ftruncate(s64 length) override {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    s64 getdents(void* buf, u32 nbytes, s64* basep) override {
        return ORBIS_KERNEL_ERROR_ENOTDIR;
    }

    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override {
        return static_cast<s64>(nbytes);
    }
};

} // namespace Core::Devices
