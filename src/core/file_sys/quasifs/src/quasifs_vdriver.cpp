// INAA License @marecl 2025

#include <cstring>

#include "common/logging/log.h"

#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"

namespace QuasiFS {

s32 QFS::OperationImpl::Open(const fs::path& path, int flags, u16 mode) {
    Resolved res;
    // Resolve for parent dir to avoid treating ENOENT as missing just the end file
    int resolve_status = qfs.Resolve(path, res);

    // enoent on last element in the path is good
    if (-QUASI_ENOENT == resolve_status) {
        if (nullptr == res.parent)
            return -QUASI_ENOENT;
    } else if (0 != resolve_status)
        return resolve_status;

    partition_ptr part = res.mountpoint;
    dir_ptr parent_node = std::static_pointer_cast<Directory>(res.parent);

    bool request_read = !(flags & QUASI_O_WRONLY) || (flags & QUASI_O_RDWR);
    bool request_write = (flags & (QUASI_O_WRONLY | QUASI_O_RDWR)) != 0;
    bool request_append = flags & QUASI_O_APPEND;

    //
    // Orbis-specific checks, or at least ones that differ between Linux and Orbis
    // Universal checks are embedded into QFS
    // Some of those are universal, but order matters (a lot)
    //

    if ((flags & (QUASI_O_WRONLY | QUASI_O_RDWR)) == (QUASI_O_WRONLY | QUASI_O_RDWR))
        return -QUASI_EINVAL;

    // if it doesn't exist, check the parent
    inode_ptr checked_node = nullptr == res.node ? parent_node : res.node;

    if ((request_read && !checked_node->CanRead()) || (request_write && !checked_node->CanWrite()))
        return -QUASI_EACCES;

    if ((request_write || request_append) && qfs.IsPartitionRO(part))
        return -QUASI_EROFS;

    if (flags & QUASI_O_DIRECTORY && flags & QUASI_O_CREAT)
        return -QUASI_ENOTDIR;

    // O_TRUNC | O_RDONLY - throw einval but touch a file
    // TODO: find out what happens when can't create file before throwing einval
    if ((flags & (QUASI_O_TRUNC | QUASI_O_WRONLY | QUASI_O_RDWR)) == QUASI_O_TRUNC) {
        if (int status = this->Close(this->Creat(path, mode)); status != 0)
            return status;
        return -QUASI_ENOENT;
    }

    //
    // Proceed
    //

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Open(host_path_target, flags, mode); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.Open(res.local_path, flags, mode);
    qfs.vio_driver.ClearCtx();

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
    auto next_free_handle = qfs.GetFreeHandleNo();
    qfs.open_fd[next_free_handle] = handle;
    return next_free_handle;
}

s32 QFS::OperationImpl::Creat(const fs::path& path, u16 mode) {
    return Open(path, QUASI_O_CREAT | QUASI_O_WRONLY | QUASI_O_TRUNC, mode);
};

s32 QFS::OperationImpl::Close(s32 fd) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    // if it fails, it fails
    if (int hio_status = qfs.hio_driver.Close(handle->host_fd); hio_status < 0)
        return hio_status;

    // no further action is required, this is pro-forma
    qfs.vio_driver.Close(fd);

    // if it's the last entry, remove it to avoid blowing up fd table
    // not really helping with fragmentation, but may save resources on burst opens

    if (fd < (qfs.open_fd.size() - 1)) {
        qfs.open_fd.at(fd) = nullptr;
        return 0;
    }

    qfs.open_fd.pop_back();
    return 0;
}

s32 QFS::OperationImpl::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    Resolved src_res;
    Resolved dst_res;
    int status_what = qfs.Resolve(src, src_res);
    int status_where = qfs.Resolve(dst, dst_res);

    // source may not exist and can point wherever it wants, so we skip checks for it

    if (0 == status_where)
        return -QUASI_EEXIST;
    // destination parent directory must exist though
    if (0 != status_where && nullptr == dst_res.parent)
        return -QUASI_ENOENT;

    partition_ptr src_part = src_res.mountpoint;
    partition_ptr dst_part = dst_res.mountpoint;

    if (qfs.IsPartitionRO(dst_part))
        return -QUASI_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    // for this to work, both files must be on host partition
    // mixed source/destination will need a bit more effort
    if (src_part->IsHostMounted() && dst_part->IsHostMounted()) {
        // if target partition doesn't exist or is not mounted, we can'tqfs.Resolve host path
        if (nullptr == src_part)
            return -QUASI_ENOENT;

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
                  "Symlinks can be only created if both source and destination are host-bound");
        return -QUASI_ENOSYS;
    }

    qfs.vio_driver.SetCtx(&dst_res, host_used, nullptr);
    // src stays 1:1
    vio_status = qfs.vio_driver.LinkSymbolic(src, dst_res.local_path);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Link(const fs::path& src, const fs::path& dst) {
    Resolved src_res;
    Resolved dst_res;
    int status_what = qfs.Resolve(src, src_res);
    int status_where = qfs.Resolve(dst, dst_res);

    if (0 != status_what)
        return status_what;
    if (0 == status_where)
        return -QUASI_EEXIST;

    // cross-partition linking is not supported
    if (src_res.mountpoint != dst_res.mountpoint)
        return -QUASI_EXDEV;

    partition_ptr src_part = src_res.mountpoint;
    partition_ptr dst_part = dst_res.mountpoint;

    if (qfs.IsPartitionRO(dst_part))
        return -QUASI_EROFS;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (dst_part->IsHostMounted() && src_part->IsHostMounted()) {
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
    } else if (dst_part->IsHostMounted() ^ src_part->IsHostMounted()) {
        LOG_ERROR(Kernel_Fs,
                  "Links can be only created if both source and destination are host-bound");
        return -QUASI_ENOSYS;
    }

    qfs.vio_driver.SetCtx(&src_res, host_used, nullptr);
    vio_status = qfs.vio_driver.Link(src_res.local_path, dst_res.local_path);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Unlink(const fs::path& path) {
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
        return -QUASI_ENOTDIR;

    partition_ptr part = res.mountpoint;
    if (qfs.IsPartitionRO(part))
        return -QUASI_EROFS;

    dir_ptr parent = std::static_pointer_cast<Directory>(res.node);
    inode_ptr target = parent->lookup(leaf);

    if (nullptr == target)
        return -QUASI_ENOENT;

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

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.Unlink(res.local_path);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Flush(const s32 fd) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->read)
        return -QUASI_EBADF;

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

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.Flush(fd);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FSync(const s32 fd) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->read)
        return -QUASI_EBADF;

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

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.FSync(fd);
    qfs.vio_driver.ClearCtx();

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
        return -QUASI_EROFS;

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

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.Truncate(res.local_path, length);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FTruncate(const s32 fd, u64 length) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->write)
        return -QUASI_EBADF;

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

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.FTruncate(fd, length);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::LSeek(const s32 fd, u64 offset, SeekOrigin origin) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.LSeek(host_fd, offset, origin); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.LSeek(fd, offset, origin);
    qfs.vio_driver.ClearCtx();

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

s64 QFS::OperationImpl::Write(const s32 fd, const void* buf, u64 count) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->write)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.Write(host_fd, buf, count); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.Write(fd, buf, count);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::PWrite(const s32 fd, const void* buf, u64 count, u64 offset) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->write)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PWrite(host_fd, buf, count, offset); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.PWrite(fd, buf, count, offset);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
};

s64 QFS::OperationImpl::Read(const s32 fd, void* buf, u64 count) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->read)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.Read(host_fd, buf, count); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.Read(fd, buf, count);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s64 QFS::OperationImpl::PRead(const s32 fd, void* buf, u64 count, u64 offset) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->read)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.PRead(host_fd, buf, count, offset); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.PRead(fd, buf, count, offset);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
};

s32 QFS::OperationImpl::MKDir(const fs::path& path, u16 mode) {
    Resolved res;
    int resolve_status = qfs.Resolve(path, res);

    if (0 == resolve_status)
        return -QUASI_EEXIST;

    if (-QUASI_ENOENT == resolve_status) {
        if (nullptr == res.parent)
            return -QUASI_ENOENT;
    } else if (0 != resolve_status)
        return resolve_status;

    partition_ptr part = res.mountpoint;

    if (qfs.IsPartitionRO(part))
        return -QUASI_EROFS;

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

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.MKDir(res.local_path, mode);
    qfs.vio_driver.ClearCtx();

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

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    status = qfs.vio_driver.RMDir(res.local_path);
    qfs.vio_driver.ClearCtx();

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

    partition_ptr part = res.mountpoint;
    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    Libraries::Kernel::OrbisKernelStat hio_stat;
    Libraries::Kernel::OrbisKernelStat vio_stat;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Stat(host_path_target, &hio_stat); 0 != hio_status) {
            // hosts operation must succeed in order to continue
            return hio_status;
        }

        host_used = true;
    }

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.Stat(res.local_path, &vio_stat);
    qfs.vio_driver.ClearCtx();

    if (host_used) {
        vio_stat.st_mode = hio_stat.st_mode;
        vio_stat.st_size = hio_stat.st_size;
        vio_stat.st_blksize = hio_stat.st_blksize;
        vio_stat.st_blocks = hio_stat.st_blocks;
        vio_stat.st_atim.tv_sec = hio_stat.st_atim.tv_sec;
        vio_stat.st_atim.tv_nsec = hio_stat.st_atim.tv_nsec;
        vio_stat.st_mtim.tv_sec = hio_stat.st_mtim.tv_sec;
        vio_stat.st_mtim.tv_nsec = hio_stat.st_mtim.tv_nsec;
        vio_stat.st_ctim.tv_sec = hio_stat.st_ctim.tv_sec;
        vio_stat.st_ctim.tv_nsec = hio_stat.st_ctim.tv_nsec;
    }

    memcpy(statbuf, &vio_stat, sizeof(Libraries::Kernel::OrbisKernelStat));

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FStat(const s32 fd, Libraries::Kernel::OrbisKernelStat* statbuf) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    Libraries::Kernel::OrbisKernelStat hio_stat;
    Libraries::Kernel::OrbisKernelStat vio_stat;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        hio_status = qfs.hio_driver.FStat(host_fd, &hio_stat);
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.FStat(fd, &vio_stat);
    qfs.vio_driver.ClearCtx();

    if (host_used) {
        vio_stat.st_mode = hio_stat.st_mode;
        vio_stat.st_size = hio_stat.st_size;
        vio_stat.st_blksize = hio_stat.st_blksize;
        vio_stat.st_blocks = hio_stat.st_blocks;
        vio_stat.st_atim.tv_sec = hio_stat.st_atim.tv_sec;
        vio_stat.st_atim.tv_nsec = hio_stat.st_atim.tv_nsec;
        vio_stat.st_mtim.tv_sec = hio_stat.st_mtim.tv_sec;
        vio_stat.st_mtim.tv_nsec = hio_stat.st_mtim.tv_nsec;
        vio_stat.st_ctim.tv_sec = hio_stat.st_ctim.tv_sec;
        vio_stat.st_ctim.tv_nsec = hio_stat.st_ctim.tv_nsec;
    }

    memcpy(statbuf, &vio_stat, sizeof(Libraries::Kernel::OrbisKernelStat));

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::Chmod(const fs::path& path, u16 mode) {
    Resolved res;
    int resolve_status = qfs.Resolve(path, res);

    if (nullptr == res.node || resolve_status < 0) {
        // parent node must exist, file does not
        return resolve_status;
    }

    partition_ptr part = res.mountpoint;
    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (part->IsHostMounted()) {
        fs::path host_path_target{};
        if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path);
            hostpath_status != 0)
            return hostpath_status;

        if (hio_status = qfs.hio_driver.Chmod(host_path_target, mode); 0 != hio_status)
            // hosts operation must succeed in order to continue
            return hio_status;

        host_used = true;
    }

    qfs.vio_driver.SetCtx(&res, host_used, nullptr);
    vio_status = qfs.vio_driver.Chmod(res.local_path, mode);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

s32 QFS::OperationImpl::FChmod(const s32 fd, u16 mode) {
    fd_handle_ptr handle = qfs.GetHandle(fd);
    if (nullptr == handle)
        return -QUASI_EBADF;

    if (!handle->read)
        return -QUASI_EBADF;

    bool host_used = false;
    int hio_status = 0;
    int vio_status = 0;

    if (handle->IsHostBound()) {
        int host_fd = handle->host_fd;
        if (hio_status = qfs.hio_driver.FChmod(host_fd, mode); hio_status < 0)
            // hosts operation must succeed in order to continue
            return hio_status;
        host_used = true;
    }

    qfs.vio_driver.SetCtx(nullptr, host_used, handle);
    vio_status = qfs.vio_driver.FChmod(fd, mode);
    qfs.vio_driver.ClearCtx();

    if (host_used && (hio_status != vio_status))
        LOG_ERROR(Kernel_Fs, "Host returned {}, but virtual driver returned {}", hio_status,
                  vio_status);

    return vio_status;
}

} // namespace QuasiFS