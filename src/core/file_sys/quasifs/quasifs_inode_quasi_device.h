// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Device : public Inode {

public:
    Device();
    ~Device();

    static dev_ptr Create() {
        return std::make_shared<Device>();
    }

    dev_ptr Clone() const {
        auto _out = std::make_shared<Device>(*this);
        _out->st.st_ino = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    virtual s64 read(void* buf, u64 count) override;
    virtual s64 write(const void* buf, u64 count) override;

    virtual s64 lseek(s64 offset, s32 whence) final override;
};

} // namespace QuasiFS