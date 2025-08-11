// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/assert.h"

#include "native_fs.h"

namespace Common::FS::Native {

int Open(const std::filesystem::path& path, int flags, int mode) {
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

constexpr int ToSeekOrigin(SeekOrigin origin) {
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

/**
 * >=0 on success
 * -1 on error
 */
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

/**
 * 1 if exists, 0 if not, -1 on error
 */
bool Exists(const std::filesystem::path& path) {
    struct stat statbuf{};
    return -1 != stat(path.c_str(), &statbuf);
}

bool Exists(const std::filesystem::path& path, std::error_code& ec) {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (-1 == stat(path.c_str(), &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    return true;
}

bool IsDirectory(const std::filesystem::path& path) {
    struct stat statbuf{};
    if (-1 == stat(path.c_str(), &statbuf))
        return false;

    return S_ISDIR(statbuf.st_mode);
}

bool IsDirectory(const std::filesystem::path& path, std::error_code& ec) {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (-1 == stat(path.c_str(), &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    return S_ISDIR(statbuf.st_mode);
}

bool CreateDirectory(const std::filesystem::path& path, int mode) {
    if (Exists(path))
        return false;
    return -1 == mkdir(path.c_str(), mode);
}

bool CreateDirectory(const std::filesystem::path& path, std::error_code& ec, int mode) {
    ec.clear();
    errno = 0;

    if (Exists(path))
        return false;

    if (-1 == mkdir(path.c_str(), mode)) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    return true;
}

bool CreateDirectories(const std::filesystem::path& path, int mode) {
    char sep = std::filesystem::path::preferred_separator;
    const std::string path_str = path.string();
    size_t path_idx = 0;

    bool made_something = false;
    bool end_reached = false;

    while (!end_reached) {
        path_idx = path_str.find(sep, path_idx + 1);
        const std::string part = path_str.substr(0, path_idx);
        if (std::string::npos == path_idx)
            end_reached = true;
        if (Exists(part))
            continue;
        if (CreateDirectory(part.c_str(), mode))
            made_something = true;
    }

    return made_something;
}

bool CreateDirectories(const std::filesystem::path& path, std::error_code& ec, int mode) {
    ec.clear();

    char sep = std::filesystem::path::preferred_separator;
    const std::string path_str = path.string();
    size_t path_idx = 0;

    std::error_code err{};
    bool made_something = false;
    bool end_reached = false;

    while (!end_reached) {
        path_idx = path_str.find(sep, path_idx + 1);
        const std::string part = path_str.substr(0, path_idx);
        if (std::string::npos == path_idx)
            end_reached = true;

        if (Exists(part, err))
            continue;

        if (err) {
            ec = err;
            return false;
        }

        if (CreateDirectory(part.c_str(), err, mode))
            made_something = true;

        if (err) {
            ec = err;
            return false;
        }
    }

    return made_something;
}

} // namespace Common::FS::Native
