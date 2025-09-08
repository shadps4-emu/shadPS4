// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/kernel/orbis_error.h>
#include "common/types.h"
#include "common/va_ctx.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
struct OrbisKernelIovec;
} // namespace Libraries::Kernel

namespace Core::Devices {

class BaseDevice {
public:
    explicit BaseDevice();

    virtual ~BaseDevice() = 0;

    virtual int ioctl(u64 cmd, Common::VaCtx* args) {
        return ORBIS_KERNEL_ERROR_ENOTTY;
    }

    virtual s64 write(const void* buf, size_t nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual size_t readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual size_t writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual size_t pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 lseek(s64 offset, int whence) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 read(void* buf, size_t nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual int fstat(Libraries::Kernel::OrbisKernelStat* sb) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s32 fsync() {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual int ftruncate(s64 length) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual int getdents(void* buf, u32 nbytes, s64* basep) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 pwrite(const void* buf, size_t nbytes, u64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
};

} // namespace Core::Devices
