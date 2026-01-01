// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <vector>

#include "core/file_sys/quasifs/quasifs_inode_quasi_file_virtual.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

s64 VirtualFile::read(void* buf, u64 count) {
    st.st_atim.tv_sec = time(0);

    s64 read_amt = this->data.size() - descriptor_offset - count;
    // if >= 0 - we're good to go
    // <0 - n-bytes are missing, won't enter loop
    read_amt = count + read_amt * (read_amt < 0);

    for (s64 idx = 0; idx < read_amt; idx++) {
        char c = this->data.at(idx + descriptor_offset);
        static_cast<char*>(buf)[idx] = c;
    }
    descriptor_offset += read_amt;
    return read_amt;
}

s64 VirtualFile::write(const void* buf, u64 count) {
    st.st_mtim.tv_sec = time(0);

    auto& size = this->st.st_size;
    auto end_pos = descriptor_offset + count;

    // size can only be greater, so it will always scale up
    if (end_pos < size) {
        this->data.resize(size, 0);
        size = end_pos;
    }

    for (u64 idx = descriptor_offset; idx < size; idx++)
        this->data[idx] = static_cast<const char*>(buf)[idx - descriptor_offset];
    descriptor_offset += count;
    return count;
}

s32 VirtualFile::fsync() {
    return 0;
}

s32 VirtualFile::ftruncate(s64 length) {
    if (length < 0)
        return -POSIX_EINVAL;
    this->data.resize(length, 0);
    this->st.st_size = length;
    st.st_mtim.tv_sec = time(0);
    return 0;
}

} // namespace QuasiFS
