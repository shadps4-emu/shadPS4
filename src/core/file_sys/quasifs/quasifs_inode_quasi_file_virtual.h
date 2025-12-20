// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode_quasi_file.h"

namespace QuasiFS {

class VirtualFile final : public QuasiFile {
    std::vector<char> data{};

public:
    VirtualFile() = default;
    ~VirtualFile() = default;

    static std::shared_ptr<VirtualFile> Create() {
        return std::make_shared<VirtualFile>();
    }

    file_ptr Clone() const {
        auto _out = std::make_shared<VirtualFile>(*this);
        _out->fileno = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    s64 pread(void* buf, u64 count, s64 offset) override;
    s64 pwrite(const void* buf, u64 count, s64 offset) override;
    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS
