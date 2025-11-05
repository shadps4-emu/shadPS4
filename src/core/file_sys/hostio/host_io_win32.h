// INAA License @marecl 2025

#pragma once

#ifdef _WIN32

// #error unimplemented

#include <cstdint>
#include <unordered_map>
#include <fcntl.h>
#include <windows.h>

#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "src/host_io_base.h"

namespace HostIODriver {

class HostIO_Win32 final : public HostIO_Base {

public:
    //
    // Conversion helpers
    //

    static constexpr s32 ToWIN32SeekOrigin(QuasiFS::SeekOrigin origin) {
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

    static constexpr s32 ToWIN32OpenFlags(int quasi_flags) {
        s32 flags = 0;
        if (quasi_flags & QUASI_O_RDONLY)
            flags |= _O_RDONLY;
        if (quasi_flags & QUASI_O_WRONLY)
            flags |= _O_WRONLY;
        if (quasi_flags & QUASI_O_RDWR)
            flags |= _O_RDWR;
        if (quasi_flags & QUASI_O_CREAT)
            flags |= _O_CREAT;
        if (quasi_flags & QUASI_O_EXCL)
            flags |= _O_EXCL;
        if (quasi_flags & QUASI_O_TRUNC)
            flags |= _O_TRUNC;
        if (quasi_flags & QUASI_O_APPEND)
            flags |= _O_APPEND;

        // No info, maybe there are some equivalents
        // if (quasi_flags & QUASI_O_NONBLOCK)
        //     flags |= _O_NONBLOCK;
        // if (quasi_flags & QUASI_O_SYNC)
        //     flags |= _O_SYNC;
        // if (quasi_flags & QUASI_O_FSYNC)
        //     flags |= _O_FSYNC;

        if (quasi_flags & QUASI_O_DIRECTORY)
            flags |= _O_OBTAIN_DIR; // I don't like how this one looks

        // if (quasi_flags & QUASI_O_DIRECT)
        //     flags |= _O_DIRECT;
        // if (quasi_flags & QUASI_O_DSYNC)
        //     flags |= _O_DSYNC;

        return flags;
    }

    HostIO_Win32();
    ~HostIO_Win32();

    s32 Open(const fs::path& path, s32 flags, u16 mode = 0755) override;
    s32 Creat(const fs::path& path, u16 mode = 0755) override;
    s32 Close(const s32 fd) override;

    // s32 Link(const fs::path& src, const fs::path& dst) override;
    // s32 Unlink(const fs::path& path) override;
    // s32 LinkSymbolic(const fs::path& src, const fs::path& dst) override;

    s32 Flush(const s32 fd) override;
    s32 FSync(const s32 fd) override;
    s64 LSeek(const s32 fd, u64 offset, QuasiFS::SeekOrigin origin) override;
    s64 Tell(const s32 fd) override;

    s32 Truncate(const fs::path& path, u64 size) override;
    s32 FTruncate(const s32 fd, u64 size) override;

    s64 Write(const s32 fd, const void* buf, u64 count) override;
    s64 Read(const s32 fd, void* buf, u64 count) override;

    s64 PWrite(const s32 fd, const void* buf, u64 count, u64 offset) override;
    s64 PRead(const s32 fd, void* buf, u64 count, u64 offset) override;

    s32 MKDir(const fs::path& path, u16 mode = 0755) override;
    s32 RMDir(const fs::path& path) override;

    // s32 Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) override;
    // s32 FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) override;

    // s32 Chmod(const fs::path& path, u16 mode) override;
    // s32 FChmod(const s32 fd, u16 mode) override;
};

} // namespace HostIODriver
#endif