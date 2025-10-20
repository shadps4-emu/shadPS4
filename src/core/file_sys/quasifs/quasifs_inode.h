// INAA License @marecl 2025

#pragma once

#include <vector>

#include "common/types.h"
#include "common/va_ctx.h"
#include "core/libraries/kernel/file_system.h"

#include "quasi_errno.h"
#include "quasi_sys_stat.h"
#include "quasi_types.h"

namespace QuasiFS {

class Inode {
public:
    Inode() {
        st.st_mode = 0000755;
        st.st_nlink = 0;
    }

    virtual ~Inode() = default;

    virtual s32 ioctl(u64 cmd, Common::VaCtx* args) {
        return -QUASI_ENOTTY;
    }

    virtual s64 read(void* buf, size_t count) {
        return -QUASI_EBADF;
    }

    virtual s64 write(const void* buf, size_t count) {
        return -QUASI_EBADF;
    }

    virtual s64 pread(void* buf, size_t count, u64 offset) {
        return -QUASI_EBADF;
    }

    virtual s64 pwrite(const void* buf, size_t count, u64 offset) {
        return -QUASI_EBADF;
    }

    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
        return -QUASI_EBADF;
    }

    virtual s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
        return -QUASI_EBADF;
    }

    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
        return -QUASI_EBADF;
    }

    virtual s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
        return -QUASI_EBADF;
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
        return -QUASI_EBADF;
    }

    virtual s32 getdents(void* buf, u32 nbytes, s64* basep) {
        return -QUASI_EBADF;
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
        return 0;
    }
};

} // namespace QuasiFS