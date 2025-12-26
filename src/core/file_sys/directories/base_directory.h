// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <vector>
#include "common/types.h"
#include "common/va_ctx.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
struct OrbisKernelIovec;
struct OrbisKernelDirent;
} // namespace Libraries::Kernel

namespace Core::Directories {

class BaseDirectory {
protected:
    static inline u32 fileno_pool{10};

    static u32 next_fileno() {
        return ++fileno_pool;
    }

    s64 file_offset = 0;
    u64 directory_size = 0;
    std::vector<u8> dirent_cache_bin{};

public:
    explicit BaseDirectory();

    virtual ~BaseDirectory() = 0;

    virtual s64 read(void* buf, u64 nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt);
    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset);

    virtual s64 write(const void* buf, u64 nbytes) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 lseek(s64 offset, s32 whence);

    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* stat) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    virtual s64 getdents(void* buf, u64 nbytes, s64* basep) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
};

} // namespace Core::Directories