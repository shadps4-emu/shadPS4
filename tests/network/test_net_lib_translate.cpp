// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The guest <-> host translation used by the new libSceNet (src/core/libraries/net): pure
// functions, tested without sockets.

#include <cstddef>
#include <cstring>

#include <gtest/gtest.h>

#ifndef _WIN32
#include <sys/un.h>
#else
#include <winsock2.h>
#include <afunix.h>
#endif

#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_translate.h"

#ifndef _WIN32
#include <netinet/tcp.h>
#endif

using namespace Libraries::Net;

namespace {

OrbisNetSockaddrIn GuestIn(const char* ip, u16 port, u16 vport = 0, u8 sa_len = 16) {
    OrbisNetSockaddrIn in{};
    in.sin_len = sa_len;
    in.sin_family = ORBIS_NET_AF_INET;
    in.sin_port = htons(port);
    inet_pton(AF_INET, ip, &in.sin_addr);
    in.sin_vport = htons(vport);
    return in;
}

const OrbisNetSockaddr* AsGuest(const void* p) {
    return static_cast<const OrbisNetSockaddr*>(p);
}

TEST(NetLibTranslate, SocketKinds) {
    int error = 0;
    auto k = ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0, &error);
    ASSERT_TRUE(k);
    EXPECT_EQ(k->family, AF_INET);
    EXPECT_EQ(k->type, SOCK_STREAM);
    EXPECT_FALSE(k->p2p);

    k = ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, ORBIS_NET_IPPROTO_UDP, &error);
    ASSERT_TRUE(k);
    EXPECT_EQ(k->type, SOCK_DGRAM);
    EXPECT_EQ(k->protocol, IPPROTO_UDP);

    // IPv4-only stack: no IPv6 sockets, P2P included.
    for (const int type : {ORBIS_NET_SOCK_STREAM, ORBIS_NET_SOCK_DGRAM, ORBIS_NET_SOCK_DGRAM_P2P}) {
        error = 0;
        EXPECT_FALSE(ToHostSocketKind(ORBIS_NET_AF_INET6, type, 0, &error));
        EXPECT_EQ(error, ORBIS_NET_EPROTONOSUPPORT); // sceNetSocket's code for a bad family
    }

    k = ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM_P2P, 0, &error);
    ASSERT_TRUE(k);
    EXPECT_TRUE(k->p2p);
    EXPECT_EQ(k->type, SOCK_STREAM);
    k = ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0, &error);
    ASSERT_TRUE(k);
    EXPECT_TRUE(k->p2p);
    EXPECT_EQ(k->type, SOCK_DGRAM);

    // AF_UNIX: stream and datagram, protocol 0, never P2P.
    k = ToHostSocketKind(ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_DGRAM, 0, &error);
    ASSERT_TRUE(k);
    EXPECT_EQ(k->family, AF_UNIX);
    EXPECT_EQ(k->type, SOCK_DGRAM);
    EXPECT_EQ(k->protocol, 0);
    EXPECT_FALSE(ToHostSocketKind(ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_DGRAM_P2P, 0, &error));
    EXPECT_EQ(error, ORBIS_NET_EPROTONOSUPPORT);

    EXPECT_FALSE(ToHostSocketKind(99, ORBIS_NET_SOCK_STREAM, 0, &error));
    EXPECT_EQ(error, ORBIS_NET_EPROTONOSUPPORT);
    // Non-RAW types take 0 or their own protocol; anything else is EPROTOTYPE.
    EXPECT_FALSE(
        ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, ORBIS_NET_IPPROTO_TCP, &error));
    EXPECT_EQ(error, ORBIS_NET_EPROTOTYPE);
    EXPECT_FALSE(ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P,
                                  ORBIS_NET_IPPROTO_UDP, &error));
    EXPECT_EQ(error, ORBIS_NET_EPROTOTYPE);
    EXPECT_TRUE(
        ToHostSocketKind(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_RAW, ORBIS_NET_IPPROTO_ICMP, &error));
    EXPECT_FALSE(ToHostSocketKind(ORBIS_NET_AF_INET, 77, 0, &error));
    EXPECT_EQ(error, ORBIS_NET_EPROTONOSUPPORT);

    EXPECT_EQ(ToOrbisSocketType(SOCK_STREAM, false), static_cast<int>(ORBIS_NET_SOCK_STREAM));
    EXPECT_EQ(ToOrbisSocketType(SOCK_DGRAM, true), static_cast<int>(ORBIS_NET_SOCK_DGRAM_P2P));
    EXPECT_EQ(ToOrbisSocketType(SOCK_STREAM, true), static_cast<int>(ORBIS_NET_SOCK_STREAM_P2P));
    EXPECT_EQ(ToOrbisFamily(AF_INET6), static_cast<int>(ORBIS_NET_AF_INET6));
}

TEST(NetLibTranslate, Ipv4SockaddrBothWays) {
    // sa_len is not trusted: games often leave it 0.
    for (const u8 sa_len : {u8{16}, u8{0}}) {
        const auto guest = GuestIn("192.0.2.7", 3658, 0, sa_len);
        sockaddr_storage host{};
        socklen_t len = 0;
        ASSERT_EQ(ToHostSockaddr(AsGuest(&guest), sizeof(guest), &host, &len), 0);
        ASSERT_EQ(len, static_cast<socklen_t>(sizeof(sockaddr_in)));
        const auto* in = reinterpret_cast<const sockaddr_in*>(&host);
        EXPECT_EQ(in->sin_family, AF_INET);
        EXPECT_EQ(ntohs(in->sin_port), 3658);
        char text[32];
        inet_ntop(AF_INET, &in->sin_addr, text, sizeof(text));
        EXPECT_STREQ(text, "192.0.2.7");
    }

    sockaddr_in host{};
    host.sin_family = AF_INET;
    host.sin_port = htons(80);
    inet_pton(AF_INET, "10.1.2.3", &host.sin_addr);
    OrbisNetSockaddrIn back{};
    u32 len = sizeof(back);
    ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), sizeof(host),
                    reinterpret_cast<OrbisNetSockaddr*>(&back), &len);
    EXPECT_EQ(len, 16u);
    EXPECT_EQ(back.sin_len, 16);
    EXPECT_EQ(back.sin_family, ORBIS_NET_AF_INET);
    EXPECT_EQ(back.sin_port, htons(80));
    EXPECT_EQ(back.sin_vport, 0);
}

TEST(NetLibTranslate, Ipv6Sockaddr) {
    OrbisNetSockaddrIn6 guest{};
    guest.sin6_len = sizeof(guest);
    guest.sin6_family = ORBIS_NET_AF_INET6;
    guest.sin6_port = htons(443);
    guest.sin6_scope_id = 3;
    inet_pton(AF_INET6, "2001:db8::1", guest.sin6_addr);
    sockaddr_storage host{};
    socklen_t len = 0;
    ASSERT_EQ(ToHostSockaddr(AsGuest(&guest), sizeof(guest), &host, &len), 0);
    const auto* in6 = reinterpret_cast<const sockaddr_in6*>(&host);
    EXPECT_EQ(in6->sin6_family, AF_INET6);
    EXPECT_EQ(ntohs(in6->sin6_port), 443);
    EXPECT_EQ(in6->sin6_scope_id, 3u);

    OrbisNetSockaddrIn6 back{};
    u32 back_len = sizeof(back);
    ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), len,
                    reinterpret_cast<OrbisNetSockaddr*>(&back), &back_len);
    EXPECT_EQ(back_len, 28u);
    EXPECT_EQ(back.sin6_len, 28);
    EXPECT_EQ(back.sin6_family, ORBIS_NET_AF_INET6);
    EXPECT_EQ(std::memcmp(back.sin6_addr, guest.sin6_addr, 16), 0);
}

TEST(NetLibTranslate, SockaddrErrorsAndTruncation) {
    sockaddr_storage host{};
    socklen_t len = 0;
    EXPECT_EQ(ToHostSockaddr(nullptr, 16, &host, &len), ORBIS_NET_EFAULT);
    const auto guest = GuestIn("1.2.3.4", 1);
    EXPECT_EQ(ToHostSockaddr(AsGuest(&guest), 8, &host, &len), ORBIS_NET_EINVAL); // too short
    auto unknown = guest;
    unknown.sin_family = 99;
    EXPECT_EQ(ToHostSockaddr(AsGuest(&unknown), 16, &host, &len), ORBIS_NET_EAFNOSUPPORT);
    sockaddr_in in{};
    in.sin_family = AF_INET;
    in.sin_port = htons(0x1234);
    u8 buffer[4]{};
    u32 cap = sizeof(buffer);
    ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&in), sizeof(in),
                    reinterpret_cast<OrbisNetSockaddr*>(buffer), &cap);
    EXPECT_EQ(cap, 16u);
    EXPECT_EQ(buffer[0], 16);
    EXPECT_EQ(buffer[1], ORBIS_NET_AF_INET);
    EXPECT_EQ(buffer[2], 0x12);
    EXPECT_EQ(buffer[3], 0x34);
}

TEST(NetLibTranslate, UnixAddresses) {
    OrbisNetSockaddrUn guest{};
    guest.sun_len = sizeof(guest);
    guest.sun_family = ORBIS_NET_AF_UNIX;
    std::strcpy(guest.sun_path, "/tmp/shadps4.sock");
    sockaddr_storage host{};
    socklen_t len = 0;
    ASSERT_EQ(
        ToHostSockaddr(reinterpret_cast<OrbisNetSockaddr*>(&guest), sizeof(guest), &host, &len), 0);
    const auto* un = reinterpret_cast<const sockaddr_un*>(&host);
    EXPECT_EQ(un->sun_family, AF_UNIX);
    EXPECT_STREQ(un->sun_path, "/tmp/shadps4.sock");
    EXPECT_EQ(len, offsetof(sockaddr_un, sun_path) + std::strlen("/tmp/shadps4.sock") + 1);

    OrbisNetSockaddrUn back{};
    u32 back_len = sizeof(back);
    ToOrbisSockaddr(reinterpret_cast<const sockaddr*>(&host), len,
                    reinterpret_cast<OrbisNetSockaddr*>(&back), &back_len);
    EXPECT_EQ(back_len, sizeof(OrbisNetSockaddrUn));
    EXPECT_EQ(back.sun_family, ORBIS_NET_AF_UNIX);
    EXPECT_STREQ(back.sun_path, "/tmp/shadps4.sock");

    // A path that does not fit the host's sun_path.
    std::memset(guest.sun_path, 'a', sizeof(guest.sun_path));
    EXPECT_EQ(
        ToHostSockaddr(reinterpret_cast<OrbisNetSockaddr*>(&guest), sizeof(guest), &host, &len),
        sizeof(guest.sun_path) >= sizeof(un->sun_path) ? ORBIS_NET_ENAMETOOLONG : 0);
}

TEST(NetLibTranslate, P2PDatagramAddresses) {
    // sin_port = the peer's UDP port, sin_vport = the virtual port.
    const auto guest = GuestIn("198.51.100.4", 3658, 5000);
    Core::Net::P2P::Endpoint endpoint;
    u16 vport = 0;
    ASSERT_EQ(ToP2PAddress(AsGuest(&guest), sizeof(guest), P2PKind::Datagram, &endpoint, &vport),
              0);
    EXPECT_EQ(endpoint, Core::Net::P2P::Endpoint::IPv4("198.51.100.4", 3658));
    EXPECT_EQ(vport, 5000); // host byte order

    OrbisNetSockaddrIn back{};
    u32 len = sizeof(back);
    ToOrbisP2PAddress(endpoint, vport, P2PKind::Datagram,
                      reinterpret_cast<OrbisNetSockaddr*>(&back), &len);
    EXPECT_EQ(len, 16u);
    EXPECT_EQ(std::memcmp(&back, &guest, sizeof(back)), 0); // exact round trip
}

TEST(NetLibTranslate, P2PStreamAddresses) {
    // sin_port = the TCP port inside the encapsulation, sin_vport = the peer's UDP port.
    const auto guest = GuestIn("198.51.100.4", 49200, 3659);
    Core::Net::P2P::Endpoint endpoint;
    u16 tcp_port = 0;
    ASSERT_EQ(ToP2PAddress(AsGuest(&guest), sizeof(guest), P2PKind::Stream, &endpoint, &tcp_port),
              0);
    EXPECT_EQ(endpoint, Core::Net::P2P::Endpoint::IPv4("198.51.100.4", 3659));
    EXPECT_EQ(tcp_port, 49200);

    OrbisNetSockaddrIn back{};
    u32 len = sizeof(back);
    ToOrbisP2PAddress(endpoint, tcp_port, P2PKind::Stream,
                      reinterpret_cast<OrbisNetSockaddr*>(&back), &len);
    EXPECT_EQ(std::memcmp(&back, &guest, sizeof(back)), 0);

    // No UDP port given: the standard PlayStation P2P port.
    const auto no_udp = GuestIn("198.51.100.4", 49200, 0);
    ASSERT_EQ(ToP2PAddress(AsGuest(&no_udp), sizeof(no_udp), P2PKind::Stream, &endpoint, &tcp_port),
              0);
    EXPECT_EQ(endpoint.Port(), DefaultP2PUdpPort);
}

TEST(NetLibTranslate, P2PAddressesAreIPv4Only) {
    auto guest = GuestIn("198.51.100.4", 3658, 5000);
    Core::Net::P2P::Endpoint endpoint;
    u16 port = 0;
    // Exactly 16 bytes: not shorter, and not the 32-byte IPv6 P2P sockaddr either.
    EXPECT_EQ(ToP2PAddress(AsGuest(&guest), 15, P2PKind::Datagram, &endpoint, &port),
              ORBIS_NET_EINVAL);
    EXPECT_EQ(ToP2PAddress(AsGuest(&guest), 32, P2PKind::Datagram, &endpoint, &port),
              ORBIS_NET_EINVAL);
    guest.sin_family = ORBIS_NET_AF_INET6;
    EXPECT_EQ(ToP2PAddress(AsGuest(&guest), sizeof(guest), P2PKind::Stream, &endpoint, &port),
              ORBIS_NET_EAFNOSUPPORT);
    EXPECT_EQ(ToP2PAddress(nullptr, sizeof(guest), P2PKind::Stream, &endpoint, &port),
              ORBIS_NET_EFAULT);
}

TEST(NetLibTranslate, MessageFlags) {
    auto f = ToHostMsgFlags(ORBIS_NET_MSG_PEEK | ORBIS_NET_MSG_DONTWAIT);
    EXPECT_TRUE(f.peek);
    EXPECT_TRUE(f.dontwait);
    EXPECT_EQ(f.host, MSG_PEEK); // DONTWAIT is the guest layer's business
    EXPECT_EQ(f.unsupported, 0);

    f = ToHostMsgFlags(ORBIS_NET_MSG_WAITALL | 0x20000 /* MSG_NOSIGNAL */);
    EXPECT_EQ(f.host, MSG_WAITALL); // 0x40 on the PS4, 0x100 on Linux, 0x8 on Windows
    EXPECT_EQ(f.unsupported, 0);

    f = ToHostMsgFlags(ORBIS_NET_MSG_USECRYPTO | ORBIS_NET_MSG_PEEKLEN);
    EXPECT_TRUE(f.peek);
    EXPECT_TRUE(f.peeklen);
    EXPECT_TRUE(f.crypto); // per-message protection is understood now
    EXPECT_FALSE(f.signature);
    EXPECT_EQ(f.unsupported, 0);
    EXPECT_TRUE(ToHostMsgFlags(ORBIS_NET_MSG_WAITALL).waitall);
    EXPECT_EQ(ToHostMsgFlags(0x1 /* MSG_OOB */).unsupported, 0x1);
}

TEST(NetLibTranslate, OutOfBandIsNotPassedThrough) {
    // The PS4's TCP has no urgent data: MSG_OOB is reported, not handed to the host.
    const auto f = ToHostMsgFlags(0x1 | ORBIS_NET_MSG_DONTWAIT);
    EXPECT_EQ(f.host & MSG_OOB, 0);
    EXPECT_EQ(f.unsupported, 0x1);
    EXPECT_TRUE(f.dontwait);
}

TEST(NetLibTranslate, ReservedPorts) {
    // UDP, TCP and TCP over UDPP2P ports.
    for (const u16 port : {1, 80, 1023, 5353, 8540, 8579, 9293, 9310, 40000, 49152, 65535}) {
        EXPECT_TRUE(IsReservedPort(port)) << port;
    }
    for (const u16 port : {0, 1024, 3658, 5352, 5354, 8539, 8580, 9292, 9311, 39999}) {
        EXPECT_FALSE(IsReservedPort(port)) << port;
    }
    // UDPP2P virtual ports.
    for (const u16 vport : {5353, 32768, 49999, 65535}) {
        EXPECT_TRUE(IsReservedP2PVport(vport)) << vport;
    }
    for (const u16 vport : {0, 1, 80, 3658, 32767}) {
        EXPECT_FALSE(IsReservedP2PVport(vport)) << vport;
    }
}

TEST(NetLibTranslate, DefaultBufferSizes) {
    EXPECT_EQ(DefaultSendBuffer(SOCK_STREAM), 32768);
    EXPECT_EQ(DefaultReceiveBuffer(SOCK_STREAM), 65536);
    EXPECT_EQ(DefaultSendBuffer(SOCK_DGRAM), 9216);
    EXPECT_EQ(DefaultReceiveBuffer(SOCK_DGRAM), 40960);
    EXPECT_EQ(DefaultReceiveBuffer(SOCK_RAW), 40960);
}

TEST(NetLibTranslate, OptionsMapByName) {
    // Host numbering differs (on Linux IP_TTL is 2, not 4): the mapping must use host names.
    auto o = ToHostOption(ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_TTL);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->level, IPPROTO_IP);
    EXPECT_EQ(o->name, IP_TTL);
    o = ToHostOption(ORBIS_NET_IPPROTO_IP, ORBIS_NET_IP_ADD_MEMBERSHIP);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->name, IP_ADD_MEMBERSHIP);
    EXPECT_EQ(o->value, OptionValue::Raw);
    o = ToHostOption(ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVBUF);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->level, SOL_SOCKET);
    EXPECT_EQ(o->name, SO_RCVBUF);
    o = ToHostOption(ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_LINGER);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->value, OptionValue::Linger);
    o = ToHostOption(ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_NODELAY);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->name, TCP_NODELAY);

    // Guest-owned options are not host options.
    EXPECT_FALSE(ToHostOption(ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO));
    EXPECT_FALSE(ToHostOption(ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_RCVTIMEO));
    EXPECT_FALSE(ToHostOption(0x1234, 1));
}

TEST(NetLibTranslate, EpollEventsAndErrors) {
    const u32 all =
        ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLOUT | ORBIS_NET_EPOLLERR | ORBIS_NET_EPOLLHUP;
    EXPECT_EQ(ToOrbisEpollEvents(ToHostEpollEvents(all)), all);
    EXPECT_EQ(ToHostEpollEvents(ORBIS_NET_EPOLLHUP), static_cast<u32>(Host::EvHup));

    EXPECT_EQ(ToOrbisErrno(Host::Error::WouldBlock), ORBIS_NET_EWOULDBLOCK);
    EXPECT_EQ(ToOrbisErrno(Host::Error::ConnRefused), ORBIS_NET_ECONNREFUSED);
    EXPECT_EQ(ToOrbisErrno(Host::Error::NotSock), ORBIS_NET_EBADF);
    EXPECT_EQ(ToOrbisErrno(Host::Error::Intr), ORBIS_NET_EINTR);
}

} // namespace
