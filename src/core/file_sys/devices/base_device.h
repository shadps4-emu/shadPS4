// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core {
struct VmObject;
enum class MemoryProt : u8;
enum class MemoryMapFlags : u32;
} // namespace Core

namespace Libraries::Kernel {
struct OrbisKernelStat;
struct OrbisKernelIovec;
} // namespace Libraries::Kernel

namespace Core::Devices {

class BaseDevice {
public:
    explicit BaseDevice();

    virtual ~BaseDevice() = 0;

    virtual s32 ioctl(u32 cmd, void* args) {
        return ORBIS_KERNEL_ERROR_ENOTTY;
    }

    virtual s32 mmap(u64 offset, u64 size, Core::MemoryProt prot, Core::MemoryProt* max_prot,
                     Core::MemoryMapFlags flags, std::shared_ptr<VmObject>* out_object) {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    virtual s64 write(const void* buf, u64 nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 lseek(s64 offset, int whence) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 read(void* buf, u64 nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s32 fsync() {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s32 ftruncate(s64 length) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 getdents(void* buf, u32 nbytes, s64* basep) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 pwrite(const void* buf, u64 nbytes, s64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
};

} // namespace Core::Devices
