// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/path_util.h"
#include "core/libraries/kernel/file_system.h"

#include "native_fs.h"

namespace Common::FS::Native {

[[nodiscard]] int Open(const std::filesystem::path& path, int flags, int mode) {
    return open(path.c_str(), flags, mode);
}

bool Close(const int fd) {
    return close(fd) == 0;
}

bool Unlink(const std::filesystem::path path) {
    return unlink(path.c_str()) == 0;
}

bool Flush(const int fd) {
    // POSIX rawdogs the file descriptor
    return true;
}

bool Commit(const int fd) {
    return fsync(fd) == 0;
}

bool SetSize(const int fd, u64 size) {
    return ftruncate(fd, static_cast<s64>(size)) == 0;
}

u64 GetSize(const int fd) {
    struct stat statbuf{};
    if (0 != fstat(fd, &statbuf))
        return -1;
    return statbuf.st_size;
}

bool Seek(const int fd, s64 offset, SeekOrigin origin) {
    int _origin = ToSeekOrigin(origin);
    s64 seek_start = Tell(fd);
    if (-1 == seek_start)
        return false;

    const off_t seek_pos = lseek(fd, offset, _origin);
    if (-1 == seek_pos)
        return false;

    switch (_origin) {
    case SEEK_SET:
        return seek_pos == offset;
        break;
    case SEEK_END: {
        u64 tmp = GetSize(fd);
        return (-1 == tmp) ? false : seek_pos == tmp;
    } break;
    case SEEK_CUR:
        return seek_start == (seek_pos + offset);
        break;
    }
    UNREACHABLE_MSG("Seek {}", static_cast<u32>(origin));
}

s64 Tell(int fd) {
    return lseek(fd, 0, SEEK_CUR);
}

s64 Write(int __fd, const void* __buf, size_t __n) {
    return write(__fd, __buf, __n);
}

s64 Read(int __fd, void* __buf, size_t __n) {
    return read(__fd, __buf, __n);
}

[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin) {
    switch (origin) {
    case SeekOrigin::SetOrigin:
        return SEEK_SET;
    case SeekOrigin::CurrentPosition:
        return SEEK_CUR;
    case SeekOrigin::End:
        return SEEK_END;
    default:
        UNREACHABLE_MSG("Impossible SeekOrigin {}", static_cast<u32>(origin));
    }
}

u64 GetDirectorySize(const std::filesystem::path& path) {
    u64 total = 0;

    struct stat statbuf{};
    const int file_size = statbuf.st_size;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (0 != stat(entry.path().c_str(), &statbuf))
            return -1;

        if (S_ISREG(statbuf.st_mode)) {
            total += statbuf.st_size;
        }
    }
    return total;
}

} // namespace Common::FS::Native
