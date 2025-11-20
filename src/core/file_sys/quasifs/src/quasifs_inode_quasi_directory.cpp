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
    // complete dirents are divided into 512 (0-filled) byte blocks
    // if more than 512 bytes are read, it draws bytes from the next block
    // anything after last dirent is zeroed-out
    // reclen of last complete dirent (in aligned segment) has size filling up to the end of the
    // segment

    return getdents(buf, count, offset, nullptr);
}

s64 QuasiDirectory::lseek(s64 current, s64 offset, s32 whence) {
    RebuildDirents();
    this->dirents_changed = false;
    return Inode::lseek(current, offset, whence);
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
    st.st_atim.tv_sec = time(0);

    // at this point count is ALWAYS >512 (checked in VIO Driver)
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
    u64 to_read = bytes_available > minimum_read ? minimum_read : bytes_available;

    std::copy(dirent_cache_bin.data() + offset, dirent_cache_bin.data() + offset + to_read,
              static_cast<u8*>(buf));

    if (basep)
        *basep = to_read;
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
    if (this->dirents_changed)
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
        tmp.d_fileno = node->fileno;
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