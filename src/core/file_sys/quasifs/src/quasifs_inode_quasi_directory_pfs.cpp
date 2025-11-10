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

    memset(buf, 0, count);
    u64 cumulative_offset = 0;
    u32* reclen_location = nullptr;
    for (; it != dirent_cache.end(); it++) {
        auto dirent = it->second;
        if (dirent.d_reclen + cumulative_offset > count)
            break;

        auto partial_base = static_cast<u8*>(buf) + cumulative_offset;
        auto _fileno = reinterpret_cast<u32*>(partial_base);
        auto _type = reinterpret_cast<u32*>(partial_base) + 1;
        auto _namlen = reinterpret_cast<u32*>(partial_base) + 2;
        reclen_location = reinterpret_cast<u32*>(partial_base) + 3;

        *_fileno = dirent.d_fileno;
        *_type = dirent.d_type;
        *_namlen = dirent.d_namlen;
        *reclen_location = dirent.d_reclen;

        memcpy(reinterpret_cast<u8*>(reclen_location + 1), &dirent.d_name, dirent.d_namlen);
        cumulative_offset += dirent.d_reclen;
    }

    *reclen_location += Common::AlignUp(cumulative_offset, count) - cumulative_offset;

    return count;
}

s64 DirectoryPFS::lseek(s64 current, s64 offset, s32 whence) {
    switch (whence) {
    case 0:
        return offset;
        break;
    case 1:
        if ((current + offset) >= dirents_size)
            return current + offset;
        {
            auto _tmp = dirent_cache.lower_bound(current + offset);
            if (_tmp == dirent_cache.end())
                return -QUASI_EINVAL;
            return _tmp->first;
        }
        break;
    case 2:
        return dirents_size + offset;
        break;
    }
    UNREACHABLE_MSG("lseek with unknown whence {}", whence);
    return -QUASI_ENOSYS;
}

s64 DirectoryPFS::getdents(void* buf, u32 count, s64 offset, s64* basep) {
    RebuildDirents();

    auto it = dirent_cache.lower_bound(offset);

    if (it == dirent_cache.end())
        return 0;

    u64 cumulative_offset = 0;
    for (; it != dirent_cache.end(); it++) {
        auto dirent = it->second;

        if (dirent.d_reclen + cumulative_offset > count)
            break;

        memcpy(static_cast<u8*>(buf) + cumulative_offset, &dirent, dirent.d_reclen);
        cumulative_offset += dirent.d_reclen;
    }

    if (basep)
        *basep = cumulative_offset;

    return cumulative_offset;
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
    this->dirent_cache.clear();

    for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
        dirent_t tmp{};
        inode_ptr node = entry->second;
        std::string name = entry->first;

        tmp.d_fileno = node->fileno;
        tmp.d_namlen = name.size();
        strncpy(tmp.d_name, name.data(), tmp.d_namlen + 1);
        tmp.d_type = node->type() >> 12;
        tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 8);

        dirent_cache[dirent_size] = tmp;
        dirent_size += tmp.d_reclen;
    }

    this->dirents_size = dirent_size;

    return;
}

} // namespace QuasiFS