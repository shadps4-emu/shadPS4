// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include "common/logging/log.h"

#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory_pfs.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

Partition::Partition() : Partition(Directory::Create(), "", 0755, 4096) {}

Partition::Partition(const fs::path& host_root, const int root_permissions, const u32 ioblock_size)
    : Partition(dir_ptr(), host_root, root_permissions, ioblock_size) {}

Partition::Partition(dir_ptr root_directory, const fs::path& host_root, const int root_permissions,
                     const u32 ioblock_size)
    : root(root_directory), block_id(next_block_id++), host_root(host_root.lexically_normal()),
      ioblock_size(ioblock_size) {
    // clear defaults, write
    chmod(this->root, root_permissions);
    IndexInode(this->root);
    mkrelative(this->root, this->root);
}

fs::path Partition::SanitizePath(const fs::path& path) {
    // lexically normal to resolve relative calls
    // avoids going OOB with malicious file names
    fs::path tmp = path.lexically_normal();
    if (tmp.string().find(this->host_root.string(), 0) == 0)
        return tmp;

    return {};
}

int Partition::GetHostPath(fs::path& output_path, const fs::path& local_path) {
    if (this->host_root.empty())
        return -POSIX_ENODEV;

    // must be relative to root, otherwise lvalue is overwritten
    fs::path host_path_target = (this->host_root / local_path.lexically_relative("/"));
    if (host_path_target.empty())
        return -POSIX_EINVAL;

    fs::path host_path_target_sanitized = SanitizePath(host_path_target);
    if (host_path_target_sanitized.empty()) {
        LOG_ERROR(Kernel_Fs, "Malicious path detected: {}", host_path_target.string());
        return -POSIX_EACCES;
    }
    output_path = host_path_target_sanitized;

    // #ifdef _WIN32
    //     // Windows is case insensitive
    //     output_path = host_path_target_sanitized;
    // #endif

    //     fs::path current_path{};
    //     for (auto part : host_path_target_sanitized) {
    //         fs::path tmp = Common::ToLower(part.string());
    //         if (!fs::exists(tmp))
    //             continue;
    //         current_path /= tmp;
    //     }

    //     if (fs::equivalent(current_path, host_path_target_sanitized)) {
    //         output_path = current_path;
    //         return 0;
    //     }

    //     output_path = "";
    //     LOG_ERROR(Kernel_Fs, "Resolving [{}] to host's [{}] failed", local_path.string(),
    //               host_path_target_sanitized.string());

    // Log("Resolving local {} to {}", local_path.string(), host_path_target_sanitized.string());
    return 0;
}

inode_ptr Partition::GetInodeByFileno(fileno_t fileno) {
    auto ret = inode_table.find(fileno);
    return (inode_table.end() == ret) ? nullptr : ret->second;
}

// DO NOT, AND I SWEAR  D O  N O T touch this function
// Debugging it is a royal PITA
int Partition::Resolve(fs::path& path, Resolved& res) {
    if (path.empty() || !path.string().starts_with("/"))
        return -POSIX_EINVAL;

    // if (path.is_relative())
    //     return -POSIX_EBADF;

    if (path.string().size() >= 256)
        return -POSIX_ENAMETOOLONG;

    res.mountpoint = shared_from_this();
    res.local_path = "/";
    res.parent = this->root;
    res.node = this->root;
    res.leaf = "";

    // these hold up between iterations, but setting them can be ignored if the function is about to
    // return
    dir_ptr parent = res.parent;
    inode_ptr current = res.node;

    bool is_final = false;

    for (auto _part = path.begin(); _part != path.end(); _part++) {
        std::string part = (*_part).string();
        is_final = std::next(_part) == path.end();

        if ("/" == part) {
            res.local_path = "/";
            res.parent = res.parent;
            res.node = res.node;
            res.leaf = "/";
            continue;
        }

        // empty means trailing / which is interpreted only when using dirs or symlinks
        if (part.empty()) {
            if (!is_final)
                // something went wrong
                throw 666;

            if (!(current->is_link() || current->is_dir()))
                // trailing slash after a file
                return -POSIX_ENOTDIR;

            return 0;
        }

        if (!(current->is_link() || current->is_dir()) && !is_final) {
            // path elements must be a dir or a symlink
            return -POSIX_ENOTDIR;
        }

        //

        if (current->is_dir()) {
            if (!current->CanRead())
                return -POSIX_EACCES;

            dir_ptr dir = std::static_pointer_cast<Directory>(current);
            parent = dir;
            current = dir->lookup(part);

            res.local_path /= part;
            res.parent = parent;
            res.node = current;
            res.leaf = part;
        }

        // file not found in current directory, ENOENT
        if (nullptr == res.node) {
            // zero out everything
            // avoids a condition where calling function may think that parent
            // directory is real parent directory, when in fact we might just stop in the
            // middle of the path
            if (!is_final) {
                res.node = nullptr;
                res.parent = nullptr;
            }
            return -POSIX_ENOENT;
        }

        //

        /**
         * WARNING
         * These relations must be saved as-is
         * Currently CWD is NOT implemented. Might be, might not in the future.
         * For current purposes all irrelevant Resolved members could be set to nullptr,
         * but then i'll forget what they were supposed to be.
         * They need to be saved to build CWD history and enable relative paths (since we may jump
         * over parents)
         */

        // quick lookahead if this directory is a mountpoint
        if (current->is_dir()) {
            dir_ptr current_dir = std::static_pointer_cast<Directory>(current);

            if (current_dir->mounted_root != nullptr) {
                // if it's a mountpoint, the preceeding path is invalid from target partition's POV
                // we take remainder of the path (current leaf name belongs to upstream) and leave
                // everything AFTER host's path since QFS holds mountpoint meta, its it's job to
                // resolve mounted root directory.
                fs::path remainder = "/";
                for (auto p = std::next(_part); p != path.end(); p++)
                    remainder /= *p;
                path = remainder;

                res.parent = current_dir; // no point, unused in this context
                res.node = current_dir->mounted_root;
                res.leaf = part;

                return 0;
            }
        }

        //

        if (current->is_link()) {
            // just like with mountpoints, we discard everything up until the "next" element in path
            fs::path remainder = "";
            for (auto p = std::next(_part); p != path.end(); p++)
                remainder /= *p;
            path = remainder;

            res.parent = parent;
            res.node = current;
            res.leaf = part;
            return 0;
        }
    }

    return 0;
}

int Partition::touch(dir_ptr parent, const std::string& name, inode_ptr child) {
    if (nullptr == parent)
        return -POSIX_EINVAL;

    auto ret = parent->link(name, child);
    if (ret == 0)
        IndexInode(child);
    return ret;
}

int Partition::mkdir(dir_ptr parent, const std::string& name) {
    if (nullptr == parent)
        return -POSIX_ENOENT;

    dir_ptr real_parent = parent->mounted_root ? parent->mounted_root : parent;
    dir_ptr child = real_parent->Spawn();

    child->st.st_blksize = ioblock_size;
    int ret = parent->link(name, child);

    if (ret == 0)
        IndexInode(child);

    mkrelative(real_parent, child);

    return ret;
}

int Partition::rmdir(dir_ptr parent, const std::string& name) {
    if (nullptr == parent)
        return -POSIX_ENOENT;
    if (name.empty())
        return -POSIX_EINVAL;

    inode_ptr target = parent->lookup(name);
    if (nullptr == target)
        return -POSIX_ENOENT;

    if (!target->is_dir())
        return -POSIX_ENOTDIR;

    int status = parent->unlink(name);
    if (0 != status)
        return status;

    return rmInode(target);
}

int Partition::link(inode_ptr source, dir_ptr destination_parent, const std::string& name) {
    if (nullptr == source || nullptr == destination_parent)
        return -POSIX_ENOENT;
    if (name.empty())
        return -POSIX_EINVAL;
    if (destination_parent->lookup(name) != nullptr)
        return -POSIX_EEXIST;
    if (source->is_dir())
        return -POSIX_EPERM;

    return destination_parent->link(name, source);
}

int Partition::unlink(dir_ptr parent, const std::string& name) {
    if (nullptr == parent)
        return -POSIX_ENOENT;
    if (name.empty())
        return -POSIX_EINVAL;

    inode_ptr target = parent->lookup(name);
    if (nullptr == target)
        return -POSIX_ENOENT;

    if (target->is_dir())
        return -POSIX_EISDIR;

    int status = parent->unlink(name);
    if (0 != status)
        return status;

    return rmInode(target);
}

int Partition::chmod(inode_ptr target, u16 mode) {
    if (nullptr == target)
        return -POSIX_EINVAL;

    return target->chmod(mode);
}

int Partition::rmInode(fileno_t fileno) {
    inode_ptr target = GetInodeByFileno(fileno);
    if (nullptr == target)
        return -POSIX_EINVAL;

    return rmInode(target);
}

int Partition::rmInode(inode_ptr node) {
    if (nullptr == node)
        return -POSIX_EINVAL;

    // truly remove if there are no hardlinks
    if (node->st.st_nlink > 0)
        return 0;

    // TODO: check for open file handles, return -POSIX_EEBUSY

    this->inode_table.erase(node->__GetFileno());
    return 0;
}

bool Partition::IndexInode(inode_ptr node) {
    if (nullptr == node)
        return false;

    // Assign fileno and add it to the fs table
    fileno_t node_fileno = node->__GetFileno();
    if (node_fileno == 0) {
        node_fileno = this->NextFileno();
        node->__SetFileno(node_fileno);
    }

    inode_table[node_fileno] = node;
    if (node->is_dir()) {
        auto dir = std::static_pointer_cast<Directory>(node);
        for (auto& kv : dir->Entries())
            IndexInode(dir->lookup(kv));
        if (dir->mounted_root)
            IndexInode(dir->mounted_root);
    }

    node->st.st_dev = block_id;
    node->st.st_blksize = ioblock_size;
    node->st.st_blocks = Common::AlignUp(static_cast<u64>(node->st.st_size), ioblock_size) / 512;

    return true;
}

void Partition::mkrelative(dir_ptr parent, dir_ptr child) {
    child->link(".", child);
    child->link("..", parent);
}
}; // namespace QuasiFS