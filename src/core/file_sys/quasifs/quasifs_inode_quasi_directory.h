// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/assert.h"
#include "common/logging/log.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

// Directory
class QuasiDirectory : public Inode {

protected:
#pragma pack(push, 1)
    typedef struct dirent_t {
        u32 d_fileno;
        u16 d_reclen;
        u8 d_type;
        u8 d_namlen;
        char d_name[256];
    } dirent_t;
#pragma pack(pop)
    std::map<std::string, inode_ptr> entries{};

    void RebuildDirents(void);
    time_t last_dirent_rebuild_time{0};
    std::map<s64, dirent_t> dirent_cache{};

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

    // Create out of thin air
    static dir_ptr Create() {
        return std::make_shared<QuasiDirectory>();
    }

    // Allow "inheriting" type of directory
    virtual dir_ptr Spawn() const {
        return std::make_shared<QuasiDirectory>();
    }

    //
    // Inode overrides
    //

    virtual s64 pread(void* buf, u64 count, s64 offset) override;
    // s64 pwrite(const void* buf, size_t count, u64 offset) override;
    s64 lseek(s64 current, s64 offset, s32 whence) override;
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override;
    s32 ftruncate(s64 length) final override;

    virtual s64 getdents(void* buf, u32 count, s64 offset, s64* basep) override;

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