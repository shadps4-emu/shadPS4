// INAA License @marecl 2025

#include <cstring>
#include <filesystem>

#include "common/logging/log.h"
#include "core/file_sys/hostio/host_io_posix.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"
#include "core/file_sys/quasifs/quasifs_inode_symlink.h"
#include "core/file_sys/quasifs/quasifs_partition.h"
#include "src/core/file_sys/hostio/host_io_virtual.h"
#include "src/core/file_sys/quasifs/quasi_errno.h"
#include "src/core/file_sys/quasifs/quasi_sys_fcntl.h"
#include "src/core/file_sys/quasifs/quasi_types.h"
#include "src/core/file_sys/quasifs/quasifs_inode_quasi_file_virtual.h"

#include "host_io_base.h"

namespace HostIODriver {

HostIO_Virtual::HostIO_Virtual() = default;
HostIO_Virtual::~HostIO_Virtual() = default;

s32 HostIO_Virtual::Open(const fs::path& path, s32 flags, u16 mode) {
    if (int remainder = flags & ~__QUASI_O_ALLFLAGS_AT_ONCE; remainder != 0)
        LOG_WARNING(Kernel_Fs, "open() received unknown flags: {:x}", remainder);

    if (nullptr == this->res)
        return -QUASI_EINVAL;

    partition_ptr part = this->res->mountpoint;
    dir_ptr parent = this->res->parent;
    inode_ptr target = this->res->node;

    bool exists = this->res->node != nullptr;

    if (exists && (flags & QUASI_O_EXCL) && (flags & QUASI_O_CREAT))
        return -QUASI_EEXIST;

    if (!exists) {
        if ((flags & QUASI_O_CREAT) == 0)
            return -QUASI_ENOENT;

        target =
            this->host_bound ? QuasiFile::Create<RegularFile>() : QuasiFile::Create<VirtualFile>();
        target->chmod(mode);
        if (0 != part->touch(parent, this->res->leaf, target))
            // touch failed in target directory, issue with resolve() is most likely
            return -QUASI_EFAULT;

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
        return -QUASI_EISDIR;

    if ((flags & QUASI_O_DIRECTORY) && !target->is_dir())
        // opening dirs isn't supported yet
        return -QUASI_ENOTDIR;

    if (flags & (QUASI_O_NONBLOCK | QUASI_O_SYNC | QUASI_O_DIRECT | QUASI_O_DSYNC)) {
        // unused, not affecting file manip per-se
    }

    return 0;
}

s32 HostIO_Virtual::Creat(const fs::path& path, u16 mode) {
    return Open(path, QUASI_O_CREAT | QUASI_O_TRUNC | QUASI_O_WRONLY);
}

s32 HostIO_Virtual::Close(const s32 fd) {
    // N/A
    return 0;
}

s32 HostIO_Virtual::Link(const fs::path& src, const fs::path& dst) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    partition_ptr part = this->res->mountpoint;
    inode_ptr src_node = this->res->node;

    Resolved dst_res;
    fs::path dst_path = dst.parent_path();
    std::string dst_name = dst.filename().string();

    if (int res_status = part->Resolve(dst_path, dst_res); res_status < 0)
        return res_status;

    if (!dst_res.node->is_dir())
        return -QUASI_ENOTDIR;

    dir_ptr dst_parent = std::static_pointer_cast<Directory>(dst_res.node);
    return part->link(src_node, dst_parent, dst_name);
}

s32 HostIO_Virtual::Unlink(const fs::path& path) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;
    if ("." == this->res->leaf)
        return -QUASI_EINVAL;

    if (nullptr == this->res->node)
        return -QUASI_ENOENT;

    partition_ptr part = this->res->mountpoint;
    dir_ptr parent = this->res->parent;
    return part->unlink(parent, this->res->leaf);
}

s32 HostIO_Virtual::LinkSymbolic(const fs::path& src, const fs::path& dst) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    symlink_ptr sym = Symlink::Create(src);
    // symlink counter is never increased
    sym->st.st_nlink = 1;

    return this->res->mountpoint->touch(this->res->parent, dst.filename().string(), sym);
}

s32 HostIO_Virtual::Flush(const s32 fd) {
    // not applicable
    return 0;
}

s32 HostIO_Virtual::FSync(const s32 fd) {
    if (nullptr == handle)
        return -QUASI_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    return handle->node->fsync();
}

s64 HostIO_Virtual::LSeek(const s32 fd, u64 offset, QuasiFS::SeekOrigin origin) {
    if (nullptr == handle)
        return -QUASI_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    auto ptr = &handle->pos;

    u64 new_ptr = ((SeekOrigin::ORIGIN == origin) * offset) +
                  ((SeekOrigin::CURRENT == origin) * (*(ptr) + offset)) +
                  ((SeekOrigin::END == origin) * (node->st.st_size + offset));

    if (new_ptr < 0)
        return -QUASI_EINVAL;

    *ptr = new_ptr;
    return *ptr;
}

s64 HostIO_Virtual::Tell(const s32 fd) {
    return LSeek(fd, 0, SeekOrigin::CURRENT);
}

s32 HostIO_Virtual::Truncate(const fs::path& path, u64 size) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    if (node->is_dir())
        return -QUASI_EISDIR;

    if (!node->is_file())
        return -QUASI_EINVAL;

    return node->ftruncate(size);
}

s32 HostIO_Virtual::FTruncate(const s32 fd, u64 size) {
    if (nullptr == handle)
        return -QUASI_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    return handle->node->ftruncate(size);
}

s64 HostIO_Virtual::Read(const s32 fd, void* buf, u64 count) {
    s64 br = PRead(fd, buf, count, handle->pos);

    if (br > 0)
        handle->pos += br;

    return br;
}

s64 HostIO_Virtual::PRead(const s32 fd, void* buf, u64 count, u64 offset) {
    if (nullptr == handle)
        return -QUASI_EINVAL;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

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
        return -QUASI_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

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

s64 HostIO_Virtual::PWrite(const s32 fd, const void* buf, u64 count, u64 offset) {
    if (nullptr == handle)
        return -QUASI_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

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
    if (nullptr == handle)
        return -QUASI_EBADF;

    inode_ptr node = handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    if (handle->append)
        offset = node->st.st_size;

    return node->pwritev(iov, iovcnt, offset);
}

s32 HostIO_Virtual::MKDir(const fs::path& path, u16 mode) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    dir_ptr new_dir = Directory::Create<Directory>();
    return this->res->mountpoint->mkdir(this->res->parent, this->res->leaf, new_dir);
}

s32 HostIO_Virtual::RMDir(const fs::path& path) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    // EINVAL on . as last element

    // don't remove partition root ;___;
    dir_ptr parent = this->res->parent;

    if (parent->mounted_root)
        return -QUASI_EBUSY;

    if (int unlink_status = res->mountpoint->rmdir(parent, this->res->leaf); unlink_status != 0)
        return unlink_status;

    auto target_nlink = res->node->st.st_nlink;
    if (target_nlink != 0) {
        LOG_ERROR(Kernel_Fs, "RMDir'd directory nlink is not 0!", "(is ", target_nlink, ")");
        return -QUASI_ENOTEMPTY;
    }

    return 0;
}

s32 HostIO_Virtual::Stat(const fs::path& path, OrbisKernelStat* statbuf) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -QUASI_ENOENT;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::FStat(const s32 fd, OrbisKernelStat* statbuf) {
    if (nullptr == this->handle)
        return -QUASI_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    memcpy(statbuf, &node->st, sizeof(OrbisKernelStat));

    return 0;
}

s32 HostIO_Virtual::Chmod(const fs::path& path, u16 mode) {
    if (nullptr == this->res)
        return -QUASI_EINVAL;

    inode_ptr node = this->res->node;

    if (nullptr == node)
        return -QUASI_ENOENT;

    return Partition::chmod(node, mode);
}

s32 HostIO_Virtual::FChmod(const s32 fd, u16 mode) {
    if (nullptr == this->handle)
        return -QUASI_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    return Partition::chmod(node, mode);
}

s64 HostIO_Virtual::GetDents(s32 fd, void* buf, u32 nbytes, s64* basep) {
    if (nullptr == this->handle)
        return -QUASI_EINVAL;

    inode_ptr node = this->handle->node;

    if (nullptr == node)
        return -QUASI_EBADF;

    if (nullptr != basep)
        *basep = handle->pos;

    s64 br = node->getdents(static_cast<char*>(buf) + handle->pos, nbytes, basep);
    if (br > 0)
        handle->pos += br;

    return br;
}

} // namespace HostIODriver
