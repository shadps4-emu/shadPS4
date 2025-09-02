// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/error.h"

#ifdef _WIN64
#include <io.h>
#include <share.h>
#include <windows.h>
#include "common/ntapi.h"
#endif

namespace Core::FileSys {

#ifdef _WIN64

int File::Open(const std::filesystem::path& path, Common::FS::FileAccessMode f_access) {
    DWORD access{};
    if (f_access == Common::FS::FileAccessMode::Read) {
        access = GENERIC_READ;
    } else if (f_access == Common::FS::FileAccessMode::Write) {
        access = GENERIC_WRITE;
    } else if (f_access == Common::FS::FileAccessMode::ReadWrite) {
        access = GENERIC_READ | GENERIC_WRITE;
    } else {
        UNREACHABLE();
    }
    handle = CreateFileW(path.native().c_str(), access, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return ENOENT;
    }
}

s64 File::Read(void* buf, size_t nbytes) {
    DWORD bytes_read;
    if (!ReadFile(handle, buf, nbytes, &bytes_read, nullptr)) {
        UNREACHABLE_MSG("ReadFile failed: {}", Common::GetLastErrorMsg());
    }
    return bytes_read;
}

s64 File::Pread(void* buf, size_t nbytes, s64 offset) {
    OVERLAPPED ol{};
    ol.Offset = offset;
    ol.OffsetHigh = offset >> 32;
    DWORD bytes_read;
    if (!ReadFile(handle, buf, nbytes, &bytes_read, &ol)) {
        UNREACHABLE_MSG("ReadFile failed: {}", Common::GetLastErrorMsg());
    }
    return bytes_read;
}

s64 File::Write(const void* buf, size_t nbytes) {
    DWORD bytes_written;
    if (!WriteFile(handle, buf, nbytes, &bytes_written, nullptr)) {
        UNREACHABLE_MSG("WriteFile failed: {}", Common::GetLastErrorMsg());
    }
    return bytes_written;
}

s64 File::Pwrite(const void* buf, size_t nbytes, s64 offset) {
    OVERLAPPED ol{};
    ol.Offset = offset;
    ol.OffsetHigh = offset >> 32;
    DWORD bytes_written;
    if (!WriteFile(handle, buf, nbytes, &bytes_written, &ol)) {
        UNREACHABLE_MSG("WriteFile failed: {}", Common::GetLastErrorMsg());
    }
    return bytes_written;
}

void File::SetSize(s64 size) {
    Lseek(size, 0);
    if (!SetEndOfFile(handle)) {
        UNREACHABLE_MSG("SetEndOfFile failed: {}", Common::GetLastErrorMsg());
    }
}

void File::Flush() {
    FlushFileBuffers(handle);
}

s64 File::Lseek(s64 offset, int whence) {
    LARGE_INTEGER new_file_pointer;
    DWORD origin{};
    if (whence == 0) {
        origin = FILE_BEGIN;
    } else if (whence == 1) {
        origin = FILE_CURRENT;
    } else if (whence == 2) {
        origin = FILE_END;
    }
    if (!SetFilePointerEx(handle, LARGE_INTEGER{.QuadPart = offset}, &new_file_pointer, origin)) {
        UNREACHABLE_MSG("SetFilePointerEx failed: {}", Common::GetLastErrorMsg());
    }
    return new_file_pointer.QuadPart;
}

void File::Unlink() {
    FILE_DISPOSITION_INFORMATION disposition;
    IO_STATUS_BLOCK iosb;
    disposition.DeleteFile = TRUE;
    NtSetInformationFile(handle, &iosb, &disposition, sizeof(disposition),
                         FileDispositionInformation);
}

#else

#endif

} // namespace Core::FileSys