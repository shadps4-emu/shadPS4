// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET net_socket;
typedef int socklen_t;
#else
#include <cerrno>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int net_socket;
#endif
#if defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif

#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string.h>
#include "net_util.h"

namespace NetUtil {

const std::array<u8, 6>& NetUtilInternal::GetEthernetAddr() const {
    return ether_address;
}

bool NetUtilInternal::RetrieveEthernetAddr() {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    std::vector<u8> adapter_infos(sizeof(IP_ADAPTER_INFO));
    ULONG size_infos = sizeof(IP_ADAPTER_INFO);

    if (GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data()), &size_infos) ==
        ERROR_BUFFER_OVERFLOW)
        adapter_infos.resize(size_infos);

    if (GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data()), &size_infos) ==
            NO_ERROR &&
        size_infos) {
        PIP_ADAPTER_INFO info = reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data());
        memcpy(ether_address.data(), info[0].Address, 6);
        return true;
    }
#elif defined(__APPLE__)
    ifaddrs* ifap;

    if (getifaddrs(&ifap) == 0) {
        ifaddrs* p;
        for (p = ifap; p; p = p->ifa_next) {
            if (p->ifa_addr->sa_family == AF_LINK) {
                sockaddr_dl* sdp = reinterpret_cast<sockaddr_dl*>(p->ifa_addr);
                memcpy(ether_address.data(), sdp->sdl_data + sdp->sdl_nlen, 6);
                freeifaddrs(ifap);
                return true;
            }
        }
        freeifaddrs(ifap);
    }
#else
    ifreq ifr;
    ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
        return false;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
        return false;

    ifreq* it = ifc.ifc_req;
    const ifreq* const end = it + (ifc.ifc_len / sizeof(ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
    }

    if (success) {
        memcpy(ether_address.data(), ifr.ifr_hwaddr.sa_data, 6);
        return true;
    }
#endif
    return false;
}
} // namespace NetUtil