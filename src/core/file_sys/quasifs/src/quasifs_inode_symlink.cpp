// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include "core/file_sys/quasifs/quasifs_inode_symlink.h"

namespace QuasiFS {

Symlink::Symlink(fs::path target) : target(target) {
    // fileno and blkdev assigned by partition
    this->st.st_size = target.string().size();
    this->st.st_mode |= QUASI_S_IFLNK;
    // not incrementing target, this type is a softlink
}

fs::path Symlink::follow(void) {
    st.st_atim.tv_sec = time(0);
    return target;
}
} // namespace QuasiFS