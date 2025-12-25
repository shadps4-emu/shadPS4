// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include <map>
// #include <unordered_map>
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

// TODO: sort on rebuild, PS4's dirents are neatly sorted, and debugging it is a huge
// inconvenience
#warning Suboptimal structure for holding directory entries
    std::map<std::string, inode_ptr> entries{};
    // std::unordered_map<std::string, inode_ptr> entries{};

    bool dirents_changed{false};
    std::vector<u8> dirent_cache_bin{};

private:
    virtual void RebuildDirents(void);

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

    dir_ptr Clone() const {
        auto _out = std::make_shared<QuasiDirectory>(*this);
        _out->st.st_ino = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    //
    // Inode overrides
    //

    virtual s64 pread(void* buf, u64 count, s64 offset) override;
    // s64 pwrite(const void* buf, size_t count, u64 offset) override;
    s64 lseek(s64 current, s64 offset, s32 whence) override;
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override;
    s32 ftruncate(s64 length) final override;

    virtual s64 getdents(void* buf, u64 count, s64 offset, s64* basep) override;

    //
    // Dir-specific
    //

    // Find an element with [name]
    virtual inode_ptr lookup(const std::string& name);

    // Add hardlink to [child] with [name]
    int link(const std::string& name, inode_ptr child);
    // Remove hardlink to [name]
    int unlink(const std::string& name);
    // list entries
};

/**
 * Data in buffer is contigious, i.e. everything is basically raw
 * Dirents are divided into 512 byte segments, each containing full dirents only, i.e. dirent can't
 leak between segments
 * basep - where read starts for current invocation (always)
 *
 * Read - similar to how files work, just raw read. If an offset is specified, any data beyond
 *buffer* is not copied
 *          return - min(n, available), basep = offset+return
 *
 * GetDents - dirents are aligned into 512B segments too, but they are **always** returned in 512B
 chunks
 *          if there is an offset, n-first bytes are cut out, segment is read up to 512 bytes and
 remaining space is untouched. doesn't matter if it's 1 byte or entire dirents
 *          return - min(n, available), basep is always aligned to 512

 */

} // namespace QuasiFS