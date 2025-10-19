// INAA License @marecl 2025

#include "core/file_sys/quasifs/quasifs_inode_symlink.h"

namespace QuasiFS {

Symlink::Symlink(fs::path target) : target(target) {
    // fileno and blkdev assigned by partition
    this->st.st_size = target.string().size();
    this->st.st_mode = 0000755 | QUASI_S_IFLNK;
    this->st.st_nlink = 0;
    // not incrementing target, this type is a softlink
}

fs::path Symlink::follow(void) {
    return target;
}
} // namespace QuasiFS