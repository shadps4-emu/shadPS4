// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <core/libraries/libs.h>
#include "net.h"

namespace Libraries::Kernel {
int PS4_SYSV_ABI posix_socket(int domain, int type, int protocol) {}
int PS4_SYSV_ABI posix_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {}
u32 PS4_SYSV_ABI posix_htonl(u32 hostlong) {}
u16 PS4_SYSV_ABI posix_htons(u16 hostshort) {}
int PS4_SYSV_ABI posix_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {}
int PS4_SYSV_ABI posix_listen(int sockfd, int backlog) {}
int PS4_SYSV_ABI posix_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {}

void RegisterNet(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", 1, 1, posix_connect);
    LIB_FUNCTION("TU-d9PfIHPM", "libkernel", 1, "libkernel", 1, 1, posix_socket);
    LIB_FUNCTION("K1S8oc61xiM", "libkernel", 1, "libkernel", 1, 1, posix_htonl);
    LIB_FUNCTION("jogUIsOV3-U", "libkernel", 1, "libkernel", 1, 1, posix_htons);
    LIB_FUNCTION("KuOmgKoqCdY", "libkernel", 1, "libkernel", 1, 1, posix_bind);
    LIB_FUNCTION("pxnCmagrtao", "libkernel", 1, "libkernel", 1, 1, posix_listen);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libkernel", 1, "libkernel", 1, 1, posix_accept);
}
} // namespace Libraries::Kernel