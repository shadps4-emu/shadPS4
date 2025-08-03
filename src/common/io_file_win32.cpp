// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#error Don't bother compiling yet

#include <cerrno>
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

/**
 * A general convention regarding porting Windows calls
 * All native functions are in xImpl()
 * File attributes **should** be derived only from posix access attributes
 * (since Orbis uses posix).
 * functions should translate errors into errno equivalent (error messages are based on errno)
 * Errno interactions are through _seterrno(int) and _geterrno()
 */

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
    return _SH_DENYRW;
}

int WindowsErrorToErrno(DWORD error) {
    switch (error) {
    case ERROR_FILE_NOT_FOUND:
        return ENOENT;
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_ACCESS_DENIED:
        return EACCES;
    case ERROR_ALREADY_EXISTS:
        return EEXIST;
    case ERROR_INVALID_HANDLE:
        return EBADF;
    case ERROR_NOT_ENOUGH_MEMORY:
        return ENOMEM;
    case ERROR_OUTOFMEMORY:
        return ENOMEM;
    case ERROR_INVALID_PARAMETER:
        return EINVAL;
    case ERROR_DISK_FULL:
        return ENOSPC;
    case ERROR_SHARING_VIOLATION:
        return EBUSY;
    case ERROR_LOCK_VIOLATION:
        return EBUSY;
    case ERROR_HANDLE_EOF:
        return EOF;
    case ERROR_FILE_EXISTS:
        return EEXIST;
    default:
        return EIO; // domyślnie błąd wejścia/wyjścia
    }
    return EIO;
}

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

    result = WindowsErrorToErrno(GetLastError());

    if (file_descriptor == INVALID_HANDLE_VALUE)
        return result;

    if (!IsOpen()) {
        const auto ec = std::error_code{result, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to open the file at path={}, error_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    return result;
}

bool IOFile::CloseImpl() {
    return CloseHandle(file_descriptor);
}

bool IOFile::UnlinkImpl() {
    return DeleteFileW(file_path.c_str());
}

uintptr_t IOFile::GetFileMappingImpl() {
    return file_descriptor;
}

bool IOFile::FlushImpl() const {
    // POSIX rawdogs the file descriptor
    return true;
}

bool IOFile::CommitImpl() const {
    return FlushFileBuffers(file_descriptor);
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
    int seek_pos = 0;
    bool seek_result = SetFilePointerEx(file_descriptor, offset, &seek_pos, _origin);
    if (!seek_result)
        return false;

    switch (_origin) {
    default:
    case FILE_BEGIN:
        return seek_pos == offset;
        break;
    case FILE_END:
        return seek_pos == GetSize();
        break;
    case FILE_CURRENT:
        return seek_start == (seek_pos + offset);
        break;
    }
    return false;
}

s64 IOFile::TellImpl() {
    return lseek(file_descriptor, 0, SEEK_CUR);
}

s64 IOFile::WriteImpl(int __fd, const void* __buf, size_t __n) {
    return write(__fd, __buf, __n);
}

s64 IOFile::ReadImpl(int __fd, void* __buf, size_t __n) {
    return read(__fd, __buf, __n);
}

const int IOFile::GetErrno(void) const {
    return WindowsErrorToErrno(GetLastError());
}
void IOFile::ClearErrno(void) const {
    SetLastError(0);
}

[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin) {
    switch (origin) {
    case SeekOrigin::SetOrigin:
        return FILE_BEGIN;
    case SeekOrigin::CurrentPosition:
        return FILE_CURRENT;
    case SeekOrigin::End:
        return FILE_END;
    }
    UNREACHABLE_MSG("Impossible SeekOrigin {}", static_cast<u32>(origin));
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
