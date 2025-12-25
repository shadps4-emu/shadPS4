// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

QuasiDirectory::QuasiDirectory() {
    this->st.st_mode |= QUASI_S_IFDIR;
    this->dirent_cache_bin.reserve(512);
}

s64 QuasiDirectory::pread(void* buf, u64 count, s64 offset) {
    RebuildDirents();
    st.st_atim.tv_sec = time(0);

    // data is contiguous. read goes like any regular file would: start at offset, read n bytes
    // output is always aligned up to 512 bytes with 0s
    // offset - classic. however at the end of read any unused (exceeding dirent buffer size) buffer
    // space will be left untouched
    // reclen always sums up to end of current alignment

    s64 bytes_available = this->dirent_cache_bin.size() - offset;
    if (bytes_available <= 0)
        return 0;
    bytes_available = std::min<s64>(bytes_available, static_cast<s64>(count));

    // data
    memcpy(buf, this->dirent_cache_bin.data() + offset, bytes_available);

    return bytes_available;
}

s64 QuasiDirectory::lseek(s64 current, s64 offset, s32 whence) {
    RebuildDirents();
    return Inode::lseek(current, offset, whence);
}

s32 QuasiDirectory::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    RebuildDirents();
    *sb = st;
    return 0;
}

s32 QuasiDirectory::ftruncate(s64 length) {
    return -POSIX_EISDIR;
}

s64 QuasiDirectory::getdents(void* buf, u64 count, s64 offset, s64* basep) {
    RebuildDirents();
    st.st_atim.tv_sec = time(0);

    if (basep)
        *basep = offset;

    // at this point count is ALWAYS >=512 (checked in VIO Driver)
    // always returns up to 512 bytes
    // return always alignd final fptr to 512 bytes
    // doesn't zero-out remaining space in buffer

    // we're assuming this is always aligned, no check here
    s64 bytes_available = this->dirent_cache_bin.size() - offset;
    if (bytes_available <= 0)
        return 0;

    // offset might push it too far so read count becomes misaligned
    u64 apparent_end = offset + count;
    u64 minimum_read = Common::AlignDown(apparent_end, 512) - offset;
    s64 to_read = std::min<s64>(bytes_available, static_cast<s64>(minimum_read));

    std::copy(dirent_cache_bin.data() + offset, dirent_cache_bin.data() + offset + to_read,
              static_cast<u8*>(buf));

    return to_read;
}

inode_ptr QuasiDirectory::lookup(const std::string& name) {
    st.st_atim.tv_sec = time(0);
    auto it = entries.find(name);
    if (it == entries.end())
        return nullptr;
    return it->second;
}

int QuasiDirectory::link(const std::string& name, inode_ptr child) {
    if (name.empty())
        return -POSIX_ENOENT;
    if (entries.count(name))
        return -POSIX_EEXIST;
    entries[name] = child;
    if (!child->is_link())
        child->st.st_nlink++;
    st.st_mtim.tv_sec = time(0);
    dirents_changed = true;
    return 0;
}

int QuasiDirectory::unlink(const std::string& name) {
    auto it = entries.find(name);
    if (it == entries.end())
        return -POSIX_ENOENT;

    inode_ptr target = it->second;
    // if directory and not empty -> EBUSY or ENOTEMPTY
    if (target->is_dir()) {
        dir_ptr target_dir = std::reinterpret_pointer_cast<QuasiDirectory>(target);
        for (auto entry : target_dir->entries) {
            if (entry.first == ".")
                continue;
            if (entry.first == "..")
                continue;
            return -POSIX_ENOTEMPTY;
        }

        // parent loses reference from subdir [ .. ]
        this->st.st_nlink--;
        // target loses reference from itself [ . ]
        target->st.st_nlink--;
    }

    // not referenced in original location anymore
    target->st.st_nlink--;
    entries.erase(it);
    dirents_changed = true;
    st.st_mtim.tv_sec = time(0);
    return 0;
}

void QuasiDirectory::RebuildDirents(void) {
    if (!this->dirents_changed)
        return;
    this->dirents_changed = false;

    constexpr u32 dirent_meta_size = sizeof(dirent_t::d_fileno) + sizeof(dirent_t::d_type) +
                                     sizeof(dirent_t::d_namlen) + sizeof(dirent_t::d_reclen);

    u64 next_ceiling = 0;
    u64 dirent_offset = 0;
    u64 last_reclen_offset = 4;
    this->dirent_cache_bin.clear();

    for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
        dirent_t tmp{};
        inode_ptr node = entry->second;
        std::string name = entry->first;

        // prepare dirent
        tmp.d_fileno = node->GetFileno();
        tmp.d_namlen = name.size();
        strncpy(tmp.d_name, name.data(), tmp.d_namlen + 1);
        tmp.d_type = node->type() >> 12;
        tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 4);

        // next element may break 512 byte alignment
        if (tmp.d_reclen + dirent_offset > next_ceiling) {
            // align previous dirent's size to the current ceiling
            *reinterpret_cast<u16*>(static_cast<u8*>(dirent_cache_bin.data()) +
                                    last_reclen_offset) += next_ceiling - dirent_offset;
            // set writing pointer to the aligned start position (current ceiling)
            dirent_offset = next_ceiling;
            // move the ceiling up and zero-out the buffer
            next_ceiling += 512;
            dirent_cache_bin.resize(next_ceiling);
            std::fill(dirent_cache_bin.begin() + dirent_offset,
                      dirent_cache_bin.begin() + next_ceiling, 0);
        }

        // current dirent's reclen position
        last_reclen_offset = dirent_offset + 4;
        memcpy(dirent_cache_bin.data() + dirent_offset, &tmp, tmp.d_reclen);
        dirent_offset += tmp.d_reclen;
    }

    // last reclen, as before
    *reinterpret_cast<u16*>(static_cast<u8*>(dirent_cache_bin.data()) + last_reclen_offset) +=
        next_ceiling - dirent_offset;

    // i have no idea if this is the case, but lseek returns size aligned to 512
    this->st.st_size = next_ceiling;
}

} // namespace QuasiFS