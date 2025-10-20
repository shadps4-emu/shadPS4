// INAA License @marecl 2025

#pragma once

#ifdef __linux__

#include "core/file_sys/quasifs/quasi_types.h"

#include <sys/fcntl.h>
#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "core/libraries/kernel/file_system.h"
#include "src/host_io_base.h"

namespace HostIODriver {

/**
 * This driver bends time and space a bit, because it conforms to POSIX-like returns,
 * i.e. -1 for error, 0 for success
 * However there is no indication that the function executed (because it doesn't have to)
 */

class HostIO_POSIX : public HostIO_Base {

public:
    //
    // Conversion helpers
    //

    static constexpr int ToPOSIXSeekOrigin(QuasiFS::SeekOrigin origin) {
        switch (origin) {
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

    static constexpr int ToPOSIXOpenFlags(int quasi_flags) {
        int flags = 0;
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
        if (quasi_flags & QUASI_O_DIRECT)
            flags |= O_DIRECT;
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

    int Open(const fs::path& path, int flags, u16 mode = 0755) override;
    int Creat(const fs::path& path, u16 mode = 0755) override;
    int Close(const int fd) override;

    int LinkSymbolic(const fs::path& src, const fs::path& dst) override;
    int Link(const fs::path& src, const fs::path& dst) override;
    int Unlink(const fs::path& path) override;
    int Flush(const int fd) override;
    int FSync(const int fd) override;
    int Truncate(const fs::path& path, u64 size) override;
    int FTruncate(const int fd, u64 size) override;
    u64 LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin) override;
    s64 Tell(const int fd) override;
    s64 Write(const int fd, const void* buf, u64 count) override;
    s64 PWrite(const int fd, const void* buf, u64 count, u64 offset) override;
    s64 Read(const int fd, void* buf, u64 count) override;
    s64 PRead(const int fd, void* buf, u64 count, u64 offset) override;
    int MKDir(const fs::path& path, u16 mode = 0755) override;
    int RMDir(const fs::path& path) override;

    int Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) override;
    int FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf) override;

    int Chmod(const fs::path& path, u16 mode) override;
    int FChmod(const int fd, u16 mode) override;
};
} // namespace HostIODriver

#endif