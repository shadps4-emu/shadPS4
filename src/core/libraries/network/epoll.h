// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
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
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int net_socket;
#endif
#include <map>
#include <memory>
#include <mutex>

namespace Libraries::Net {

union OrbisNetEpollData {
    void* ptr;
    u32 _u32;
    int fd;
    u64 _u64;
};

struct OrbisNetEpollEvent {
    u32 events;
    u32 reserved;
    u32 ident;
    OrbisNetEpollData data;
};

struct EpollSocket {
    unsigned int events;
    OrbisNetEpollData data;
    net_socket sock;
};

enum SceNetEpollControlFlag : u32 {
    ORBIS_NET_EPOLL_CTL_ADD = 1,
    ORBIS_NET_EPOLL_CTL_MOD,
    ORBIS_NET_EPOLL_CTL_DEL
};

struct NetEpoll {
    std::map<int, EpollSocket> eventEntries;

    int Add(int id, net_socket sock, OrbisNetEpollEvent* ev);
    int Del(int id, net_socket sock, OrbisNetEpollEvent* ev);
    int Mod(int id, net_socket sock, OrbisNetEpollEvent* ev);
    int Wait(OrbisNetEpollEvent* events, int maxevents, int timeout);
};

typedef std::shared_ptr<NetEpoll> EpollPtr;

class NetEpollInternal {
public:
    explicit NetEpollInternal() = default;
    ~NetEpollInternal() = default;
    EpollPtr FindEpoll(int sockid) {
        std::scoped_lock lock{m_mutex};
        const auto it = epolls.find(sockid);
        if (it != epolls.end()) {
            return it->second;
        }
        return 0;
    }
    int EraseEpoll(int eid) {
        std::scoped_lock lock{m_mutex};
        return epolls.erase(eid);
    }

public:
    std::mutex m_mutex;
    typedef std::map<int, EpollPtr> NetEpolls;
    NetEpolls epolls;
    int next_epool_sock_id = 0;
};
} // namespace Libraries::Net