// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <vector>

#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

s64 QuasiFile::read(void* buf, u64 count) {
    st.st_atim.tv_sec = time(0);

    s64& size = this->st.st_size;
    s64 tbr = (descriptor_offset + count) > size ? size - descriptor_offset : count;
    descriptor_offset += tbr;
    return tbr;
}

s64 QuasiFile::write(const void* buf, u64 count) {
    st.st_mtim.tv_sec = time(0);

    s64& size = this->st.st_size;
    s64 end_pos = descriptor_offset + count;
    size = std::max(end_pos, size);
    descriptor_offset += count;

    return count;
}

s32 QuasiFile::fsync() {
    return 0;
}

s32 QuasiFile::ftruncate(s64 length) {
    if (length < 0)
        return -POSIX_EINVAL;
    st.st_mtim.tv_sec = time(0);
    this->st.st_size = length;
    return 0;
}

} // namespace QuasiFS
