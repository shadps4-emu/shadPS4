// INAA License @marecl 2025

#include <cstdint>
#include <cstdio>
#include <dirent.h>

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "../../quasifs/quasi_types.h"

#include "../../quasifs/quasifs_inode_quasi_device.h"
#include "../../quasifs/quasifs_inode_quasi_directory.h"
#include "../../quasifs/quasifs_inode_quasi_file.h"
#include "../../quasifs/quasifs_inode_symlink.h"
#include "../../quasifs/quasifs_partition.h"

#include "../host_io_posix.h"
#include "host_io_base.h"

namespace HostIODriver {

int HostIO_POSIX::Open(const fs::path& path, int flags, u16 mode) {
    errno = 0;
    int status = open(path.c_str(), ToPOSIXOpenFlags(flags), ToPOSIXOpenMode(mode));
    return status >= 0 ? status : -errno;
}

int HostIO_POSIX::Creat(const fs::path& path, u16 mode) {
    errno = 0;
    int status = creat(path.c_str(), ToPOSIXOpenMode(mode));
    return status >= 0 ? status : -errno;
}

int HostIO_POSIX::Close(const int fd) {
    errno = 0;
    int status = close(fd);
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    errno = 0;
    int status = symlink(src.c_str(), dst.c_str());
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::Link(const fs::path& src, const fs::path& dst) {
    errno = 0;
    int status = link(src.c_str(), dst.c_str());
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::Unlink(const fs::path& path) {
    errno = 0;
    int status = unlink(path.c_str());
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::Flush(const int fd) {
    errno = 0;
    return 0;
}

int HostIO_POSIX::FSync(const int fd) {
    errno = 0;
    int status = fsync(fd);
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::Truncate(const fs::path& path, u64 size) {
    errno = 0;
    int status = truncate(path.c_str(), size);
    return status >= 0 ? status : -errno;
}

int HostIO_POSIX::FTruncate(const int fd, u64 size) {
    errno = 0;
    int status = ftruncate(fd, size);
    return status >= 0 ? status : -errno;
}

u64 HostIO_POSIX::LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin) {
    errno = 0;
    int status = lseek(fd, offset, ToPOSIXSeekOrigin(origin));
    return status >= 0 ? status : -errno;
}

s64 HostIO_POSIX::Tell(const int fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

s64 HostIO_POSIX::Write(const int fd, const void* buf, u64 count) {
    errno = 0;
    int status = write(fd, buf, count);
    return status >= 0 ? status : -errno;
}

s64 HostIO_POSIX::PWrite(const int fd, const void* buf, u64 count, u64 offset) {
    errno = 0;
    int status = pwrite(fd, buf, count, offset);
    return status >= 0 ? status : -errno;
}

s64 HostIO_POSIX::Read(const int fd, void* buf, u64 count) {
    errno = 0;
    int status = read(fd, buf, count);
    return status >= 0 ? status : -errno;
}

s64 HostIO_POSIX::PRead(const int fd, void* buf, u64 count, u64 offset) {
    errno = 0;
    int status = pread(fd, buf, count, offset);
    return status >= 0 ? status : -errno;
}

int HostIO_POSIX::MKDir(const fs::path& path, u16 mode) {
    errno = 0;
    int status = mkdir(path.c_str(), mode);
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::RMDir(const fs::path& path) {
    errno = 0;
    int status = rmdir(path.c_str());
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) {
    errno = 0;

    struct stat st;

    if (int stat_status = stat(path.c_str(), &st); stat_status != 0)
        return stat_status;

    // handled by QFS
    // statbuf->st_dev = st.st_dev;
    // statbuf->st_ino = st.st_ino;
    statbuf->st_mode = st.st_mode;
    // statbuf->st_nlink = st.st_nlink;
    // statbuf->st_uid = st.st_uid; // always 0
    // statbuf->st_gid = st.st_gid; // always 0
    // statbuf->st_rdev = st.st_rdev;
    statbuf->st_atim.tv_sec = st.st_atim.tv_sec;
    statbuf->st_atim.tv_nsec = st.st_atim.tv_nsec;
    statbuf->st_mtim.tv_sec = st.st_mtim.tv_sec;
    statbuf->st_mtim.tv_nsec = st.st_mtim.tv_nsec;
    statbuf->st_ctim.tv_sec = st.st_ctim.tv_sec;
    statbuf->st_ctim.tv_nsec = st.st_ctim.tv_nsec;

    statbuf->st_size = st.st_size;
    statbuf->st_blocks = st.st_blocks;
    statbuf->st_blksize = st.st_blksize;

    // statbuf->st_flags = st.st_;
    // statbuf->st_gen = st.st_;
    // statbuf->st_lspare = st.st_;

    return 0;
}

int HostIO_POSIX::FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf) {
    errno = 0;

    struct stat st;

    if (int fstat_status = fstat(fd, &st); fstat_status != 0)
        return fstat_status;

    // handled by QFS
    // statbuf->st_dev = st.st_dev;
    // statbuf->st_ino = st.st_ino;
    // statbuf->st_nlink = st.st_nlink;

    // TODO: make working
    statbuf->st_mode = st.st_mode;
    statbuf->st_size = st.st_size;
    statbuf->st_blksize = st.st_blksize;
    statbuf->st_blocks = st.st_blocks;
    statbuf->st_atim.tv_sec = st.st_atim.tv_sec;
    statbuf->st_atim.tv_nsec = st.st_atim.tv_nsec;
    statbuf->st_mtim.tv_sec = st.st_mtim.tv_sec;
    statbuf->st_mtim.tv_nsec = st.st_mtim.tv_nsec;
    statbuf->st_ctim.tv_sec = st.st_ctim.tv_sec;
    statbuf->st_ctim.tv_nsec = st.st_ctim.tv_nsec;

    return 0;
}

int HostIO_POSIX::Chmod(const fs::path& path, u16 mode) {
    errno = 0;
    int status = chmod(path.c_str(), mode);
    return 0 == status ? status : -errno;
}

int HostIO_POSIX::FChmod(const int fd, u16 mode) {
    errno = 0;
    int status = fchmod(fd, mode);
    return 0 == status ? status : -errno;
}

} // namespace HostIODriver