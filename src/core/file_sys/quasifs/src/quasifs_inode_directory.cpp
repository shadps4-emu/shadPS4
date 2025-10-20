// INAA License @marecl 2025

#include <map>
#include <string>

#include "../quasi_errno.h"
#include "core/file_sys/quasifs/quasifs_inode_directory.h"

namespace QuasiFS {

Directory::Directory() {
    st.st_mode |= QUASI_S_IFDIR;
}

inode_ptr Directory::lookup(const std::string& name) {
    auto it = entries.find(name);
    if (it == entries.end())
        return nullptr;
    return it->second;
}

int Directory::link(const std::string& name, inode_ptr child) {
    if (name.empty())
        return -QUASI_ENOENT;
    if (entries.count(name))
        return -QUASI_EEXIST;
    entries[name] = child;
    if (!child->is_link())
        child->st.st_nlink++;
    return 0;
}

int Directory::unlink(const std::string& name) {
    auto it = entries.find(name);
    if (it == entries.end())
        return -QUASI_ENOENT;

    inode_ptr target = it->second;
    // if directory and not empty -> EBUSY or ENOTEMPTY
    if (target->is_dir()) {
        dir_ptr dir = std::static_pointer_cast<Directory>(target);
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
    return 0;
}

std::vector<std::string> Directory::list() {
    std::vector<std::string> r;
    for (auto& p : entries)
        r.push_back(p.first);
    return r;
}
} // namespace QuasiFS