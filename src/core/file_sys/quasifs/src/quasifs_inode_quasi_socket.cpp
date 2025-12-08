// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include "core/file_sys/quasifs/quasifs_inode_quasi_socket.h"

namespace QuasiFS {

Socket::Socket() {
    // fileno and blkdev assigned by partition
    this->st.st_size = 0;
    this->st.st_blksize = 0;
    this->st.st_blocks = 0;

    this->st.st_mode |= QUASI_S_IFSOCK;
}

Socket::~Socket() = default;

} // namespace QuasiFS