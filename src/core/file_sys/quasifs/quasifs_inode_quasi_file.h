// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

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
        _out->st.st_ino = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    virtual s64 read(void* buf, u64 count) override;
    virtual s64 write(const void* buf, u64 count) override;

    s32 fsync() override;

    s32 ftruncate(s64 length) override;
};

} // namespace QuasiFS
