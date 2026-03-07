// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode_file.h"

namespace QuasiFS {

class VirtualFile final : public QuasiFile {
    std::vector<char> data{};

public:
    VirtualFile() = default;
    ~VirtualFile() = default;

    static std::shared_ptr<VirtualFile> Create() {
        return std::make_shared<VirtualFile>();
    }

    s64 read(void* buf, u64 count) override;
    s64 write(const void* buf, u64 count) override;

    s32 fsync() override;
    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS
