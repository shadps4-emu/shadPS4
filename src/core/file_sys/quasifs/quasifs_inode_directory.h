// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

// Directory
class Directory : public Inode {
public:
    std::map<std::string, inode_ptr> entries{};
    dir_ptr mounted_root = nullptr;

    Directory();
    ~Directory() = default;

    static dir_ptr Create(void) {
        return std::make_shared<Directory>();
    }

    //
    // Inode overrides
    //

    // s64 pread(void* buf, size_t count, u64 offset) override;
    // s64 pwrite(const void* buf, size_t count, u64 offset) override;

    int fstat(Libraries::Kernel::OrbisKernelStat* sb) override {
        this->st.st_size = entries.size() * 32;
        *sb = st;
        return 0;
    }

    //
    // Dir-specific
    //

    // Find an element with [name]
    inode_ptr lookup(const std::string& name);

    // Add hardlink to [child] with [name]
    int link(const std::string& name, inode_ptr child);
    // Remove hardlink to [name]
    int unlink(const std::string& name);
    // list entries
    std::vector<std::string> list();
};

} // namespace QuasiFS