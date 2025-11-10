// INAA License @marecl 2025

#include <vector>

#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_file.h"

namespace QuasiFS {

s64 QuasiFile::pread(void* buf, size_t count, s64 offset) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;

    st.st_atim.tv_sec = time(0);
    return end_pos > *size ? *size - offset : count;
}

s64 QuasiFile::pwrite(const void* buf, size_t count, s64 offset) {
    auto& size = this->st.st_size;
    auto end_pos = offset + count;

    size = end_pos > size ? end_pos : size;

    st.st_mtim.tv_sec = time(0);
    return count;
}

s64 QuasiFile::lseek(s64 current, s64 offset, s32 whence) {
    return ((SeekOrigin::ORIGIN == whence) * offset) +
           ((SeekOrigin::CURRENT == whence) * (current + offset)) +
           ((SeekOrigin::END == whence) * (this->st.st_size + offset));
}

s32 QuasiFile::ftruncate(s64 length) {
    if (length < 0)
        return -QUASI_EINVAL;
    this->st.st_size = length;
    st.st_mtim.tv_sec = time(0);
    return 0;
}

} // namespace QuasiFS