// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "epoll.h"
#include "net_error.h"

namespace Libraries::Net {
int NetEpoll::Add(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    if (!eventEntries.try_emplace(id, EpollSocket{ev->events, ev->data, sock}).second) {
        return ORBIS_NET_ERROR_EEXIST;
    }
    return 0;
}

int NetEpoll::Del(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    if (eventEntries.erase(id) == 0) {
        return ORBIS_NET_ERROR_ENOENT;
    }
    return 0;
}

int NetEpoll::Mod(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    auto it = eventEntries.find(id);
    if (it == eventEntries.end()) {
        return ORBIS_NET_ERROR_ENOENT;
    }
    it->second.events = ev->events;
    it->second.data = ev->data;
    return 0;
}

int NetEpoll::Wait(OrbisNetEpollEvent* events, int maxevents, int timeout) {
    return 0;
}

} // namespace Libraries::Net