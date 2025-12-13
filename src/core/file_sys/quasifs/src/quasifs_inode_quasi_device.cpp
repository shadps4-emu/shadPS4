// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"

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

s64 Device::pread(void* buf, u64 count, s64 offset) {
    return read(buf, count);
}

s64 Device::pwrite(const void* buf, u64 count, s64 offset) {
    return write(buf, count);
}

s64 Device::lseek(s64 current, s64 offset, s32 whence) {
    return ((QuasiFS::SeekOrigin::ORIGIN == whence) * offset) +
           ((QuasiFS::SeekOrigin::CURRENT == whence) * (current + offset)) +
           ((QuasiFS::SeekOrigin::END == whence) * ((offset > 0) * offset));
    // ::END is pro-forma
}

} // namespace QuasiFS