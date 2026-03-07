// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "common/assert.h"
#include "core/libraries/network/net.h"

#include "core/file_sys/quasifs/quasi_types.h"
#include "core/file_sys/quasifs/quasifs_inode_socket.h"

namespace QuasiFS {
class SocketPOSIX : Socket {
    net_socket sock;
    int sockopt_so_connecttimeo = 0;
    int sockopt_so_reuseport = 0;
    int sockopt_so_onesbcast = 0;
    int sockopt_so_usecrypto = 0;
    int sockopt_so_usesignature = 0;
    int sockopt_so_nbio = 0;
    int sockopt_ip_ttlchk = 0;
    int sockopt_ip_maxttl = 0;
    int sockopt_tcp_mss_to_advertise = 0;
    int socket_type;

public:
    explicit SocketPOSIX(int domain, int type, int protocol) : Socket(domain, type, protocol) {}
};

} // namespace QuasiFS