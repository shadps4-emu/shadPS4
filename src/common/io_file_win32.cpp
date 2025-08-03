// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#error Don't bother compiling yet

#include <vector>

#include <io.h>
#include <share.h>
#include <windows.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/ntapi.h"
#include "common/path_util.h"
#include "core/libraries/kernel/file_system.h"

#ifdef _MSC_VER
#define fileno _fileno
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace Common::FS {

namespace fs = std::filesystem;

namespace {

enum class FileShareFlag {
    ShareNone,      // Provides exclusive access to the file.
    ShareReadOnly,  // Provides read only shared access to the file.
    ShareWriteOnly, // Provides write only shared access to the file.
    ShareReadWrite, // Provides read and write shared access to the file.
};

DWORD PosixToWindowsAccess(int oflag) {
    if ((oflag & O_RDWR) == O_RDWR)
        return GENERIC_READ | GENERIC_WRITE;
    if (oflag & O_WRONLY)
        return GENERIC_WRITE;
    return GENERIC_READ; // default O_RDONLY
}

DWORD PosixToWindowsCreation(int oflag) {
    if (oflag & O_CREAT) {
        if (oflag & O_EXCL)
            return CREATE_NEW;
        if (oflag & O_TRUNC)
            return CREATE_ALWAYS;
        return OPEN_ALWAYS;
    } else if (oflag & O_TRUNC) {
        return TRUNCATE_EXISTING;
    }
    return OPEN_EXISTING;
}

DWORD PosixToWindowsFlags(int oflag) {
    return FILE_ATTRIBUTE_NORMAL | ((oflag & O_APPEND) ? FILE_APPEND_DATA : 0);
}

[[nodiscard]] constexpr int ToWindowsFileShareFlag(FileShareFlag flag) {
    switch (flag) {
    case FileShareFlag::ShareNone:
    default:
        return _SH_DENYRW;
    case FileShareFlag::ShareReadOnly:
        return _SH_DENYWR;
    case FileShareFlag::ShareWriteOnly:
        return _SH_DENYRD;
    case FileShareFlag::ShareReadWrite:
        return _SH_DENYNO;
    }
}

// not ported
//[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin);

} // Anonymous namespace

int IOFile::OpenImpl(const fs::path& path, int mode) {
    Close();

    file_path = path;
    file_access_mode = mode;

    errno = 0;
    int result = 0;

    file_descriptor =
        CreateFileW(path, PosixToWindowsAccess(mode), FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    PosixToWindowsCreation(mode), PosixToWindowsFlags(mode), nullptr);

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

bool IOFile::CloseImpl() {
    return close(file_descriptor) == 0;
}

bool IOFile::UnlinkImpl() {
    return unlink(file_path.c_str()) == 0;
}

uintptr_t IOFile::GetFileMappingImpl() {
    return file_descriptor;
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

s64 IOFile::WriteImpl(int __fd, const void* __buf, size_t __n) const {
    return write(__fd, __buf, __n);
}

s64 IOFile::ReadImpl(int __fd, void* __buf, size_t __n) const {
    return read(__fd, __buf, __n);
}

u64 _GetDirectorySizeImpl(const std::filesystem::path& path) {
    if (!fs::exists(path)) {
        return 0;
    }
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
