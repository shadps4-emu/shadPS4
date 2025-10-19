// INAA License @marecl 2025

#include "core/file_sys/quasifs/quasifs_inode_device.h"

namespace QuasiFS {

Device::Device() {
    // fileno and blkdev assigned by partition
    this->st.st_size = 0;
    this->st.st_blksize = 0;
    this->st.st_blocks = 0;

    this->st.st_mode = 0000755 | QUASI_S_IFCHR;
    this->st.st_nlink = 0;
    // not incrementing target, this type is a softlink
}

} // namespace QuasiFS