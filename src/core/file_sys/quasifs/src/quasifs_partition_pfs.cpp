// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include "common/logging/log.h"

#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_directory_pfs.h"
#include "core/file_sys/quasifs/quasifs_inode_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "core/file_sys/quasifs/quasifs_partition_pfs.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

PartitionPFS::PartitionPFS(const fs::path& host_root = "") : host_root(host_root) {
    this->root = DirectoryPFS::Create();
    chmod(this->root, 0555);
    IndexInode(this->root);
}

}; // namespace QuasiFS