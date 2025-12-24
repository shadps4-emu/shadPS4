// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/directories/base_directory.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core::Directories {

BaseDirectory::BaseDirectory() = default;

BaseDirectory::~BaseDirectory() = default;

s64 BaseDirectory::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    s64 bytes_read = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        const s64 result = read(iov[i].iov_base, iov[i].iov_len);
        if (result < 0) {
            return result;
        }
        bytes_read += result;
    }
    return bytes_read;
}

s64 BaseDirectory::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    const u64 old_file_pointer = file_offset;
    file_offset = offset;
    const s64 bytes_read = readv(iov, iovcnt);
    file_offset = old_file_pointer;
    return bytes_read;
}

s64 BaseDirectory::lseek(s64 offset, s32 whence) {

    s64 file_offset_new = ((0 == whence) * offset) + ((1 == whence) * (file_offset + offset)) +
                          ((2 == whence) * (directory_size + offset));
    if (file_offset_new < 0)
        return ORBIS_KERNEL_ERROR_EINVAL;

    file_offset = file_offset_new;
    return file_offset;
}

} // namespace Core::Directories