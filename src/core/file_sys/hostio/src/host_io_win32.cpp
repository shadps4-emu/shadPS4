// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <errno.h>
#include <io.h>
#include <winerror.h>

#include "common/logging/log.h"
#include "src/core/file_sys/hostio/host_io_win32.h"

#include "host_io_linux2bsd.h"
#include "host_io_win2bsd.h"

namespace HostIODriver {

HostIO_Win32::HostIO_Win32() {
    this->symlinks.clear();
    this->symlinks.reserve(16);
}

HostIO_Win32::~HostIO_Win32() = default;

s32 HostIO_Win32::Open(const fs::path& path, s32 flags, u16 mode) {
    errno = 0;
    s32 status = _wopen(GetSymlink(path).c_str(), ToWIN32OpenFlags(flags), mode);
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_Win32::Creat(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = _wcreat(GetSymlink(path).c_str(), mode);
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_Win32::Close(const s32 fd) {
    errno = 0;
    s32 status = _close(fd);
    return 0 == status ? 0 : -unix2bsd(errno);
}

s32 HostIO_Win32::Link(const fs::path& src, const fs::path& dst) {
    errno = 0;
    s32 status = CreateHardLinkW(dst.c_str(), GetSymlink(src).c_str(), nullptr);
    return 0 == status ? 0 : -win2bsd(GetLastError());
}

// s32 HostIO_Win32::LinkSymbolic(const fs::path& src, const fs:
//     // errno = 0;
//     // s32 status = symlink(src.c_str(), dst.c_str());
//     // return 0 == status ? 0 :-unix2bsd(errno);
// }

s32 HostIO_Win32::Unlink(const fs::path& path) {
    errno = 0;
    s32 status = DeleteFileW(GetSymlink(path).c_str());

    return status > 0 ? status : -win2bsd(GetLastError());
}

s32 HostIO_Win32::Remove(const fs::path& path) {
    errno = 0;
    if (int status = DeleteFileW(GetSymlink(path).c_str()); status > 0)
        return 0;

    auto last_error = GetLastError();

    // won't happen if file got removed correctly
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND)
        return -win2bsd(last_error);

    auto status = RemoveDirectoryW(GetSymlink(path).c_str());
    return status > 0 ? status : -win2bsd(GetLastError());
}

s32 HostIO_Win32::Flush(const s32 fd) {
    errno = 0;
    return 0;
}

s32 HostIO_Win32::FSync(const s32 fd) {
    errno = 0;
    s32 status = _commit(fd);
    return 0 == status ? 0 : -unix2bsd(errno);
}

s64 HostIO_Win32::LSeek(const s32 fd, s64 offset, s32 whence) {
    errno = 0;
    s32 status = _lseeki64(fd, offset, ToWIN32SeekOrigin(whence));
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::Tell(const s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

int HostIO_Win32::Truncate(const fs::path& path, u64 size) {
    errno = 0;
    s32 fd = _wopen(GetSymlink(path).c_str(), _O_RDONLY);
    if (fd < 0)
        return -unix2bsd(errno);
    s32 status = _chsize_s(fd, size);
    _close(fd);
    return status >= 0 ? status : -unix2bsd(errno);
}

int HostIO_Win32::FTruncate(const s32 fd, u64 size) {
    errno = 0;
    s32 status = _chsize_s(fd, size);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::Read(const s32 fd, void* buf, u64 count) {
    errno = 0;
    s32 status = _read(fd, buf, count);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::PRead(const s32 fd, void* buf, u64 count, s64 offset) {
    errno = 0;
    s64 bak = LSeek(fd, 0, SeekOrigin::CURRENT);
    if (bak < 0)
        return -unix2bsd(errno);
    LSeek(fd, offset, SeekOrigin::ORIGIN);

    s32 status = _read(fd, buf, count);

    LSeek(fd, bak, SeekOrigin::ORIGIN);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::ReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    return 0;
}
s64 HostIO_Win32::PReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    return 0;
}

s64 HostIO_Win32::Write(const s32 fd, const void* buf, u64 count) {
    errno = 0;
    s32 status = _write(fd, buf, count);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) {
    errno = 0;
    s64 bak = LSeek(fd, 0, SeekOrigin::CURRENT);
    if (bak < 0)
        return -unix2bsd(errno);
    LSeek(fd, offset, SeekOrigin::ORIGIN);

    s32 status = _write(fd, buf, count);

    LSeek(fd, bak, SeekOrigin::ORIGIN);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_Win32::WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    return 0;
}
s64 HostIO_Win32::PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    return 0;
}

s32 HostIO_Win32::MKDir(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = _wmkdir(GetSymlink(path).c_str());
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_Win32::RMDir(const fs::path& path) {
    errno = 0;
    s32 status = _wrmdir(GetSymlink(path).c_str());
    return status >= 0 ? status : -unix2bsd(errno);
}

// s32 HostIO_Win32::Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) {}
// s32 HostIO_Win32::FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) {}

// s32 HostIO_Win32::Chmod(const fs::path& path, u16 mode) {}
// s32 HostIO_Win32::FChmod(const s32 fd, u16 mode) {}

s32 HostIO_Win32::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    errno = 0;
    auto status = CopyFileW(GetSymlink(src).c_str(), dst.c_str(), fail_if_exists);
    return status > 0 ? 0 : -win2bsd(GetLastError());
}

s32 HostIO_Win32::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    errno = 0;
    // TODO: this will fail if moving/merging directories
    // check behaviour
    auto status = MoveFileWithProgressW(GetSymlink(src).c_str(), dst.c_str(), nullptr, nullptr,
                                        MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH |
                                            (MOVEFILE_REPLACE_EXISTING * !fail_if_exists));
    return status > 0 ? 0 : -win2bsd(GetLastError());
}

fs::path HostIO_Win32::GetSymlink(fs::path path) {
    // this is a very, **very** bad implementation of symlinks
    // may work for files, but certainly not for directories
    // in fact, i am terribly ashamed of going this route, but
    // since games don't use symlinks often, i can get away with this
    auto sym = this->symlinks.find(path);
    if (this->symlinks.end() == sym)
        return path;
    return sym->second;
}

} // namespace HostIODriver