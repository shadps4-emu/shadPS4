// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include <ctime>
#include <vector>
#include <assert.h>

#include "common/scope_exit.h"
#include "common/types.h"
#include "common/va_ctx.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/posix_error.h"

#include "quasi_sys_stat.h"
#include "quasi_types.h"

namespace QuasiFS {

static_assert(sizeof(time_t) == 8, "Time is not stored in 64 bits");

class Inode {
protected:
    // This variable is SACRIFICIAL and TEMPORARY
    // It's set by calling function, as multiple fd's can be used with a single file;
    // ergo every caller must remember to set it to its liking
    s64 descriptor_offset{0};

public:
    Inode() {
        st.st_ino = 0;
        st.st_mode = 0000755;
        st.st_nlink = 0;
        st.st_uid = 0;
        st.st_gid = 0;

        st.st_birthtim.tv_sec = time(nullptr);
        st.st_birthtim.tv_nsec = 0;
        st.st_ctim.tv_sec = time(nullptr);
        st.st_ctim.tv_nsec = 0;
        st.st_mtim.tv_sec = time(nullptr);
        st.st_mtim.tv_nsec = 0;
        st.st_atim.tv_sec = time(nullptr);
        st.st_atim.tv_nsec = 0;
    }

    virtual ~Inode() = default;

    inode_ptr Clone() const {
        auto _out = std::make_shared<Inode>(*this);
        _out->st.st_ino = 0;
        _out->st.st_nlink = 0;
        return _out;
    }

    virtual s32 ioctl(u64 cmd, Common::VaCtx* args) {
        return -POSIX_ENOTTY;
    }

    virtual s64 read(void* buf, u64 count) {
        return -POSIX_EBADF;
    }

    virtual s64 write(const void* buf, u64 count) {
        return -POSIX_EBADF;
    }

    virtual s64 pread(void* buf, u64 count, s64 offset) {
        s64 old_offset = this->descriptor_offset;
        this->descriptor_offset = offset;
        SCOPE_EXIT {
            this->descriptor_offset = old_offset;
        };

        return this->read(buf, count);
    }

    virtual s64 pwrite(const void* buf, u64 count, s64 offset) {
        s64 old_offset = this->descriptor_offset;
        this->descriptor_offset = offset;
        SCOPE_EXIT {
            this->descriptor_offset = old_offset;
        };

        return this->write(buf, count);
    }

    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt) {
        u64 tbr = 0;
        for (unsigned int idx = 0; idx < iovcnt; idx++) {
            int status = this->read(iov[idx].iov_base, iov[idx].iov_len);
            if (status < 0)
                return status;
            tbr += status;
        }
        return tbr;
    }

    virtual s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt) {
        u64 tbw = 0;
        for (unsigned int idx = 0; idx < iovcnt; idx++) {
            int status = this->write(iov[idx].iov_base, iov[idx].iov_len);
            if (status < 0)
                return status;
            tbw += status;
        }
        return tbw;
    }

    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
        s64 old_offset = this->descriptor_offset;
        this->descriptor_offset = offset;
        SCOPE_EXIT {
            this->descriptor_offset = old_offset;
        };

        return this->readv(iov, iovcnt);
    }

    virtual s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
        s64 old_offset = this->descriptor_offset;
        this->descriptor_offset = offset;
        SCOPE_EXIT {
            this->descriptor_offset = old_offset;
        };

        return this->writev(iov, iovcnt);
    }

    virtual s64 lseek(s64 offset, s32 whence) {
        this->descriptor_offset =
            ((SeekOrigin::ORIGIN == whence) * offset) +
            ((SeekOrigin::CURRENT == whence) * (this->descriptor_offset + offset)) +
            ((SeekOrigin::END == whence) * (this->st.st_size + offset));
        return this->descriptor_offset >= 0 ? this->descriptor_offset : -POSIX_EINVAL;
    }

    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) {
        *sb = this->st;
        return 0;
    }

    virtual s32 fsync() {
        return -POSIX_EBADF;
    }

    virtual s32 ftruncate(s64 length) {
        return -POSIX_EINVAL;
    }

    virtual s64 getdents(void* buf, u64 count, s64* basep) {
        return -POSIX_EINVAL;
    }

    // type helpers
    u16 type(void) const {
        return st.st_mode & QUASI_S_IFMT;
    }

    bool is_fifo(void) const {
        return QUASI_S_ISLNK(st.st_mode);
    }
    bool is_char(void) const {
        return QUASI_S_ISCHR(st.st_mode);
    }
    bool is_dir(void) const {
        return QUASI_S_ISDIR(st.st_mode);
    }
    bool is_block(void) const {
        return QUASI_S_ISBLK(st.st_mode);
    }
    bool is_file(void) const {
        return QUASI_S_ISREG(st.st_mode);
    }
    bool is_link(void) const {
        return QUASI_S_ISLNK(st.st_mode);
    }
    bool is_socket(void) const {
        return QUASI_S_ISSOCK(st.st_mode);
    }

    bool CanRead(void) {
        return this->st.st_mode & (QUASI_S_IRUSR | QUASI_S_IRGRP | QUASI_S_IROTH);
    }
    bool CanWrite(void) {
        return this->st.st_mode & (QUASI_S_IWUSR | QUASI_S_IWGRP | QUASI_S_IWOTH);
    }
    bool CanExecute(void) {
        return this->st.st_mode & (QUASI_S_IXUSR | QUASI_S_IXGRP | QUASI_S_IXOTH);
    }

    // init - used by qfs/partition
    u32 __GetFileno(void) {
        return this->st.st_ino;
    }
    void __SetFileno(fileno_t fileno) {
        this->st.st_ino = fileno;
    }

    // file descriptors - set
    s64 __GetOffset(void) {
        return this->descriptor_offset;
    }
    void __SetOffset(s64 new_offset) {
        this->descriptor_offset = new_offset;
    }

    int chmod(u16 mode) {
        u16& st_mode = this->st.st_mode;
        st_mode = ((st_mode) & (~0x1FF)) | (mode & 0x1FF);
        st.st_birthtim.tv_sec = time(0);
        return 0;
    }

    struct Libraries::Kernel::OrbisKernelStat st{};
};

} // namespace QuasiFS