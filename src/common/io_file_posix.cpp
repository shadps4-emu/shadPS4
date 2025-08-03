// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/kernel/file_system.h"

namespace Common::FS {

namespace fs = std::filesystem;

int IOFile::AccessModeToNative(FileAccessMode mode, bool truncate) {
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

int IOFile::AccessModeOrbisToNative(int mode) {
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
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_DIRECT) ///< can ignore
        out |= O_DIRECT;
    if (mode & Libraries::Kernel::ORBIS_KERNEL_O_DIRECTORY)
        out |= O_DIRECTORY;
    return out;
}

uintptr_t IOFile::GetFileMappingImpl() {
    return file_descriptor;
}

s64 IOFile::WriteImpl(int __fd, const void* __buf, size_t __n) const {
    return write(__fd, __buf, __n);
}

s64 IOFile::ReadImpl(int __fd, void* __buf, size_t __n) const {
    return read(__fd, __buf, __n);
}

int IOFile::OpenImpl(const fs::path& path, int mode) {
    Close();

    file_path = path;
    file_access_mode = mode;

    errno = 0;
    int result = 0;

    file_descriptor = open(path.c_str(), mode);
    if (!file_descriptor)
        return errno;
    result = errno;

    if (!IsOpen()) {
        const auto ec = std::error_code{result, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to open the file at path={}, error_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    return result;
}

int IOFile::OpenImpl(const fs::path& path, FileAccessMode mode, bool truncate) {
    return OpenImpl(path, AccessModeToNative(mode, truncate));
}

bool IOFile::CloseImpl() {
    return close(file_descriptor) == 0;
}

bool IOFile::UnlinkImpl() {
    return unlink(file_path.c_str()) == 0;
}

bool IOFile::FlushImpl() const {
    // POSIX rawdogs the file descriptor
    return true;
}

bool IOFile::CommitImpl() const {
    return fsync(file_descriptor) == 0;
}

bool IOFile::SetSizeImpl(u64 size) const {
    return ftruncate(file_descriptor, static_cast<s64>(size)) == 0;
}

u64 IOFile::GetSizeImpl() const {
    struct stat statbuf{};
    fstat(file_descriptor, &statbuf); // catch return!!!
    return statbuf.st_size;
}

bool IOFile::SeekImpl(s64 offset, SeekOrigin origin) const {
    int _origin = ToSeekOrigin(origin);
    int seek_start = Tell();
    const off_t seek_pos = lseek(file_descriptor, offset, _origin);
    bool seek_result = false;
    switch (_origin) {
    default:
    case SEEK_SET:
        return seek_pos == offset;
        break;
    case SEEK_END:
        return seek_pos == GetSize();
        break;
    case SEEK_CUR:
        return seek_start == (seek_pos + offset);
        break;
    }
    return false;
}

s64 IOFile::TellImpl() const {
    return lseek(file_descriptor, 0, SEEK_CUR);
}

u64 _GetDirectorySizeImpl(const std::filesystem::path& path) {
    u64 total = 0;

    struct stat statbuf{};
    const int file_size = statbuf.st_size;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        stat(entry.path().c_str(), &statbuf); // catch return

        if (S_ISREG(statbuf.st_mode)) {
            total += statbuf.st_size;
        }
    }

    return total;
}

} // namespace Common::FS
