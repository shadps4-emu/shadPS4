// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"
#include "quasifs_inode_quasi_directory.h"

namespace QuasiFS {

// Directory
class DirectoryPFS final : public QuasiDirectory {

protected:
#pragma pack(push, 1)
    typedef struct dirent_pfs_t {
        u32 d_fileno;
        u32 d_type;
        u32 d_namlen;
        u32 d_reclen;
        char d_name[256];
    } dirent_pfs_t;
#pragma pack(pop)

private:
    // void RebuildDirents(void);

public:
    DirectoryPFS();
    ~DirectoryPFS();

    s64 pread(void* buf, u64 count, s64 offset) override;
    
    s64 getdents(void* buf, u32 count, s64 offset, s64* basep) override;

};

} // namespace QuasiFS