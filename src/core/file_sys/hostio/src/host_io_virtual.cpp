// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <cstring>
#include <filesystem>
#include <mutex>

#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/file_sys/hostio/host_io_posix.h"
#include "core/file_sys/hostio/host_io_virtual.h"
#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_device.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_file.h"
#include "core/file_sys/quasifs/quasifs_inode_file_virtual.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "core/libraries/kernel/posix_error.h"

#include "host_io_base.h"

namespace HostIODriver {

HostIO_Virtual::HostIO_Virtual(Resolved* resolved, fd_handle_ptr handle)
    : resolved(resolved), handle(handle) {}
HostIO_Virtual::~HostIO_Virtual() = default;

s32 HostIO_Virtual::Open(const fs::path& path, s32 flags, u16 mode) {
    if (path.string().size() > 255)
        return -POSIX_ENAMETOOLONG;

    if (int remainder = flags & ~__QUASI_O_ALLFLAGS_AT_ONCE; remainder != 0)
        LOG_WARNING(Kernel_Fs, "open() received unknown flags: {:x}", remainder);

    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    partition_ptr part = this->resolved->mountpoint;
    dir_ptr parent = this->resolved->parent;
    inode_ptr target = this->resolved->node;

    bool exists = this->resolved->node != nullptr;

    if (exists && (flags & QUASI_O_EXCL) && (flags & QUASI_O_CREAT))
        return -POSIX_EEXIST;

    if (!exists) {
        if ((flags & QUASI_O_CREAT) == 0)
            return -POSIX_ENOENT;

        target = part->IsHostMounted() ? RegularFile::Create() : VirtualFile::Create();
        target->chmod(mode);
        if (0 != part->touch(parent, this->resolved->leaf, target))
            // touch failed in target directory, issue with resolve() is most likely
            return -POSIX_EFAULT;

        this->resolved->node = target;
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
    return Open(path, QUASI_O_CREAT | QUASI_O_TRUNC | QUASI_O_WRONLY);
}

s32 HostIO_Virtual::Close(s32 fd) {
    // We came here, so fd is valid
    return 0;
}

s32 HostIO_Virtual::Link(const fs::path& src, const fs::path& dst) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    partition_ptr part = this->resolved->mountpoint;
    inode_ptr src_node = this->resolved->node;

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
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    symlink_ptr sym = Symlink::Create(src);
    // symlink counter is never increased
    sym->st.st_nlink = 1;

    return this->resolved->mountpoint->touch(this->resolved->parent, dst.filename().string(), sym);
}

s32 HostIO_Virtual::Unlink(const fs::path& path) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;
    if ("." == this->resolved->leaf)
        return -POSIX_EINVAL;

    if (nullptr == this->resolved->node)
        return -POSIX_ENOENT;

    partition_ptr part = this->resolved->mountpoint;
    dir_ptr parent = this->resolved->parent;
    return part->unlink(parent, this->resolved->leaf);
}

s32 HostIO_Virtual::Remove(const fs::path& path) {
    return -POSIX_ENOSYS;
}

s32 HostIO_Virtual::Flush(s32 fd) {
    // not applicable
    return 0;
}

s32 HostIO_Virtual::FSync(s32 fd) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return handle->node->fsync();
}

s64 HostIO_Virtual::LSeek(s32 fd, s64 offset, s32 whence) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    node->__SetOffset(handle->pos);
    s64 new_position = node->lseek(offset, whence);
    if (new_position >= 0)
        handle->pos = new_position;
    return new_position;
}

s64 HostIO_Virtual::Tell(s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

s32 HostIO_Virtual::Truncate(const fs::path& path, s64 size) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    inode_ptr node = this->resolved->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (node->is_dir())
        return -POSIX_EISDIR;

    if (!node->is_file())
        return -POSIX_EINVAL;

    return node->ftruncate(size);
}

s32 HostIO_Virtual::FTruncate(s32 fd, s64 size) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return handle->node->ftruncate(size);
}

s64 HostIO_Virtual::Read(s32 fd, void* buf, u64 count) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    SCOPE_EXIT {
        handle->pos = node->__GetOffset();
    };

    node->__SetOffset(handle->pos);
    return node->read(buf, count);
}

s64 HostIO_Virtual::PRead(s32 fd, void* buf, u64 count, s64 offset) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    node->__SetOffset(handle->pos);
    return node->pread(buf, count, offset);
}

s64 HostIO_Virtual::ReadV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    SCOPE_EXIT {
        handle->pos = node->__GetOffset();
    };

    node->__SetOffset(handle->pos);
    return node->readv(iov, iovcnt);
}

s64 HostIO_Virtual::PReadV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    if (nullptr == this->handle)
        return -POSIX_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    node->__SetOffset(handle->pos);
    return node->preadv(iov, iovcnt, offset);
}

s64 HostIO_Virtual::Write(s32 fd, const void* buf, u64 count) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append) {
        node->__SetOffset(node->st.st_size);
        s64 tbw = node->write(buf, count);
        node->__SetOffset(handle->pos);
        return tbw;
    }

    SCOPE_EXIT {
        handle->pos = node->__GetOffset();
    };

    node->__SetOffset(handle->pos);
    return node->write(buf, count);
}

s64 HostIO_Virtual::PWrite(s32 fd, const void* buf, u64 count, s64 offset) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    node->__SetOffset(handle->pos);
    return node->pwrite(buf, count, offset);
}

s64 HostIO_Virtual::WriteV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append) {
        node->__SetOffset(node->st.st_size);
        s64 tbw = node->writev(iov, iovcnt);
        node->__SetOffset(handle->pos);
        return tbw;
    }

    SCOPE_EXIT {
        handle->pos = node->__GetOffset();
    };

    node->__SetOffset(handle->pos);
    return node->writev(iov, iovcnt);
}

s64 HostIO_Virtual::PWriteV(s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
    if (nullptr == this->handle)
        return -POSIX_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    node->__SetOffset(handle->pos);
    return node->pwritev(iov, iovcnt, offset);
}

s32 HostIO_Virtual::MKDir(const fs::path& path, u16 mode) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    return this->resolved->mountpoint->mkdir(this->resolved->parent, this->resolved->leaf);
}

s32 HostIO_Virtual::RMDir(const fs::path& path) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    // EINVAL on . as last element

    // don't remove partition root ;___;
    dir_ptr parent = this->resolved->parent;

    if (parent->mounted_root)
        return -POSIX_EBUSY;

    if (int unlink_status = resolved->mountpoint->rmdir(parent, this->resolved->leaf);
        unlink_status != 0)
        return unlink_status;

    auto target_nlink = resolved->node->st.st_nlink;
    if (target_nlink != 0) {
        LOG_ERROR(Kernel_Fs, "RMDir'd directory nlink is not 0!", "(is ", target_nlink, ")");
        return -POSIX_ENOTEMPTY;
    }

    return 0;
}

s32 HostIO_Virtual::Stat(const fs::path& path, OrbisKernelStat* statbuf) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    inode_ptr node = this->resolved->node;

    if (nullptr == node)
        return -POSIX_ENOENT;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::FStat(s32 fd, OrbisKernelStat* statbuf) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::Chmod(const fs::path& path, u16 mode) {
    if (nullptr == this->resolved)
        return -POSIX_EINVAL;

    inode_ptr node = this->resolved->node;

    if (nullptr == node)
        return -POSIX_ENOENT;

    return Partition::chmod(node, mode);
}

s32 HostIO_Virtual::FChmod(s32 fd, u16 mode) {
    if (nullptr == this->handle)
        return -POSIX_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -POSIX_EBADF;

    return Partition::chmod(node, mode);
}

s64 HostIO_Virtual::GetDents(s32 fd, void* buf, u64 count, s64* basep) {
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

    SCOPE_EXIT {
        handle->pos = node->__GetOffset();
    };

    node->__SetOffset(handle->pos);
    return node->getdents(buf, count, basep);
}

s32 HostIO_Virtual::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    return -POSIX_ENOSYS;
}

s32 HostIO_Virtual::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    return -POSIX_ENOSYS;
}

} // namespace HostIODriver
