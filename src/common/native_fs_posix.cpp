// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <format>

#include <errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/assert.h"

#include "native_fs.h"

// Order is reversed. `noexcept` contains the meat, throwables are wrappers catching error codes

namespace Common::FS::Native {

namespace fs = std::filesystem;

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

int Open(const fs::path path, std::error_code& ec, int flags, int mode) noexcept {
    ec.clear();
    errno = 0;

    int new_fd = open(path.c_str(), flags, mode);
    if (-1 == new_fd)
        ec = std::error_code{errno, std::generic_category()};

    return new_fd;
}

bool IsOpen(const int fd) noexcept {
    int fcntl_ret = fcntl(fd, F_GETFD);
    // EBADF = fd not opened
    return fcntl_ret != -1 || errno != EBADF;
}

bool Close(const int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    if (0 == close(fd))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool Unlink(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    if (0 == unlink(path.c_str()))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool Flush(const int fd, std::error_code& ec) noexcept {
    // No buffering for file descriptor
    ec.clear();
    errno = 0;
    return true;
}

bool Commit(const int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    if (0 == fsync(fd))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool SetSize(const int fd, std::error_code& ec, u64 size) noexcept {
    ec.clear();
    errno = 0;

    if (0 == ftruncate(fd, static_cast<s64>(size)))
        return true;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

size_t GetSize(const int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;
    struct stat statbuf{};
    if (0 != fstat(fd, &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return -1;
    }

    return statbuf.st_size;
}

size_t GetSize(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;
    struct stat statbuf{};
    if (0 == stat(path.c_str(), &statbuf))
        return statbuf.st_size;

    ec = std::error_code{errno, std::generic_category()};
    return -1;
}

bool Seek(const int fd, std::error_code& ec, s64 offset, SeekOrigin origin) noexcept {
    std::error_code err{};
    int _origin = ToSeekOrigin(origin);
    s64 seek_start = Tell(fd, err);

    if (err) {
        ec = err;
        return false;
    }

    if (-1 == seek_start)
        return false;

    errno = 0;
    const off_t seek_pos = lseek(fd, offset, _origin);
    if (-1 == seek_pos) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    switch (_origin) {
    case SEEK_SET:
        return seek_pos == offset;
        break;
    case SEEK_END: {
        u64 tmp = GetSize(fd, err);
        if (err) {
            ec = err;
            return false;
        }
        return seek_pos == tmp;
    } break;
    case SEEK_CUR:
        return seek_start == (seek_pos + offset);
        break;
    }
    UNREACHABLE_MSG("Seek {}", static_cast<u32>(origin));
}

size_t Tell(int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    size_t pointer_position = lseek(fd, 0, SEEK_CUR);
    if (-1 == pointer_position)
        ec = std::error_code{errno, std::generic_category()};

    return pointer_position;
}

size_t Write(int __fd, std::error_code& ec, const void* __buf, size_t __n) noexcept {
    ec.clear();
    errno = 0;

    const size_t bytes_written = write(__fd, __buf, __n);
    if (bytes_written != __n)
        ec = std::error_code{errno, std::generic_category()};

    return bytes_written;
}

size_t Read(int __fd, std::error_code& ec, void* __buf, size_t __n) noexcept {
    ec.clear();
    errno = 0;

    const size_t bytes_read = read(__fd, __buf, __n);
    if (bytes_read != __n)
        ec = std::error_code{errno, std::generic_category()};

    return bytes_read;
}

size_t GetDirectorySize(const fs::path path) noexcept {
    size_t total = 0;

    struct stat statbuf{};
    const int file_size = statbuf.st_size;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        errno = 0;
        if (0 != stat(entry.path().c_str(), &statbuf)) {
            return -1;
        }

        if (S_ISREG(statbuf.st_mode)) {
            total += statbuf.st_size;
        }
    }
    return total;
}

bool Exists(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (0 == stat(path.c_str(), &statbuf))
        return true;

    if (ENOENT == errno)
        return false;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool IsDirectory(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    struct stat statbuf{};
    if (0 == stat(path.c_str(), &statbuf))
        return S_ISDIR(statbuf.st_mode);

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

fs::path AbsolutePath(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
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

bool Remove(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    if (0 == remove(path.c_str()))
        return true;

    if (ENOENT == errno)
        return false;

    ec = std::error_code{errno, std::generic_category()};
    return false;
}

bool CreateDirectory(const fs::path path, std::error_code& ec, int mode) noexcept {
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

bool CreateDirectories(const fs::path path, std::error_code& ec, int mode) noexcept {
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

} // namespace Common::FS::Native
