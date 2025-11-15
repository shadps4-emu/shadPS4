// INAA License @marecl 2025

#include <errno.h>
#include <io.h>

#include "common/logging/log.h"
#include "src/core/file_sys/hostio/host_io_win32.h"

namespace HostIODriver {

HostIO_Win32::HostIO_Win32() = default;
HostIO_Win32::~HostIO_Win32() = default;

s32 HostIO_Win32::Open(const fs::path& path, s32 flags, u16 mode) {
    errno = 0;
    s32 status = _wopen(path.c_str(), ToWIN32OpenFlags(flags), mode);
    return status >= 0 ? status : -errno;
}

s32 HostIO_Win32::Creat(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = _wcreat(path.c_str(), mode);
    return status >= 0 ? status : -errno;
}

s32 HostIO_Win32::Close(const s32 fd) {
    errno = 0;
    s32 status = _close(fd);
    return 0 == status ? 0 : -errno;
}

// s32 HostIO_Win32::Link(const fs::path& src, const fs::path& dst) {
//     // errno = 0;
//     // s32 status = link(src.c_str(), dst.c_str());
//     // return 0 == status ? 0 : -errno;
// }

// s32 HostIO_Win32::Unlink(const fs::path& path) {
//     // errno = 0;
//     // s32 status = unlink(path.c_str());
//     // return 0 == status ? 0 : -errno;
// }

// s32 HostIO_Win32::LinkSymbolic(const fs::path& src, const fs:
//     // errno = 0;
//     // s32 status = symlink(src.c_str(), dst.c_str());
//     // return 0 == status ? 0 : -errno;
// }

s32 HostIO_Win32::Flush(const s32 fd) {
    errno = 0;
    return 0;
}

s32 HostIO_Win32::FSync(const s32 fd) {
    errno = 0;
    s32 status = _commit(fd);
    return 0 == status ? 0 : -errno;
}

s64 HostIO_Win32::LSeek(const s32 fd, s64 offset, s32 whence) {
    errno = 0;
    s32 status = _lseeki64(fd, offset, ToWIN32SeekOrigin(whence));
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::Tell(const s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

int HostIO_Win32::Truncate(const fs::path& path, u64 size) {
    errno = 0;
    s32 fd = _wopen(path.c_str(), _O_RDONLY);
    if (fd < 0)
        return -errno;
    s32 status = _chsize_s(fd, size);
    _close(fd);
    return status >= 0 ? status : -errno;
}

int HostIO_Win32::FTruncate(const s32 fd, u64 size) {
    errno = 0;
    s32 status = _chsize_s(fd, size);
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::Read(const s32 fd, void* buf, u64 count) {
    errno = 0;
    s32 status = _read(fd, buf, count);
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::PRead(const s32 fd, void* buf, u64 count, s64 offset) {
    errno = 0;
    s64 bak = LSeek(fd, 0, SeekOrigin::CURRENT);
    if (bak < 0)
        return -errno;
    LSeek(fd, offset, SeekOrigin::ORIGIN);

    s32 status = _read(fd, buf, count);

    LSeek(fd, bak, SeekOrigin::ORIGIN);
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::ReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt) {
    return 0;
}
s64 HostIO_Win32::PReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    return 0;
}

s64 HostIO_Win32::Write(const s32 fd, const void* buf, u64 count) {
    errno = 0;
    s32 status = _write(fd, buf, count);
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) {
    errno = 0;
    s64 bak = LSeek(fd, 0, SeekOrigin::CURRENT);
    if (bak < 0)
        return -errno;
    LSeek(fd, offset, SeekOrigin::ORIGIN);

    s32 status = _write(fd, buf, count);

    LSeek(fd, bak, SeekOrigin::ORIGIN);
    return status >= 0 ? status : -errno;
}

s64 HostIO_Win32::WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    return 0;
}
s64 HostIO_Win32::PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    return 0;
}

s32 HostIO_Win32::MKDir(const fs::path& path, u16 mode) {
    errno = 0;
    s32 status = _wmkdir(path.c_str());
    return status >= 0 ? status : -errno;
}

s32 HostIO_Win32::RMDir(const fs::path& path) {
    errno = 0;
    s32 status = _wrmdir(path.c_str());
    return status >= 0 ? status : -errno;
}

// s32 HostIO_Win32::Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) {}
// s32 HostIO_Win32::FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) {}

// s32 HostIO_Win32::Chmod(const fs::path& path, u16 mode) {}
// s32 HostIO_Win32::FChmod(const s32 fd, u16 mode) {}

} // namespace HostIODriver