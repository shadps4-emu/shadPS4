// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <vector>
#include <core/libraries/kernel/orbis_error.h>
#include "base_directory.h"
#include "common/types.h"

namespace Core::Directories {

class NormalDirectory final : BaseDirectory {
public:
    static std::shared_ptr<BaseDirectory> Create(std::string_view guest_path);
    explicit NormalDirectory(std::string_view guest_path);
    ~NormalDirectory() override = default;

    virtual s64 read(void* buf, u64 nbytes) override;
    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt,
                       s64 offset) override;
    virtual s64 lseek(s64 offset, s32 whence) override;
    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    virtual s64 getdents(void* buf, u64 nbytes, s64* basep) override;

private:
    static constexpr s32 MAX_LENGTH = 255;
    struct NormalDirectoryDirent {
        u32 d_fileno;
        u16 d_reclen;
        u8 d_type;
        u8 d_namlen;
        char d_name[MAX_LENGTH + 1];
    };

    u64 directory_size = 0;
    s64 dirents_index = 0;
    std::vector<NormalDirectoryDirent*> dirents;
};
} // namespace Core::Directories
