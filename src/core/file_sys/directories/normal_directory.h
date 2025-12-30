// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <vector>
#include "common/types.h"
#include "core/file_sys/directories/base_directory.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core::Directories {

class NormalDirectory final : public BaseDirectory {
public:
    static std::shared_ptr<BaseDirectory> Create(std::string_view guest_path);
    explicit NormalDirectory(std::string_view guest_path);
    ~NormalDirectory() override = default;

    virtual s64 read(void* buf, u64 nbytes) override;
    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    virtual s64 getdents(void* buf, u64 nbytes, s64* basep) override;

private:
#pragma pack(push, 1)
    struct NormalDirectoryDirent {
        u32 d_fileno;
        u16 d_reclen;
        u8 d_type;
        u8 d_namlen;
        char d_name[256];
    };
#pragma pack(pop)

    std::string_view guest_directory{};
    s64 previous_file_offset = -1;

    void RebuildDirents(void);
};
} // namespace Core::Directories
