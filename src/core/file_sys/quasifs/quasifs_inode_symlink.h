// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Symlink final : public Inode {
    const fs::path target;

public:
    Symlink(fs::path target);
    ~Symlink() = default;

    static symlink_ptr Create(fs::path target) {
        return std::make_shared<Symlink>(target);
    }

    symlink_ptr Clone() const {
        auto _out = std::make_shared<Symlink>(*this);
        _out->fileno = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    // symlinked path
    fs::path follow(void);
};

} // namespace QuasiFS