// INAA License @marecl 2025

#include <map>
#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory_pfs.h"

namespace QuasiFS {

DirectoryPFS::DirectoryPFS() {
    this->st.st_size = 65536;
}

DirectoryPFS::~DirectoryPFS() = default;

s64 DirectoryPFS::pread(void* buf, u64 count, s64 offset) {
    RebuildDirents();
    memset(buf, 0, count);

    // data is contiguous. i think that's how it's said
    // anyway, everything goes raw
    // always returns count
    // always zeroes buffer
    // no cut-off, will just spew out data whenever you ask for it

    if (offset >= this->dirent_cache_bin.size())
        return count;

    s64 bytes_available = this->dirent_cache_bin.size() - offset;
    // bytes_to_read but retains the same variable lmao
    if (bytes_available >= count)
        bytes_available = count;

    memcpy(buf, this->dirent_cache_bin.data() + offset, bytes_available);

    // always returns count
    return count;
}

// s64 DirectoryPFS::lseek(s64 current, s64 offset, s32 whence) {
//     LOG_ERROR(Kernel_Fs, "(PFS STUB)");

//     // TBD, most likely acts like a file would

//     switch (whence) {
//     case 0:
//         return offset;
//     case 1:
//         return current + offset;
//     case 2:
//         return this->st.st_size + offset;
//     }
//     UNREACHABLE_MSG("lseek with unknown whence {}", whence);
//     return -QUASI_ENOSYS;
// }

s64 DirectoryPFS::getdents(void* buf, u32 count, s64 offset, s64* basep) {
    RebuildDirents();
    memset(buf, 0, count);

    // honestly i have no idea on how to implement this
    // buffer behaves like a regular directory
    // variable sizes are same as dirent_t, but data is still aligned to 8 bytes
    // initial call pulls all COMPLETE dirents (like base), but returns real amount of bytes
    // for some reason, subsequent calls return 0

    return count;
}

void DirectoryPFS::RebuildDirents(void) {

    // adding/removing entries changes mtime
    if (this->st.st_mtim.tv_sec == this->last_dirent_rebuild_time)
        return;
    this->last_dirent_rebuild_time = this->st.st_mtim.tv_sec;

    constexpr u32 dirent_meta_size = sizeof(dirent_pfs_t::d_fileno) + sizeof(dirent_pfs_t::d_type) +
                                     sizeof(dirent_pfs_t::d_namlen) +
                                     sizeof(dirent_pfs_t::d_reclen);

    u64 dirent_size = 0;
    this->dirent_cache_bin.clear();

    for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
        dirent_pfs_t tmp{};
        inode_ptr node = entry->second;
        std::string name = entry->first;

        tmp.d_fileno = node->fileno;
        tmp.d_namlen = name.size();
        strncpy(tmp.d_name, name.data(), tmp.d_namlen + 1);
        tmp.d_type = node->type() >> 12;
        tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 8);

        auto dirent_ptr = reinterpret_cast<const u8*>(&tmp);
        dirent_cache_bin.insert(dirent_cache_bin.end(), dirent_ptr, dirent_ptr + tmp.d_reclen);
        dirent_size += tmp.d_reclen;
    }

    // directory size is always 65536 bytes

    return;
}

} // namespace QuasiFS