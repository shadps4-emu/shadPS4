// INAA License @marecl 2025

#include <vector>

#include "../quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_regularfile.h"

namespace QuasiFS {

s64 RegularFile::read(void* buf, size_t count) {
    return pread(buf, count, 0);
}

s64 RegularFile::write(const void* buf, size_t count) {
    return pwrite(buf, count, 0);
}

s64 RegularFile::pread(void* buf, size_t count, u64 offset) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;

    return end_pos > *size ? *size - offset : count;
}

s64 RegularFile::pwrite(const void* buf, size_t count, u64 offset) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;

    *size = end_pos > *size ? end_pos : *size;

    return count;
}

s32 RegularFile::ftruncate(s64 length) {
    if (length < 0)
        return -QUASI_EINVAL;
    this->st.st_size = length;
    return 0;
}

} // namespace QuasiFS