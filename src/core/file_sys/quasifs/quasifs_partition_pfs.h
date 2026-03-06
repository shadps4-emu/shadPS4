// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasifs_inode_directory_pfs.h"
#include "core/file_sys/quasifs/quasifs_partition.h"

namespace QuasiFS {

class PartitionPFS : public Partition {
private:
    const u32 block_size{65536};

    fs::path host_patch_path{};
    dir_ptr root_patch{};

public:
    // host-bound directory, permissions for root directory
    PartitionPFS(const fs::path& host_root = "");
    ~PartitionPFS() = default;

    static partition_ptr Create(const fs::path& host_root = "") {
        return std::make_shared<PartitionPFS>(host_root);
    }
};

}; // namespace QuasiFS