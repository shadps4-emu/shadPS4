// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include "core/file_sys/quasifs/quasifs_inode_device.h"

namespace QuasiFS {

Device::Device() {
    // fileno and blkdev assigned by partition
    this->st.st_size = 0;
    this->st.st_blksize = 0;
    this->st.st_blocks = 0;

    this->st.st_mode |= QUASI_S_IFCHR;
}

Device::~Device() = default;

s64 Device::read(void* buf, u64 count) {
    return -POSIX_EBADF;
}

s64 Device::write(const void* buf, u64 count) {
    return -POSIX_EBADF;
}

s64 Device::lseek(s64 offset, s32 whence) {
    this->descriptor_offset =
        ((SeekOrigin::ORIGIN == whence) * offset) +
        ((SeekOrigin::CURRENT == whence) * (this->descriptor_offset + offset)) +
        ((SeekOrigin::END == whence) * ((offset > 0) * offset));
    // ::END is pro-forma
    return this->descriptor_offset >= 0 ? this->descriptor_offset : -POSIX_EINVAL;
}

} // namespace QuasiFS