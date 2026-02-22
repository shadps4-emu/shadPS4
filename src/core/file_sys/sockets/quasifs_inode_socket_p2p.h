// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "common/assert.h"
#include "core/libraries/network/net.h"

#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_socket.h"

namespace QuasiFS {
class SocketP2P : Socket {
public:
    explicit SocketP2P(int domain, int type, int protocol) : Socket(domain, type, protocol) {}
};

} // namespace QuasiFS