// INAA License @marecl 2025

#include <map>
#include <string>

#include "core/file_sys/quasifs/quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_directory.h"

namespace QuasiFS {

QuasiDirectory::QuasiDirectory() {
    st.st_mode |= QUASI_S_IFDIR;
}

inode_ptr QuasiDirectory::lookup(const std::string& name) {
    auto it = entries.find(name);
    if (it == entries.end())
        return nullptr;
    st.st_atim.tv_sec = time(0);
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
} // namespace QuasiFS