// INAA License @marecl 2025

#include <map>
#include <string>

#include "common/alignment.h"
#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"

namespace QuasiFS {

QuasiDirectory::QuasiDirectory() {
    st.st_mode |= QUASI_S_IFDIR;
}

s64 QuasiDirectory::pread(void* buf, u64 count, s64 offset) {
    return getdents(buf, count, offset, nullptr);
}

s32 QuasiDirectory::ftruncate(s64 length) {
    return -QUASI_EISDIR;
}

s32 QuasiDirectory::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    RebuildDirents();
    *sb = st;
    return 0;
}

s64 QuasiDirectory::getdents(void* buf, u32 count, s64 offset, s64* basep) {
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
        tmp.d_reclen =
            Common::AlignUp(sizeof(tmp.d_fileno) + sizeof(tmp.d_type) + sizeof(tmp.d_namlen) +
                                sizeof(tmp.d_reclen) + (tmp.d_namlen + 1),
                            4);

        dirent_cache[dirent_size] = tmp;
        dirent_size += tmp.d_reclen;
    }

    this->st.st_size = dirent_size;

    return;
}

} // namespace QuasiFS