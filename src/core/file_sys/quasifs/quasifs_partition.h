// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include <unordered_map>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"

#include "quasi_types.h"

namespace QuasiFS {

class Partition : public std::enable_shared_from_this<Partition> {
private:
    fileno_t NextFileno(void) {
        return this->next_fileno++;
    };

    // file list
    std::unordered_map<fileno_t, inode_ptr> inode_table{};

    // root directory
    dir_ptr root;
    // next available fileno
    fileno_t next_fileno = 2;
    // technically it's a device+partition id, but block id is enough lmao
    const blkid_t block_id;

    static inline blkid_t next_block_id = 1;

    // path to host's directory this will be bound to
    const fs::path host_root{};

    // IO block size, allocation unit (multiples of 512)
    const u32 ioblock_size{4096};

public:
    // host-bound directory, permissions for root directory
    Partition();
    Partition(const fs::path& host_root = "", int root_permissions = 0755, u32 ioblock_size = 4096);
    Partition(dir_ptr root_directory = Directory::Create(), const fs::path& host_root = "",
              int root_permissions = 0755, u32 ioblock_size = 4096);
    ~Partition() = default;

    static partition_ptr Create(const fs::path& host_root = "", int root_permissions = 0755,
                                u32 ioblock_size = 4096) {
        return std::make_shared<Partition>(Directory::Create(), host_root, root_permissions,
                                           ioblock_size);
    }

    static partition_ptr Create(const dir_ptr& root_directory = Directory::Create(),
                                const fs::path& host_root = "", int root_permissions = 0755,
                                u32 ioblock_size = 4096) {
        return std::make_shared<Partition>(root_directory, host_root, root_permissions,
                                           ioblock_size);
    }

    // empty - invalid, not empty - safe
    fs::path SanitizePath(const fs::path& path);
    // return - valid, out_path - sanitized path
    int GetHostPath(fs::path& output_path, const fs::path& local_path = "/");

    dir_ptr GetRoot(void) {
        return this->root;
    }
    bool IsHostMounted(void) {
        return !this->host_root.empty();
    }
    blkid_t GetBlkId(void) {
        return this->block_id;
    }
    inode_ptr GetInodeByFileno(fileno_t fileno);

    int Resolve(fs::path& path, Resolved& res);

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    template <typename T>
    int touch(const dir_ptr& parent, const std::string& name) {
        if constexpr (std::is_base_of_v<Inode, T>)
            return touch(parent, name, T::Create());
        UNREACHABLE_MSG(" QuasiFS:Partition:Touch Created element must derive from Inode");
        static_assert(std::is_base_of_v<Inode, T>,
                      " QuasiFS:Partition:Touch Created element must derive from Inode");
        return -666;
    }
    int touch(const dir_ptr& parent, const std::string& name, inode_ptr child);

    int mkdir(const dir_ptr& parent, const std::string& name);
    int rmdir(const dir_ptr& parent, const std::string& name);

    int link(const inode_ptr& source, const dir_ptr& destination_parent, const std::string& name);
    int unlink(const dir_ptr& parent, const std::string& name);

    static int chmod(const inode_ptr& target, u16 mode);

private:
    int rmInode(fileno_t fileno);
    int rmInode(const inode_ptr& node);
    bool IndexInode(const inode_ptr& node);
    static void mkrelative(const dir_ptr& parent, const dir_ptr& child);
};

}; // namespace QuasiFS