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
    if (0 == new_fd)
        ec = std::error_code{errno, std::generic_category()};

    return new_fd;
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

u64 GetSize(const int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;
    struct stat statbuf{};
    if (0 != fstat(fd, &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return -1;
    }

    return statbuf.st_size;
}

u64 GetSize(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;
    struct stat statbuf{};
    if (0 != stat(path.c_str(), &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return -1;
    }

    return statbuf.st_size;
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
        return (-1 == tmp) ? false : seek_pos == tmp;
    } break;
    case SEEK_CUR:
        return seek_start == (seek_pos + offset);
        break;
    }
    UNREACHABLE_MSG("Seek {}", static_cast<u32>(origin));
}

s64 Tell(int fd, std::error_code& ec) noexcept {
    ec.clear();
    errno = 0;

    s64 pointer_position = lseek(fd, 0, SEEK_CUR);
    if (-1 == pointer_position)
        ec = std::error_code{errno, std::generic_category()};

    return pointer_position;
}

s64 Write(int __fd, std::error_code& ec, const void* __buf, size_t __n) noexcept {
    ec.clear();
    errno = 0;

    const s64 bytes_written = write(__fd, __buf, __n);
    if (bytes_written != __n)
        ec = std::error_code{errno, std::generic_category()};

    return bytes_written;
}

s64 Read(int __fd, std::error_code& ec, void* __buf, size_t __n) noexcept {
    ec.clear();
    errno = 0;

    const s64 bytes_read = read(__fd, __buf, __n);
    if (bytes_read != __n)
        ec = std::error_code{errno, std::generic_category()};

    return bytes_read;
}

s64 GetDirectorySize(const fs::path path, std::error_code& ec) noexcept {
    ec.clear();
    u64 total = 0;

    struct stat statbuf{};
    const int file_size = statbuf.st_size;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        errno = 0;
        if (0 != stat(entry.path().c_str(), &statbuf)) {
            ec = std::error_code{errno, std::generic_category()};
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
    if (-1 == stat(path.c_str(), &statbuf)) {
        ec = std::error_code{errno, std::generic_category()};
        return false;
    }

    return S_ISDIR(statbuf.st_mode);
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

// Same as above, but the throwing wrapper

int Open(const fs::path path, int flags, int mode) {
    std::error_code ec{};
    int new_fd = Open(path, ec, flags, mode);

    if (!ec)
        return new_fd;

    throw fs::filesystem_error("Open(): ", path, std::error_code{errno, std::generic_category()});
}

bool Close(const int fd) {
    std::error_code ec{};
    bool close_success = Close(fd, ec);

    if (!ec)
        return close_success;

    throw fs::filesystem_error("Close(): ", std::format("File descriptor: {}", fd),
                               std::error_code{errno, std::generic_category()});
}

bool Unlink(const fs::path path) {
    std::error_code ec{};
    bool unlink_success = Unlink(path, ec);

    if (!ec)
        return unlink_success;

    throw fs::filesystem_error("Unlink(): ", path, std::error_code{errno, std::generic_category()});
}

bool Flush(const int fd) {
    return true;
}

bool Commit(const int fd) {
    std::error_code ec{};
    bool committed = Commit(fd, ec);

    if (!ec)
        return committed;

    throw fs::filesystem_error("Commit(): ", std::format("File descriptor: {}", fd),
                               std::error_code{errno, std::generic_category()});
}

bool SetSize(const int fd, u64 size) {
    std::error_code ec{};
    bool resize_success = SetSize(fd, ec, size);

    if (!ec)
        return resize_success;

    throw fs::filesystem_error("SetSize(): ", std::format("File descriptor: {}", fd),
                               std::error_code{errno, std::generic_category()});
}

u64 GetSize(const int fd) {
    std::error_code ec{};
    u64 file_size = GetSize(fd, ec);

    if (!ec)
        return file_size;

    throw fs::filesystem_error("GetSize(): ", std::format("File descriptor: {}", fd),
                               std::error_code{errno, std::generic_category()});
}

u64 GetSize(const fs::path path) {
    std::error_code ec{};
    u64 file_size = GetSize(path, ec);

    if (!ec)
        return file_size;

    throw fs::filesystem_error("GetSize(): ", path,
                               std::error_code{errno, std::generic_category()});
}

bool Seek(const int fd, s64 offset, SeekOrigin origin) {
    std::error_code ec{};
    bool seek_success = Seek(fd, ec, offset, origin);

    if (!ec)
        return seek_success;

    throw fs::filesystem_error("Seek(): ", std::format("File descriptor: {}", fd),
                               std::error_code{errno, std::generic_category()});
}

s64 Tell(int fd) {
    std::error_code ec{};
    s64 pointer_position = Tell(fd, ec);

    if (!ec)
        return pointer_position;

    throw fs::filesystem_error("Tell(): ", std::error_code{errno, std::generic_category()});
}

s64 Write(int __fd, const void* __buf, size_t __n) {
    std::error_code ec{};
    s64 bytes_written = Write(__fd, ec, __buf, __n);

    if (!ec)
        return bytes_written;

    throw fs::filesystem_error(
        "Write(): ",
        std::format("File descriptor: {} wrote {} out of {} bytes", __fd, bytes_written, __n),
        std::error_code{errno, std::generic_category()});

    return bytes_written;
}

s64 Read(int __fd, void* __buf, size_t __n) {
    std::error_code ec{};
    s64 bytes_read = Read(__fd, ec, __buf, __n);

    if (!ec)
        return bytes_read;

    throw fs::filesystem_error(
        "Read(): ",
        std::format("File descriptor: {} read {} out of {} bytes", __fd, bytes_read, __n),
        std::error_code{errno, std::generic_category()});
}

s64 GetDirectorySize(const fs::path path) {
    std::error_code ec{};
    s64 directory_size = GetDirectorySize(path, ec);

    if (!ec)
        return directory_size;

    throw fs::filesystem_error("GetDirectorySize(): ", path, ec);
}

bool Exists(const fs::path path) {
    std::error_code ec{};
    bool exists = Exists(path, ec);

    if (!ec)
        return exists;

    throw fs::filesystem_error("Exists(): ", path, std::error_code{errno, std::generic_category()});
}

bool IsDirectory(const fs::path path) {
    std::error_code ec{};
    bool is_directory = IsDirectory(path, ec);

    if (!ec)
        return is_directory;

    throw fs::filesystem_error("IsDirectory(): ", path,
                               std::error_code{errno, std::generic_category()});
}

fs::path AbsolutePath(const fs::path path) {
    std::error_code ec{};
    fs::path resolved_path = AbsolutePath(path, ec);

    if (!ec)
        return resolved_path;

    throw fs::filesystem_error("AbsolutePath(): ", path, ec);
}

bool Remove(const fs::path path) {
    std::error_code ec{};
    bool file_removed = Remove(path, ec);

    if (!ec)
        return file_removed;

    throw fs::filesystem_error("Remove(): ", path, ec);
}

bool CreateDirectory(const fs::path path, int mode) {
    std::error_code ec{};
    bool directory_created = CreateDirectory(path, ec, mode);

    if (!ec)
        return directory_created;

    throw fs::filesystem_error("CreateDirectory(): ", path,
                               std::error_code{errno, std::generic_category()});
}

bool CreateDirectories(const fs::path path, int mode) {
    std::error_code ec{};
    bool directories_created = CreateDirectories(path, ec, mode);

    if (!ec)
        return directories_created;

    throw fs::filesystem_error("CreateDirectories(): ", path,
                               std::error_code{errno, std::generic_category()});
}

} // namespace Common::FS::Native
