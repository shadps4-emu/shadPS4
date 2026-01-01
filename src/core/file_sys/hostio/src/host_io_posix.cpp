// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <cstdint>
#include <cstdio>

#include <dirent.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include "core/file_sys/hostio/host_io_posix.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"

#include "host_io_base.h"
#include "host_io_linux2bsd.h"

namespace HostIODriver {

s32 HostIO_POSIX::Open(const fs::path& path, s32 flags, u16 mode) {
    errno = 0;
    s32 status = open(path.c_str(), ToPOSIXOpenFlags(flags), mode);
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Creat(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = creat(path.c_str(), mode);
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Close(const s32 fd) {
    errno = 0;
    s32 status = close(fd);
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Link(const fs::path& src, const fs::path& dst) {
    errno = 0;
    s32 status = link(src.c_str(), dst.c_str());
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    errno = 0;
    s32 status = symlink(src.c_str(), dst.c_str());
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Unlink(const fs::path& path) {
    errno = 0;
    s32 status = unlink(path.c_str());
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Remove(const fs::path& path) {
    errno = 0;
    s32 status = remove(path.c_str());
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Flush(const s32 fd) {
    errno = 0;
    return 0;
}

s32 HostIO_POSIX::FSync(const s32 fd) {
    errno = 0;
    s32 status = fsync(fd);
    return 0 == status ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::LSeek(const s32 fd, s64 offset, s32 whence) {
    errno = 0;
    s32 status = lseek(fd, offset, ToPOSIXSeekOrigin(whence));
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::Tell(const s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

s32 HostIO_POSIX::Truncate(const fs::path& path, u64 size) {
    errno = 0;
    s32 status = truncate(path.c_str(), size);
    return status >= 0 ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::FTruncate(const s32 fd, u64 size) {
    errno = 0;
    s32 status = ftruncate(fd, size);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::Read(const s32 fd, void* buf, u64 count) {
    errno = 0;
    s32 status = read(fd, buf, count);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::PRead(const s32 fd, void* buf, u64 count, s64 offset) {
    errno = 0;
    s32 status = pread(fd, buf, count, offset);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::ReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    errno = 0;

    iovec* iov_native = new iovec[iovcnt];
    for (s32 idx = 0; idx < iovcnt; idx++)
        iov_native[idx] = {.iov_base = iov[idx].iov_base, .iov_len = iov[idx].iov_len};

    s64 ret = readv(fd, iov_native, iovcnt);
    delete[] iov_native;

    return ret;
}

s64 HostIO_POSIX::PReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    errno = 0;

    iovec* iov_native = new iovec[iovcnt];
    for (s32 idx = 0; idx < iovcnt; idx++)
        iov_native[idx] = {.iov_base = iov[idx].iov_base, .iov_len = iov[idx].iov_len};

    s64 ret = preadv(fd, iov_native, iovcnt, offset);
    delete[] iov_native;

    return ret;
}

s64 HostIO_POSIX::Write(const s32 fd, const void* buf, u64 count) {
    errno = 0;
    s32 status = write(fd, buf, count);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) {
    errno = 0;
    s32 status = pwrite(fd, buf, count, offset);
    return status >= 0 ? status : -unix2bsd(errno);
}

s64 HostIO_POSIX::WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    errno = 0;

    iovec* iov_native = new iovec[iovcnt];
    for (s32 idx = 0; idx < iovcnt; idx++)
        iov_native[idx] = {.iov_base = iov[idx].iov_base, .iov_len = iov[idx].iov_len};

    s64 ret = writev(fd, iov_native, iovcnt);
    delete[] iov_native;

    return ret;
}

s64 HostIO_POSIX::PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    errno = 0;

    iovec* iov_native = new iovec[iovcnt];
    for (s32 idx = 0; idx < iovcnt; idx++)
        iov_native[idx] = {.iov_base = iov[idx].iov_base, .iov_len = iov[idx].iov_len};

    s64 ret = pwritev(fd, iov_native, iovcnt, offset);
    delete[] iov_native;

    return ret;
}

s32 HostIO_POSIX::MKDir(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = mkdir(path.c_str(), mode);
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::RMDir(const fs::path& path) {
    errno = 0;
    s32 status = rmdir(path.c_str());
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Stat(const fs::path& path, OrbisKernelStat* statbuf) {
    errno = 0;

    struct stat st;

    if (int stat_status = stat(path.c_str(), &st); stat_status != 0)
        return stat_status;

    // handled by QFS
    // statbuf->st_dev = st.st_dev;
    // statbuf->st_ino = st.st_ino;
    statbuf->st_mode = st.st_mode;
    // statbuf->st_nlink = st.st_nlink;
    statbuf->st_uid = 0; // st.st_uid; // always 0
    statbuf->st_gid = 0; // st.st_gid; // always 0
    // statbuf->st_rdev = st.st_rdev;
    statbuf->st_atim.tv_sec = st.st_atim.tv_sec;     //
    statbuf->st_atim.tv_nsec = 0;                    // st.st_atim.tv_nsec;
    statbuf->st_mtim.tv_sec = st.st_mtim.tv_sec;     //
    statbuf->st_mtim.tv_nsec = 0;                    // st.st_mtim.tv_nsec;
    statbuf->st_ctim.tv_sec = st.st_ctim.tv_sec;     //
    statbuf->st_ctim.tv_nsec = 0;                    // st.st_ctim.tv_nsec;
    statbuf->st_birthtim.tv_sec = st.st_ctim.tv_sec; // just assuming these are the same
    statbuf->st_birthtim.tv_nsec = 0;                // st.st_ctim.tv_nsec;

    statbuf->st_size = st.st_size;
    // statbuf->st_blocks = st.st_blocks;
    // statbuf->st_blksize = st.st_blksize;

    // statbuf->st_flags = st.st_;
    // statbuf->st_gen = st.st_;
    // statbuf->st_lspare = st.st_;

    return 0;
}

s32 HostIO_POSIX::FStat(const s32 fd, OrbisKernelStat* statbuf) {
    errno = 0;

    struct stat st;

    if (int fstat_status = fstat(fd, &st); fstat_status != 0)
        return fstat_status;

    // handled by QFS
    // statbuf->st_dev = st.st_dev;
    // statbuf->st_ino = st.st_ino;
    statbuf->st_mode = st.st_mode;
    // statbuf->st_nlink = st.st_nlink;
    statbuf->st_uid = 0; // st.st_uid; // always 0
    statbuf->st_gid = 0; // st.st_gid; // always 0
    // statbuf->st_rdev = st.st_rdev;
    statbuf->st_atim.tv_sec = st.st_atim.tv_sec;     //
    statbuf->st_atim.tv_nsec = 0;                    // st.st_atim.tv_nsec;
    statbuf->st_mtim.tv_sec = st.st_mtim.tv_sec;     //
    statbuf->st_mtim.tv_nsec = 0;                    // st.st_mtim.tv_nsec;
    statbuf->st_ctim.tv_sec = st.st_ctim.tv_sec;     //
    statbuf->st_ctim.tv_nsec = 0;                    // st.st_ctim.tv_nsec;
    statbuf->st_birthtim.tv_sec = st.st_ctim.tv_sec; // just assuming these are the same
    statbuf->st_birthtim.tv_nsec = 0;                // st.st_ctim.tv_nsec;

    statbuf->st_size = st.st_size;
    // statbuf->st_blocks = st.st_blocks;
    // statbuf->st_blksize = st.st_blksize;

    // statbuf->st_flags = st.st_;
    // statbuf->st_gen = st.st_;
    // statbuf->st_lspare = st.st_;

    return 0;
}

s32 HostIO_POSIX::Chmod(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = chmod(path.c_str(), mode);
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::FChmod(const s32 fd, u16 mode) {
    errno = 0;
    s32 status = fchmod(fd, mode);
    return 0 == status ? status : -unix2bsd(errno);
}

s32 HostIO_POSIX::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    return -unix2bsd(ENOSYS);
}

s32 HostIO_POSIX::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    return -unix2bsd(ENOSYS);
}

// s32 HostIO_POSIX::GetDents(void* buf, u32 count, s64* basep) { return -POSIX_ENOSYS; }

} // namespace HostIODriver