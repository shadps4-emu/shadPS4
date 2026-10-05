// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The new libSceNet (src/core/libraries/net) driven through its exports, exactly as a game
// calls them: guest sockaddrs, guest constants, ORBIS_NET_ERROR_* returns and sceNetErrnoLoc.
// P2P runs over the real wire format against a second transport standing in for another
// console.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "core/libraries/error_codes.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_p2p.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_translate.h"
#include "core/loader/symbols_resolver.h"
#include "core/net/p2p_codec.h"

using namespace Libraries::Net;
using namespace std::chrono_literals;
namespace Host = Core::Net::Host;
namespace P2P = Core::Net::P2P;

namespace {

OrbisNetSockaddrIn Loopback(u16 port, u16 vport = 0) {
    OrbisNetSockaddrIn in{};
    in.sin_len = sizeof(in);
    in.sin_family = ORBIS_NET_AF_INET;
    in.sin_port = sceNetHtons(port);
    in.sin_addr = sceNetHtonl(0x7F000001);
    in.sin_vport = sceNetHtons(vport);
    return in;
}

OrbisNetSockaddr* Guest(OrbisNetSockaddrIn* in) {
    return reinterpret_cast<OrbisNetSockaddr*>(in);
}

double Millis(std::chrono::steady_clock::time_point since) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - since)
        .count();
}

class NetLib : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_EQ(sceNetInit(), ORBIS_OK);
    }
    static void TearDownTestSuite() {
        EXPECT_EQ(sceNetTerm(), ORBIS_OK);
    }

    /// A TCP listener on an ephemeral loopback port; returns its port.
    static u16 Listen(OrbisNetId* listener) {
        *listener = sceNetSocket("listener", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
        EXPECT_GT(*listener, 0);
        auto addr = Loopback(0);
        EXPECT_EQ(sceNetBind(*listener, Guest(&addr), sizeof(addr)), ORBIS_OK);
        EXPECT_EQ(sceNetListen(*listener, 4), ORBIS_OK);
        OrbisNetSockaddrIn bound{};
        u32 len = sizeof(bound);
        EXPECT_EQ(sceNetGetsockname(*listener, Guest(&bound), &len), ORBIS_OK);
        return sceNetNtohs(bound.sin_port);
    }

    static s32 GetInt(OrbisNetId s, s32 level, s32 name) {
        s32 value = -1;
        u32 len = sizeof(value);
        EXPECT_EQ(sceNetGetsockopt(s, level, name, &value, &len), ORBIS_OK);
        EXPECT_EQ(len, sizeof(value));
        return value;
    }

    static s32 SetInt(OrbisNetId s, s32 level, s32 name, s32 value) {
        return sceNetSetsockopt(s, level, name, &value, sizeof(value));
    }
    struct ScopedPool {
        s32 id = sceNetPoolCreate("pool", 16 * 1024, 0);
        ~ScopedPool() {
            EXPECT_EQ(sceNetPoolDestroy(id), ORBIS_OK);
        }
    };
};

// ---------------------------------------------------------------------------------------------
// Helpers and error reporting
// ---------------------------------------------------------------------------------------------

TEST_F(NetLib, ErrorCodesAndErrno) {
    EXPECT_EQ(sceNetSocketClose(12345), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EBADF);
    // sceNetSocket: "invalid socket type or protocol family" is EPROTONOSUPPORT.
    EXPECT_EQ(sceNetSocket("bad", 99, ORBIS_NET_SOCK_STREAM, 0), ORBIS_NET_ERROR_EPROTONOSUPPORT);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EPROTONOSUPPORT);
    // The PS4 stack is IPv4-only.
    EXPECT_EQ(sceNetSocket("v6", ORBIS_NET_AF_INET6, ORBIS_NET_SOCK_DGRAM, 0),
              ORBIS_NET_ERROR_EPROTONOSUPPORT);
    // A protocol that does not fit the type, and an over-long debug name.
    EXPECT_EQ(sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, ORBIS_NET_IPPROTO_UDP),
              ORBIS_NET_ERROR_BASE | ORBIS_NET_EPROTOTYPE);
    const OrbisNetId tcp =
        sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, ORBIS_NET_IPPROTO_TCP);
    EXPECT_GT(tcp, 0);
    sceNetSocketClose(tcp);
    EXPECT_EQ(sceNetSocket("0123456789abcdefghijklmnopqrstuvwxyz", ORBIS_NET_AF_INET,
                           ORBIS_NET_SOCK_DGRAM, 0),
              ORBIS_NET_ERROR_ENAMETOOLONG);

    // errno is per thread.
    std::thread([] { EXPECT_EQ(*sceNetErrnoLoc(), 0); }).join();
}

TEST_F(NetLib, LogFormatting) {
    // What the log shows for errors, addresses, options and epoll events.
    EXPECT_EQ(ErrnoName(ORBIS_NET_EWOULDBLOCK), "EWOULDBLOCK");
    EXPECT_EQ(ErrnoName(ORBIS_NET_ECONNREFUSED), "ECONNREFUSED");
    EXPECT_EQ(ErrnoName(250), "E250");
    EXPECT_EQ(ErrorCodeName(ORBIS_NET_ERROR_RESOLVER_ENODNS), "RESOLVER_ENODNS");
    EXPECT_EQ(ErrorCodeName(0x12345), "0x12345");
    auto in = Loopback(8080, 3658);
    EXPECT_EQ(FormatSockaddr(Guest(&in), sizeof(in)), "127.0.0.1:8080 vport 3658");
    in.sin_vport = 0;
    EXPECT_EQ(FormatSockaddr(Guest(&in), sizeof(in)), "127.0.0.1:8080");
    EXPECT_EQ(FormatSockaddr(nullptr, 0), "null");
    EXPECT_EQ(SocketTypeName(ORBIS_NET_SOCK_DGRAM_P2P), "DGRAM_P2P");
    EXPECT_EQ(OptionName(ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO), "SOL_SOCKET/SO_NBIO");
    EXPECT_EQ(OptionName(ORBIS_NET_IPPROTO_TCP, 0x77), "IPPROTO_TCP/option 0x77");
    EXPECT_EQ(OptionName(0x55, 1), "level 0x55/option 0x1");
    EXPECT_EQ(EpollEventsName(ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLDESCID), "IN|DESCID");
    EXPECT_EQ(EpollEventsName(0), "0");
}

TEST_F(NetLib, InitAndTerm) {
    EXPECT_EQ(sceNetInit(), ORBIS_OK);
    EXPECT_EQ(sceNetTerm(), ORBIS_OK);
    EXPECT_EQ(sceNetTerm(), ORBIS_OK);
    EXPECT_EQ(sceNetInit(), ORBIS_OK);
    // Nothing was released: sockets still work.
    const OrbisNetId s = sceNetSocket("after", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_GT(s, 0);
    sceNetSocketClose(s);
}

TEST_F(NetLib, ErrorTableValues) {
    EXPECT_EQ(ORBIS_NET_ERROR_ECANCELED, static_cast<int>(0x80410155));
    EXPECT_EQ(ORBIS_NET_ECANCELED, 85);
    EXPECT_EQ(ToOrbisErrno(Host::Error::Canceled), 85);
    EXPECT_EQ(ORBIS_NET_ERROR_EBUSY, static_cast<int>(0x80410110));
    EXPECT_EQ(ORBIS_NET_ERROR_RESOLVER_ENOTINIT, static_cast<int>(0x804101ec));
    EXPECT_EQ(ORBIS_NET_ERROR_ENOTTY, static_cast<int>(0x80410119));
    EXPECT_EQ(ORBIS_NET_ERROR_ENOTBLK, static_cast<int>(0x8041010f));
}

TEST_F(NetLib, AbortWithNothingWaitingIsENOTBLK) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(sceNetSocketAbort(s, 0), ORBIS_NET_ERROR_ENOTBLK);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_ENOTBLK);
    // With a preservation flag it is kept for the next call instead: not an error.
    EXPECT_EQ(sceNetSocketAbort(s, 1 /* RCV_PRESERVATION */), ORBIS_OK);
    char buf[4];
    EXPECT_EQ(sceNetRecv(s, buf, sizeof(buf), 0), ORBIS_NET_ERROR_EINTR);

    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    EXPECT_EQ(sceNetEpollAbort(ep, 0), ORBIS_NET_ERROR_ENOTBLK);
    EXPECT_EQ(sceNetEpollAbort(ep, 1 /* PRESERVATION */), ORBIS_OK);
    sceNetEpollDestroy(ep);
    sceNetSocketClose(s);
}

TEST_F(NetLib, UnconnectedDatagramSendNeedsAnAddress) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(sceNetSend(s, "x", 1, 0), ORBIS_NET_ERROR_EDESTADDRREQ); // every host alike
    sceNetSocketClose(s);
}

TEST_F(NetLib, ByteOrder) {
    EXPECT_EQ(sceNetHtons(0x1234), 0x3412);
    EXPECT_EQ(sceNetHtonl(0x12345678u), 0x78563412u);
    EXPECT_EQ(sceNetHtonll(0x0102030405060708ull), 0x0807060504030201ull);
    EXPECT_EQ(sceNetNtohll(sceNetHtonll(0xDEADBEEFCAFEF00Dull)), 0xDEADBEEFCAFEF00Dull);
    EXPECT_EQ(sceNetNtohl(sceNetHtonl(42)), 42u);
    EXPECT_EQ(sceNetNtohs(sceNetHtons(42)), 42);
}

TEST_F(NetLib, InetPtonNtop) {
    u32 v4 = 0;
    EXPECT_EQ(sceNetInetPton(ORBIS_NET_AF_INET, "192.168.1.20", &v4), 1);
    EXPECT_EQ(sceNetNtohl(v4), 0xC0A80114u);
    EXPECT_EQ(sceNetInetPton(ORBIS_NET_AF_INET, "not an address", &v4), 0);
    EXPECT_EQ(sceNetInetPton(99, "1.2.3.4", &v4), ORBIS_NET_ERROR_EAFNOSUPPORT);

    char text[64];
    EXPECT_STREQ(sceNetInetNtop(ORBIS_NET_AF_INET, &v4, text, sizeof(text)), "192.168.1.20");
    EXPECT_EQ(sceNetInetNtop(ORBIS_NET_AF_INET, &v4, text, 4), nullptr); // too small
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_ENOSPC);

    u8 v6[16];
    EXPECT_EQ(sceNetInetPton(ORBIS_NET_AF_INET6, "2001:db8::8:800:200c:417a", v6), 1);
    EXPECT_STREQ(sceNetInetNtop(ORBIS_NET_AF_INET6, v6, text, sizeof(text)),
                 "2001:db8::8:800:200c:417a");
}

TEST_F(NetLib, RegisterLibRuns) {
    Core::Loader::SymbolsResolver symbols;
    RegisterLib(&symbols); // every NID registers without touching emulator state
}

// ---------------------------------------------------------------------------------------------
// Native sockets
// ---------------------------------------------------------------------------------------------

TEST_F(NetLib, TcpThroughExports) {
    OrbisNetId listener = 0;
    const u16 port = Listen(&listener);

    OrbisNetId accepted = -1;
    OrbisNetSockaddrIn peer{};
    u32 peer_len = sizeof(peer);
    std::thread server([&] { accepted = sceNetAccept(listener, Guest(&peer), &peer_len); });

    const OrbisNetId client = sceNetSocket("client", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    to.sin_len = 0; // games often leave sa_len unset
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    server.join();
    ASSERT_GT(accepted, 0);

    // The accepted peer address comes back in guest layout, sa_len included.
    EXPECT_EQ(peer_len, sizeof(OrbisNetSockaddrIn));
    EXPECT_EQ(peer.sin_len, sizeof(OrbisNetSockaddrIn));
    EXPECT_EQ(peer.sin_family, ORBIS_NET_AF_INET);
    EXPECT_EQ(peer.sin_addr, sceNetHtonl(0x7F000001));

    OrbisNetSockaddrIn remote{};
    u32 remote_len = sizeof(remote);
    ASSERT_EQ(sceNetGetpeername(client, Guest(&remote), &remote_len), ORBIS_OK);
    EXPECT_EQ(sceNetNtohs(remote.sin_port), port);

    EXPECT_EQ(sceNetSend(client, "hello", 5, 0), 5);
    char buf[16]{};
    EXPECT_EQ(sceNetRecv(accepted, buf, sizeof(buf), 0), 5);
    EXPECT_EQ(std::memcmp(buf, "hello", 5), 0);

    EXPECT_EQ(sceNetShutdown(client, ORBIS_NET_SHUT_WR), ORBIS_OK);
    EXPECT_EQ(sceNetRecv(accepted, buf, sizeof(buf), 0), 0); // end of stream
    EXPECT_EQ(sceNetShutdown(client, 7), ORBIS_NET_ERROR_EINVAL);

    sceNetSocketClose(client);
    sceNetSocketClose(accepted);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, UdpWithAddresses) {
    const OrbisNetId a = sceNetSocket("a", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    const OrbisNetId b = sceNetSocket("b", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(a, Guest(&any), sizeof(any));
    sceNetBind(b, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn a_addr{}, b_addr{};
    u32 len = sizeof(a_addr);
    sceNetGetsockname(a, Guest(&a_addr), &len);
    len = sizeof(b_addr);
    sceNetGetsockname(b, Guest(&b_addr), &len);

    EXPECT_EQ(sceNetSendto(a, "datagram", 8, 0, Guest(&b_addr), sizeof(b_addr)), 8);
    char buf[32]{};
    OrbisNetSockaddrIn from{};
    u32 from_len = sizeof(from);
    // PEEK leaves the datagram queued.
    EXPECT_EQ(sceNetRecvfrom(b, buf, sizeof(buf), ORBIS_NET_MSG_PEEK, Guest(&from), &from_len), 8);
    EXPECT_EQ(sceNetRecvfrom(b, buf, sizeof(buf), 0, Guest(&from), &from_len), 8);
    EXPECT_EQ(from.sin_port, a_addr.sin_port);
    EXPECT_EQ(from.sin_len, sizeof(OrbisNetSockaddrIn));

    // Nothing left: MSG_DONTWAIT and SO_NBIO both give EWOULDBLOCK.
    EXPECT_EQ(sceNetRecvfrom(b, buf, sizeof(buf), ORBIS_NET_MSG_DONTWAIT, nullptr, nullptr),
              ORBIS_NET_ERROR_EWOULDBLOCK);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EWOULDBLOCK);
    EXPECT_EQ(SetInt(b, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, 1), ORBIS_OK);
    EXPECT_EQ(sceNetRecv(b, buf, sizeof(buf), 0), ORBIS_NET_ERROR_EWOULDBLOCK);
    sceNetSocketClose(a);
    sceNetSocketClose(b);
}

TEST_F(NetLib, SocketOptions) {
    const OrbisNetId s = sceNetSocket("opts", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);

    // Guest-owned options.
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_TYPE),
              static_cast<int>(ORBIS_NET_SOCK_STREAM));
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO), 0);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, 1);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO), 1);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO, 250000);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO), 250000);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR, 1);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR), 1);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ERROR), 0);
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_TYPE, 2),
              ORBIS_NET_ERROR_BASE | ORBIS_NET_ENOPROTOOPT);

    // Host options, mapped by name (IP_TTL is 4 on the PS4, 2 on Linux).
    EXPECT_EQ(SetInt(s, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_TTL, 42), ORBIS_OK);
    EXPECT_EQ(GetInt(s, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_TTL), 42);
    EXPECT_EQ(SetInt(s, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_NODELAY, 1), ORBIS_OK);
    EXPECT_NE(GetInt(s, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_NODELAY), 0);
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF, 32768), ORBIS_OK);
    EXPECT_GE(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 32768);

    OrbisNetLinger linger{1, 5};
    EXPECT_EQ(
        sceNetSetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &linger, sizeof(linger)),
        ORBIS_OK);
    OrbisNetLinger back{};
    u32 len = sizeof(back);
    EXPECT_EQ(sceNetGetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &back, &len),
              ORBIS_OK);
    EXPECT_NE(back.l_onoff, 0);
    EXPECT_EQ(back.l_linger, 5);

    // Every documented option is known: anything else is ENOPROTOOPT, at every level.
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, 0x7777, 1), ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(SetInt(s, ORBIS_NET_IPPROTO_TCP, 0x77, 1), ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(SetInt(s, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_KEEPLISTEN, 1), ORBIS_OK);
    EXPECT_EQ(GetInt(s, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_KEEPLISTEN), 1);
    EXPECT_EQ(SetInt(s, 12345, 1, 1), ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(sceNetSetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, nullptr, 4),
              ORBIS_NET_ERROR_EFAULT);
    sceNetSocketClose(s);
}

TEST_F(NetLib, StoredOptionsReadBack) {
    const OrbisNetId s = sceNetSocket("opts", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    const struct {
        s32 level;
        s32 name;
        s32 value;
    } options[] = {
        {ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_CONNECTTIMEO, 5000000},
        {ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ACCEPTTIMEO, 3000000},
        {ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, 1},
        {ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ONESBCAST, 1},
        {ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_TTLCHK, 1},
        {ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_MSS_TO_ADVERTISE, 1200},
    };
    for (const auto& o : options) {
        EXPECT_EQ(GetInt(s, o.level, o.name), 0) << o.name; // unset reads as 0
        EXPECT_EQ(SetInt(s, o.level, o.name, o.value), ORBIS_OK) << o.name;
        EXPECT_EQ(GetInt(s, o.level, o.name), o.value) << o.name;
    }
    // Timeouts: <= 0 disables them.
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_CONNECTTIMEO, -5), ORBIS_OK);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_CONNECTTIMEO), 0);
    // MSS to advertise: 0 or at least 216.
    EXPECT_EQ(SetInt(s, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_MSS_TO_ADVERTISE, 100),
              ORBIS_NET_ERROR_EINVAL);
    sceNetSocketClose(s);
}

TEST_F(NetLib, OptionRulesFollowTheReference) {
    const OrbisNetId tcp = sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    const OrbisNetId udp = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    s32 value = 0;
    u32 len = sizeof(value);
    // "get not possible" / "set not possible": ENOPROTOOPT.
    EXPECT_EQ(sceNetGetsockopt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NAME, &value, &len),
              ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_MAXTTL, 5),
              ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ERROR_EX, 0),
              ORBIS_NET_ERROR_ENOPROTOOPT);
    EXPECT_EQ(GetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ERROR_EX), 0);
    // Options for other socket kinds: EPROCUNAVAIL ...
    EXPECT_EQ(SetInt(udp, ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_NODELAY, 1),
              ORBIS_NET_ERROR_EPROCUNAVAIL);
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_KEEPALIVE, 1),
              ORBIS_NET_ERROR_EPROCUNAVAIL);
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO, 1),
              ORBIS_NET_ERROR_EPROCUNAVAIL); // P2P only
    // ... except SO_BROADCAST, which "other socket types are not affected" by.
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_BROADCAST, 1), ORBIS_OK);
    // Priority: only 8 to 23 may be set.
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_PRIORITY, 7), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_PRIORITY, 23), ORBIS_OK);
    // Buffers: at most 512 KiB.
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF, 4 << 20), ORBIS_OK);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 512 * 1024);
    // Multicast TTL and loop take an unsigned char or an int.
    const u8 ttl = 4;
    EXPECT_EQ(sceNetSetsockopt(udp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_MULTICAST_TTL, &ttl, 1),
              ORBIS_OK);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_MULTICAST_TTL), 4);
    u8 loop = 7;
    len = 1;
    EXPECT_EQ(sceNetGetsockopt(udp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_MULTICAST_LOOP, &loop, &len),
              ORBIS_OK);
    EXPECT_EQ(loop, 1); // default: loopback
    // BSD SO_RCVTIMEO (libkernel setsockopt): struct timeval.
    const s64 tv[2] = {1, 500000};
    EXPECT_EQ(sceNetSetsockopt(udp, ORBIS_NET_SOL_SOCKET, 0x1006, tv, sizeof(tv)), ORBIS_OK);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO), 1500000);
    s64 back[2]{};
    len = sizeof(back);
    EXPECT_EQ(sceNetGetsockopt(udp, ORBIS_NET_SOL_SOCKET, 0x1006, back, &len), ORBIS_OK);
    EXPECT_EQ(back[0], 1);
    EXPECT_EQ(back[1], 500000);
    // IP_DONTFRAG reaches the host and reads back.
    EXPECT_EQ(SetInt(udp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_DONTFRAG, 1), ORBIS_OK);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_DONTFRAG), 1);
    // Policy: 0-15 or ORBIS_NET_SOCK_POLICY_NA (-1).
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_POLICY, ORBIS_NET_SOCK_POLICY_NA),
              ORBIS_OK);
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_POLICY, 16), ORBIS_NET_ERROR_EINVAL);

    // UDP_SND_ON_SUSPEND: UDP only, and not together with connect.
    OrbisNetSockaddrIn dest = Loopback(9);
    char payload[] = "bye";
    OrbisNetUdpSndOnSuspend suspend{1, Guest(&dest), sizeof(dest), payload, 3};
    EXPECT_EQ(sceNetSetsockopt(tcp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, &suspend,
                               sizeof(suspend)),
              ORBIS_NET_ERROR_EPROCUNAVAIL);
    suspend.datalen = 513;
    EXPECT_EQ(sceNetSetsockopt(udp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, &suspend,
                               sizeof(suspend)),
              ORBIS_NET_ERROR_EINVAL);
    suspend.datalen = 3;
    ASSERT_EQ(sceNetSetsockopt(udp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, &suspend,
                               sizeof(suspend)),
              ORBIS_OK);
    OrbisNetUdpSndOnSuspend back_suspend{};
    len = sizeof(back_suspend);
    EXPECT_EQ(sceNetGetsockopt(udp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND,
                               &back_suspend, &len),
              ORBIS_OK);
    EXPECT_EQ(back_suspend.onoff, 1);
    EXPECT_EQ(sceNetConnect(udp, Guest(&dest), sizeof(dest)), ORBIS_NET_ERROR_EPROCUNAVAIL);
    suspend.onoff = 0;
    sceNetSetsockopt(udp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, &suspend,
                     sizeof(suspend));
    ASSERT_EQ(sceNetConnect(udp, Guest(&dest), sizeof(dest)), ORBIS_OK);
    suspend.onoff = 1;
    EXPECT_EQ(sceNetSetsockopt(udp, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, &suspend,
                               sizeof(suspend)),
              ORBIS_NET_ERROR_EPROCUNAVAIL); // after connect
    sceNetSocketClose(tcp);
    sceNetSocketClose(udp);
}

TEST_F(NetLib, ConnectAndAcceptTimeouts) {
    // A listener that never accepts, with a backlog of one, so the second connect hangs.
    // Instead, an address nothing answers (TEST-NET-1): the SYN goes unanswered.
    const OrbisNetId s = sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_CONNECTTIMEO, 200000), ORBIS_OK);
    OrbisNetSockaddrIn nowhere = Loopback(9);
    nowhere.sin_addr = sceNetHtonl(0xC0000201); // 192.0.2.1
    auto start = std::chrono::steady_clock::now();
    const s32 r = sceNetConnect(s, Guest(&nowhere), sizeof(nowhere));
    EXPECT_LT(r, 0);
    // Where the host has no route (or something answers for it), the connect fails at once;
    // only an unanswered SYN shows the timeout.
    if (Millis(start) >= 100.0) {
        EXPECT_EQ(r, ORBIS_NET_ERROR_EWOULDBLOCK);
        EXPECT_LT(Millis(start), 3000.0);
    }
    sceNetSocketClose(s);

    OrbisNetId listener = 0;
    Listen(&listener);
    EXPECT_EQ(SetInt(listener, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_ACCEPTTIMEO, 150000), ORBIS_OK);
    start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetAccept(listener, nullptr, nullptr), ORBIS_NET_ERROR_EWOULDBLOCK);
    EXPECT_GE(Millis(start), 140.0);
    // Connecting a listening socket is EOPNOTSUPP.
    auto self = Loopback(1);
    EXPECT_EQ(sceNetConnect(listener, Guest(&self), sizeof(self)), ORBIS_NET_ERROR_EOPNOTSUPP);
    // TCP to the broadcast address: EACCES.
    const OrbisNetId t = sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto broadcast = Loopback(80);
    broadcast.sin_addr = ORBIS_NET_INADDR_BROADCAST;
    EXPECT_EQ(sceNetConnect(t, Guest(&broadcast), sizeof(broadcast)), ORBIS_NET_ERROR_EACCES);
    sceNetSocketClose(t);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, WaitallPeeklenAndConnectedSendto) {
    OrbisNetId listener = 0;
    const u16 port = Listen(&listener);
    const OrbisNetId client = sceNetSocket("c", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    const OrbisNetId server = sceNetAccept(listener, nullptr, nullptr);
    ASSERT_GT(server, 0);

    // MSG_WAITALL waits for the whole length even when it arrives in pieces.
    std::thread writer([&] {
        for (const char* piece : {"ab", "cd", "ef"}) {
            std::this_thread::sleep_for(30ms);
            sceNetSend(client, piece, 2, 0);
        }
    });
    char buf[8]{};
    EXPECT_EQ(sceNetRecv(server, buf, 6, ORBIS_NET_MSG_WAITALL), 6);
    writer.join();
    EXPECT_STREQ(buf, "abcdef");
    // ... and returns what it has at FIN.
    sceNetSend(client, "xyz", 3, 0);
    sceNetShutdown(client, ORBIS_NET_SHUT_WR);
    std::memset(buf, 0, sizeof(buf));
    EXPECT_EQ(sceNetRecv(server, buf, 8, ORBIS_NET_MSG_WAITALL), 3);

    // MSG_PEEKLEN: the size waiting, with no buffer.
    const OrbisNetId udp = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(udp, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(udp, Guest(&self), &len);
    sceNetSendto(udp, "12345", 5, 0, Guest(&self), sizeof(self));
    std::this_thread::sleep_for(20ms);
    EXPECT_EQ(sceNetRecv(udp, nullptr, 1500, ORBIS_NET_MSG_PEEKLEN), 5);
    EXPECT_EQ(sceNetRecv(udp, buf, sizeof(buf), 0), 5); // still there

    // A destination for a connected socket: EISCONN.
    ASSERT_EQ(sceNetConnect(udp, Guest(&self), sizeof(self)), ORBIS_OK);
    EXPECT_EQ(sceNetSendto(udp, "x", 1, 0, Guest(&self), sizeof(self)), ORBIS_NET_ERROR_EISCONN);
    EXPECT_EQ(sceNetSend(udp, "x", 1, 0), 1);
    sceNetSocketClose(udp);
    sceNetSocketClose(server);
    sceNetSocketClose(client);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, SendPreservationAgain) {
    // An aborted send that already moved data returns the partial count; with
    // SND_PRESERVATION_AGAIN the send after it fails with EINTR once.
    OrbisNetId listener = 0;
    const u16 port = Listen(&listener);
    const OrbisNetId client = sceNetSocket("c", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    const OrbisNetId server = sceNetAccept(listener, nullptr, nullptr); // never reads
    std::vector<u8> big(64 << 20);
    s32 sent = 0;
    std::thread sender([&] { sent = sceNetSend(client, big.data(), big.size(), 0); });
    std::this_thread::sleep_for(300ms); // the buffers fill, the send blocks
    EXPECT_EQ(sceNetSocketAbort(client, 0x4 /* SND_PRESERVATION_AGAIN */), ORBIS_OK);
    sender.join();
    EXPECT_GT(sent, 0);
    EXPECT_LT(sent, static_cast<s32>(big.size()));
    EXPECT_EQ(sceNetSend(client, "x", 1, ORBIS_NET_MSG_DONTWAIT), ORBIS_NET_ERROR_EINTR);
    EXPECT_NE(sceNetSend(client, "x", 1, ORBIS_NET_MSG_DONTWAIT), ORBIS_NET_ERROR_EINTR); // once
    sceNetSocketClose(server);
    sceNetSocketClose(client);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, ReservedPortsCannotBeBound) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    for (const u16 port : {80, 5353, 8545, 9300, 45000}) {
        auto addr = Loopback(port);
        EXPECT_EQ(sceNetBind(s, Guest(&addr), sizeof(addr)), ORBIS_NET_ERROR_EACCES) << port;
    }
    auto ephemeral = Loopback(0);
    EXPECT_EQ(sceNetBind(s, Guest(&ephemeral), sizeof(ephemeral)), ORBIS_OK);
    sceNetSocketClose(s);
}

TEST_F(NetLib, PS4BufferSizesAndDatagramLimit) {
    const OrbisNetId tcp = sceNetSocket("tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    EXPECT_EQ(GetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF), 32768);
    EXPECT_EQ(GetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 65536);
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF, 20000), ORBIS_OK);
    EXPECT_EQ(GetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 20000); // not the host's
    EXPECT_EQ(SetInt(tcp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF, 0), ORBIS_NET_ERROR_EINVAL);
    sceNetSocketClose(tcp);

    const OrbisNetId udp = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF), 9216);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 40960);
    auto any = Loopback(0);
    sceNetBind(udp, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(udp, Guest(&self), &len);

    // A datagram larger than the send buffer fails with EMSGSIZE, as on FreeBSD.
    std::vector<u8> big(9217);
    EXPECT_EQ(sceNetSendto(udp, big.data(), 9216, 0, Guest(&self), sizeof(self)), 9216);
    EXPECT_EQ(sceNetSendto(udp, big.data(), big.size(), 0, Guest(&self), sizeof(self)),
              ORBIS_NET_ERROR_EMSGSIZE);
    EXPECT_EQ(SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF, 20000), ORBIS_OK);
    EXPECT_EQ(sceNetSendto(udp, big.data(), big.size(), 0, Guest(&self), sizeof(self)),
              static_cast<s32>(big.size()));
    sceNetSocketClose(udp);
}

TEST_F(NetLib, DebugNamePolicyAndPriority) {
    const OrbisNetId s = sceNetSocket("echo_server", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    // SO_NAME cannot be read back; sceNetGetSockInfo shows it.
    OrbisNetSockInfo info{};
    ASSERT_EQ(sceNetGetSockInfo(s, &info, 1, 0), 1);
    EXPECT_STREQ(info.name, "echo_server");

    // At most 31 characters: a longer name is ENAMETOOLONG and leaves the old one.
    const char long_name[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    EXPECT_EQ(
        sceNetSetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NAME, long_name, sizeof(long_name)),
        ORBIS_NET_ERROR_ENAMETOOLONG);
    const char name31[] = "0123456789abcdefghijklmnopqrstu";
    ASSERT_EQ(sceNetSetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NAME, name31, sizeof(name31)),
              ORBIS_OK);
    sceNetGetSockInfo(s, &info, 1, 0);
    EXPECT_STREQ(info.name, name31);

    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_POLICY), 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_PRIORITY), 16);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_POLICY, 3);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_POLICY), 3);
    sceNetSocketClose(s);
}

TEST_F(NetLib, AcceptedSocketsInheritTheListenersOptions) {
    OrbisNetId listener;
    const u16 port = Listen(&listener);
    ASSERT_EQ(SetInt(listener, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO, 250000), ORBIS_OK);
    ASSERT_EQ(SetInt(listener, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDTIMEO, 125000), ORBIS_OK);
    ASSERT_EQ(SetInt(listener, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR, 1), ORBIS_OK);
    const OrbisNetId client = sceNetSocket("client", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    const OrbisNetId accepted = sceNetAccept(listener, nullptr, nullptr);
    ASSERT_GT(accepted, 0);
    // As FreeBSD's sonewconn: timeouts and socket-level flags come from the listener.
    EXPECT_EQ(GetInt(accepted, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO), 250000);
    EXPECT_EQ(GetInt(accepted, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDTIMEO), 125000);
    EXPECT_EQ(GetInt(accepted, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR), 1);
    sceNetSocketClose(accepted);
    sceNetSocketClose(client);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, ReusePortIsItsOwnOption) {
    const OrbisNetId a = sceNetSocket("a", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    const OrbisNetId b = sceNetSocket("b", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    ASSERT_EQ(SetInt(a, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, 1), ORBIS_OK);
    ASSERT_EQ(SetInt(b, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, 1), ORBIS_OK);
    EXPECT_EQ(GetInt(a, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT), 1);
    EXPECT_EQ(GetInt(a, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR), 0); // untouched
    // Both bind the same port (an explicit one: the PS4 reserves 40000 and up, where hosts
    // hand out ephemeral ports).
    auto bound = Loopback(static_cast<u16>(20000 + std::random_device{}() % 10000));
    ASSERT_EQ(sceNetBind(a, Guest(&bound), sizeof(bound)), ORBIS_OK);
    EXPECT_EQ(sceNetBind(b, Guest(&bound), sizeof(bound)), ORBIS_OK);
    // Without it, the port is taken.
    const OrbisNetId c = sceNetSocket("c", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(sceNetBind(c, Guest(&bound), sizeof(bound)), ORBIS_NET_ERROR_EADDRINUSE);
    sceNetSocketClose(a);
    sceNetSocketClose(b);
    sceNetSocketClose(c);
}

TEST_F(NetLib, LingerOnCloseFollowsTheGuestMode) {
    // A connected pair whose receiver never reads, and a sender with unsent data queued.
    const auto unsent_pair = [&](OrbisNetId* listener, OrbisNetId* sender, OrbisNetId* receiver) {
        const u16 port = Listen(listener);
        // A small, fixed receive buffer (no autotuning), so the data really stays unsent.
        SetInt(*listener, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF, 4096);
        *sender = sceNetSocket("sender", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
        SetInt(*sender, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF, 16 * 1024);
        auto to = Loopback(port);
        ASSERT_EQ(sceNetConnect(*sender, Guest(&to), sizeof(to)), ORBIS_OK);
        *receiver = sceNetAccept(*listener, nullptr, nullptr);
        ASSERT_GT(*receiver, 0);
        SetInt(*sender, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, 1);
        // Fill until the socket stays full across a pause. One EWOULDBLOCK is not enough:
        // macOS keeps growing the receive buffer and drains the send queue right after.
        std::vector<char> chunk(64 * 1024, 'x');
        bool stuck = false;
        for (int round = 0; round < 200 && !stuck; ++round) {
            bool progressed = false;
            for (int i = 0; i < 4096; ++i) {
                if (sceNetSend(*sender, chunk.data(), chunk.size(), 0) < 0) {
                    break;
                }
                progressed = true;
            }
            ASSERT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EWOULDBLOCK);
            stuck = !progressed && round > 0;
            std::this_thread::sleep_for(20ms);
        }
        ASSERT_TRUE(stuck) << "the receiver kept taking data";
    };
    const OrbisNetLinger linger{1, 1};

    // Non-blocking guest socket: close returns at once and the data goes out in the background.
    OrbisNetId listener, sender, receiver;
    unsent_pair(&listener, &sender, &receiver);
    ASSERT_EQ(sceNetSetsockopt(sender, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &linger,
                               sizeof(linger)),
              ORBIS_OK);
    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetSocketClose(sender), ORBIS_OK);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 500ms);
    sceNetSocketClose(receiver);
    sceNetSocketClose(listener);

    // Blocking guest socket: close waits up to the linger time.
    unsent_pair(&listener, &sender, &receiver);
    SetInt(sender, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, 0);
    ASSERT_EQ(sceNetSetsockopt(sender, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &linger,
                               sizeof(linger)),
              ORBIS_OK);
    start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetSocketClose(sender), ORBIS_OK);
    const auto waited = std::chrono::steady_clock::now() - start;
    EXPECT_GE(waited, 500ms);
    EXPECT_LT(waited, 3s);
    sceNetSocketClose(receiver);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, PoolIdsAndRetailOnlyFeatures) {
    // Pool errors are returned without touching sce_net_errno.
    *sceNetErrnoLoc() = 0;
    EXPECT_EQ(sceNetPoolCreate("small", 4095, 0), ORBIS_NET_ERROR_EINVAL); // at least 4 KiB
    EXPECT_EQ(sceNetPoolCreate("flags", 4096, 1), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetPoolCreate("0123456789abcdefghijklmnopqrstuvwxyz", 4096, 0),
              ORBIS_NET_ERROR_ENAMETOOLONG);
    EXPECT_EQ(*sceNetErrnoLoc(), 0);

    std::vector<s32> pools;
    for (int i = 0; i < 32; ++i) {
        pools.push_back(sceNetPoolCreate("pool", 4096, 0));
    }
    EXPECT_EQ(pools.front(), 3000);
    EXPECT_EQ(pools[30], 3030);
    EXPECT_EQ(pools.back(), ORBIS_NET_ERROR_ENFILE); // at most 31 pools
    pools.pop_back();
    EXPECT_EQ(sceNetPoolDestroy(3005), ORBIS_OK);
    EXPECT_EQ(sceNetPoolDestroy(3005), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetPoolCreate("again", 4096, 0), 3005); // lowest free id

    // A pool with a resolver created from it is in use until the resolver is destroyed.
    const OrbisNetId rid = sceNetResolverCreate("user", 3005, 0);
    ASSERT_GT(rid, 0);
    EXPECT_EQ(sceNetPoolDestroy(3005), ORBIS_NET_ERROR_ENOTEMPTY);
    EXPECT_EQ(sceNetResolverDestroy(rid), ORBIS_OK);
    for (const s32 id : pools) {
        EXPECT_EQ(sceNetPoolDestroy(id), ORBIS_OK);
    }

    // Network emulation is not available on retail units.
    EXPECT_EQ(sceNetEmulationSet(), ORBIS_NET_ERROR_EOPNOTSUPP);
    EXPECT_EQ(sceNetEmulationGet(), ORBIS_NET_ERROR_EOPNOTSUPP);
}

TEST_F(NetLib, RecvTimeoutAndAbort) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(s, Guest(&any), sizeof(any));
    char buf[4];

    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO, 200000); // microseconds
    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetRecv(s, buf, sizeof(buf), 0), ORBIS_NET_ERROR_EWOULDBLOCK);
    EXPECT_GE(Millis(start), 190.0);
    EXPECT_LT(Millis(start), 2000.0);

    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO, 0);
    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        EXPECT_EQ(sceNetSocketAbort(s, 1 /* RCV_PRESERVATION */), ORBIS_OK);
    });
    start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetRecv(s, buf, sizeof(buf), 0), ORBIS_NET_ERROR_EINTR);
    EXPECT_LT(Millis(start), 2000.0);
    aborter.join();
    sceNetSocketClose(s);
}

TEST_F(NetLib, EpollThroughExports) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(s, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(s, Guest(&self), &len);

    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    ASSERT_GT(ep, 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    ev.data.data_u64 = 0xC0FFEE;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_OK);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_NET_ERROR_EEXIST);
    EXPECT_EQ(sceNetEpollControl(ep, 9, s, &ev), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_MOD, s, nullptr), ORBIS_NET_ERROR_EINVAL);

    OrbisNetEpollEvent out[4]{};
    EXPECT_EQ(sceNetEpollWait(ep, out, 4, 10000), 0); // 10 ms, nothing yet
    sceNetSendto(s, "x", 1, 0, Guest(&self), sizeof(self));
    ASSERT_EQ(sceNetEpollWait(ep, out, 4, 1000000), 1);
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLIN);
    EXPECT_EQ(out[0].ident, static_cast<u64>(s));
    EXPECT_EQ(out[0].data.data_u64, 0xC0FFEEu);
    EXPECT_EQ(sceNetEpollWait(ep, out, 0, 0), ORBIS_NET_ERROR_EINVAL);

    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        sceNetEpollAbort(ep, 1 /* PRESERVATION */);
    });
    char buf[4];
    sceNetRecv(s, buf, sizeof(buf), 0); // drain, so the wait below would block forever
    EXPECT_EQ(sceNetEpollWait(ep, out, 4, -1), ORBIS_NET_ERROR_EINTR);
    aborter.join();

    EXPECT_EQ(sceNetEpollDestroy(ep), ORBIS_OK);
    EXPECT_EQ(sceNetEpollDestroy(ep), ORBIS_NET_ERROR_EBADF);
    sceNetSocketClose(s);
}

TEST_F(NetLib, EpollErrorCodesMatchThePS4) {
    EXPECT_EQ(sceNetEpollCreate("flags", 1), ORBIS_NET_ERROR_EINVAL); // no flags defined

    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    ASSERT_GT(ep, 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;

    // DEL of an unregistered id: EBADF, where Linux would say ENOENT.
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_DEL, s, nullptr), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EBADF);
    // MOD of an unregistered id registers it
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_MOD, s, &ev), ORBIS_OK);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_DEL, s, nullptr), ORBIS_OK);
    // ADD/MOD need event bits.
    OrbisNetEpollEvent none{};
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &none), ORBIS_NET_ERROR_EINVAL);

    // Delete takes no event.
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_OK);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_DEL, s, &ev), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_DEL, s, nullptr), ORBIS_OK);

    sceNetEpollDestroy(ep);
    sceNetSocketClose(s);
}

TEST_F(NetLib, EpollIdKindsNamesAndAbortFlags) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    const OrbisNetId other = sceNetEpollCreate("other", 0);
    ASSERT_GT(ep, 0);
    ASSERT_GT(other, 0);
    EXPECT_EQ(sceNetEpollCreate("0123456789abcdefghijklmnopqrstuvwxyz", 0),
              ORBIS_NET_ERROR_ENAMETOOLONG);

    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    // Another epoll cannot be waited on; a socket is not an epoll; a free id is nothing.
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, other, &ev), ORBIS_NET_ERROR_EPERM);
    EXPECT_EQ(sceNetEpollControl(s, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, 12345, &ev), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetEpollDestroy(s), ORBIS_NET_ERROR_EPERM);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EPERM);
    OrbisNetEpollEvent out[2]{};
    EXPECT_EQ(sceNetEpollWait(s, out, 2, 0), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetEpollAbort(s, 0), ORBIS_NET_ERROR_EBADF);

    // Only ORBIS_NET_EPOLL_ABORT_FLAG_PRESERVATION; with it, the next wait fails even though an
    // event is there.
    EXPECT_EQ(sceNetEpollAbort(ep, 2), ORBIS_NET_ERROR_EINVAL);
    ev.events = ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLOUT;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_OK);
    auto any = Loopback(0);
    sceNetBind(s, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(s, Guest(&self), &len);
    sceNetSendto(s, "x", 1, 0, Guest(&self), sizeof(self));
    std::this_thread::sleep_for(20ms);
    EXPECT_EQ(sceNetEpollAbort(ep, 1), ORBIS_OK);
    EXPECT_EQ(sceNetEpollWait(ep, out, 2, 0), ORBIS_NET_ERROR_EINTR);
    // A UDP socket reports OUT too, it is always writable.
    ASSERT_EQ(sceNetEpollWait(ep, out, 2, 1'000'000), 1);
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLOUT);

    sceNetEpollDestroy(other);
    sceNetEpollDestroy(ep);
    sceNetSocketClose(s);
}

TEST_F(NetLib, EpollReportsAbortedSocketsAsHangup) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(s, Guest(&any), sizeof(any));
    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    ev.data.data_u64 = 7;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev), ORBIS_OK);

    // Nobody waiting anywhere: nothing to abort.
    EXPECT_EQ(sceNetSocketAbort(s, 0), ORBIS_NET_ERROR_ENOTBLK);

    std::atomic<s32> abort_result{1};
    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        abort_result = sceNetSocketAbort(s, 0);
    });
    OrbisNetEpollEvent out[2]{};
    ASSERT_EQ(sceNetEpollWait(ep, out, 2, 5'000'000), 1);
    aborter.join();
    EXPECT_EQ(abort_result.load(), ORBIS_OK); // the epoll wait counts as waiting
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLHUP);
    EXPECT_EQ(out[0].ident, static_cast<u64>(s));
    EXPECT_EQ(out[0].data.data_u64, 7u);
    // Reported once; the registration keeps working.
    EXPECT_EQ(sceNetEpollWait(ep, out, 2, 10'000), 0);

    sceNetEpollDestroy(ep);
    sceNetSocketClose(s);
}

TEST_F(NetLib, ResolverNeedsEpollInToBeReported) {
    ScopedPool pool;
    const OrbisNetId rid = sceNetResolverCreate("resolver", pool.id, 0);
    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLOUT; // ignored for a resolver: nothing is waited for
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, rid, &ev), ORBIS_OK);
    OrbisNetInAddr addr{};
    ASSERT_EQ(sceNetResolverStartNtoa(rid, "127.0.0.1", &addr, 0, 0, ORBIS_NET_RESOLVER_ASYNC),
              ORBIS_OK);
    OrbisNetEpollEvent out[2]{};
    EXPECT_EQ(sceNetEpollWait(ep, out, 2, 200'000), 0);
    // Asking for ORBIS_NET_EPOLLIN afterwards reports the finished lookup.
    ev.events = ORBIS_NET_EPOLLIN;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_MOD, rid, &ev), ORBIS_OK);
    ASSERT_EQ(sceNetEpollWait(ep, out, 2, 5'000'000), 1);
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLDESCID);
    EXPECT_EQ(sceNetEpollWait(ep, out, 2, 10'000), 0); // once
    sceNetEpollDestroy(ep);
    sceNetResolverDestroy(rid);
}

TEST_F(NetLib, SendmsgGathersAndRecvmsgScatters) {
    const OrbisNetId s = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(s, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(s, Guest(&self), &len);

    char a[] = "hello ", b[] = "world";
    OrbisNetIovec out_iov[] = {{a, 6}, {b, 5}};
    OrbisNetMsghdr out{};
    out.msg_name = &self;
    out.msg_namelen = sizeof(self);
    out.msg_iov = out_iov;
    out.msg_iovlen = 2;
    ASSERT_EQ(sceNetSendmsg(s, &out, 0), 11); // one datagram

    char x[4]{}, y[16]{};
    OrbisNetIovec in_iov[] = {{x, 4}, {y, sizeof(y)}};
    OrbisNetSockaddrIn from{};
    OrbisNetMsghdr in{};
    in.msg_name = &from;
    in.msg_namelen = sizeof(from);
    in.msg_iov = in_iov;
    in.msg_iovlen = 2;
    in.msg_controllen = 99;
    ASSERT_EQ(sceNetRecvmsg(s, &in, 0), 11);
    EXPECT_EQ(std::string(x, 4), "hell");
    EXPECT_STREQ(y, "o world");
    EXPECT_EQ(in.msg_namelen, sizeof(from));
    EXPECT_EQ(from.sin_port, self.sin_port);
    EXPECT_EQ(in.msg_controllen, 99u); // "not used": left alone

    in.msg_iovlen = -1;
    EXPECT_EQ(sceNetRecvmsg(s, &in, 0), ORBIS_NET_ERROR_EINVAL);
    in.msg_iovlen = 1025; // at most 1024 elements
    EXPECT_EQ(sceNetRecvmsg(s, &in, 0), ORBIS_NET_ERROR_EMSGSIZE);
    in.msg_iovlen = 2;
    EXPECT_EQ(sceNetRecvmsg(s, &in, ORBIS_NET_MSG_PEEKLEN), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetSendmsg(s, nullptr, 0), ORBIS_NET_ERROR_EFAULT);
    sceNetSocketClose(s);
}

TEST_F(NetLib, UnixDatagramSockets) {
#ifdef _WIN32
    GTEST_SKIP() << "Windows has no AF_UNIX datagram sockets";
#else
    const std::string path = "/tmp/shadps4_net_test_" + std::to_string(::getpid());
    ::unlink(path.c_str());
    OrbisNetSockaddrUn addr{};
    addr.sun_len = sizeof(addr);
    addr.sun_family = ORBIS_NET_AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    auto* guest = reinterpret_cast<OrbisNetSockaddr*>(&addr);

    const OrbisNetId server = sceNetSocket("unix", ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_DGRAM, 0);
    ASSERT_GT(server, 0);
    ASSERT_EQ(sceNetBind(server, guest, sizeof(addr)), ORBIS_OK);
    const OrbisNetId client = sceNetSocket("unix", ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(sceNetSendto(client, "hi", 2, 0, guest, sizeof(addr)), 2);
    char buf[8]{};
    EXPECT_EQ(sceNetRecv(server, buf, sizeof(buf), 0), 2);
    EXPECT_STREQ(buf, "hi");

    OrbisNetSockaddrUn name{};
    u32 len = sizeof(name);
    ASSERT_EQ(sceNetGetsockname(server, reinterpret_cast<OrbisNetSockaddr*>(&name), &len),
              ORBIS_OK);
    EXPECT_EQ(name.sun_family, ORBIS_NET_AF_UNIX);
    EXPECT_EQ(std::string(name.sun_path), path);
    EXPECT_EQ(sceNetSocket("unix", ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_DGRAM_P2P, 0),
              ORBIS_NET_ERROR_EPROTONOSUPPORT);
    sceNetSocketClose(client);
    sceNetSocketClose(server);
    ::unlink(path.c_str());
#endif
}

TEST_F(NetLib, SockInfoDescribesSockets) {
    // No buffer and s < 0: the number of sockets.
    EXPECT_EQ(sceNetGetSockInfo(-1, nullptr, 0, 0), 0);
    const OrbisNetId udp = sceNetSocket("sockinfo_udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(udp, Guest(&any), sizeof(any));
    SetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, 1);
    const OrbisNetId tcp =
        sceNetSocket("sockinfo_tcp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    EXPECT_EQ(sceNetGetSockInfo(-1, nullptr, 0, 0), 2);

    OrbisNetSockInfo info{};
    ASSERT_EQ(sceNetGetSockInfo(udp, &info, 1, 0), 1);
    EXPECT_STREQ(info.name, "sockinfo_udp");
    EXPECT_EQ(info.s, udp);
    EXPECT_EQ(info.socket_type, ORBIS_NET_SOCK_DGRAM);
    EXPECT_EQ(info.local_adr.inaddr_addr, sceNetHtonl(0x7F000001));
    EXPECT_NE(info.local_port, 0);
    EXPECT_EQ(info.flags, ORBIS_NET_SOCKINFO_F_SELF | ORBIS_NET_SOCKINFO_F_NONBLOCK);
    EXPECT_EQ(info.recv_buffer_size, 40960);
    EXPECT_EQ(info.state, ORBIS_NET_SOCKINFO_STATE_OPENED);

    OrbisNetSockInfo all[4]{};
    ASSERT_EQ(sceNetGetSockInfo(-1, all, 4, 0), 2);
    EXPECT_EQ(all[1].s, tcp);
    EXPECT_EQ(all[1].socket_type, ORBIS_NET_SOCK_STREAM);
    EXPECT_EQ(sceNetGetSockInfo(-1, all, 1, 0), 1); // no more than asked for

    EXPECT_EQ(sceNetGetSockInfo(9999, &info, 1, 0), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetGetSockInfo(udp, &info, 0, 0), ORBIS_NET_ERROR_EINVAL);
    sceNetSocketClose(udp);
    sceNetSocketClose(tcp);
}

TEST_F(NetLib, ResolverLooksUpNames) {
    ScopedPool pool;
    const OrbisNetId rid = sceNetResolverCreate("resolver", pool.id, 0);
    ASSERT_GT(rid, 0);
    EXPECT_EQ(sceNetResolverCreate("bad", pool.id, 1), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetResolverCreate("bad", 0, 0), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetResolverCreate("bad", 3099, 0), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetResolverCreate("0123456789abcdef0123456789abcdef", pool.id, 0),
              ORBIS_NET_ERROR_ENAMETOOLONG);
    EXPECT_EQ(sceNetShowRouteWithMemory(0), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetShowRouteWithMemory(3099), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetShowRouteWithMemory(pool.id), ORBIS_OK);

    // Synchronous: a numeric address needs no DNS.
    OrbisNetInAddr addr{};
    ASSERT_EQ(sceNetResolverStartNtoa(rid, "127.0.0.1", &addr, 0, 0, 0), ORBIS_OK);
    EXPECT_EQ(addr.inaddr_addr, sceNetHtonl(0x7F000001));
    OrbisNetResolverInfo info{};
    ASSERT_EQ(sceNetResolverStartNtoaMultipleRecords(rid, "127.0.0.2", &info, 0, 0, 0), ORBIS_OK);
    EXPECT_EQ(info.records, 1u);
    EXPECT_EQ(info.recordsv4, 1u);
    EXPECT_EQ(info.addrs[0].af, static_cast<u32>(ORBIS_NET_AF_INET));
    EXPECT_EQ(info.addrs[0].u.addr.inaddr_addr, sceNetHtonl(0x7F000002));

    // Asynchronous: completion shows up in an epoll as ORBIS_NET_EPOLLDESCID.
    const OrbisNetId ep = sceNetEpollCreate("resolver", 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    ev.data.data_u64 = 42;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, rid, &ev), ORBIS_OK);
    addr = {};
    ASSERT_EQ(sceNetResolverStartNtoa(rid, "127.0.0.3", &addr, 0, 0, ORBIS_NET_RESOLVER_ASYNC),
              ORBIS_OK);
    OrbisNetEpollEvent out[2]{};
    ASSERT_EQ(sceNetEpollWait(ep, out, 2, 5'000'000), 1);
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLDESCID);
    EXPECT_EQ(out[0].ident, static_cast<u64>(rid));
    EXPECT_EQ(out[0].data.data_u64, 42u);
    s32 status = -1;
    ASSERT_EQ(sceNetResolverGetError(rid, &status), ORBIS_OK);
    EXPECT_EQ(status, 0);
    EXPECT_EQ(addr.inaddr_addr, sceNetHtonl(0x7F000003));
    EXPECT_EQ(sceNetResolverGetError(rid, nullptr), ORBIS_NET_ERROR_EINVAL);

    // Offline, names fail with ENODNS and the status says so. Addresses, "localhost" and a
    // trailing dot are handled locally, unless DISABLE_IPADDRESS sends an address to DNS.
    SetSystemHooks({.is_online = [] { return false; }});
    EXPECT_EQ(sceNetResolverStartNtoa(rid, "example.com", &addr, 0, 0, 0),
              ORBIS_NET_ERROR_RESOLVER_ENODNS);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_RESOLVER_ENODNS);
    sceNetResolverGetError(rid, &status);
    EXPECT_EQ(status, ORBIS_NET_ERROR_RESOLVER_ENODNS);
    ASSERT_EQ(sceNetResolverStartNtoa(rid, "10.1.2.3", &addr, 0, 0, 0), ORBIS_OK);
    EXPECT_EQ(addr.inaddr_addr, sceNetHtonl(0x0A010203));
    ASSERT_EQ(sceNetResolverStartNtoa(rid, "localhost.", &addr, 0, 0, 0), ORBIS_OK);
    EXPECT_EQ(addr.inaddr_addr, sceNetHtonl(ORBIS_NET_INADDR_LOOPBACK));
    EXPECT_EQ(sceNetResolverStartNtoa(rid, "10.1.2.3", &addr, 0, 0,
                                      ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS),
              ORBIS_NET_ERROR_RESOLVER_ENODNS);
    EXPECT_EQ(sceNetResolverStartNtoa(rid, "", &addr, 0, 0, 0), ORBIS_NET_ERROR_EINVAL);
    OrbisNetResolverInfo records{};
    EXPECT_EQ(sceNetResolverStartNtoaMultipleRecordsEx(rid, "10.1.2.3", &records, 0, 0, 0x2000000),
              ORBIS_OK);
    EXPECT_EQ(records.records, 1u);
    EXPECT_EQ(sceNetResolverStartNtoaMultipleRecordsEx(rid, "10.1.2.3", &records, 0, 0, 0x1000000),
              ORBIS_NET_ERROR_RESOLVER_ENORECORD);
    EXPECT_EQ(sceNetResolverStartNtoaMultipleRecordsEx(rid, "10.1.2.3", &records, 0, 0, 0x4),
              ORBIS_NET_ERROR_EINVAL);
    SetSystemHooks({});

    // Destroying removes it from the epoll and frees the id.
    EXPECT_EQ(sceNetResolverDestroy(rid), ORBIS_OK);
    EXPECT_EQ(sceNetResolverDestroy(rid), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetResolverStartNtoa(rid, "127.0.0.1", &addr, 0, 0, 0), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_DEL, rid, nullptr), ORBIS_NET_ERROR_EBADF);
    sceNetEpollDestroy(ep);
}

TEST_F(NetLib, EtherPoolsAndStatistics) {
    OrbisNetEtherAddr mac{};
    ASSERT_EQ(sceNetEtherStrton("00:1A:2b:3c:4D:5e", &mac), ORBIS_OK);
    EXPECT_EQ(mac.data[1], 0x1a);
    EXPECT_EQ(mac.data[5], 0x5e);
    char text[ORBIS_NET_ETHER_ADDRSTRLEN]{};
    ASSERT_EQ(sceNetEtherNtostr(&mac, text, sizeof(text)), ORBIS_OK);
    EXPECT_STREQ(text, "00:1a:2b:3c:4d:5e");
    EXPECT_EQ(sceNetEtherNtostr(&mac, text, 17), ORBIS_NET_ERROR_ENOSPC);
    EXPECT_EQ(sceNetEtherStrton("00:1a:2b", &mac), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetEtherStrton("00:1a:2b:3c:4d:5e:6f", &mac), ORBIS_NET_ERROR_EINVAL);

    const s32 pool = sceNetPoolCreate("pool", 64 * 1024, 0);
    OrbisNetMemoryPoolStats stats{};
    ASSERT_EQ(sceNetGetMemoryPoolStats(pool, &stats), ORBIS_OK);
    EXPECT_EQ(stats.pool_size, 64u * 1024u);
    EXPECT_EQ(stats.current_inuse_size, 0u);
    EXPECT_EQ(sceNetGetMemoryPoolStats(pool, nullptr), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EINVAL);
    sceNetPoolDestroy(pool);
    EXPECT_EQ(sceNetGetMemoryPoolStats(pool, &stats), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EBADF);

    OrbisNetStatisticsInfo info{};
    ASSERT_EQ(sceNetGetStatisticsInfo(&info, 0), ORBIS_OK);
    EXPECT_GT(info.kernel_mem_free_size, 0);
    EXPECT_GT(info.libnet_mem_free_size, 0);
}

TEST_F(NetLib, EpollReportsPeerCloseAsReadable) {
    OrbisNetId listener = 0;
    const u16 port = Listen(&listener);
    OrbisNetId accepted = -1;
    std::thread server([&] { accepted = sceNetAccept(listener, nullptr, nullptr); });
    const OrbisNetId client = sceNetSocket("client", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    server.join();
    ASSERT_GT(accepted, 0);

    const OrbisNetId ep = sceNetEpollCreate("ep", 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    ASSERT_EQ(sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, accepted, &ev), ORBIS_OK);

    // The PS4 epoll is kqueue read/write filters: the peer closing shows up as IN only, and
    // the read returns the end of stream.
    sceNetSocketClose(client);
    OrbisNetEpollEvent out[2]{};
    ASSERT_EQ(sceNetEpollWait(ep, out, 2, 2'000'000), 1);
    EXPECT_EQ(out[0].events, ORBIS_NET_EPOLLIN);
    char buf[4];
    EXPECT_EQ(sceNetRecv(accepted, buf, sizeof(buf), 0), 0);

    sceNetEpollDestroy(ep);
    sceNetSocketClose(accepted);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, SmallHelpers) {
    u32 random = 0;
    EXPECT_EQ(sceNetGetRandom(&random), ORBIS_OK);
    EXPECT_EQ(sceNetGetRandom(nullptr), ORBIS_NET_ERROR_EINVAL);

    u64 t1 = 0;
    u64 t2 = 0;
    sceNetGetSystemTime(&t1);
    std::this_thread::sleep_for(2ms);
    sceNetGetSystemTime(&t2);
    EXPECT_GT(t2, t1);
    sceNetGetSystemTime(nullptr); // ignored

    EXPECT_STREQ(sceNetGetIfName(0), "lo0");
    EXPECT_STREQ(sceNetGetIfName(1), "eth0");
    EXPECT_STREQ(sceNetGetIfName(10), "");
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EINVAL);

    EXPECT_EQ(sceNetMemoryAllocate(0, 0), nullptr);
    auto* zeroed = static_cast<u8*>(sceNetMemoryAllocate(64, 2));
    ASSERT_NE(zeroed, nullptr);
    EXPECT_EQ(std::count(zeroed, zeroed + 64, u8{0}), 64);
    sceNetMemoryFree(zeroed);

    EXPECT_EQ(sceNetUsleep(-1), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetUsleep(10), ORBIS_OK);
}

TEST_F(NetLib, IoctlAndAddressEdgeCasesOnNativeSockets) {
    const OrbisNetId udp = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    ASSERT_EQ(sceNetBind(udp, Guest(&any), sizeof(any)), ORBIS_OK);
    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    sceNetGetsockname(udp, Guest(&self), &len);

    // FIONBIO and FIONREAD reach the socket like they would the kernel.
    s32 on = 1;
    ASSERT_EQ(sceNetIoctl(udp, 0x8004667e, &on), ORBIS_OK);
    EXPECT_EQ(GetInt(udp, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO), 1);
    s32 ready = -1;
    ASSERT_EQ(sceNetIoctl(udp, 0x4004667f, &ready), ORBIS_OK);
    EXPECT_EQ(ready, 0);
    sceNetSendto(udp, "12345", 5, 0, Guest(&self), sizeof(self));
    std::this_thread::sleep_for(20ms);
    ASSERT_EQ(sceNetIoctl(udp, 0x4004667f, &ready), ORBIS_OK);
    EXPECT_GE(ready, 5);
    EXPECT_EQ(sceNetIoctl(udp, 0x12345678, &ready), ORBIS_NET_ERROR_EINVAL);

    // An address buffer without a length pointer is ignored, not EFAULT.
    char buf[8];
    OrbisNetSockaddrIn from{};
    EXPECT_EQ(sceNetRecvfrom(udp, buf, sizeof(buf), 0, Guest(&from), nullptr), 5);
    sceNetSocketClose(udp);

    // A connected stream ignores a destination address.
    OrbisNetId listener = 0;
    const u16 port = Listen(&listener);
    OrbisNetId accepted = -1;
    std::thread server([&] { accepted = sceNetAccept(listener, nullptr, nullptr); });
    const OrbisNetId client = sceNetSocket("client", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    auto to = Loopback(port);
    ASSERT_EQ(sceNetConnect(client, Guest(&to), sizeof(to)), ORBIS_OK);
    server.join();
    ASSERT_GT(accepted, 0);
    EXPECT_EQ(sceNetSendto(client, "hi", 2, 0, Guest(&to), sizeof(to)), 2);
    EXPECT_EQ(sceNetRecv(accepted, buf, sizeof(buf), 0), 2);
    sceNetSocketClose(client);
    sceNetSocketClose(accepted);
    sceNetSocketClose(listener);
}

TEST_F(NetLib, DnsInfoOverride) {
    std::array<u32, 2> dns{};
    EXPECT_EQ(sceNetGetDnsInfo(nullptr, 0), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetGetDnsInfo(dns.data(), 1), ORBIS_NET_ERROR_EINVAL);

    const std::array<u32, 2> mine{sceNetHtonl(0x0A000001), 0};
    EXPECT_EQ(sceNetSetDnsInfo(mine.data(), 0), ORBIS_OK);
    EXPECT_EQ(sceNetGetDnsInfo(dns.data(), 0), 1); // the count
    EXPECT_EQ(dns, mine);
    const std::array<u32, 2> no_primary{0, sceNetHtonl(0x0A000001)};
    EXPECT_EQ(sceNetSetDnsInfo(no_primary.data(), 0), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetSetDnsInfo(nullptr, 0), ORBIS_OK); // clears it

    std::array<u8, 32> dns6{};
    std::array<u8, 32> mine6{};
    mine6[0] = 0x20;
    mine6[15] = 1;
    EXPECT_EQ(sceNetGetDns6Info(dns6.data(), 0), 0);
    EXPECT_EQ(sceNetSetDns6Info(mine6.data(), 0), ORBIS_OK);
    EXPECT_EQ(sceNetGetDns6Info(dns6.data(), 0), 1);
    EXPECT_EQ(dns6, mine6);
    EXPECT_EQ(sceNetSetDns6Info(nullptr, 0), ORBIS_OK);
}

TEST_F(NetLib, ResolverReverseLookupAndPreservedAbort) {
    ScopedPool pool;
    const OrbisNetId rid = sceNetResolverCreate("aton", pool.id, 0);
    ASSERT_GT(rid, 0);
    // A preserved abort fails the next lookup of its kind only.
    EXPECT_EQ(sceNetResolverAbort(rid, ORBIS_NET_RESOLVER_ABORT_FLAG_ATON_PRESERVATION), ORBIS_OK);
    OrbisNetInAddr addr{};
    EXPECT_EQ(sceNetResolverStartNtoa(rid, "127.0.0.1", &addr, 0, 0, 0), ORBIS_OK);
    const OrbisNetInAddr loopback{sceNetHtonl(ORBIS_NET_INADDR_LOOPBACK)};
    char name[256]{};
    EXPECT_EQ(sceNetResolverStartAton(rid, &loopback, name, sizeof(name), 0, 0, 0),
              ORBIS_NET_ERROR_EINTR);
    // Then the reverse lookup works: 127.0.0.1 is "localhost", answered locally.
    ASSERT_EQ(sceNetResolverStartAton(rid, &loopback, name, sizeof(name), 0, 0, 0), ORBIS_OK);
    EXPECT_STREQ(name, "localhost");
    char tiny[2]{};
    EXPECT_EQ(sceNetResolverStartAton(rid, &loopback, tiny, sizeof(tiny), 0, 0, 0),
              ORBIS_NET_ERROR_RESOLVER_ENOSPACE);
    // Host names longer than 255 characters are refused.
    const std::string long_name(256, 'a');
    EXPECT_EQ(sceNetResolverStartNtoa(rid, long_name.c_str(), &addr, 0, 0, 0),
              ORBIS_NET_ERROR_EINVAL);
    sceNetResolverDestroy(rid);
}

TEST_F(NetLib, MacAddressComesFromTheHost) {
    OrbisNetEtherAddr mac{};
    EXPECT_EQ(sceNetGetMacAddress(nullptr, 0), ORBIS_NET_ERROR_EINVAL);
    SetSystemHooks({.mac_address = [](std::array<u8, 6>* out) {
        *out = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        return true;
    }});
    ASSERT_EQ(sceNetGetMacAddress(&mac, 0), ORBIS_OK);
    EXPECT_EQ(mac.data[5], 0x55);
    SetSystemHooks({});
    ASSERT_EQ(sceNetGetMacAddress(&mac, 0), ORBIS_OK);
    EXPECT_EQ(mac.data[5], 0x00); // no hook: zeros
}

TEST_F(NetLib, P2PTransportLifecycleHooks) {
    u16 started = 0;
    u16 stopped = 0;
    int starts = 0;
    u16 port_setting = 0; // any free port
    SetSystemHooks({
        .p2p_port = [&] { return port_setting; },
        .p2p_started =
            [&](u16 port) {
                started = port;
                ++starts;
            },
        .p2p_stopped = [&](u16 port) { stopped = port; },
        .public_addr = [] { return sceNetHtonl(0xC6336401); }, // 198.51.100.1
    });
    EXPECT_EQ(GetP2PConfiguredPort(), 0); // the setting wins over ConfigureP2P
    EXPECT_EQ(GetP2PAdvertisedAddr(), sceNetHtonl(0xC6336401));
    EXPECT_FALSE(P2PTransportIsReady());

    // First use starts it and tells the hook the port it got; later uses do not restart it.
    ASSERT_TRUE(EnsureP2PTransport());
    EXPECT_NE(started, 0);
    EXPECT_EQ(started, GetP2PBoundPort());
    EXPECT_EQ(GetP2PAdvertisedPort(), started);
    EXPECT_TRUE(EnsureP2PTransport());
    EXPECT_EQ(starts, 1);

    StopP2P();
    EXPECT_EQ(stopped, started);
    EXPECT_FALSE(P2PTransportIsReady());
    EXPECT_EQ(GetP2PAdvertisedPort(), 0); // not running: the configured port

    // A port that is taken, e.g. by another instance on the same PC: the transport moves to a
    // nearby free port, and that is the one advertised and forwarded.
    const OrbisNetId blocker = sceNetSocket("blocker", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    OrbisNetSockaddrIn at{};
    at.sin_len = sizeof(at);
    at.sin_family = ORBIS_NET_AF_INET;
    bool bound = false;
    for (u16 port = 20000; port < 30000 && !bound; port += 97) {
        at.sin_port = sceNetHtons(port);
        bound = sceNetBind(blocker, Guest(&at), sizeof(at)) == ORBIS_OK;
    }
    ASSERT_TRUE(bound);
    port_setting = sceNetNtohs(at.sin_port);
    ASSERT_TRUE(EnsureP2PTransport());
    EXPECT_NE(GetP2PBoundPort(), port_setting);
    EXPECT_GT(GetP2PBoundPort(), port_setting);
    EXPECT_LE(GetP2PBoundPort(), port_setting + 16);
    EXPECT_EQ(started, GetP2PBoundPort());
    EXPECT_EQ(GetP2PAdvertisedPort(), started);
    EXPECT_EQ(starts, 2);
    sceNetSocketClose(blocker);

    // An explicit StartP2P gets exactly the port it asks for, or nothing.
    StopP2P();
    const OrbisNetId blocker2 =
        sceNetSocket("blocker2", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    ASSERT_EQ(sceNetBind(blocker2, Guest(&at), sizeof(at)), ORBIS_OK);
    EXPECT_EQ(StartP2P(port_setting), 0);
    sceNetSocketClose(blocker2);
    EXPECT_EQ(StartP2P(port_setting), port_setting);

    StopP2P();
    SetSystemHooks({});
}

// ---------------------------------------------------------------------------------------------
// P2P, over the real wire format
// ---------------------------------------------------------------------------------------------

class NetLibP2P : public NetLib {
protected:
    static void SetUpTestSuite() {
        NetLib::SetUpTestSuite();
        // The transport starts with the first P2P socket, on the configured port.
        ConfigureP2P(0, 0); // any free port for the test
        EXPECT_FALSE(P2PTransportIsReady());
        const OrbisNetId first =
            sceNetSocket("first", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
        EXPECT_GT(first, 0);
        EXPECT_TRUE(P2PTransportIsReady());
        sceNetSocketClose(first);
        local_keys_ = GetP2PKeyring();
        local_port_ = GetP2PBoundPort();
        ASSERT_NE(local_port_, 0);
        Host::Error error;
        remote_keys_ = std::make_shared<P2P::Keyring>();
        remote_ =
            P2P::Transport::Create(AF_INET, P2P::Endpoint::IPv4("127.0.0.1", 0),
                                   std::make_unique<P2P::FramingCodec>(remote_keys_), {}, &error);
        ASSERT_TRUE(remote_);
    }
    static void TearDownTestSuite() {
        StopP2P();
        remote_.reset();
        NetLib::TearDownTestSuite();
    }

    static P2P::Endpoint LocalEndpoint() {
        return P2P::Endpoint::IPv4("127.0.0.1", local_port_);
    }

    static inline u16 local_port_ = 0;
    static inline std::shared_ptr<P2P::Keyring> local_keys_;
    static inline std::shared_ptr<P2P::Keyring> remote_keys_;
    static inline std::shared_ptr<P2P::Transport> remote_;
};

// shadNet's STUN server is a plain UDP socket. Signaling reaches it as a P2P datagram from and
// to vport 0xFFFF, and it answers the same way.
TEST_F(NetLibP2P, SignalingReachesShadNetStunServer) {
    const OrbisNetId server = sceNetSocket("server", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    ASSERT_GT(server, 0);
    OrbisNetSockaddrIn addr{};
    addr.sin_len = sizeof(addr);
    addr.sin_family = ORBIS_NET_AF_INET;
    addr.sin_addr = sceNetHtonl(0x7F000001);
    ASSERT_EQ(sceNetBind(server, Guest(&addr), sizeof(addr)), ORBIS_OK);
    u32 len = sizeof(addr);
    ASSERT_EQ(sceNetGetsockname(server, Guest(&addr), &len), ORBIS_OK);

    const std::array<u8, 6> header{0xFF, 0x80 | 3, 0xFF, 0xFF, 0xFF, 0xFF};
    const std::array<u8, 21> ping{0x01, 'S', 't', 'e', 'p', 'h', 'e', 'n'};
    ASSERT_EQ(P2PSignalingSendTo(ping.data(), ping.size(), addr.sin_addr, addr.sin_port),
              static_cast<int>(ping.size()));
    std::array<u8, 64> got{};
    OrbisNetSockaddrIn from{};
    u32 from_len = sizeof(from);
    ASSERT_EQ(sceNetRecvfrom(server, got.data(), got.size(), 0, Guest(&from), &from_len),
              static_cast<int>(header.size() + ping.size()));
    EXPECT_TRUE(std::equal(header.begin(), header.end(), got.begin()));
    EXPECT_TRUE(std::equal(ping.begin(), ping.end(), got.begin() + header.size()));
    EXPECT_EQ(sceNetNtohs(from.sin_port), local_port_);

    // STUN echo: header, then [ext ip 4][ext port 2]
    std::vector<u8> echo(header.begin(), header.end());
    const std::array<u8, 6> body{
        0x7F, 0, 0, 1, static_cast<u8>(local_port_ >> 8), static_cast<u8>(local_port_)};
    echo.insert(echo.end(), body.begin(), body.end());
    ASSERT_EQ(sceNetSendto(server, echo.data(), echo.size(), 0, Guest(&from), sizeof(from)),
              static_cast<int>(echo.size()));
    std::array<u8, 64> reply{};
    u32 reply_addr = 0;
    u16 reply_port = 0;
    int n = -1;
    for (int i = 0; i < 200 && n < 0; ++i) {
        n = P2PSignalingRecvFrom(reply.data(), reply.size(), &reply_addr, &reply_port);
        if (n < 0) {
            std::this_thread::sleep_for(5ms);
        }
    }
    ASSERT_EQ(n, static_cast<int>(body.size()));
    EXPECT_TRUE(std::equal(body.begin(), body.end(), reply.begin()));
    EXPECT_EQ(reply_port, addr.sin_port);
    sceNetSocketClose(server);
}

TEST_F(NetLibP2P, DatagramWakesEpollAndReportsVport) {
    const OrbisNetId s = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    ASSERT_GT(s, 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_TYPE),
              static_cast<int>(ORBIS_NET_SOCK_DGRAM_P2P));
    auto bind_addr = Loopback(local_port_, 3001);
    ASSERT_EQ(sceNetBind(s, Guest(&bind_addr), sizeof(bind_addr)), ORBIS_OK);

    OrbisNetSockaddrIn name{};
    u32 len = sizeof(name);
    ASSERT_EQ(sceNetGetsockname(s, Guest(&name), &len), ORBIS_OK);
    EXPECT_EQ(sceNetNtohs(name.sin_port), local_port_);
    EXPECT_EQ(sceNetNtohs(name.sin_vport), 3001);

    const OrbisNetId ep = sceNetEpollCreate("p2p", 0);
    OrbisNetEpollEvent ev{};
    ev.events = ORBIS_NET_EPOLLIN;
    ev.data.data_u64 = 77;
    sceNetEpollControl(ep, ORBIS_NET_EPOLL_CTL_ADD, s, &ev);

    auto remote = remote_->CreateDatagram();
    remote->Bind(4001);
    std::thread sender([&] {
        std::this_thread::sleep_for(150ms);
        const u8 msg[] = "ping";
        const auto to = LocalEndpoint();
        remote->SendTo(msg, &to, 3001);
    });
    OrbisNetEpollEvent out[2]{};
    EXPECT_EQ(sceNetEpollWait(ep, out, 2, -1), 1); // infinite wait wakes on P2P data
    sender.join();
    EXPECT_EQ(out[0].data.data_u64, 77u);

    char buf[16]{};
    OrbisNetSockaddrIn from{};
    u32 from_len = sizeof(from);
    EXPECT_EQ(sceNetRecvfrom(s, buf, sizeof(buf), 0, Guest(&from), &from_len), 5);
    EXPECT_STREQ(buf, "ping");
    EXPECT_EQ(sceNetNtohs(from.sin_port), remote_->BoundPort()); // UDP port
    EXPECT_EQ(sceNetNtohs(from.sin_vport), 4001);                // virtual port

    // Reply through the guest API to the address it reported.
    EXPECT_EQ(sceNetSendto(s, "pong", 4, 0, Guest(&from), from_len), 4);
    u8 reply[8]{};
    size_t got = 0;
    for (int i = 0; i < 200 && got == 0; ++i) {
        got = remote->RecvFrom(reply, false, nullptr, nullptr).bytes;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(got, 4u);

    sceNetEpollDestroy(ep);
    sceNetSocketClose(s);
}

TEST_F(NetLibP2P, PortRangesAndDefaults) {
    const OrbisNetId dgram = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    // Reserved virtual ports cannot be bound.
    for (const u16 vport : {5353, 32768, 60000}) {
        auto addr = Loopback(local_port_, vport);
        EXPECT_EQ(sceNetBind(dgram, Guest(&addr), sizeof(addr)), ORBIS_NET_ERROR_EACCES) << vport;
    }
    // Ephemeral virtual ports come from 32768-49999.
    auto any = Loopback(local_port_, 0);
    ASSERT_EQ(sceNetBind(dgram, Guest(&any), sizeof(any)), ORBIS_OK);
    OrbisNetSockaddrIn name{};
    u32 len = sizeof(name);
    sceNetGetsockname(dgram, Guest(&name), &len);
    EXPECT_GE(sceNetNtohs(name.sin_vport), 32768);
    EXPECT_LE(sceNetNtohs(name.sin_vport), 49999);
    EXPECT_EQ(GetInt(dgram, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF), 9216);
    EXPECT_EQ(GetInt(dgram, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 40960);
    std::vector<u8> big(9217);
    auto to = Loopback(remote_->BoundPort(), 4000);
    EXPECT_EQ(sceNetSendto(dgram, big.data(), big.size(), 0, Guest(&to), sizeof(to)),
              ORBIS_NET_ERROR_EMSGSIZE);
    sceNetSocketClose(dgram);

    // Stream ports follow the TCP table.
    const OrbisNetId stream = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    auto reserved = Loopback(1000, 0);
    EXPECT_EQ(sceNetBind(stream, Guest(&reserved), sizeof(reserved)), ORBIS_NET_ERROR_EACCES);
    auto allowed = Loopback(30000, 0);
    EXPECT_EQ(sceNetBind(stream, Guest(&allowed), sizeof(allowed)), ORBIS_OK);
    EXPECT_EQ(GetInt(stream, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_SNDBUF), 32768);
    EXPECT_EQ(GetInt(stream, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF), 65536);
    sceNetSocketClose(stream);
}

TEST_F(NetLibP2P, AddressesAreCheckedStrictly) {
    const OrbisNetId dgram = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    auto to = Loopback(remote_->BoundPort(), 4300);
    // Connecting needs a bound socket.
    EXPECT_EQ(sceNetConnect(dgram, Guest(&to), sizeof(to)), ORBIS_NET_ERROR_EINVAL);

    // Bind: exactly 16 bytes; the UDP port named; no multicast address. sa_len may be 0, as
    // games leave it.
    auto addr = Loopback(local_port_, 3300);
    EXPECT_EQ(sceNetBind(dgram, Guest(&addr), 32), ORBIS_NET_ERROR_EINVAL);
    addr.sin_family = ORBIS_NET_AF_INET6;
    EXPECT_EQ(sceNetBind(dgram, Guest(&addr), sizeof(addr)), ORBIS_NET_ERROR_EAFNOSUPPORT);
    addr.sin_family = ORBIS_NET_AF_INET;
    auto no_udp_port = Loopback(0, 3300);
    EXPECT_EQ(sceNetBind(dgram, Guest(&no_udp_port), sizeof(no_udp_port)),
              ORBIS_NET_ERROR_EADDRNOTAVAIL);
    auto multicast = Loopback(local_port_, 3300);
    multicast.sin_addr = sceNetHtonl(0xE0000001); // 224.0.0.1
    EXPECT_EQ(sceNetBind(dgram, Guest(&multicast), sizeof(multicast)),
              ORBIS_NET_ERROR_EADDRNOTAVAIL);
    addr.sin_len = 0;
    ASSERT_EQ(sceNetBind(dgram, Guest(&addr), sizeof(addr)), ORBIS_OK);
    EXPECT_EQ(sceNetBind(dgram, Guest(&addr), sizeof(addr)), ORBIS_NET_ERROR_EINVAL); // once

    // Sendto and connect need an address and both ports.
    auto no_vport = Loopback(remote_->BoundPort(), 0);
    EXPECT_EQ(sceNetSendto(dgram, "x", 1, 0, Guest(&no_vport), sizeof(no_vport)),
              ORBIS_NET_ERROR_EADDRNOTAVAIL); // "0 was set to ... sin_vport"
    EXPECT_EQ(sceNetSendto(dgram, "x", 1, 0, Guest(&to), 20), ORBIS_NET_ERROR_EINVAL);
    EXPECT_EQ(sceNetConnect(dgram, Guest(&no_vport), sizeof(no_vport)),
              ORBIS_NET_ERROR_EADDRNOTAVAIL);
    ASSERT_EQ(sceNetConnect(dgram, Guest(&to), sizeof(to)), ORBIS_OK);
    EXPECT_EQ(sceNetConnect(dgram, Guest(&to), sizeof(to)), ORBIS_NET_ERROR_EISCONN);
    sceNetSocketClose(dgram);

    // Streams: listen needs a bind (connect binds implicitly, see
    // StreamConnectBindsImplicitly); the UDP port is 0 or the P2P port.
    const OrbisNetId stream = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    EXPECT_EQ(sceNetListen(stream, 4), ORBIS_NET_ERROR_EINVAL);
    auto peer = Loopback(5000, remote_->BoundPort());
    auto odd_udp_port = Loopback(30100, 1234);
    EXPECT_EQ(sceNetBind(stream, Guest(&odd_udp_port), sizeof(odd_udp_port)),
              ORBIS_NET_ERROR_EINVAL);
    auto well_known = Loopback(30100, 3658);
    ASSERT_EQ(sceNetBind(stream, Guest(&well_known), sizeof(well_known)), ORBIS_OK);
    auto nowhere = Loopback(0, 0);
    nowhere.sin_addr = 0;
    EXPECT_EQ(sceNetConnect(stream, Guest(&nowhere), sizeof(nowhere)),
              ORBIS_NET_ERROR_EADDRNOTAVAIL);
    // Bound to a non-zero TCP port: a listener, which cannot connect (EPROTO).
    EXPECT_EQ(sceNetConnect(stream, Guest(&peer), sizeof(peer)),
              ORBIS_NET_ERROR_BASE | ORBIS_NET_EPROTO);
    EXPECT_EQ(sceNetListen(stream, 4), ORBIS_OK);
    EXPECT_EQ(sceNetConnect(stream, Guest(&peer), sizeof(peer)), ORBIS_NET_ERROR_EOPNOTSUPP);
    sceNetSocketClose(stream);
    // Datagram P2P sockets cannot listen or accept.
    const OrbisNetId dgram2 = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    EXPECT_EQ(sceNetAccept(dgram2, nullptr, nullptr), ORBIS_NET_ERROR_EOPNOTSUPP);
    sceNetSocketClose(dgram2);
}

TEST_F(NetLibP2P, ConnectTimeout) {
    // The peer is a plain UDP socket that drops everything: the SYN is never answered.
    const OrbisNetId sink = sceNetSocket("sink", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    auto any = Loopback(0);
    sceNetBind(sink, Guest(&any), sizeof(any));
    OrbisNetSockaddrIn sink_addr{};
    u32 len = sizeof(sink_addr);
    sceNetGetsockname(sink, Guest(&sink_addr), &len);

    const OrbisNetId s = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    auto self = Loopback(0, 0);
    ASSERT_EQ(sceNetBind(s, Guest(&self), sizeof(self)), ORBIS_OK);
    ASSERT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_CONNECTTIMEO, 200000), ORBIS_OK);
    auto to = Loopback(5000, sceNetNtohs(sink_addr.sin_port));
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(sceNetConnect(s, Guest(&to), sizeof(to)), ORBIS_NET_ERROR_EWOULDBLOCK);
    EXPECT_GE(Millis(start), 190.0);
    EXPECT_LT(Millis(start), 2000.0);
    sceNetSocketClose(s);
    sceNetSocketClose(sink);
}

TEST_F(NetLibP2P, IoctlInstallsKeysAndCommunicationId) {
    const OrbisNetId p2p = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    std::array<u8, 36> record{};
    for (u8 i = 0; i < 16; ++i) {
        record[i] = i;
    }
    ASSERT_EQ(sceNetIoctl(p2p, 0x801050d8, record.data()), ORBIS_OK);
    EXPECT_EQ(GetP2PKeyring()->GetCommunicationId(),
              (P2P::CommunicationId{0x54, 0x33, 0x12, 0x2f}));
    ASSERT_EQ(sceNetIoctl(p2p, 0x200050d9, nullptr), ORBIS_OK); // clear it again
    EXPECT_FALSE(GetP2PKeyring()->GetCommunicationId());

    u16 np_port = 0;
    ASSERT_EQ(sceNetIoctl(p2p, 0xc00250cf, &np_port), ORBIS_OK);
    EXPECT_EQ(sceNetNtohs(np_port), local_port_);

    EXPECT_EQ(sceNetIoctl(p2p, 0x802450ff, record.data()), ORBIS_NET_ERROR_EINVAL);
    const OrbisNetId udp = sceNetSocket("udp", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    EXPECT_EQ(sceNetIoctl(udp, 0x801050d8, record.data()), ORBIS_NET_ERROR_EINVAL);
    sceNetSocketClose(udp);
    sceNetSocketClose(p2p);
}

TEST_F(NetLibP2P, PerMessageEncryption) {
    const OrbisNetId s = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    auto bind_addr = Loopback(local_port_, 3200);
    sceNetBind(s, Guest(&bind_addr), sizeof(bind_addr));
    auto remote = remote_->CreateDatagram();
    remote->Bind(4200);
    auto to = Loopback(remote_->BoundPort(), 4200);
    const auto received = [&] {
        u8 buf[16]{};
        for (int i = 0; i < 40; ++i) {
            if (const auto r = remote->RecvFrom(buf, false, nullptr, nullptr); r.bytes != 0) {
                return std::string(reinterpret_cast<char*>(buf), r.bytes);
            }
            std::this_thread::sleep_for(5ms);
        }
        return std::string();
    };

    P2P::P2PKey key;
    key.value.fill(0x6b);
    local_keys_->SetWildcardKey(key);
    // MSG_USECRYPTO on this message only: a receiver without the key drops it ...
    EXPECT_EQ(sceNetSendto(s, "secret", 6, ORBIS_NET_MSG_USECRYPTO, Guest(&to), sizeof(to)), 6);
    EXPECT_EQ(received(), "");
    // ... one with the key decrypts it.
    remote_keys_->SetWildcardKey(key);
    EXPECT_EQ(sceNetSendto(s, "secret", 6, ORBIS_NET_MSG_USECRYPTO, Guest(&to), sizeof(to)), 6);
    EXPECT_EQ(received(), "secret");
    // Without the flag the same socket sends in the clear, which the receiver also accepts.
    EXPECT_EQ(sceNetSendto(s, "plain", 5, 0, Guest(&to), sizeof(to)), 5);
    EXPECT_EQ(received(), "plain");

    local_keys_->SetWildcardKey(std::nullopt);
    remote_keys_->SetWildcardKey(std::nullopt);
    sceNetSocketClose(s);
}

TEST_F(NetLibP2P, NpChannelsAreKeptApart) {
    const u32 loopback = sceNetHtonl(0x7F000001);
    const u16 port = sceNetHtons(local_port_);
    const auto packet = [](u8 type) { return std::array<u8, 6>{'S', 'H', 'A', 'D', type, 0x42}; };
    const auto control = packet(0x10);
    const auto matching2 = packet(0x21);
    const auto handshake = packet(0x07);
    const std::array<u8, 3> other{'x', 'y', 'z'};
    ASSERT_EQ(P2PControlSendTo(control.data(), 6, loopback, port), 6);
    ASSERT_EQ(P2PMatching2SendTo(matching2.data(), 6, loopback, port), 6);
    ASSERT_EQ(P2PSignalingSendTo(handshake.data(), 6, loopback, port), 6);
    ASSERT_EQ(P2PSignalingSendTo(other.data(), 3, loopback, port), 3);
    std::this_thread::sleep_for(100ms);

    u8 buf[16]{};
    u32 from_addr = 0;
    u16 from_port = 0;
    ASSERT_EQ(P2PControlRecvFrom(buf, sizeof(buf), &from_addr, &from_port), 6);
    EXPECT_EQ(buf[4], 0x10);
    EXPECT_EQ(from_addr, loopback);
    EXPECT_EQ(from_port, port);
    EXPECT_EQ(P2PControlRecvFrom(buf, sizeof(buf), nullptr, nullptr), -1); // nothing else
    ASSERT_EQ(P2PMatching2RecvFrom(buf, sizeof(buf), nullptr, nullptr), 6);
    EXPECT_EQ(buf[4], 0x21);
    ASSERT_EQ(P2PSignalingRecvFrom(buf, sizeof(buf), nullptr, nullptr), 6);
    EXPECT_EQ(buf[4], 0x07);
    ASSERT_EQ(P2PSignalingRecvFrom(buf, sizeof(buf), nullptr, nullptr), 3);
    EXPECT_EQ(P2PSignalingRecvFrom(buf, sizeof(buf), nullptr, nullptr), -1);

    EXPECT_TRUE(EnsureP2PTransport());
    EXPECT_EQ(GetP2PConfiguredPort(), 0); // what the test configured
    EXPECT_EQ(GetP2PAdvertisedAddr(), 0u);
}

// JoJo: Eyes of Heaven connects STREAM_P2P sockets without binding them, which works on the
// PS4. Connect binds them to a free port, as BSD TCP does.
TEST_F(NetLibP2P, StreamConnectBindsImplicitly) {
    auto listener = remote_->CreateStream();
    ASSERT_EQ(listener->Bind(5100), Host::Error::Ok);
    ASSERT_EQ(listener->Listen(4), Host::Error::Ok);

    const OrbisNetId s = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    ASSERT_GT(s, 0);
    auto to = Loopback(5100, remote_->BoundPort());
    ASSERT_EQ(sceNetConnect(s, Guest(&to), sizeof(to)), ORBIS_OK); // no bind first

    OrbisNetSockaddrIn self{};
    u32 len = sizeof(self);
    ASSERT_EQ(sceNetGetsockname(s, Guest(&self), &len), ORBIS_OK);
    EXPECT_GE(sceNetNtohs(self.sin_port), 49152); // ephemeral, as with an explicit bind to 0
    EXPECT_EQ(sceNetNtohs(self.sin_vport), local_port_);

    std::shared_ptr<P2P::StreamSocket> server;
    Host::Error error;
    for (int i = 0; i < 400 && !server; ++i) {
        server = listener->Accept(nullptr, nullptr, &error);
        std::this_thread::sleep_for(5ms);
    }
    ASSERT_TRUE(server);
    ASSERT_EQ(sceNetSend(s, "jojo", 4, 0), 4);
    std::array<u8, 8> got{};
    size_t n = 0;
    for (int i = 0; i < 400 && n == 0; ++i) {
        n = server->Recv(std::span<u8>(got), false).bytes;
        if (n == 0) {
            std::this_thread::sleep_for(5ms);
        }
    }
    EXPECT_EQ(n, 4u);
    EXPECT_EQ(std::memcmp(got.data(), "jojo", 4), 0);
    sceNetSocketClose(s);

    // Datagram P2P sockets still have to be bound first.
    const OrbisNetId dgram = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    auto dgram_to = Loopback(remote_->BoundPort(), 4300);
    EXPECT_EQ(sceNetConnect(dgram, Guest(&dgram_to), sizeof(dgram_to)), ORBIS_NET_ERROR_EINVAL);
    sceNetSocketClose(dgram);
}

TEST_F(NetLibP2P, LingerIsKeptWholeOnP2PStreams) {
    const OrbisNetId s = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    ASSERT_GT(s, 0);
    const OrbisNetLinger set{1, 7};
    ASSERT_EQ(sceNetSetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &set, sizeof(set)),
              ORBIS_OK);
    OrbisNetLinger got{};
    u32 len = sizeof(got);
    ASSERT_EQ(sceNetGetsockopt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER, &got, &len), ORBIS_OK);
    EXPECT_EQ(len, sizeof(got)); // the struct, not an int
    EXPECT_EQ(got.l_onoff, 1);
    EXPECT_EQ(got.l_linger, 7);
    sceNetSocketClose(s);
}

TEST_F(NetLibP2P, StreamConnectSendRecv) {
    auto listener = remote_->CreateStream();
    ASSERT_EQ(listener->Bind(5000), Host::Error::Ok);
    ASSERT_EQ(listener->Listen(4), Host::Error::Ok);

    const OrbisNetId s = sceNetSocket("p2ps", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_TYPE),
              static_cast<int>(ORBIS_NET_SOCK_STREAM_P2P));
    auto self_addr = Loopback(0, 0); // any TCP port, the P2P UDP port
    ASSERT_EQ(sceNetBind(s, Guest(&self_addr), sizeof(self_addr)), ORBIS_OK);
    // Stream layout: sin_port = the TCP port, sin_vport = the peer's UDP port.
    auto to = Loopback(5000, remote_->BoundPort());
    ASSERT_EQ(sceNetConnect(s, Guest(&to), sizeof(to)), ORBIS_OK); // blocking handshake

    OrbisNetSockaddrIn peer{};
    u32 len = sizeof(peer);
    ASSERT_EQ(sceNetGetpeername(s, Guest(&peer), &len), ORBIS_OK);
    EXPECT_EQ(sceNetNtohs(peer.sin_port), 5000);
    EXPECT_EQ(sceNetNtohs(peer.sin_vport), remote_->BoundPort());
    EXPECT_EQ(peer.sin_addr, sceNetHtonl(0x7F000001));

    OrbisNetSockaddrIn self{};
    len = sizeof(self);
    ASSERT_EQ(sceNetGetsockname(s, Guest(&self), &len), ORBIS_OK);
    EXPECT_GE(sceNetNtohs(self.sin_port), 49152); // ephemeral TCP-over-UDPP2P port
    EXPECT_EQ(sceNetNtohs(self.sin_vport), local_port_);

    std::shared_ptr<P2P::StreamSocket> server;
    Host::Error error;
    for (int i = 0; i < 400 && !server; ++i) {
        server = listener->Accept(nullptr, nullptr, &error);
        std::this_thread::sleep_for(5ms);
    }
    ASSERT_TRUE(server);

    std::vector<u8> data(200 * 1024);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<u8>(i * 31);
    }
    // A blocking send larger than both buffers completes only while the peer reads, so it
    // runs on its own thread, as a game's network thread would.
    s32 sent = 0;
    std::thread sender([&] { sent = sceNetSend(s, data.data(), data.size(), 0); });
    std::vector<u8> got;
    u8 buf[8192];
    for (int i = 0; i < 2000 && got.size() < data.size(); ++i) {
        const auto r = server->Recv(buf, false);
        got.insert(got.end(), buf, buf + r.bytes);
        if (r.bytes == 0) {
            std::this_thread::sleep_for(1ms);
        }
    }
    sender.join();
    EXPECT_EQ(sent, static_cast<s32>(data.size()));
    EXPECT_TRUE(got == data) << "received " << got.size();

    server->Send(std::span<const u8>(reinterpret_cast<const u8*>("reply"), 5));
    char reply[8]{};
    EXPECT_EQ(sceNetRecv(s, reply, sizeof(reply), 0), 5); // blocking recv
    EXPECT_STREQ(reply, "reply");

    sceNetSocketClose(s);
    listener->Close();
}

TEST_F(NetLibP2P, ProtectionOptions) {
    const OrbisNetId s = sceNetSocket("p2p", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO), 0);
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO, 1), ORBIS_OK);
    EXPECT_EQ(SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USESIGNATURE, 1), ORBIS_OK);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO), 1);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO, 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO), 0);
    EXPECT_EQ(GetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USESIGNATURE), 1); // independent

    // Encrypted + signed end to end once both sides hold the key.
    P2P::P2PKey key;
    key.value.fill(0x33);
    local_keys_->SetWildcardKey(key);
    remote_keys_->SetWildcardKey(key);
    SetInt(s, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_USECRYPTO, 1);
    auto bind_addr = Loopback(local_port_, 3100);
    sceNetBind(s, Guest(&bind_addr), sizeof(bind_addr));
    auto remote = remote_->CreateDatagram();
    remote->Bind(4100);
    auto to = Loopback(remote_->BoundPort(), 4100);
    EXPECT_EQ(sceNetSendto(s, "secret", 6, 0, Guest(&to), sizeof(to)), 6);
    u8 buf[16]{};
    size_t got = 0;
    for (int i = 0; i < 200 && got == 0; ++i) {
        got = remote->RecvFrom(buf, false, nullptr, nullptr).bytes;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(got, 6u);
    EXPECT_EQ(std::memcmp(buf, "secret", 6), 0);
    local_keys_->SetWildcardKey(std::nullopt);
    remote_keys_->SetWildcardKey(std::nullopt);
    sceNetSocketClose(s);
}

} // namespace
