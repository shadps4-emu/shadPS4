// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#ifdef _WIN32

#include <cstdint>
#include <unordered_map>
#include <fcntl.h>
#include <windows.h>

#include "common/logging/log.h"
#include "core/file_sys/quasifs/quasi_sys_fcntl.h"

#include "src/host_io_base.h"

namespace HostIODriver {

class HostIO_Win32 final : public HostIO_Base {

    std::unordered_map<fs::path, fs::path> symlinks;
    fs::path GetSymlink(fs::path path);

public:
    //
    // Conversion helpers
    //

    static constexpr s32 ToWIN32SeekOrigin(s32 origin) {
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

        if (quasi_flags & QUASI_O_DIRECTORY)
            flags |= _O_OBTAIN_DIR; // I don't like how this one looks

        if (int status = quasi_flags & (QUASI_O_NONBLOCK | QUASI_O_SYNC | QUASI_O_FSYNC |
                                        QUASI_O_DIRECT | QUASI_O_DSYNC);
            0 != status)
            LOG_WARNING(Kernel_Fs, "Received unknown or unsupported flags: {:x}", status);

        return flags;
    }

    HostIO_Win32();
    ~HostIO_Win32();

    s32 Open(const fs::path& path, s32 flags, u16 mode = 0755) override;
    s32 Creat(const fs::path& path, u16 mode = 0755) override;
    s32 Close(s32 fd) override;

    s32 Link(const fs::path& src, const fs::path& dst) override;
    // s32 LinkSymbolic(const fs::path& src, const fs::path& dst) override;
    s32 Unlink(const fs::path& path) override;
    s32 Remove(const fs::path& path) override;

    s32 Flush(s32 fd) override;
    s32 FSync(s32 fd) override;
    s64 LSeek(s32 fd, s64 offset, s32 whence) override;
    s64 Tell(s32 fd) override;

    s32 Truncate(const fs::path& path, u64 size) override;
    s32 FTruncate(s32 fd, u64 size) override;

    s64 Read(s32 fd, void* buf, u64 count) override;
    s64 PRead(s32 fd, void* buf, u64 count, s64 offset) override;
    s64 ReadV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PReadV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s64 Write(s32 fd, const void* buf, u64 count) override;
    s64 PWrite(s32 fd, const void* buf, u64 count, s64 offset) override;
    s64 WriteV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PWriteV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s32 MKDir(const fs::path& path, u16 mode = 0755) override;
    s32 RMDir(const fs::path& path) override;

    // s32 Stat(const fs::path& path, OrbisKernelStat* statbuf) override;
    // s32 FStat(s32 fd, OrbisKernelStat* statbuf) override;

    // s32 Chmod(const fs::path& path, u16 mode) override;
    // s32 FChmod(s32 fd, u16 mode) override;

    s32 Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
    s32 Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
};

} // namespace HostIODriver
#endif