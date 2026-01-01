// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"
#include "core/libraries/kernel/posix_error.h"

namespace QuasiFS {

QuasiDirectory::QuasiDirectory() {
    this->st.st_mode |= QUASI_S_IFDIR;
    this->dirent_cache_bin.reserve(512);
}

s64 QuasiDirectory::read(void* buf, u64 count) {
    RebuildDirents();
    st.st_atim.tv_sec = time(0);

    // data is contiguous. read goes like any regular file would: start at offset, read n bytes
    // output is always aligned up to 512 bytes with 0s
    // offset - classic. however at the end of read any unused (exceeding dirent buffer size) buffer
    // space will be left untouched
    // reclen always sums up to end of current alignment

    s64 bytes_available = this->dirent_cache_bin.size() - this->descriptor_offset;
    if (bytes_available <= 0)
        return 0;
    bytes_available = std::min<s64>(bytes_available, static_cast<s64>(count));

    // data
    memcpy(buf, this->dirent_cache_bin.data() + this->descriptor_offset, bytes_available);

    this->descriptor_offset += bytes_available;
    return bytes_available;
}

s64 QuasiDirectory::lseek(s64 offset, s32 whence) {
    RebuildDirents();
    return Inode::lseek(offset, whence);
}

s32 QuasiDirectory::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    RebuildDirents();
    *sb = st;
    return 0;
}

s32 QuasiDirectory::ftruncate(s64 length) {
    return -POSIX_EISDIR;
}

s64 QuasiDirectory::getdents(void* buf, u64 count, s64* basep) {
    RebuildDirents();
    st.st_atim.tv_sec = time(0);

    if (basep)
        *basep = this->descriptor_offset;

    // same as others, we just don't need a variable
    if (this->descriptor_offset >= this->st.st_size)
        return 0;

    s64 bytes_written = 0;
    s64 working_offset = this->descriptor_offset;
    s64 dirent_buffer_offset = 0;
    s64 aligned_count = Common::AlignDown(count, 512);

    const u8* dirent_buffer = this->dirent_cache_bin.data();
    while (dirent_buffer_offset < this->dirent_cache_bin.size()) {
        const u8* normal_dirent_ptr = dirent_buffer + dirent_buffer_offset;
        const dirent_t* normal_dirent = reinterpret_cast<const dirent_t*>(normal_dirent_ptr);
        auto d_reclen = normal_dirent->d_reclen;

        // bad, incomplete or OOB entry
        if (normal_dirent->d_namlen == 0)
            break;

        if (working_offset >= d_reclen) {
            dirent_buffer_offset += d_reclen;
            working_offset -= d_reclen;
            continue;
        }

        if ((bytes_written + d_reclen) > aligned_count)
            // dirents are aligned to the last full one
            break;

        memcpy(static_cast<u8*>(buf) + bytes_written, normal_dirent_ptr + working_offset,
               d_reclen - working_offset);
        bytes_written += d_reclen - working_offset;
        dirent_buffer_offset += d_reclen;
        working_offset = 0;
    }

    this->descriptor_offset += bytes_written;
    return bytes_written;
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
        tmp.d_fileno = node->__GetFileno();
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