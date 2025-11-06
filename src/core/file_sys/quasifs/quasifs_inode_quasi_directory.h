// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

// Directory
class QuasiDirectory : public Inode {

private:
    std::map<std::string, inode_ptr> entries{};

    time_t last_dirent_rebuild_time{0};
    void RebuildDirents(void);
    static constexpr s32 MAX_LENGTH = 255;
    static constexpr s64 DIRECTORY_ALIGNMENT = 0x200;

#pragma pack(push, 1)
    typedef struct dirent_t {
        quasi_ino_t d_ino{};
        u64 d_off{};
        unsigned short d_reclen{};
        unsigned char d_type{};
        char d_name[256]{};
    } dirent_t;
#pragma pack(pop)

public:
    dir_ptr mounted_root = nullptr;

    QuasiDirectory();
    ~QuasiDirectory() = default;

    std::vector<std::string> Entries(void) {
        std::vector<std::string> out{};
        for (auto& kv : this->entries)
            out.push_back(kv.first);
        return out;
    }

    template <typename T, typename... Args>
    static dir_ptr Create(Args&&... args) {
        if constexpr (std::is_base_of_v<QuasiDirectory, T>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        UNREACHABLE();
    }

    //
    // Inode overrides
    //

    // s64 pread(void* buf, size_t count, u64 offset) override;
    // s64 pwrite(const void* buf, size_t count, u64 offset) override;
    s32 ftruncate(s64 length) final override {
        return -QUASI_EISDIR;
    }

    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override {
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