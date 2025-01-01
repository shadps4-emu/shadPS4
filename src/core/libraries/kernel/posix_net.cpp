// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/singleton.h>
#include <core/libraries/error_codes.h>
#include <core/libraries/libs.h>
#include "posix_net.h"

namespace Libraries::Kernel {
int PS4_SYSV_ABI posix_socket(int domain, int type, int protocol) {
    auto* netcall = Common::Singleton<NetPosixInternal>::Instance();
    int socket = netcall->net_socket(domain, type, protocol);
    if (socket < 0) {
        LOG_ERROR(Lib_Kernel, "error in socket creation = {}", socket);
    }
    return socket;
}
int PS4_SYSV_ABI posix_connect(int sockfd, const struct OrbisNetSockaddr* addr, socklen_t addrlen) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) callled");
    return 0;
}
u32 PS4_SYSV_ABI posix_htonl(u32 hostlong) {
    return htonl(hostlong);
}
u16 PS4_SYSV_ABI posix_htons(u16 hostshort) {
    return htons(hostshort);
}
int PS4_SYSV_ABI posix_bind(int sockfd, const struct OrbisNetSockaddr* addr, socklen_t addrlen) {
    auto* netcall = Common::Singleton<NetPosixInternal>::Instance();
    int bind = netcall->net_bind(sockfd, addr, addrlen);
    if (bind < 0) {
        LOG_ERROR(Lib_Kernel, "error in binding = {}", bind);
    }
    return bind;
}
int PS4_SYSV_ABI posix_listen(int sockfd, int backlog) {
    auto* netcall = Common::Singleton<NetPosixInternal>::Instance();
    int listen = netcall->net_listen(sockfd, backlog);
    if (listen < 0) {
        LOG_ERROR(Lib_Kernel, "error in listen = {}", listen);
    }
    return listen;
}
int PS4_SYSV_ABI posix_accept(int sockfd, struct OrbisNetSockaddr* addr, socklen_t* addrlen) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) callled");
    return ORBIS_OK;
}

void RegisterNet(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", 1, 1, posix_connect);
    LIB_FUNCTION("TU-d9PfIHPM", "libkernel", 1, "libkernel", 1, 1, posix_socket);
    LIB_FUNCTION("K1S8oc61xiM", "libkernel", 1, "libkernel", 1, 1, posix_htonl);
    LIB_FUNCTION("jogUIsOV3-U", "libkernel", 1, "libkernel", 1, 1, posix_htons);
    LIB_FUNCTION("KuOmgKoqCdY", "libkernel", 1, "libkernel", 1, 1, posix_bind);
    LIB_FUNCTION("pxnCmagrtao", "libkernel", 1, "libkernel", 1, 1, posix_listen);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libkernel", 1, "libkernel", 1, 1, posix_accept);
}

int NetPosixInternal::net_socket(int domain, int type, int protocol) {
    std::scoped_lock lock{m_mutex};
    s_socket sock = ::socket(domain, type, protocol);
    auto id = ++next_id;
    socks.emplace(id, sock);
    LOG_INFO(Lib_Kernel, "socket created with id = {}", id);
    return id;
}

static void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr));
    const OrbisNetSockaddrIn* src_in = (const OrbisNetSockaddrIn*)src;
    sockaddr_in* dst_in = (sockaddr_in*)dst;
    dst_in->sin_family = src_in->sin_family;
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

int NetPosixInternal::net_bind(int sockfd, const struct OrbisNetSockaddr* addr, socklen_t addrlen) {
    std::scoped_lock lock{m_mutex};
    const auto it = socks.find(sockfd);
    if (it != socks.end()) {
        s_socket sock = it->second;
        sockaddr addr2;
        convertOrbisNetSockaddrToPosix(addr, &addr2);
        return ::bind(sock, &addr2, sizeof(sockaddr_in));
    }
    return 0; // TODO logging and error return
}
int NetPosixInternal::net_listen(int sockfd, int backlog) {
    std::scoped_lock lock{m_mutex};
    const auto it = socks.find(sockfd);
    if (it != socks.end()) {
        s_socket sock = it->second;
        return ::listen(sock, backlog);
    }
    return 0; // TODO logging and error return
}

} // namespace Libraries::Kernel