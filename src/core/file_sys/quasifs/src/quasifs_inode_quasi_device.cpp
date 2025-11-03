// INAA License @marecl 2025

#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"

namespace QuasiFS {

Device::Device() {
    // fileno and blkdev assigned by partition
    this->st.st_size = 0;
    this->st.st_blksize = 0;
    this->st.st_blocks = 0;

    this->st.st_mode |= QUASI_S_IFCHR;
}

Device::~Device() = default;

s64 Device::pread(void* buf, u64 count, s64 offset) {
    return read(buf, count);
}

s64 Device::pwrite(const void* buf, u64 count, s64 offset) {
    return write(buf, count);
}

} // namespace QuasiFS