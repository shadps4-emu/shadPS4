// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <vector>
#include "common/logging/log.h"
#include "shadnet/client.h" // socket platform typedefs + protocol constants
#include "shadnet/server_probe.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
static void ProbePlatformInit() {
    static bool done = false;
    if (!done) {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
        done = true;
    }
}
#else
static void ProbePlatformInit() {}
#endif

namespace ShadNet {

static u32 ProbeGetLE32(const u8* p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) |
           (static_cast<u32>(p[3]) << 24);
}

// recv() exactly n bytes (blocking, bounded by the socket's SO_RCVTIMEO).
static bool ProbeRecvN(ShadSocketHandle sock, u8* buf, u32 n) {
    u32 got = 0;
    while (got < n) {
        const int r =
            ::recv(sock, reinterpret_cast<char*>(buf) + got, static_cast<int>(n - got), 0);
        if (r <= 0)
            return false;
        got += static_cast<u32>(r);
    }
    return true;
}

ProbeInfo ProbeServer(const std::string& host, u16 port, u32 timeout_ms) {
    ProbeInfo info{};

    if (host.empty()) {
        LOG_WARNING(ShadNet, "probe skipped: empty shadNet host");
        return info; // Unreachable
    }

    ProbePlatformInit();

    struct addrinfo hints{}, *res_list = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res_list) != 0 ||
        !res_list) {
        LOG_WARNING(ShadNet, "probe: DNS resolution failed for '{}'", host);
        return info; // Unreachable
    }

    ShadSocketHandle sock = ::socket(res_list->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock == SHAD_INVALID_SOCK) {
        ::freeaddrinfo(res_list);
        return info; // Unreachable
    }

#ifdef _WIN32
    {
        u_long nb = 1;
        ::ioctlsocket(sock, FIONBIO, &nb);
    }
#else
    {
        int fl = ::fcntl(sock, F_GETFL, 0);
        ::fcntl(sock, F_SETFL, fl | O_NONBLOCK);
    }
#endif

    const int cr = ::connect(sock, res_list->ai_addr, static_cast<int>(res_list->ai_addrlen));
    ::freeaddrinfo(res_list);

    bool connected = false;
#ifdef _WIN32
    const bool in_progress = (cr < 0 && WSAGetLastError() == WSAEWOULDBLOCK);
#else
    const bool in_progress = (cr < 0 && errno == EINPROGRESS);
#endif
    if (cr == 0 || in_progress) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        struct timeval tv{};
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(timeout_ms / 1000);
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>((timeout_ms % 1000) * 1000);
        if (::select(static_cast<int>(sock) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
            int err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
            connected = (err == 0);
        }
    }

    if (!connected) {
        SHAD_CLOSE(sock);
        LOG_WARNING(ShadNet, "probe: server {}:{} is unreachable (timeout {} ms)", host, port,
                    timeout_ms);
        return info; // Unreachable
    }

    // Restore blocking mode and apply a receive timeout for the ServerInfo read.
#ifdef _WIN32
    {
        u_long nb = 0;
        ::ioctlsocket(sock, FIONBIO, &nb);
    }
    DWORD so_rcv = static_cast<DWORD>(timeout_ms);
#else
    {
        int fl = ::fcntl(sock, F_GETFL, 0);
        ::fcntl(sock, F_SETFL, fl & ~O_NONBLOCK);
    }
    struct timeval so_rcv{};
    so_rcv.tv_sec = static_cast<decltype(so_rcv.tv_sec)>(timeout_ms / 1000);
    so_rcv.tv_usec = static_cast<decltype(so_rcv.tv_usec)>((timeout_ms % 1000) * 1000);
#endif
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&so_rcv),
                 sizeof(so_rcv));

    info.result = ProbeResult::ProtocolError;

    u8 hdr[SHAD_HEADER_SIZE];
    if (!ProbeRecvN(sock, hdr, SHAD_HEADER_SIZE)) {
        SHAD_CLOSE(sock);
        LOG_WARNING(ShadNet, "probe: {}:{} reachable but no ServerInfo header received", host,
                    port);
        return info;
    }
    if (static_cast<PacketType>(hdr[0]) != PacketType::ServerInfo) {
        SHAD_CLOSE(sock);
        LOG_WARNING(ShadNet, "probe: {}:{} sent packet type {:02x} instead of ServerInfo", host,
                    port, hdr[0]);
        return info;
    }

    const u32 total_sz = ProbeGetLE32(hdr + 3);
    if (total_sz < SHAD_HEADER_SIZE || total_sz > SHAD_MAX_PACKET_SIZE) {
        SHAD_CLOSE(sock);
        LOG_WARNING(ShadNet, "probe: {}:{} sent corrupt ServerInfo (total_sz={})", host, port,
                    total_sz);
        return info;
    }
    const u32 payload_sz = total_sz - SHAD_HEADER_SIZE;
    std::vector<u8> payload(payload_sz);
    if (payload_sz > 0 && !ProbeRecvN(sock, payload.data(), payload_sz)) {
        SHAD_CLOSE(sock);
        LOG_WARNING(ShadNet, "probe: {}:{} ServerInfo payload read failed", host, port);
        return info;
    }
    SHAD_CLOSE(sock);

    if (payload_sz < 4) {
        info.result = ProbeResult::Ok;
        LOG_INFO(ShadNet, "probe: server {}:{} is reachable (no version field in ServerInfo)", host,
                 port);
        return info;
    }

    info.server_version = ProbeGetLE32(payload.data());
    if (info.server_version != SHAD_PROTOCOL_VERSION) {
        info.result = ProbeResult::VersionMismatch;
        LOG_WARNING(ShadNet, "probe: server {}:{} protocol version mismatch (server v{}, ours v{})",
                    host, port, info.server_version, SHAD_PROTOCOL_VERSION);
        return info;
    }

    info.result = ProbeResult::Ok;
    LOG_INFO(ShadNet, "probe: server {}:{} is reachable (protocol v{})", host, port,
             info.server_version);
    return info;
}

} // namespace ShadNet
