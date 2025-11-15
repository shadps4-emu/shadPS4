// INAA License @marecl 2025

#include <map>
#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory_pfs.h"

// PFS is a bit different from regular dirents, see comments below
// Although it's pretty simple, every tested game (sample size: 1) reads it exclusively with
// count=65536, so it doesn't need much mumbo-jambo like regular dirents
// I'll worry if a game uses something different than that count and offset=0

namespace QuasiFS {

DirectoryPFS::DirectoryPFS() {
    this->st.st_size = 65536; // pro forma, gets erased on sync. TODO: fixme
}

DirectoryPFS::~DirectoryPFS() = default;

s64 DirectoryPFS::pread(void* buf, u64 count, s64 offset) {
    RebuildDirents();

    // data is contiguous. i think that's how it's said
    // anyway, everything goes raw
    // aligned to 65536 bytes, everything is zeroed-out in that range
    // currently not testing for anything higher than that

    s64 apparent_end = offset + count;

    if (apparent_end > 65536) {
        // i really don't want to do this yet
        LOG_CRITICAL(Kernel_Fs,
                     "PFS directory size larger than 65536 bytes is not implemented yet");
        if (offset > 65536)
            return 0;
        count = 65536 - offset;
    }

    s64 bytes_available = this->dirent_cache_bin.size() - offset;
    if (0 >= bytes_available)
        return 0;
    bytes_available = bytes_available > count ? count : bytes_available;

    // data
    memcpy(buf, this->dirent_cache_bin.data() + offset, bytes_available);
    // remainders
    s64 filler = count - bytes_available;
    if (filler > 0)
        memset(static_cast<u8*>(buf) + bytes_available, 0, filler);

    // always returns count
    return count;
}

s64 DirectoryPFS::lseek(s64 current, s64 offset, s32 whence) {
    RebuildDirents();
    return Inode::lseek(current, offset, whence);
}

s64 DirectoryPFS::getdents(void* buf, u32 count, s64 offset, s64* basep) {
    RebuildDirents();

    if (count != 65536)
        LOG_CRITICAL(Kernel_Fs, "PFS dirents read with count={} (which is not 65536) (report this)",
                     count);

    // honestly i have no idea on how to implement this
    // buffer behaves like a regular directory
    // variable sizes are same as dirent_t, but data is still aligned to 8 bytes
    // initial call pulls all COMPLETE dirents (like base), but returns real amount of bytes
    // for some reason, subsequent calls return 0
    // doesn't zero-out memory after last dirent

    s64 apparent_end = offset + count;

    if (apparent_end > 65536) {
        // i really don't want to do this yet
        LOG_CRITICAL(Kernel_Fs,
                     "PFS directory size larger than 65536 bytes is not implemented yet");
        if (offset > 65536)
            return 0;
        count = 65536 - offset;
    }

    s64 bytes_available = this->dirent_cache_bin.size() - offset;
    if (0 >= bytes_available)
        return 0;
    bytes_available = bytes_available > count ? count : bytes_available;

    u64 bytes_written = 0;
    u64 dirent_offset = 0;
    while (dirent_offset < bytes_available) {
        const dirent_pfs_t* pfs_dirent =
            reinterpret_cast<dirent_pfs_t*>(this->dirent_cache_bin.data() + bytes_written);
        dirent_t normal_dirent{};

        // we're transposing u32 into smaller types, so there miiight be some issues
        normal_dirent.d_fileno = pfs_dirent->d_fileno;
        normal_dirent.d_reclen = pfs_dirent->d_reclen;
        normal_dirent.d_type = pfs_dirent->d_type;
        normal_dirent.d_namlen = pfs_dirent->d_namlen;
        memcpy(normal_dirent.d_name, pfs_dirent->d_name, pfs_dirent->d_namlen);

        s64 bytes_remaining = bytes_available - bytes_written;
        u64 to_write = 0;

        if (bytes_remaining > normal_dirent.d_reclen)
            to_write = normal_dirent.d_reclen;
        else if (bytes_remaining > 0) {
            to_write = bytes_remaining;
        }

        memcpy(static_cast<u8*>(buf) + bytes_written, &normal_dirent, to_write);
        dirent_offset += pfs_dirent->d_reclen;
        bytes_written += to_write;
    }

    // always returns count
    return bytes_written;
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
    // it gets erased on FS sync in QFS, but this fn is rarely called,
    // especially for PFS which is RO
    this->st.st_size = 65536;

    return;
}

} // namespace QuasiFS