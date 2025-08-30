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
    virtual s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override;
    virtual s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt,
                       s64 offset) override;
    virtual s64 lseek(s64 offset, s32 whence) override;
    virtual s32 fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    virtual s64 getdents(void* buf, u64 nbytes, s64* basep) override;

private:
    static constexpr s32 MAX_LENGTH = 255;
    static constexpr s64 DIRECTORY_ALIGNMENT = 0x200;
    struct NormalDirectoryDirent {
        u32 d_fileno;
        u16 d_reclen;
        u8 d_type;
        u8 d_namlen;
        char d_name[MAX_LENGTH + 1];
    };

    u64 directory_size = 0;
    s64 file_offset = 0;
    std::vector<u8> data_buffer;
    std::vector<NormalDirectoryDirent> dirents;
};
} // namespace Core::Directories
