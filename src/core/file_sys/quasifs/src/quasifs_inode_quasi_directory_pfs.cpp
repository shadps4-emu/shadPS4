// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include <map>
#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory_pfs.h"
#include "core/libraries/kernel/posix_error.h"

// Read-wise PFS is almost identical to regular directory
// Dirents need a bit of tuning, since they're converted from PFS to normal for getdirents
// one important thing (aka. FIXME) - file pointer is moved to the end when last dirent is
// encountered. it's a bit problematic to implement but well, it's for future me :)
// Other than that, same rules as with regular directory

namespace QuasiFS {

DirectoryPFS::DirectoryPFS() {
    this->st.st_size = 65536;            // starting value, will get updated on rebuild
    this->dirent_cache_bin.reserve(512); // It will grow anyway
}

DirectoryPFS::~DirectoryPFS() = default;

s64 DirectoryPFS::read(void* buf, u64 count) {
    this->RebuildDirents();
    st.st_atim.tv_sec = time(0);

    s64 bytes_available = this->dirent_cache_bin.size() - descriptor_offset;
    if (bytes_available <= 0)
        return 0;

    bytes_available = std::min<s64>(bytes_available, static_cast<s64>(count));
    memcpy(buf, this->dirent_cache_bin.data() + descriptor_offset, bytes_available);

    s64 to_fill = (std::min<s64>(this->st.st_size, static_cast<s64>(count))) - bytes_available -
                  descriptor_offset;
    if (to_fill < 0) {
        LOG_ERROR(Kernel_Fs, "Dirent may have leaked {} bytes", -to_fill);
        return -to_fill + bytes_available;
    }
    memset(static_cast<u8*>(buf) + bytes_available, 0, to_fill);

    descriptor_offset += to_fill + bytes_available;
    return to_fill + bytes_available;
}

s64 DirectoryPFS::getdents(void* buf, u64 count, s64* basep) {
    RebuildDirents();
    st.st_atim.tv_sec = time(0);

    if (basep)
        *basep = this->descriptor_offset;

    // same as others, we just don't need a variable
    if (this->descriptor_offset >= this->st.st_size)
        return 0;

    u64 bytes_written = 0;
    u64 starting_offset = 0;
    u64 buffer_position = 0;
    while (buffer_position < this->dirent_cache_bin.size()) {

        const dirent_pfs_t* pfs_dirent =
            reinterpret_cast<dirent_pfs_t*>(this->dirent_cache_bin.data() + buffer_position);

        // bad, incomplete or OOB entry
        if (pfs_dirent->d_namlen == 0)
            break;

        if (starting_offset < this->descriptor_offset) {
            // reading starts from the nearest full dirent
            starting_offset += pfs_dirent->d_reclen;
            buffer_position = bytes_written + starting_offset;
            continue;
        }

        if ((bytes_written + pfs_dirent->d_reclen) > count)
            // dirents are aligned to the last full one
            break;

        // we're transposing u32 into smaller types, so there miiight be some issues
        // reclen for both is the same despite difference in var sizes, extra 0s are padded after
        // the name
        dirent_t normal_dirent{};
        normal_dirent.d_fileno = pfs_dirent->d_fileno;
        normal_dirent.d_reclen = pfs_dirent->d_reclen;
        normal_dirent.d_type = pfs_dirent->d_type;
        normal_dirent.d_namlen = pfs_dirent->d_namlen;
        memcpy(normal_dirent.d_name, pfs_dirent->d_name, pfs_dirent->d_namlen);

        memcpy(static_cast<u8*>(buf) + bytes_written, &normal_dirent, normal_dirent.d_reclen);
        bytes_written += normal_dirent.d_reclen;
        buffer_position = bytes_written + starting_offset;
    }

    this->descriptor_offset = (buffer_position >= this->dirent_cache_bin.size())
                                  ? this->st.st_size
                                  : (this->descriptor_offset + bytes_written);
    return bytes_written;
}

void DirectoryPFS::RebuildDirents(void) {
    if (!this->dirents_changed)
        return;
    this->dirents_changed = false;

    constexpr u32 dirent_meta_size = sizeof(dirent_pfs_t::d_fileno) + sizeof(dirent_pfs_t::d_type) +
                                     sizeof(dirent_pfs_t::d_namlen) +
                                     sizeof(dirent_pfs_t::d_reclen);

    this->dirent_cache_bin.clear();

    for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
        dirent_pfs_t tmp{};
        inode_ptr node = entry->second;
        std::string name = entry->first;

        tmp.d_fileno = node->__GetFileno();
        tmp.d_namlen = name.size();
        strncpy(tmp.d_name, name.data(), tmp.d_namlen + 1);
        tmp.d_type = node->type() >> 12;
        tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 8);
        auto dirent_ptr = reinterpret_cast<const u8*>(&tmp);

        dirent_cache_bin.insert(dirent_cache_bin.end(), dirent_ptr, dirent_ptr + tmp.d_reclen);
    }

    // directory size is always aligned to 65536 bytes
    // it gets erased on FS sync in QFS, but this fn is rarely called,
    // especially for PFS which is RO
    this->st.st_size = Common::AlignUp(dirent_cache_bin.size(), 0x10000);

    return;
}

// PFS is case-insensitive
inode_ptr DirectoryPFS::lookup(const std::string& name) {
    st.st_atim.tv_sec = time(0);
    std::string name_normalized = name;
    transform(name_normalized.begin(), name_normalized.end(), name_normalized.begin(), ::toupper);

    for (auto entry : entries) {
        std::string entry_name = entry.first;
        std::transform(entry_name.begin(), entry_name.end(), entry_name.begin(), ::toupper);

        if (entry_name == name_normalized)
            return entry.second;
    }

    return nullptr;
}

} // namespace QuasiFS
