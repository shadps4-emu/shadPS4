// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include "common/assert.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

class Socket : public Inode {

public:
    Socket();
    ~Socket();

    static socket_ptr Create() {
        return std::make_shared<Socket>();
    }

    socket_ptr Clone() const {
        auto _out = std::make_shared<Socket>(*this);
        _out->st.st_ino = -1;
        _out->st.st_nlink = 0;
        return _out;
    }
};

} // namespace QuasiFS