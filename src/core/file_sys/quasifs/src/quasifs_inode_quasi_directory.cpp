// INAA License @marecl 2025

#include <map>
#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"

namespace QuasiFS {

QuasiDirectory::QuasiDirectory() {
    st.st_mode |= QUASI_S_IFDIR;
    dirent_cache_bin.reserve(512);
}

s64 QuasiDirectory::pread(void* buf, u64 count, s64 offset) {
    return getdents(buf, count, offset, nullptr);
}

s64 QuasiDirectory::lseek(s64 current, s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUB)");
    switch (whence) {
    case 0:
        return offset;
    case 1:
        return current + offset;
    case 2:
        return this->st.st_size + offset;
    }
    UNREACHABLE_MSG("lseek with unknown whence {}", whence);
    return -QUASI_ENOSYS;
}

s32 QuasiDirectory::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    RebuildDirents();
    *sb = st;
    return 0;
}

s32 QuasiDirectory::ftruncate(s64 length) {
    return -QUASI_EISDIR;
}

s64 QuasiDirectory::getdents(void* buf, u32 count, s64 offset, s64* basep) {
    RebuildDirents();
    memset(buf, 0, count);

    if (offset >= this->st.st_size)
        return 0;

    auto _start = std::lower_bound(dirent_offset.begin(), dirent_offset.end(), offset);
    auto _end = std::lower_bound(dirent_offset.begin(), dirent_offset.end(), offset + count);

    // lower_bound will *always* select an element that's NOT SMALLER than x
    // so if arg lands between two values, it will always select the bigger one
    if (_start != dirent_offset.begin() && *_start > offset)
        _start--;

    u64 real_start = 0;
    u64 real_end = 0;

    u8* dirent_data = static_cast<u8*>(dirent_cache_bin.data());
    u8* buffer = static_cast<u8*>(buf);

    for (; _start != dirent_offset.end(); _start++) {
        if ((*reinterpret_cast<u16*>(dirent_data + 4) + *_start) < offset)
            continue;
        real_start = *_start;
    }

    


    u64 bytes_to_read = 0;
    u64 reclen_offset = 0;
    // there's always data left
    for (auto _end_reverse = std::make_reverse_iterator(_end); _end_reverse != dirent_offset.rend();
         _end_reverse++) {
        reclen_offset = *_end_reverse + 4;
        bytes_to_read = *_end_reverse - *_start +
                        *reinterpret_cast<u16*>(dirent_cache_bin.data() + reclen_offset);
        if (bytes_to_read < count)
            break;
    }

    std::copy(dirent_cache_bin.begin() + *_start,
              dirent_cache_bin.begin() + *_start + bytes_to_read, reinterpret_cast<u8*>(buf));

    u8* tmp = static_cast<u8*>(buf);
    *(reinterpret_cast<u16*>(tmp + reclen_offset)) +=
        Common::AlignUp(bytes_to_read, count) - bytes_to_read;

    if (basep)
        *basep = count;
    return count;
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
        return -QUASI_ENOENT;
    if (entries.count(name))
        return -QUASI_EEXIST;
    entries[name] = child;
    if (!child->is_link())
        child->st.st_nlink++;
    st.st_mtim.tv_sec = time(0);
    return 0;
}

int QuasiDirectory::unlink(const std::string& name) {
    auto it = entries.find(name);
    if (it == entries.end())
        return -QUASI_ENOENT;

    inode_ptr target = it->second;
    // if directory and not empty -> EBUSY or ENOTEMPTY
    if (target->is_dir()) {
        dir_ptr dir = std::static_pointer_cast<QuasiDirectory>(target);
        auto children = dir->list();
        children.erase(std::remove(children.begin(), children.end(), "."), children.end());
        children.erase(std::remove(children.begin(), children.end(), ".."), children.end());
        if (!children.empty())
            return -QUASI_ENOTEMPTY;

        // parent loses reference from subdir [ .. ]
        this->st.st_nlink--;
        // target loses reference from itself [ . ]
        target->st.st_nlink--;
    }

    // not referenced in original location anymore
    target->st.st_nlink--;
    entries.erase(it);
    st.st_mtim.tv_sec = time(0);
    return 0;
}

std::vector<std::string> QuasiDirectory::list() {
    st.st_atim.tv_sec = time(0);
    std::vector<std::string> r;
    for (auto& p : entries)
        r.push_back(p.first);
    return r;
}

void QuasiDirectory::RebuildDirents(void) {
    // adding/removing entries changes mtime
    if (this->st.st_mtim.tv_sec == this->last_dirent_rebuild_time)
        return;
    this->last_dirent_rebuild_time = this->st.st_mtim.tv_sec;

    constexpr u32 dirent_meta_size = sizeof(dirent_t::d_fileno) + sizeof(dirent_t::d_type) +
                                     sizeof(dirent_t::d_namlen) + sizeof(dirent_t::d_reclen);

    u64 dirent_size = 0;
    this->dirent_cache_bin.clear();
    this->dirent_offset.clear();

    for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
        dirent_t tmp{};
        inode_ptr node = entry->second;
        std::string name = entry->first;

        tmp.d_fileno = node->fileno;
        tmp.d_namlen = name.size();
        strncpy(tmp.d_name, name.data(), tmp.d_namlen + 1);
        tmp.d_type = node->type() >> 12;
        tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 4);

        auto dirent_ptr = reinterpret_cast<const u8*>(&tmp);
        dirent_cache_bin.insert(dirent_cache_bin.end(), dirent_ptr, dirent_ptr + tmp.d_reclen);
        dirent_offset.push_back(dirent_size);
        dirent_size += tmp.d_reclen;
    }

    this->st.st_size = dirent_size;

    return;
}

} // namespace QuasiFS