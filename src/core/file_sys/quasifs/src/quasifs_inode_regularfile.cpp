// INAA License @marecl 2025

#include <vector>

#include "../quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_regularfile.h"

namespace QuasiFS {

RegularFile::RegularFile() {
    st.st_mode = 0000755 | QUASI_S_IFREG;
    st.st_nlink = 0;
}

s64 RegularFile::pread(void* buf, size_t count, u64 offset) {
    s64 idx;
    u64 read_amt = this->data.size() - offset - count;

    // if >= 0 - we're good to go
    // <0 - n-bytes are missing, won't enter loop
    read_amt = count + read_amt * (read_amt < 0);

    for (idx = 0; idx < read_amt; idx++) {
        char c = this->data.at(idx + offset);
        static_cast<char*>(buf)[idx] = c;
    }

    return read_amt;
}

s64 RegularFile::pwrite(const void* buf, size_t count, u64 offset) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;
    *size = end_pos > *size ? end_pos : *size;

    // size can only be greater, so it will always scale up
    this->data.resize(*size, 0);

    for (u64 idx = offset; idx < *size; idx++)
        this->data[idx] = static_cast<const char*>(buf)[idx];

    return count;
}

s32 RegularFile::ftruncate(s64 length) {
    if (length < 0)
        return -QUASI_EINVAL;
    this->data.resize(length, 0);
    this->st.st_size = length;
    return 0;
}

s64 RegularFile::MockRead(u64 offset, void* buf, u64 count) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;

    return end_pos > *size ? *size - offset : count;
}

s64 RegularFile::MockWrite(u64 offset, const void* buf, u64 count) {
    auto size = &this->st.st_size;
    auto end_pos = offset + count;

    *size = end_pos > *size ? end_pos : *size;

    return count;
}

int RegularFile::MockTruncate(u64 length) {
    if (length < 0)
        return -QUASI_EINVAL;
    this->st.st_size = length;
    return 0;
}
} // namespace QuasiFS