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

int IOFile::OpenImpl(const fs::path& path, int mode) {
    Close();

    file_path = path;
    file_access_mode = mode;

    ClearErrno();
    int result = 0;

    file_descriptor = open(path.c_str(), mode);
    if (!file_descriptor)
        return GetErrno();
    result = GetErrno();


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

const int IOFile::GetErrno(void) const {
    return errno;
}

void IOFile::ClearErrno(void) const {
    errno = 0;
}

[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin) {
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
