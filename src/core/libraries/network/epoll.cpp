// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "epoll.h"

namespace Libraries::Net {
int NetEpoll::Add(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    return 0;
}

int NetEpoll::Del(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    return 0;
}

int NetEpoll::Mod(int id, net_socket sock, OrbisNetEpollEvent* ev) {
    return 0;
}

int NetEpoll::Wait(OrbisNetEpollEvent* events, int maxevents, int timeout) {
    return 0;
}

} // namespace Libraries::Net