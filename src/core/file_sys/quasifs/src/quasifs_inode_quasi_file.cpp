// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include <vector>

#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

s64 QuasiFile::pread(void* buf, u64 count, s64 offset) {
    auto& size = this->st.st_size;
    auto end_pos = offset + count;

    st.st_atim.tv_sec = time(0);
    return end_pos > size ? size - offset : count;
}

s64 QuasiFile::pwrite(const void* buf, u64 count, s64 offset) {
    auto& size = this->st.st_size;
    auto end_pos = offset + count;

    size = end_pos > size ? end_pos : size;

    st.st_mtim.tv_sec = time(0);
    return count;
}

s32 QuasiFile::fsync() {
    return 0;
}

s32 QuasiFile::ftruncate(s64 length) {
    if (length < 0)
        return -POSIX_EINVAL;
    this->st.st_size = length;
    st.st_mtim.tv_sec = time(0);
    return 0;
}

} // namespace QuasiFS
