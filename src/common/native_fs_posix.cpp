// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/assert.h"

#include "native_fs.h"

namespace Common::FS::Native {

namespace fs = std::filesystem;

int Open(const fs::path& path, int flags, int mode) {
    errno = 0;
    return open(path.c_str(), flags, mode);
}

bool Close(const int fd) {
    errno = 0;
    return close(fd) == 0;
}

bool Unlink(const fs::path path) {
    errno = 0;
    return unlink(path.c_str()) == 0;
}

bool Flush(const int fd) {
    // POSIX rawdogs the file descriptor
    errno = 0;
    return true;
}

bool Commit(const int fd) {
    errno = 0;
    return fsync(fd) == 0;
}

bool SetSize(const int fd, u64 size) {
    errno = 0;
    return ftruncate(fd, static_cast<s64>(size)) == 0;
}

u64 GetSize(const int fd) {
    errno = 0;
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
    errno = 0;
    return lseek(fd, 0, SEEK_CUR);
}

s64 Write(int __fd, const void* __buf, size_t __n) {
    errno = 0;
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
u64 GetDirectorySize(const fs::path& path) {
    u64 total = 0;

    struct stat statbuf{};
    const int file_size = statbuf.st_size;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (0 != stat(entry.path().c_str(), &statbuf))
            return -1;

        if (S_ISREG(statbuf.st_mode)) {
            total += statbuf.st_size;
        }
    }
    return total;
}

// Semi C++ mess
// Most used functions are throwable, so we need to follow this
// This applies for C++ std::filesystem ports, but should also be applied to calls above

bool Exists(const fs::path& path) {
    errno = 0;
    struct stat statbuf{};
    if (0 == stat(path.c_str(), &statbuf))
        return true;

    if (ENOENT == errno)
        return false;

    throw fs::filesystem_error("Exception: Exists: ", path,
                               std::error_code{errno, std::generic_category()});
    return false;
}

bool Exists(const fs::path& path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (0 == stat(path.c_str(), &statbuf))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool IsDirectory(const fs::path& path) {

    std::error_code ec{};
    bool is_directory = IsDirectory(path, ec);

    if (!ec) {
        return is_directory;
    }

    throw fs::filesystem_error("Exception: IsDirectory: ", path,
                               std::error_code{errno, std::generic_category()});
}

bool IsDirectory(const fs::path& path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (-1 == stat(path.c_str(), &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    return S_ISDIR(statbuf.st_mode);
}

bool CreateDirectory(const fs::path& path, int mode) {
    std::error_code ec{};
    bool directory_created = CreateDirectory(path, ec, mode);

    if (!ec)
        return directory_created;

    throw fs::filesystem_error("Exception: CreateDirectory: ", path,
                               std::error_code{errno, std::generic_category()});
    return false;
}

bool CreateDirectory(const fs::path& path, std::error_code& ec, int mode) noexcept {
    ec.clear();

    std::error_code err{};
    if (Exists(path, err))
        return false;

    if (err) {
        ec = err;
        return false;
    }

    errno = 0;
    if (0 == mkdir(path.c_str(), mode))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool CreateDirectories(const fs::path& path, int mode) {
    std::error_code ec{};
    bool directories_created = CreateDirectories(path, ec, mode);

    if (!ec)
        return directories_created;

    throw fs::filesystem_error("Exception: CreateDirectories: ", path,
                               std::error_code{errno, std::generic_category()});
}

bool CreateDirectories(const fs::path& path, std::error_code& ec, int mode) noexcept {
    ec.clear();

    char sep = fs::path::preferred_separator;
    const std::string path_str = path.string();
    size_t path_idx = 0;

    std::error_code err{};
    bool directories_created = false;
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
            directories_created = true;

        if (err) {
            ec = err;
            return false;
        }
    }

    return directories_created;
}

fs::path AbsolutePath(const fs::path& path) {
    std::error_code ec{};
    fs::path resolved_path = AbsolutePath(path, ec);

    if (!ec)
        return resolved_path;

    throw fs::filesystem_error("Exception: AbsolutePath: ", path, ec);
}

fs::path AbsolutePath(const fs::path& path, std::error_code& ec) noexcept {
    errno = 0;

    char* resolved_path = realpath(path.c_str(), nullptr);
    if (resolved_path == NULL) {
        ec = std::error_code{errno, std::generic_category()};
        return fs::path{};
    }

    fs::path _resolved_path{resolved_path};
    free(resolved_path); // man(3) realpath
    return _resolved_path;
}

} // namespace Common::FS::Native
