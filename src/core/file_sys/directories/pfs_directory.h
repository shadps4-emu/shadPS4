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

class PfsDirectory final : public BaseDirectory {
public:
    static std::shared_ptr<BaseDirectory> Create(std::string_view guest_path);
    explicit PfsDirectory(std::string_view guest_path);
    ~PfsDirectory() override = default;

    virtual s64 read(void* buf, u64 nbytes) override;
    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt,
                       s64 offset) override;
    virtual s64 lseek(s64 offset, s32 whence) override;
    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    virtual s64 getdents(void* buf, u64 nbytes, s64* basep) override;

private:
    static constexpr s32 MAX_LENGTH = 255;
    static constexpr s32 DIRECTORY_ALIGNMENT = 0x10000;
    struct PfsDirectoryDirent {
        u32 d_fileno;
        u32 d_type;
        u32 d_namlen;
        u32 d_reclen;
        char d_name[MAX_LENGTH + 1];
    };

    struct NormalDirectoryDirent {
        u32 d_fileno;
        u16 d_reclen;
        u8 d_type;
        u8 d_namlen;
        char d_name[MAX_LENGTH + 1];
    };

    u64 directory_size = 0;
    u64 directory_content_size = 0;
    s64 dirents_index = 0;
    std::vector<PfsDirectoryDirent> dirents;
};
} // namespace Core::Directories
