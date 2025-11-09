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

    auto it = dirent_cache.lower_bound(offset);

    if (it == dirent_cache.end())
        return 0;

    u64 cumulative_offset = 0;
    u16 last_reclen = 0;
    for (; it != dirent_cache.end(); it++) {
        auto dirent = it->second;

        if (dirent.d_reclen + cumulative_offset > count)
            break;

        memcpy(static_cast<u8*>(buf) + cumulative_offset, &dirent, dirent.d_reclen);
        last_reclen = dirent.d_reclen;
        cumulative_offset += last_reclen;
    }

    // offset of the last reclen
    auto _val = cumulative_offset - last_reclen + 4;
    // pointer to last reclen in buffer
    u8* placement = static_cast<u8*>(buf) + _val;
    // casting buffer to u16 to write whole value at once (2 bytes)
    u16* pos = reinterpret_cast<u16*>(placement);
    *pos += Common::AlignUp(cumulative_offset, count) - cumulative_offset;

    return count;
}

s64 DirectoryPFS::getdents(void* buf, u32 count, s64 offset, s64* basep) {
    RebuildDirents();

    auto it = dirent_cache.lower_bound(offset);

    if (it == dirent_cache.end())
        return 0;

    u64 cumulative_offset = 0;
    u16 last_reclen = 0;
    for (; it != dirent_cache.end(); it++) {
        auto dirent = it->second;

        if (dirent.d_reclen + cumulative_offset > count)
            break;

        memcpy(static_cast<u8*>(buf) + cumulative_offset, &dirent, dirent.d_reclen);
        last_reclen = dirent.d_reclen;
        cumulative_offset += last_reclen;
    }

    if (basep)
        *basep = cumulative_offset;
    // offset of the last reclen
    auto _val = cumulative_offset - last_reclen + 4;
    // pointer to last reclen in buffer
    u8* placement = static_cast<u8*>(buf) + _val;
    // casting buffer to u16 to write whole value at once (2 bytes)
    u16* pos = reinterpret_cast<u16*>(placement);
    *pos += Common::AlignUp(cumulative_offset, count) - cumulative_offset;

    return count;
}

} // namespace QuasiFS