// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include <string>
#include <vector>

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"
#include "quasifs_inode_directory.h"

namespace QuasiFS {

// Directory
class DirectoryPFS final : public QuasiDirectory {

protected:
#pragma pack(push, 1)
    typedef struct dirent_pfs_t {
        u32 d_fileno;
        u32 d_type;
        u32 d_namlen;
        u32 d_reclen;
        char d_name[256];
    } dirent_pfs_t;
#pragma pack(pop)

private:
    void RebuildDirents(void) final;

public:
    DirectoryPFS();
    ~DirectoryPFS();

    static dir_ptr Create() {
        return std::make_shared<DirectoryPFS>();
    }

    virtual dir_ptr Spawn() const final override {
        return std::make_shared<DirectoryPFS>();
    }

    s64 read(void* buf, u64 count) final override;

    s64 getdents(void* buf, u64 count, s64* basep) final override;

    // Find an element with [name, case insensitive]
    inode_ptr lookup(const std::string& name) final override;
};

} // namespace QuasiFS