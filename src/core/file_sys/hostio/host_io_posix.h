// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#ifdef __linux__

#include <sys/fcntl.h>
#include     <sys/uio.h>

#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/libraries/kernel/file_system.h"

#include "src/host_io_base.h"

namespace HostIODriver {

static_assert(std::is_same_v<decltype(OrbisKernelIovec::iov_base), decltype(iovec::iov_base)>,
              "IOV base pointer is incompatible with native call");
static_assert(std::is_same_v<decltype(OrbisKernelIovec::iov_len), decltype(iovec::iov_len)>,
              "IOV length specifier is incompatible with native call");

/**
 * This driver bends time and space a bit, because it conforms to POSIX-like returns,
 * i.e. -1 for error, 0 for success
 * However there is no indication that the function executed (because it doesn't have to)
 */

class HostIO_POSIX final : public HostIO_Base {

public:
    //
    // Conversion helpers
    //

    static constexpr s32 ToPOSIXSeekOrigin(s32 whence) {
        switch (whence) {
        case QuasiFS::SeekOrigin::ORIGIN:
            return SEEK_SET;
        case QuasiFS::SeekOrigin::CURRENT:
            return SEEK_CUR;
        case QuasiFS::SeekOrigin::END:
            return SEEK_END;
        default:
            return -1;
        }
    }

    static constexpr s32 ToPOSIXOpenFlags(int quasi_flags) {
        s32 flags = 0;
        if (quasi_flags & QUASI_O_RDONLY)
            flags |= O_RDONLY;
        if (quasi_flags & QUASI_O_WRONLY)
            flags |= O_WRONLY;
        if (quasi_flags & QUASI_O_RDWR)
            flags |= O_RDWR;
        if (quasi_flags & QUASI_O_CREAT)
            flags |= O_CREAT;
        if (quasi_flags & QUASI_O_EXCL)
            flags |= O_EXCL;
        if (quasi_flags & QUASI_O_TRUNC)
            flags |= O_TRUNC;
        if (quasi_flags & QUASI_O_APPEND)
            flags |= O_APPEND;
        if (quasi_flags & QUASI_O_NONBLOCK)
            flags |= O_NONBLOCK;
        if (quasi_flags & QUASI_O_SYNC)
            flags |= O_SYNC;
        if (quasi_flags & QUASI_O_FSYNC)
            flags |= O_FSYNC;
        if (quasi_flags & QUASI_O_DIRECTORY)
            flags |= O_DIRECTORY;
#ifndef __APPLE_CC__
        if (quasi_flags & QUASI_O_DIRECT)
            flags |= O_DIRECT;
#endif
        if (quasi_flags & QUASI_O_DSYNC)
            flags |= O_DSYNC;

        return flags;
    }

    static constexpr mode_t ToPOSIXOpenMode(u16 quasi_mode) {
        return quasi_mode;
    }

    //
    // POSIX Functions
    //

    s32 Open(const fs::path& path, s32 flags, u16 mode = 0755) override;
    s32 Creat(const fs::path& path, u16 mode = 0755) override;
    s32 Close(const s32 fd) override;

    s32 Link(const fs::path& src, const fs::path& dst) override;
    s32 LinkSymbolic(const fs::path& src, const fs::path& dst) override;
    s32 Unlink(const fs::path& path) override;
    s32 Remove(const fs::path& path) override;

    s32 Flush(const s32 fd) override;
    s32 FSync(const s32 fd) override;
    s64 LSeek(const s32 fd, s64 offset, s32 whence) override;
    s64 Tell(const s32 fd) override;

    s32 Truncate(const fs::path& path, u64 size) override;
    s32 FTruncate(const s32 fd, u64 size) override;

    s64 Read(const s32 fd, void* buf, u64 count) override;
    s64 PRead(const s32 fd, void* buf, u64 count, s64 offset) override;
    s64 ReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s64 Write(const s32 fd, const void* buf, u64 count) override;
    s64 PWrite(const s32 fd, const void* buf, u64 count, s64 offset) override;
    s64 WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s32 MKDir(const fs::path& path, u16 mode = 0755) override;
    s32 RMDir(const fs::path& path) override;

    s32 Stat(const fs::path& path, OrbisKernelStat* statbuf) override;
    s32 FStat(const s32 fd, OrbisKernelStat* statbuf) override;

    s32 Chmod(const fs::path& path, u16 mode) override;
    s32 FChmod(const s32 fd, u16 mode) override;

    s32 Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
    s32 Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
};
} // namespace HostIODriver

#endif
