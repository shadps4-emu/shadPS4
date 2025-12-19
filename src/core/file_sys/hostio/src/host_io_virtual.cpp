// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include <cstring>
#include <filesystem>
#include <mutex>

#include "common/logging/log.h"
#include "core/file_sys/hostio/host_io_posix.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "core/libraries/kernel/posix_error.h"
#include "src/core/file_sys/hostio/host_io_virtual.h"
#include "src/core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "src/core/file_sys/quasifs/quasi_types.h"
#include "src/core/file_sys/quasifs/quasifs_inode_quasi_file_virtual.h"

#include "host_io_base.h"

namespace HostIODriver {

HostIO_Virtual::HostIO_Virtual() = default;
HostIO_Virtual::~HostIO_Virtual() = default;

s32 HostIO_Virtual::Open(const fs::path& path, s32 flags, u16 mode) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (path.string().size() > 255)
        return -POSIX_ENAMETOOLONG;

    if (int remainder = flags & ~__QUASI_O_ALLFLAGS_AT_ONCE; remainder != 0)
        LOG_WARNING(Kernel_Fs, "open() received unknown flags: {:x}", remainder);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    partition_ptr part = this->res->mountpoint;
    dir_ptr parent = this->res->parent;
    inode_ptr target = this->res->node;

    bool exists = this->res->node != nullptr;

    if (exists && (flags & QUASI_O_EXCL) && (flags & QUASI_O_CREAT))
        return -POSIX_EEXIST;

    if (!exists) {
        if ((flags & QUASI_O_CREAT) == 0)
            return -POSIX_ENOENT;

        target = this->host_bound ? RegularFile::Create() : VirtualFile::Create();
        target->chmod(mode);
        if (0 != part->touch(parent, this->res->leaf, target))
            // touch failed in target directory, issue with resolve() is most likely
            return -POSIX_EFAULT;

        this->res->node = target;
    }

    // at this point target should exist
    if (flags & QUASI_O_TRUNC) {
        if (int status = target->ftruncate(0); status != 0)
            return status;
    }

    // if exists and is a directory, can't be opened with any kind of write
    if (exists && (target->is_dir() || (flags & QUASI_O_DIRECTORY)) &&
        (flags & (QUASI_O_TRUNC | QUASI_O_RDWR | QUASI_O_WRONLY)))
        return -POSIX_EISDIR;

    if ((flags & QUASI_O_DIRECTORY) && !target->is_dir())
        // opening dirs isn't supported yet
        return -POSIX_ENOTDIR;

    if (flags & (QUASI_O_NONBLOCK | QUASI_O_SYNC | QUASI_O_DIRECT | QUASI_O_DSYNC)) {
        // unused, not affecting file manip per-se
    }

    return 0;
}

s32 HostIO_Virtual::Creat(const fs::path& path, u16 mode) {
    // std::lock_guard<std::mutex> lock(ctx_mutex);
    return Open(path, QUASI_O_CREAT | QUASI_O_TRUNC | QUASI_O_WRONLY);
}

s32 HostIO_Virtual::Close(const s32 fd) {
    // std::lock_guard<std::mutex> lock(ctx_mutex);
    // N/A
    return 0;
}

s32 HostIO_Virtual::Link(const fs::path& src, const fs::path& dst) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    partition_ptr part = this->res->mountpoint;
    inode_ptr src_node = this->res->node;

    Resolved dst_res;
    fs::path dst_path = dst.parent_path();
    std::string dst_name = dst.filename().string();

    if (int res_status = part->Resolve(dst_path, dst_res); res_status < 0)
        return res_status;

    if (!dst_res.node->is_dir())
        return -POSIX_ENOTDIR;

    dir_ptr dst_parent = std::static_pointer_cast<Directory>(dst_res.node);
    return part->link(src_node, dst_parent, dst_name);
}

s32 HostIO_Virtual::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    symlink_ptr sym = Symlink::Create(src);
    // symlink counter is never increased
    sym->st.st_nlink = 1;

    return this->res->mountpoint->touch(this->res->parent, dst.filename().string(), sym);
}

s32 HostIO_Virtual::Unlink(const fs::path& path) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;
    if ("." == this->res->leaf)
        return -POSIX_EINVAL;

    if (nullptr == this->res->node)
        return -POSIX_ENOENT;

    partition_ptr part = this->res->mountpoint;
    dir_ptr parent = this->res->parent;
    return part->unlink(parent, this->res->leaf);
}

s32 HostIO_Virtual::Remove(const fs::path& path) {
    // std::lock_guard<std::mutex> lock(ctx_mutex);

    return -POSIX_ENOSYS;
}

s32 HostIO_Virtual::Flush(const s32 fd) {
    // std::lock_guard<std::mutex> lock(ctx_mutex);

    // not applicable
    return 0;
}

s32 HostIO_Virtual::FSync(const s32 fd) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return handle->node->fsync();
}

s64 HostIO_Virtual::LSeek(const s32 fd, s64 offset, s32 whence) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    s64 new_position = node->lseek(handle->pos, offset, whence);

    if (new_position < 0)
        return -POSIX_EINVAL;

    handle->pos = new_position;
    return handle->pos;
}

s64 HostIO_Virtual::Tell(const s32 fd) {
    // std::lock_guard<std::mutex> lock(ctx_mutex);

    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

s32 HostIO_Virtual::Truncate(const fs::path& path, u64 size) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (node->is_dir())
        return -POSIX_EISDIR;

    if (!node->is_file())
        return -POSIX_EINVAL;

    return node->ftruncate(size);
}

s32 HostIO_Virtual::FTruncate(const s32 fd, u64 size) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return handle->node->ftruncate(size);
}

s64 HostIO_Virtual::Read(const s32 fd, void* buf, u64 count) {
    s64 br = PRead(fd, buf, count, handle->pos);

    if (br > 0)
        handle->pos += br;

    return br;
}

s64 HostIO_Virtual::PRead(const s32 fd, void* buf, u64 count, s64 offset) {
    if (nullptr == handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return node->pread(buf, count, offset);
}

s64 HostIO_Virtual::ReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt) {
    s64 br = PReadV(fd, iov, iovcnt, handle->pos);

    if (br > 0)
        handle->pos += br;

    return br;
}

s64 HostIO_Virtual::PReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    if (nullptr == handle)
        return -POSIX_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    return node->preadv(iov, iovcnt, offset);
}

s64 HostIO_Virtual::Write(const s32 fd, const void* buf, u64 count) {
    s64 bw = this->PWrite(fd, buf, count, handle->pos);

    if (bw > 0)
        handle->pos += bw;

    return bw;
}

s64 HostIO_Virtual::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) {
    if (nullptr == handle)
        return -POSIX_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    return node->pwrite(buf, count, offset);
}

s64 HostIO_Virtual::WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    s64 bw = this->PWriteV(fd, iov, iovcnt, handle->pos);

    if (bw > 0)
        handle->pos += bw;

    return bw;
}

s64 HostIO_Virtual::PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == handle)
        return -POSIX_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    return node->pwritev(iov, iovcnt, offset);
}

s32 HostIO_Virtual::MKDir(const fs::path& path, u16 mode) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    return this->res->mountpoint->mkdir(this->res->parent, this->res->leaf);
}

s32 HostIO_Virtual::RMDir(const fs::path& path) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    // EINVAL on . as last element

    // don't remove partition root ;___;
    dir_ptr parent = this->res->parent;

    if (parent->mounted_root)
        return -POSIX_EBUSY;

    if (int unlink_status = res->mountpoint->rmdir(parent, this->res->leaf); unlink_status != 0)
        return unlink_status;

    auto target_nlink = res->node->st.st_nlink;
    if (target_nlink != 0) {
        LOG_ERROR(Kernel_Fs, "RMDir'd directory nlink is not 0!", "(is ", target_nlink, ")");
        return -POSIX_ENOTEMPTY;
    }

    return 0;
}

s32 HostIO_Virtual::Stat(const fs::path& path, OrbisKernelStat* statbuf) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -POSIX_ENOENT;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::FStat(const s32 fd, OrbisKernelStat* statbuf) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::Chmod(const fs::path& path, u16 mode) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->res)
        return -POSIX_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -POSIX_ENOENT;

    return Partition::chmod(node, mode);
}

s32 HostIO_Virtual::FChmod(const s32 fd, u16 mode) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return Partition::chmod(node, mode);
}

s64 HostIO_Virtual::GetDents(const s32 fd, void* buf, u64 count, s64* basep) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    // GetDents must have read size equal to or greater than block size for that FS
    // In most cases it's 512 bytes
    // Not sure yet if other partitions share the same size
    if (count < 512) {
        LOG_ERROR(Kernel_Fs, "Read size must be greater than sector size (512B)");
        return -POSIX_EINVAL;
    }

    s64 br = node->getdents(buf, count, handle->pos, basep);

    if (br > 0)
        handle->pos += br;

    return br;
}

s32 HostIO_Virtual::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    return POSIX_ENOSYS;
}

s32 HostIO_Virtual::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    std::lock_guard<std::mutex> lock(ctx_mutex);

    return POSIX_ENOSYS;
}

} // namespace HostIODriver
