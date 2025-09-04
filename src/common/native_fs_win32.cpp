
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

#ifdef _WIN32

namespace {

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
} // namespace

#endif