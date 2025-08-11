// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>

#include <fcntl.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/kernel/file_system.h"

#include "io_file.h"
#include "native_fs.h"

namespace Common::FS {

namespace NativeFS = Common::FS::Native;

IOFile::IOFile() = default;

IOFile::IOFile(IOFile&& other) noexcept {
    std::swap(file_path, other.file_path);
    std::swap(file_access_mode, other.file_access_mode);
    std::swap(file_descriptor, other.file_descriptor);
}

IOFile& IOFile::operator=(IOFile&& other) noexcept {
    std::swap(file_path, other.file_path);
    std::swap(file_access_mode, other.file_access_mode);
    std::swap(file_descriptor, other.file_descriptor);
    return *this;
}

int IOFile::Open(const std::filesystem::path& path, FileAccessMode flags, bool truncate, int mode) {
    return Open(path, AccessModeToPOSIX(flags, truncate), mode);
}

int IOFile::Open(const std::filesystem::path& path, int flags, int mode) {
    Close();

    errno = 0;
    file_path = path;
    file_access_mode = flags;
    file_access_permissions = mode;
    file_descriptor = NativeFS::Open(path, file_access_mode, file_access_permissions);

    if (!IsOpen()) {
        const auto ec = std::error_code{errno, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to open the file at path={}, error_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    return errno;
}

void IOFile::Close() {
    if (!IsOpen()) {
        return;
    }

    errno = 0;
    if (!NativeFS::Close(file_descriptor)) {
        const auto ec = std::error_code{errno, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to close the file at path={}, ec_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    file_descriptor = 0;
}

void IOFile::Unlink() {
    if (!IsOpen()) {
        return;
    }

    errno = 0;
    if (NativeFS::Unlink(file_path))
        return;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to unlink the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());
}

uintptr_t IOFile::GetFileMapping() {
    // posix only???
    return file_descriptor;
}

bool IOFile::Flush() const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (NativeFS::Flush(file_descriptor))
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to flush the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::Commit() const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (NativeFS::Commit(file_descriptor))
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to commit the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::SetSize(u64 size) const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (NativeFS::SetSize(file_descriptor, size))
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to resize the file at path={}, size={}, ec_message={}",
              PathToUTF8String(file_path), size, ec.message());

    return false;
}

u64 IOFile::GetSize() const {
    if (!IsOpen()) {
        return 0;
    }

    errno = 0;
    const u64 file_size = NativeFS::GetSize(file_descriptor);
    if (0 < file_size)
        return file_size;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to resize the file at path={}, size={}, ec_message={}",
              PathToUTF8String(file_path), file_size, ec.message());

    return file_size;
}

bool IOFile::Seek(s64 offset, SeekOrigin origin) const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (NativeFS::Seek(file_descriptor, offset, origin))
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem,
              "Failed to seek the file at path={}, offset={}, origin={}, ec_message={}",
              PathToUTF8String(file_path), offset, static_cast<u32>(origin), ec.message());

    return false;
}

s64 IOFile::Tell() const {
    if (!IsOpen()) {
        return 0;
    }

    errno = 0;
    if (s64 ret = NativeFS::Tell(file_descriptor); -1 != ret)
        return ret;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to tell file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return -1;
}

std::string IOFile::ReadString(size_t length) const {
    std::vector<char> string_buffer(length);

    const auto chars_read = ReadSpan<char>(string_buffer);
    const auto string_size = chars_read != length ? chars_read : length;

    return std::string{string_buffer.data(), string_size};
}

size_t IOFile::WriteString(std::span<const char> string) const {
    return WriteSpan(string);
}

u64 GetDirectorySize(const std::filesystem::path& path) {
    if (!NativeFS::Exists(path)) {
        return 0;
    }

    return NativeFS::GetDirectorySize(path);
}

/**
 * These two functions are final for Linux/macOS, but intermediate
 * for Windows. Native calls are converted to POSIX, and then
 * will be processed by Windows to its own flags and attributes.
 * This simplifies calls a lot, leaving Linux as a main convention and forcing Windows to adapt
 */

int AccessModeToPOSIX(FileAccessMode mode, bool truncate) {
    const int _truncate = truncate ? 1 : 0;
    // recreating how stream would handle access modes
    switch (mode) {
    default:
    case FileAccessMode::Read:
        return O_RDONLY;
    case FileAccessMode::Write:
        return O_WRONLY | O_CREAT | (O_TRUNC * _truncate);
    case FileAccessMode::Append:
        return O_APPEND | O_CREAT;
    case FileAccessMode::ReadExtended:
        return O_RDWR;
    case FileAccessMode::WriteExtended:
        return O_RDWR | O_CREAT | (O_TRUNC * _truncate);
    case FileAccessMode::AppendExtended:
        return O_RDONLY | O_APPEND | O_CREAT;
    }
    return O_RDONLY;
}

int AccessModeOrbisToPOSIX(int mode) {
    int out = O_RDONLY; // O_RDONLY = 0, so we can safely assume it as the default

    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_WRONLY)
        out |= O_WRONLY;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_RDWR)
        out |= O_RDWR;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_NONBLOCK) //< can ignore
        out |= O_NONBLOCK;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_APPEND)
        out |= O_APPEND;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_FSYNC) //< can ignore
        out |= O_FSYNC;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_SYNC) ///< can ignore
        out |= O_SYNC;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_CREAT)
        out |= O_CREAT;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_TRUNC)
        out |= O_TRUNC;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_EXCL)
        out |= O_EXCL;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_DSYNC) ///< can ignore
        out |= O_DSYNC;
#ifndef __APPLE__
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_DIRECT) ///< can ignore
        out |= O_DIRECT;
#endif
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_DIRECTORY)
        out |= O_DIRECTORY;
    return out;
}

} // namespace Common::FS