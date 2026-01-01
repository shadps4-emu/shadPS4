// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <cstring>

#include "common/logging/log.h"

#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

s32 QFS::OperationImpl::Open(const fs::path& path, int flags, u16 mode) {
    bool request_read = !(flags & QUASI_O_WRONLY) || (flags & QUASI_O_RDWR);
    bool request_write = (flags & (QUASI_O_WRONLY | QUASI_O_RDWR)) != 0;
    bool request_append = flags & QUASI_O_APPEND;

    //
    // Orbis-specific checks and order, or at least ones that differ between Linux and Orbis
    // Universal checks are embedded into QFS
    // Some of those are universal, but order matters (a lot)
    //

    if ((flags & (QUASI_O_WRONLY | QUASI_O_RDWR)) == (QUASI_O_WRONLY | QUASI_O_RDWR))
        return -POSIX_EINVAL;

    if (path.string().size() > 255)
        return -POSIX_ENAMETOOLONG;

    Resolved res;
    // Resolve for parent dir to avoid treating ENOENT as missing just the end file
    int resolve_status = qfs.Resolve(path, res);

    // enoent on last element in the path is good
    if (-POSIX_ENOENT == resolve_status) {
        if (nullptr == res.parent)
            return -POSIX_ENOENT;
    } else if (0 != resolve_status)
        return resolve_status;

    partition_ptr part = res.mountpoint;
    dir_ptr parent_node = std::static_pointer_cast<Directory>(res.parent);

    // if it doesn't exist, check the parent
    inode_ptr checked_node = nullptr == res.node ? parent_node : res.node;

    if ((request_read && !checked_node->CanRead()) || (request_write && !checked_node->CanWrite()))
        return -POSIX_EACCES;

    if ((request_write || request_append) && qfs.IsPartitionRO(part))
        return -POSIX_EROFS;

    if (flags & QUASI_O_DIRECTORY && flags & QUASI_O_CREAT)
        return -POSIX_ENOTDIR;

    // O_TRUNC | O_RDONLY - throw einval but touch a file
    // TODO: find out what happens when can't create file before throwing einval
    if ((flags & (QUASI_O_TRUNC | QUASI_O_WRONLY | QUASI_O_RDWR)) == QUASI_O_TRUNC) {
        if (int status = this->Close(this->Creat(path, mode)); status != 0)
            return status;
        return -POSIX_ENOENT;
    }

#ifndef __APPLE_CC__
    if (flags & O_DIRECT)
        LOG_WARNING(Kernel_Fs, "File open with O_DIRECT - RW performance may suffer a lot");
#endif

    //
    // Proceed
    //

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {

        if (nullptr == res.node || (res.node && !res.node->is_dir())) {
            // if doesn't exist, creation/throwing becomes host's problem
            // however we might want to open a dir for dirents, which would be suboptimal
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
                hostpath_status != 0)
                return hostpath_status;

            if (hio_status = qfs.hio_driver.Open(host_path_target, flags, mode); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }
    }

    vio_status = HostVIO(&res, nullptr).Open(res.local_path, flags, mode);

    if (int tmp_hio_status = hio_status >= 0 ? 0 : hio_status;
        host_used && (tmp_hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    if (vio_status < 0)
        return vio_status;

    fd_handle_ptr handle = File::Create();
    // nasty hack, but: if it existed, no change
    // if it didn't, VIO will update this member
    handle->node = res.node;
    // virtual fd is stored in open_fd map
    handle->host_fd = host_used ? hio_status : -1;
    handle->read = request_read;
    handle->write = request_write;
    handle->append = request_append;
    handle->pos = 0;
    auto next_free_handle = qfs.GetFreeHandleNo();
    qfs.open_fd[next_free_handle] = handle;
    return next_free_handle;
}

s32 QFS::OperationImpl::Creat(const fs::path& path, u16 mode) {
    return Open(path, QUASI_O_CREAT | QUASI_O_WRONLY | QUASI_O_TRUNC, mode);
};

s32 QFS::OperationImpl::Close(s32 fd) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (fd <= 2)
        LOG_ERROR(Kernel_Fs, "Closing std stream, this will have consequences fd={}", fd);

    // if it fails, it fails
    int hio_status = 0;
    if (handle->host_fd >= 0) {
        hio_status = qfs.hio_driver.Close(handle->host_fd);
    }

    HostVIO(nullptr, handle).Close(fd);

    // if it's the last entry, remove it to avoid blowing up fd table
    // not really helping with fragmentation, but may save resources on burst opens

    if (fd < (qfs.open_fd.size() - 1)) {
        qfs.open_fd.at(fd) = nullptr;
    } else
        qfs.open_fd.pop_back();

    return hio_status;
}

s32 QFS::OperationImpl::Link(const fs::path& src, const fs::path& dst) {
    Resolved src_res;
    Resolved dst_res;
    int status_what = qfs.Resolve(src, src_res);
    int status_where = qfs.Resolve(dst, dst_res);

    if (0 != status_what)
        return status_what;
    if (0 == status_where)
        return -POSIX_EEXIST;

    // cross-partition linking is not supported
    if (src_res.mountpoint != dst_res.mountpoint)
        return -POSIX_EXDEV;

    partition_ptr src_part = src_res.mountpoint;
    partition_ptr dst_part = dst_res.mountpoint;

    if (src_part != dst_part) {
        LOG_ERROR(Kernel_Fs, "Hard links can only be created within one partition");
        // I think this is the right error
        return -POSIX_ENOSYS;
    }

    if (qfs.IsPartitionRO(dst_part))
        return -POSIX_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (dst_part->IsHostMounted()) {
        fs::path host_path_src{};
        fs::path host_path_dst{};

        if (int hostpath_status = src_part->GetHostPath(host_path_src, src_res.local_path);
            hostpath_status != 0)
            return hostpath_status;
        if (int hostpath_status = dst_part->GetHostPath(host_path_dst, dst_res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Link(host_path_src, host_path_dst); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&src_res, nullptr).Link(src_res.local_path, dst_res.local_path);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    Resolved src_res;
    Resolved dst_res;
    int status_what = qfs.Resolve(src, src_res);
    int status_where = qfs.Resolve(dst, dst_res);

    // source may not exist and can point wherever it wants, so we skip checks for it

    if (0 == status_where)
        return -POSIX_EEXIST;
    // destination parent directory must exist though
    if (0 != status_where && nullptr == dst_res.parent)
        return -POSIX_ENOENT;

    partition_ptr src_part = src_res.mountpoint;
    partition_ptr dst_part = dst_res.mountpoint;

    if (qfs.IsPartitionRO(dst_part))
        return -POSIX_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    // for this to work, both files must be on host partition
    // mixed source/destination will need a bit more effort
    if (src_part->IsHostMounted() && dst_part->IsHostMounted()) {
        // if target partition doesn't exist or is not mounted, we can'tqfs.Resolve host path
        if (nullptr == src_part)
            return -POSIX_ENOENT;

        fs::path host_path_src{};
        fs::path host_path_dst{};

        if (int hostpath_status = src_part->GetHostPath(host_path_src, src_res.local_path);
            hostpath_status != 0)
            return hostpath_status;
        if (int hostpath_status = dst_part->GetHostPath(host_path_dst, dst_res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.LinkSymbolic(host_path_src, host_path_dst); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    } else if (dst_part->IsHostMounted() ^ src_part->IsHostMounted()) {
        LOG_ERROR(Kernel_Fs,
                  "Symlinks can be only created on the same type of partition (virtual/host)");
        return -POSIX_ENOSYS;
    }

    // src stays 1:1
    vio_status = HostVIO(&dst_res, nullptr).LinkSymbolic(src, dst_res.local_path);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Unlink(const fs::path& path) {
    if (path.string().size() > 255)
        return -POSIX_ENAMETOOLONG;

    Resolved res;
    int resolve_status;

    // symlinks mess this whole thing up, so we need toqfs.Resolve parent and leaf independently

    fs::path parent_path = path.parent_path();
    std::string leaf = path.filename().string();

    // parent, must pass
    resolve_status = qfs.Resolve(parent_path, res);
    if (resolve_status != 0)
        return resolve_status;

    if (!res.node->is_dir())
        return -POSIX_ENOTDIR;

    partition_ptr part = res.mountpoint;
    if (qfs.IsPartitionRO(part))
        return -POSIX_EROFS;

    dir_ptr parent = std::static_pointer_cast<Directory>(res.node);
    inode_ptr target = parent->lookup(leaf);

    if (nullptr == target)
        return -POSIX_ENOENT;

    // fix upqfs.Resolve result for VIO
    res.parent = parent;
    res.node = target;
    res.leaf = leaf;
    res.local_path /= leaf;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Unlink(host_path_target); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&res, nullptr).Unlink(res.local_path);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Remove(const fs::path& path) {
    return -POSIX_ENOSYS;
}

s32 QFS::OperationImpl::Flush(const s32 fd) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.Flush(host_fd); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(nullptr, handle).Flush(fd);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FSync(const s32 fd) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.FSync(host_fd); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(nullptr, handle).FSync(fd);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
};

s32 QFS::OperationImpl::Truncate(const fs::path& path, u64 length) {
    Resolved res;
    int status = qfs.Resolve(path, res);

    if (0 != status)
        return status;

    partition_ptr part = res.mountpoint;
    if (qfs.IsPartitionRO(part))
        return -POSIX_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target;
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            0 != hostpath_status)
            return hostpath_status;
        if (hio_status = qfs.hio_driver.Truncate(host_path_target, length); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&res, nullptr).Truncate(res.local_path, length);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FTruncate(const s32 fd, u64 length) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->write)
        return -POSIX_EBADF;

    // EROFS is guarded by Open()

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.FTruncate(host_fd, length); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(nullptr, handle).FTruncate(fd, length);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::LSeek(const s32 fd, s64 offset, s32 whence) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.LSeek(host_fd, offset, whence); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    // TODO: desync possible
    vio_status = HostVIO(nullptr, handle).LSeek(fd, offset, whence);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
};

s64 QFS::OperationImpl::Tell(s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
};

void UpdateStatFromHost(Libraries::Kernel::OrbisKernelStat* vfs,
                        Libraries::Kernel::OrbisKernelStat* host) {
    vfs->st_size = host->st_size;
    vfs->st_blksize = host->st_blksize;
    vfs->st_blocks = host->st_blocks;
    vfs->st_atim = host->st_atim;
    vfs->st_mtim = host->st_mtim;
    vfs->st_ctim = host->st_ctim;
}

s64 QFS::OperationImpl::Read(const s32 fd, void* buf, u64 count) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.Read(host_fd, buf, count); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    if (host_used && (hio_status < count))
        LOG_ERROR(Kernel_Fs, "Host read less bytes ({}) than requested ({})", hio_status, count);

    return HostVIO(nullptr, handle).Read(fd, buf, host_used ? hio_status : count);
}

s64 QFS::OperationImpl::PRead(const s32 fd, void* buf, u64 count, s64 offset) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PRead(host_fd, buf, count, offset); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    if (host_used && (hio_status < count))
        LOG_ERROR(Kernel_Fs, "Host read less bytes ({}) than requested ({})", hio_status, count);

    return HostVIO(nullptr, handle).PRead(fd, buf, host_used ? hio_status : count, offset);
};

s64 QFS::OperationImpl::ReadV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov,
                              u32 iovcnt) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.ReadV(host_fd, iov, iovcnt); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    // TODO: desync possible
    vio_status = HostVIO(nullptr, handle).ReadV(fd, iov, iovcnt);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::PReadV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov,
                               u32 iovcnt, s64 offset) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PReadV(host_fd, iov, iovcnt, offset); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    // TODO: desync possible
    vio_status = HostVIO(nullptr, handle).PReadV(fd, iov, iovcnt, offset);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::Write(const s32 fd, const void* buf, u64 count) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->write)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.Write(host_fd, buf, count); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    if (host_used && (hio_status < count))
        LOG_ERROR(Kernel_Fs, "Host write less bytes ({}) than requested ({})", hio_status, count);

    return HostVIO(nullptr, handle).Write(fd, buf, host_used ? hio_status : count);
}

s64 QFS::OperationImpl::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->write)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PWrite(host_fd, buf, count, offset); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    if (host_used && (hio_status < count))
        LOG_ERROR(Kernel_Fs, "Host write less bytes ({}) than requested ({})", hio_status, count);

    return HostVIO(nullptr, handle).PWrite(fd, buf, host_used ? hio_status : count, offset);
};

s64 QFS::OperationImpl::WriteV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov,
                               u32 iovcnt) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->write)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.WriteV(host_fd, iov, iovcnt); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    // TODO: desync possible
    vio_status = HostVIO(nullptr, handle).WriteV(fd, iov, iovcnt);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::PWriteV(const s32 fd, const Libraries::Kernel::OrbisKernelIovec* iov,
                                u32 iovcnt, s64 offset) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->write)
        return -POSIX_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PWriteV(host_fd, iov, iovcnt, offset); hio_status < 0) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }
        host_used = true;
    }

    // TODO: desync possible
    vio_status = HostVIO(nullptr, handle).PWriteV(fd, iov, iovcnt, offset);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::MKDir(const fs::path& path, u16 mode) {
    Resolved res;
    int resolve_status = qfs.Resolve(path, res);

    if (0 == resolve_status)
        return -POSIX_EEXIST;

    if (-POSIX_ENOENT == resolve_status) {
        if (nullptr == res.parent)
            return -POSIX_ENOENT;
    } else if (0 != resolve_status)
        return resolve_status;

    partition_ptr part = res.mountpoint;

    if (qfs.IsPartitionRO(part))
        return -POSIX_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            0 != hostpath_status)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.MKDir(host_path_target, mode); 0 != hio_status)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&res, nullptr).MKDir(res.local_path, mode);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::RMDir(const fs::path& path) {
    Resolved res;
    int status = qfs.Resolve(path, res);

    if (0 != status)
        return status;

    partition_ptr part = res.mountpoint;
    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            0 != hostpath_status)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.RMDir(host_path_target); 0 != hio_status)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&res, nullptr).RMDir(res.local_path);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return status;
}

s32 QFS::OperationImpl::Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) {
    Resolved res;
    int resolve_status = qfs.Resolve(path, res);

    if (nullptr == res.node || resolve_status < 0) {
        // parent node must exist, file does not
        return resolve_status;
    }

    return HostVIO(&res, nullptr).Stat(res.local_path, statbuf);
}

s32 QFS::OperationImpl::FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    return HostVIO(nullptr, handle).FStat(fd, statbuf);
}

s32 QFS::OperationImpl::Chmod(const fs::path& path, u16 mode) {
    Resolved res;
    int resolve_status = qfs.Resolve(path, res);

    if (nullptr == res.node || resolve_status < 0) {
        // parent node must exist, file does not
        return resolve_status;
    }

    return HostVIO(&res, nullptr).Chmod(res.local_path, mode);
}

s32 QFS::OperationImpl::FChmod(const s32 fd, u16 mode) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    return HostVIO(nullptr, handle).FChmod(fd, mode);
}

s64 QFS::OperationImpl::GetDents(const s32 fd, void* buf, u64 count, s64* basep) {
    if (fd < 0)
        return -POSIX_EBADF;

    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -POSIX_EBADF;

    if (!handle->read)
        return -POSIX_EBADF;

    return HostVIO(nullptr, handle).GetDents(fd, buf, count, basep);
}

s32 QFS::OperationImpl::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    return -POSIX_ENOSYS;
}

s32 QFS::OperationImpl::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    if (src.string().size() > 255)
        return -POSIX_ENAMETOOLONG;
    if (dst.string().size() > 255)
        return -POSIX_ENAMETOOLONG;

    Resolved src_res;
    Resolved dst_res;
    int status_what = qfs.Resolve(src, src_res);
    int status_where = qfs.Resolve(dst, dst_res);
    partition_ptr src_part = src_res.mountpoint;
    partition_ptr dst_part = dst_res.mountpoint;

    if (nullptr == src_part) {
        // source partition doesn't exist
        return -POSIX_ENOENT;
    }
    if (src_part != dst_part) {
        // moving between partitions is illegal
        return -POSIX_EXDEV;
    }
    if (src_res.node == dst_res.node)
        // hardlink to the same file, success
        return 0;

    // destination parent directory must exist though
    if (0 != status_where && nullptr == dst_res.parent)
        return -POSIX_ENOENT;

    if (qfs.IsPartitionRO(dst_part))
        return -POSIX_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    // for this to work, both files must be on host partition
    // mixed source/destination will need a bit more effort
    if (src_part->IsHostMounted() && dst_part->IsHostMounted()) {
        // if target partition doesn't exist or is not mounted, we can'tqfs.Resolve host path

        fs::path host_path_src{};
        fs::path host_path_dst{};

        if (int hostpath_status = src_part->GetHostPath(host_path_src, src_res.local_path);
            hostpath_status != 0)
            return hostpath_status;
        if (int hostpath_status = dst_part->GetHostPath(host_path_dst, dst_res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Move(host_path_src, host_path_dst, false); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    vio_status = HostVIO(&dst_res, nullptr).Move(src, dst_res.local_path, false);

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

} // namespace QuasiFS