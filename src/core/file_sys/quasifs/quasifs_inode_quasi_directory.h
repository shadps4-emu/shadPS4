// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

// Directory
class QuasiDirectory : public Inode {
public:
    std::map<std::string, inode_ptr> entries{};
    dir_ptr mounted_root = nullptr;

    QuasiDirectory();
    ~QuasiDirectory() = default;

    static dir_ptr Create(void) {
        return std::make_shared<QuasiDirectory>();
    }

    template <typename T, typename... Args>
    static file_ptr Create(Args&&... args) {
        if constexpr (std::is_base_of_v<QuasiDirectory, T>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        UNREACHABLE();
    }

    //
    // Inode overrides
    //

    // s64 pread(void* buf, size_t count, u64 offset) override;
    // s64 pwrite(const void* buf, size_t count, u64 offset) override;
    int ftruncate(s64 length) override {
        return -QUASI_EISDIR;
    }

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