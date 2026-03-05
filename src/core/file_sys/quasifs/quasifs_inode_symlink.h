// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Symlink final : public Inode {
    const fs::path target;

public:
    Symlink(const fs::path& target);
    ~Symlink() = default;

    static symlink_ptr Create(const fs::path& target) {
        return std::make_shared<Symlink>(target);
    }

    // symlinked path
    fs::path follow(void);
};

} // namespace QuasiFS