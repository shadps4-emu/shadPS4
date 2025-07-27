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
#include <ifaddrs.h>
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
#include <net/if_dl.h>
#include <net/route.h>
#endif
#if __linux__
#include <fstream>
#include <iostream>
#include <sstream>
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

    close(sock);

    if (success) {
        memcpy(ether_address.data(), ifr.ifr_hwaddr.sa_data, 6);
        return true;
    }
#endif
    return false;
}

const std::string& NetUtilInternal::GetDefaultGateway() const {
    return default_gateway;
}

bool NetUtilInternal::RetrieveDefaultGateway() {
    std::scoped_lock lock{m_mutex};

#ifdef _WIN32
    ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG family = AF_INET; // Only IPv4
    ULONG buffer_size = 15000;

    std::vector<BYTE> buffer(buffer_size);
    PIP_ADAPTER_ADDRESSES adapter_addresses =
        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    DWORD result = GetAdaptersAddresses(family, flags, nullptr, adapter_addresses, &buffer_size);
    if (result != NO_ERROR) {
        return false;
    }

    for (PIP_ADAPTER_ADDRESSES adapter = adapter_addresses; adapter != nullptr;
         adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp)
            continue;

        IP_ADAPTER_GATEWAY_ADDRESS_LH* gateway = adapter->FirstGatewayAddress;
        while (gateway) {
            sockaddr* sa = gateway->Address.lpSockaddr;
            if (sa->sa_family == AF_INET) {
                char str[INET_ADDRSTRLEN];
                sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(sa);
                if (inet_ntop(AF_INET, &sa_in->sin_addr, str, sizeof(str))) {
                    this->default_gateway = str;
                    return true;
                }
            }
            gateway = gateway->Next;
        }
    }

    return false;
#elif defined(__APPLE__)
    // adapted from
    // https://github.com/seladb/PcapPlusPlus/blob/a49a79e0b67b402ad75ffa96c1795def36df75c8/Pcap%2B%2B/src/PcapLiveDevice.cpp#L1236
    // route message struct for communication in APPLE device
    struct BSDRoutingMessage {
        struct rt_msghdr header;
        char messageSpace[512];
    };

    struct BSDRoutingMessage routingMessage;
    // It creates a raw socket that can be used for routing-related operations
    int sockfd = socket(PF_ROUTE, SOCK_RAW, 0);
    if (sockfd < 0) {
        return false;
    }
    memset(reinterpret_cast<char*>(&routingMessage), 0, sizeof(routingMessage));
    routingMessage.header.rtm_msglen = sizeof(struct rt_msghdr);
    routingMessage.header.rtm_version = RTM_VERSION;
    routingMessage.header.rtm_type = RTM_GET;
    routingMessage.header.rtm_addrs = RTA_DST | RTA_NETMASK;
    routingMessage.header.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    routingMessage.header.rtm_msglen += 2 * sizeof(sockaddr_in);

    if (write(sockfd, reinterpret_cast<char*>(&routingMessage), routingMessage.header.rtm_msglen) <
        0) {
        return false;
    }

    // Read the response from the route socket
    if (read(sockfd, reinterpret_cast<char*>(&routingMessage), sizeof(routingMessage)) < 0) {
        return false;
    }

    struct in_addr* gateAddr = nullptr;
    struct sockaddr* sa = nullptr;
    char* spacePtr = (reinterpret_cast<char*>(&routingMessage.header + 1));
    auto rtmAddrs = routingMessage.header.rtm_addrs;
    int index = 1;
    auto roundUpClosestMultiple = [](int multiple, int num) {
        return ((num + multiple - 1) / multiple) * multiple;
    };
    while (rtmAddrs) {
        if (rtmAddrs & 1) {
            sa = reinterpret_cast<sockaddr*>(spacePtr);
            if (index == RTA_GATEWAY) {
                gateAddr = &((sockaddr_in*)sa)->sin_addr;
                break;
            }
            spacePtr += sa->sa_len > 0 ? roundUpClosestMultiple(sizeof(uint32_t), sa->sa_len)
                                       : sizeof(uint32_t);
        }
        index++;
        rtmAddrs >>= 1;
    }

    if (gateAddr == nullptr) {
        return false;
    }

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, gateAddr, str, sizeof(str));
    this->default_gateway = str;
    return true;

#else
    std::ifstream route{"/proc/net/route"};
    std::string line;

    std::getline(route, line);
    while (std::getline(route, line)) {
        std::istringstream iss{line};
        std::string iface, destination, gateway;
        int flags;

        iss >> iface >> destination >> gateway >> std::hex >> flags;

        if (destination == "00000000") {
            u64 default_gateway{};
            std::stringstream ss;
            ss << std::hex << gateway;
            ss >> default_gateway;

            char str[INET_ADDRSTRLEN];
            in_addr addr;
            addr.s_addr = default_gateway;
            inet_ntop(AF_INET, &addr, str, sizeof(str));
            this->default_gateway = str;
            return true;
        }
    }
#endif
    return false;
}

const std::string& NetUtilInternal::GetNetmask() const {
    return netmask;
}

bool NetUtilInternal::RetrieveNetmask() {
    std::scoped_lock lock{m_mutex};
    char netmaskStr[INET_ADDRSTRLEN];
    auto success = false;

#ifdef _WIN32
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG family = AF_INET; // Only IPv4
    ULONG buffer_size = 15000;

    std::vector<BYTE> buffer(buffer_size);
    auto adapter_addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    DWORD result = GetAdaptersAddresses(family, flags, nullptr, adapter_addresses, &buffer_size);
    if (result == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(buffer_size);
        adapter_addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        result = GetAdaptersAddresses(family, flags, nullptr, adapter_addresses, &buffer_size);
    }

    if (result != NO_ERROR)
        return false;

    for (auto adapter = adapter_addresses; adapter != nullptr; adapter = adapter->Next) {
        // Skip loopback and down interfaces
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IfOperStatusUp)
            continue;

        for (auto unicast = adapter->FirstUnicastAddress; unicast != nullptr;
             unicast = unicast->Next) {
            if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
                ULONG prefix_length = unicast->OnLinkPrefixLength;
                ULONG mask = prefix_length == 0 ? 0 : 0xFFFFFFFF << (32 - prefix_length);

                in_addr mask_addr{};
                mask_addr.S_un.S_addr = htonl(mask);

                if (inet_ntop(AF_INET, &mask_addr, netmaskStr, INET_ADDRSTRLEN)) {
                    success = true;
                }
            }
        }
    }
#else
    ifaddrs* ifap;

    if (getifaddrs(&ifap) == 0) {
        ifaddrs* p;
        for (p = ifap; p; p = p->ifa_next) {
            if (p->ifa_addr && p->ifa_addr->sa_family == AF_INET) {
                auto sa = reinterpret_cast<sockaddr_in*>(p->ifa_netmask);
                inet_ntop(AF_INET, &sa->sin_addr, netmaskStr, INET_ADDRSTRLEN);
                success = true;
            }
        }
    }

    freeifaddrs(ifap);
#endif

    if (success) {
        netmask = netmaskStr;
    }
    return success;
}

} // namespace NetUtil