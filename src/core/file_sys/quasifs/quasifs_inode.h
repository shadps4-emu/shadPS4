// INAA License @marecl 2025

#pragma once

#include <ctime>
#include <vector>
#include <assert.h>

#include "common/types.h"
#include "common/va_ctx.h"
#include "core/libraries/kernel/file_system.h"

#include "quasi_errno.h"
#include "quasi_sys_stat.h"
#include "quasi_types.h"

namespace QuasiFS {

static_assert(sizeof(time_t) == 8, "Time is not stored in 64 bits");

class Inode {
public:
    Inode() {
        st.st_mode = 0000755;
        st.st_nlink = 0;
        st.st_uid = 0;
        st.st_gid = 0;

        st.st_birthtim.tv_sec = time(0);
        st.st_birthtim.tv_nsec = 0;
        st.st_ctim.tv_sec = time(0);
        st.st_ctim.tv_nsec = 0;
        st.st_mtim.tv_sec = time(0);
        st.st_mtim.tv_nsec = 0;
        st.st_atim.tv_sec = time(0);
        st.st_atim.tv_nsec = 0;
    }

    virtual ~Inode() = default;

    virtual s32 ioctl(u64 cmd, Common::VaCtx* args) {
        return -QUASI_ENOTTY;
    }

    virtual s64 pread(void* buf, u64 count, s64 offset) {
        return -QUASI_EBADF;
    }

    virtual s64 pwrite(const void* buf, u64 count, s64 offset) {
        return -QUASI_EBADF;
    }

    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
        u64 tb = 0;
        for (unsigned int idx = 0; idx < iovcnt; idx++) {
            int status = this->pread(iov[idx].iov_base, iov[idx].iov_len, offset);
            if (status < 0)
                return status;
            tb += status;
        }
        return tb;
    }

    virtual s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, u32 iovcnt, s64 offset) {
        u64 tb = 0;
        for (unsigned int idx = 0; idx < iovcnt; idx++) {
            int status = this->pwrite(iov[idx].iov_base, iov[idx].iov_len, offset);
            if (status < 0)
                return status;
            tb += status;
        }
        return tb;
    }

    virtual s64 lseek(s64 offset, int whence) {
        return -QUASI_EBADF;
    }

    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) {
        *sb = this->st;
        return 0;
    }

    virtual s32 fsync() {
        return -QUASI_EBADF;
    }

    virtual s32 ftruncate(s64 length) {
        return -QUASI_EINVAL;
    }

    virtual s64 getdents(void* buf, u32 nbytes, s64* basep) {
        return -QUASI_EINVAL;
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

    fileno_t GetFileno(void) {
        return this->fileno;
    }
    fileno_t SetFileno(fileno_t fileno) {
        this->fileno = fileno;
        return fileno;
    };

    fileno_t fileno{-1};
    struct Libraries::Kernel::OrbisKernelStat st{};

    int chmod(u16 mode) {
        u16* st_mode = &this->st.st_mode;
        *st_mode = ((*st_mode) & (~0x1FF)) | (mode & 0x1FF);
        st.st_birthtim.tv_sec = time(0);
        return 0;
    }
};

} // namespace QuasiFS