// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class QuasiFile : public Inode {

public:
    QuasiFile() {
        st.st_mode = 0000755 | QUASI_S_IFREG;
        st.st_nlink = 0;
    };
    ~QuasiFile() = default;

    static file_ptr Create() {
        return std::make_shared<QuasiFile>();
    }

    file_ptr Clone() const {
        auto _out = std::make_shared<QuasiFile>(*this);
        _out->fileno = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    s64 pread(void* buf, size_t count, s64 offset) override;
    s64 pwrite(const void* buf, size_t count, s64 offset) override;

    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS