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

s64 HostIO_Win32::LSeek(const s32 fd, u64 offset, QuasiFS::SeekOrigin origin) {
    errno = 0;
    s32 status = _lseeki64(fd, offset, ToWIN32SeekOrigin(origin));
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

} // namespace HostIODriver