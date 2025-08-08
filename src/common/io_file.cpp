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

namespace Common::FS {

namespace fs = std::filesystem;

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

int IOFile::Open(const std::filesystem::path& path, FileAccessMode flags, bool truncate,
                 int mode) {
    ClearErrno();
    int result = OpenImpl(path, AccessModeToPOSIX(flags, truncate), mode);

    if (!IsOpen()) {
        const auto ec = std::error_code{result, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to open the file at path={}, error_message={}",
                  PathToUTF8String(file_path), ec.message());
    }
    return result;
}

int IOFile::Open(const std::filesystem::path& path, int flags, int mode) {
    ClearErrno();
    int result = OpenImpl(path, flags, mode);

    if (!IsOpen()) {
        const auto ec = std::error_code{result, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to open the file at path={}, error_message={}",
                  PathToUTF8String(file_path), ec.message());
    }
    return result;
}

void IOFile::Close() {
    if (!IsOpen()) {
        return;
    }

    ClearErrno();
    if (!CloseImpl()) {
        const auto ec = std::error_code{GetErrno(), std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to close the file at path={}, ec_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    file_descriptor = 0;
}

void IOFile::Unlink() {
    if (!IsOpen()) {
        return;
    }

    ClearErrno();
    if (UnlinkImpl())
        return;

    const auto ec = std::error_code{GetErrno(), std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to unlink the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());
}

uintptr_t IOFile::GetFileMapping() {
    return GetFileMappingImpl();
}

bool IOFile::Flush() const {
    if (!IsOpen()) {
        return false;
    }

    ClearErrno();
    if (FlushImpl())
        return true;

    const auto ec = std::error_code{GetErrno(), std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to flush the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::Commit() const {
    if (!IsOpen()) {
        return false;
    }

    ClearErrno();
    if (CommitImpl())
        return true;

    const auto ec = std::error_code{GetErrno(), std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to commit the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::SetSize(u64 size) const {
    if (!IsOpen()) {
        return false;
    }

    ClearErrno();
    if (SetSizeImpl(size))
        return true;

    const auto ec = std::error_code{GetErrno(), std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to resize the file at path={}, size={}, ec_message={}",
              PathToUTF8String(file_path), size, ec.message());

    return false;
}

u64 IOFile::GetSize() const {
    if (!IsOpen()) {
        return 0;
    }

    ClearErrno();
    const u64 file_size = GetSizeImpl();
    // TODO: catch error (if any)

    return file_size;
}

bool IOFile::Seek(s64 offset, SeekOrigin origin) const {
    if (!IsOpen()) {
        return false;
    }

    ClearErrno();
    const bool seek_result = SeekImpl(offset, origin);

    if (!seek_result) {
        const auto ec = std::error_code{GetErrno(), std::generic_category()};
        LOG_ERROR(Common_Filesystem,
                  "Failed to seek the file at path={}, offset={}, origin={}, ec_message={}",
                  PathToUTF8String(file_path), offset, static_cast<u32>(origin), ec.message());
    }

    return seek_result;
}

s64 IOFile::Tell() const {
    if (!IsOpen()) {
        return 0;
    }

    ClearErrno();
    const s64 ret = TellImpl();

    return ret;
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
    if (!fs::exists(path)) {
        return 0;
    }

    return _GetDirectorySizeImpl(path);
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