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
private:
    static constexpr s32 MAX_LENGTH = 255;
    static constexpr s32 DIRECTORY_ALIGNMENT = 0x10000;

#pragma pack(push, 1)
    typedef struct dirent_pfs_t {
        u32 d_fileno;
        u32 d_type;
        u32 d_namlen;
        u32 d_reclen;
        char d_name[MAX_LENGTH + 1];
    } dirent_pfs_t;
#pragma pack(pop)

public:
    DirectoryPFS() = default;
    ~DirectoryPFS() = default;
};

} // namespace QuasiFS