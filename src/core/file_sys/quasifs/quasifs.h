// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include <mutex>
#include <unordered_map>

#include "common/types.h"
#include "core/libraries/kernel/posix_error.h"
#include "src/core/file_sys/hostio/host_io.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

// #ifdef _linux_
// #else
// #define QUASI_CASE_INSENSITIVE
// #endif

/**
 * Wrapper class
 * Basically a Partition object with a bit of extra functionality and single superblock
 */

namespace QuasiFS {
std::string file_mode(u16 mode);

void _printTree(const inode_ptr& node, const std::string& name, int depth);

void printTree(const dir_ptr& node, const std::string& name, int depth = 0);

// Very small QFS manager: path resolution, mount, create/unlink
class QFS {
private:
    // root partition
    partition_ptr rootfs;
    // root directory of the root partition ("/")
    dir_ptr root;

    // inode which holds partitions root in its mounted_root, mountpoint info
    // this allows us to correlate parent/root inodes with corresponding mount options
    // this will make a lot of sense when using RO filesystem opt
    std::unordered_map<partition_ptr, mount_t> block_devices{};

    // open file descriptors. search is linear, looking for first available nullptr
    std::vector<fd_handle_ptr> open_fd;

    // Drivers
    HostIO hio_driver{};
    HostVIO vio_driver{};

    //
    // Inherited from HostIOBase
    // Native filesystem operations, here because top-level FS *must* interact with every step
    // when in doubt - man(2)
    //

    class OperationImpl final : public HostIOBase {
    private:
        QFS& qfs;
        OperationImpl& operator=(const OperationImpl&) = delete;
        std::mutex c_mutex;

    public:
        OperationImpl(QFS& qfs) : qfs(qfs) {}
        s32 Open(const fs::path& path, int flags, u16 mode = 0755) override;
        s32 Creat(const fs::path& path, u16 mode = 0755) override;
        s32 Close(const s32 fd) override;

        s32 Link(const fs::path& src, const fs::path& dst) override;
        s32 LinkSymbolic(const fs::path& src, const fs::path& dst) override;
        s32 Unlink(const fs::path& path) override;
        s32 Remove(const fs::path& path) override;

        s32 Flush(const s32 fd) override;
        s32 FSync(const s32 fd) override;
        s32 Truncate(const fs::path& path, u64 size) override;
        s32 FTruncate(const s32 fd, u64 size) override;
        s64 LSeek(const s32 fd, s64 offset, s32 whence) override;
        s64 Tell(const s32 fd) override;

        s64 Read(const s32 fd, void* buf, u64 count) override;
        s64 PRead(const s32 fd, void* buf, u64 count, s64 offset) override;
        s64 ReadV(const s32 fd, Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt) override;
        s64 PReadV(const s32 fd, Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt,
                   s64 offset) override;

        s64 Write(const s32 fd, const void* buf, u64 count) override;
        s64 PWrite(const s32 fd, const void* buf, u64 count, s64 offset) override;
        s64 WriteV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov,
                   u32 iovcnt) override;
        s64 PWriteV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt,
                    s64 offset) override;

        s32 MKDir(const fs::path& path, u16 mode = 0755) override;
        s32 RMDir(const fs::path& path) override;

        s32 Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) override;
        s32 FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) override;

        s32 Chmod(const fs::path& path, u16 mode) override;
        s32 FChmod(const s32 fd, u16 mode) override;

        s64 GetDents(const s32 fd, void* buf, u64 count, s64* basep) override;

        s32 Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
        s32 Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
    };

public:
    QFS(const fs::path& host_path = "");
    ~QFS() = default;

    //
    // QFS methods
    //

    // Sync mounted partitions with host directories
    int SyncHost(void);
    int SyncHost(fs::path path);
    // Return root directory
    dir_ptr GetRoot() {
        return this->root;
    }
    // Return root partition
    partition_ptr GetRootFS() {
        return this->rootfs;
    }

    // mount fs at path (target must exist and be directory)
    // empty options imply read-only
    int Mount(const fs::path& path, partition_ptr fs,
              unsigned int options = MountOptions::MOUNT_NOOPT);
    // unmount fs from path
    int Unmount(const fs::path& path);

    // may or may not break stuff (duuun dun)
    int ForceInsert(const fs::path& path, const std::string& name, inode_ptr node);

    /**
     * Resolve path
     * This is kind of a headache, so TLDR:
     *  1. Parent is:
     *      * nullptr if resolution failed within the path, i.e. not on the last element
     *          So /dir/dir_not_exist/dir2 wil return both nulls, while /dir/dir2/file_not_exist
     *          will return all good with nullptr node
     *      * Last available inode
     *      * Inode that has something mounted (always a dir)
     *  2. Node is:
     *      * nullptr if target is not found
     *      * target inode (if found, irrespectable of type)
     *      * Inode that's mounted (always a dir)
     *  3. Mountpoint is:
     *      * Partition owning the inode
     *      * Null when partition is not found
     *  4. Local path is:
     *      * Always set by partition's resolve
     *      * Every leaf that was encountered on resolution step
     *  5. Leaf is:
     *      * Never empty
     *      * Name of the last element
     *          * path.filename() if found OR
     *          * name of the element that's NOT found
     *  6. On mountpoint resolution:
     *      * leaf - last element (no change)
     *      * mountpoint - mountpoint (no change)
     *      * parent - holding dir that is a mountpoint (/dir/INODE->mounted_dir)
     *      * node - node holding mounted root (/dir/inode->MOUNTED_DIR)
     */
    int Resolve(const fs::path& path, Resolved& res);

    int GetHostPath(fs::path& output, const fs::path& path = "/");

    //
    // Additional binds
    //

    bool IsOpen(const s32 fd) noexcept;
    int SetSize(const s32 fd, uint64_t size) noexcept;
    s64 GetSize(const s32 fd) noexcept;

    // Not a port, used by 2-3 functions that ;
    s64 GetDirectorySize(const fs::path& path) noexcept;

    //
    // FS operations
    // I think this class is huge already, so OG fs ops are moved to a separate "namespace"
    //

    OperationImpl Operation{*this};

    //
    // C++ ports with both except/noexcept
    //

    int64_t GetSize(const fs::path& path) {
        return -POSIX_EINVAL;
    };
    int64_t GetSize(const fs::path& path, std::error_code& ec) noexcept {
        return -POSIX_EINVAL;
    };
    bool Exists(const fs::path& path) {
        return -POSIX_EINVAL;
    };
    bool Exists(const fs::path& path, std::error_code& ec) noexcept {
        return -POSIX_EINVAL;
    };
    bool IsDirectory(const fs::path& path) {
        return -POSIX_EINVAL;
    };
    bool IsDirectory(const fs::path& path, std::error_code& ec) noexcept {
        return -POSIX_EINVAL;
    };
    fs::path AbsolutePath(const fs::path& path) {
        return "";
    };
    fs::path AbsolutePath(const fs::path& path, std::error_code& ec) noexcept {
        return "";
    };

    // uint64_t RemoveAll(const fs::path& path) = delete;
    // uint64_t RemoveAll(const fs::path& path, std::error_code& ec) noexcept = delete;
    // uint64_t CurrentPath(const fs::path& path) = delete;
    // uint64_t CurrentPath(const fs::path& path, std::error_code& ec) noexcept = delete;

    // 0777 to mimic default C++ mode (std::filesystem::perms::all)
    bool CreateDirectory(const fs::path& path, int mode = 0777) {
        return -POSIX_EINVAL;
    };
    bool CreateDirectory(const fs::path& path, std::error_code& ec, int mode = 0777) noexcept {
        return -POSIX_EINVAL;
    };
    bool CreateDirectory(const fs::path& path, const fs::path& existing_path,
                         int mode = 0777) = delete;
    bool CreateDirectory(const fs::path& path, const fs::path& existing_path, std::error_code& ec,
                         int mode = 0777) noexcept = delete;
    bool CreateDirectories(const fs::path& path, int mode = 0777) {
        return -POSIX_EINVAL;
    };
    bool CreateDirectories(const fs::path& path, std::error_code& ec, int mode = 0777) noexcept {
        return -POSIX_EINVAL;
    };

    fd_handle_ptr GetHandle(s32 fd);

private:
    void SyncHostImpl(partition_ptr part);

    // Get next available fd slot
    int GetFreeHandleNo();
    // partition by blkdev
    //  partition_ptr GetPartitionByBlockdev(uint64_t blkid);
    mount_t* GetPartitionInfo(const partition_ptr part);
    partition_ptr GetPartitionByPath(const fs::path& path);
    partition_ptr GetPartitionByParent(const dir_ptr dir);
    int IsPartitionRO(const partition_ptr part);
};
}; // namespace QuasiFS