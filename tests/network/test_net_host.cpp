// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Native sockets through Core::Net's guest layer: blocking semantics enforced on non-blocking
// host sockets, SO_RCVTIMEO, abort, epoll and error translation.

#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "core/net/guest_net.h"

using namespace Core::Net;
using namespace std::chrono_literals;
using Host::Error;

namespace {

class NetHostTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_TRUE(Host::Initialize());
    }

    static sockaddr_in Loopback(u16 port = 0) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        return addr;
    }

    /// A listening TCP socket on an ephemeral loopback port.
    static s32 MakeListener(sockaddr_in* bound) {
        const s32 listener = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
        EXPECT_GT(listener, 0);
        auto addr = Loopback();
        EXPECT_EQ(SocketBind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
                  Error::Ok);
        EXPECT_EQ(SocketListen(listener, 4).error, Error::Ok);
        socklen_t len = sizeof(*bound);
        EXPECT_EQ(SocketGetName(listener, reinterpret_cast<sockaddr*>(bound), &len).error,
                  Error::Ok);
        return listener;
    }

    static double Millis(std::chrono::steady_clock::time_point since) {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - since)
            .count();
    }
};

TEST_F(NetHostTest, BlockingConnectAcceptSendRecv) {
    sockaddr_in addr{};
    const s32 listener = MakeListener(&addr);

    NetResult accepted{};
    std::thread server([&] { accepted = SocketAccept(listener, nullptr, nullptr); });
    const s32 client = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    ASSERT_EQ(SocketConnect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
              Error::Ok);
    server.join();
    ASSERT_EQ(accepted.error, Error::Ok);
    const s32 conn = static_cast<s32>(accepted.value);

    EXPECT_EQ(SocketSendTo(client, "hello", 5, 0, false, nullptr, 0).value, 5);
    char buf[16]{};
    EXPECT_EQ(SocketRecvFrom(conn, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 5);
    EXPECT_EQ(std::memcmp(buf, "hello", 5), 0);

    // Peer close is end-of-stream (0), not an error.
    SocketClose(client);
    EXPECT_EQ(SocketRecvFrom(conn, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 0);
    SocketClose(conn);
    SocketClose(listener);
}

TEST_F(NetHostTest, RecvTimeoutIsEagain) {
    sockaddr_in addr{};
    const s32 listener = MakeListener(&addr);
    const s32 client = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    ASSERT_EQ(SocketConnect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
              Error::Ok);

    SocketSetRecvTimeout(client, 200ms);
    char buf[4];
    const auto start = std::chrono::steady_clock::now();
    const auto r = SocketRecvFrom(client, buf, sizeof(buf), 0, false, nullptr, nullptr);
    // BSD: an expired SO_RCVTIMEO is EAGAIN, not ETIMEDOUT.
    EXPECT_EQ(r.error, Error::WouldBlock);
    EXPECT_GE(Millis(start), 190.0);
    EXPECT_LT(Millis(start), 2000.0);
    SocketClose(client);
    SocketClose(listener);
}

TEST_F(NetHostTest, AbortWakesBlockedRecv) {
    sockaddr_in addr{};
    const s32 listener = MakeListener(&addr);
    const s32 client = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    ASSERT_EQ(SocketConnect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
              Error::Ok);

    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        SocketAbort(client, kSocketAbortPreserveRecv);
    });
    char buf[4];
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(SocketRecvFrom(client, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::Intr);
    EXPECT_LT(Millis(start), 2000.0);
    aborter.join();
    SocketClose(client);
    SocketClose(listener);
}

/// A bound UDP socket plus its own address, for abort tests that need a socket to wait on.
struct UdpPair {
    s32 sock;
    sockaddr_in addr;
};

UdpPair MakeUdp() {
    UdpPair p{static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value), {}};
    p.addr.sin_family = AF_INET;
    p.addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    SocketBind(p.sock, reinterpret_cast<sockaddr*>(&p.addr), sizeof(p.addr));
    socklen_t len = sizeof(p.addr);
    SocketGetName(p.sock, reinterpret_cast<sockaddr*>(&p.addr), &len);
    return p;
}

TEST_F(NetHostTest, AbortOnlyInterruptsCurrentWaits) {
    auto [sock, addr] = MakeUdp();
    char buf[4];
    SocketSetRecvTimeout(sock, 5s); // bounds the test if the abort fired too early
    // Wake a blocked recv.
    std::thread aborter([&] {
        std::this_thread::sleep_for(150ms);
        SocketAbort(sock, 0);
    });
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::Intr);
    aborter.join();

    // The socket is still usable: data sent now is received.
    SocketSendTo(sock, "ok", 2, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 2);

    // Nothing blocked and no preservation flag: the abort has no lasting effect.
    SocketAbort(sock, 0);
    SocketSetRecvTimeout(sock, 100ms);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::WouldBlock); // timed out normally
    SocketClose(sock);
}

TEST_F(NetHostTest, PreservedAbortHitsNextBlockingCallOnce) {
    auto [sock, addr] = MakeUdp();
    char buf[4];
    SocketSetRecvTimeout(sock, 100ms);

    // Preserved for the next receive of any kind, non-blocking included (sceNetSocketAbort:
    // "the next receive or send function will return an error immediately").
    EXPECT_EQ(SocketAbort(sock, kSocketAbortPreserveRecv).error, Error::Ok);
    // Send-side calls do not consume a receive-side abort.
    EXPECT_EQ(SocketSendTo(sock, "x", 1, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
                  .value,
              1);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, true, nullptr, nullptr).error, Error::Intr);
    // ... and only that one: the blocking receive after it gets the queued datagram.
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 1);
    EXPECT_LT(Millis(start), 50.0);

    // A blocking receive consumes it the same way, even with data queued.
    SocketSendTo(sock, "z", 1, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    SocketAbort(sock, kSocketAbortPreserveRecv);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::Intr);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 1);

    // UDP sends never wait, but a preserved send abort still fails the next one.
    SocketAbort(sock, kSocketAbortPreserveSend);
    EXPECT_EQ(SocketSendTo(sock, "y", 1, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
                  .error,
              Error::Intr);
    EXPECT_EQ(SocketSendTo(sock, "y", 1, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
                  .value,
              1);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).value, 1);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::WouldBlock);

    SocketClose(sock);

    // A send-side preservation hits the next blocking send-side call (here a connect, before it
    // starts), not receives. UDP sends never wait on the PS4, so they are not affected.
    sockaddr_in listen_addr{};
    const s32 listener = MakeListener(&listen_addr);
    const s32 tcp = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    SocketAbort(tcp, kSocketAbortPreserveSend);
    EXPECT_EQ(SocketRecvFrom(tcp, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::NotConn);
    const auto* to = reinterpret_cast<sockaddr*>(&listen_addr);
    EXPECT_EQ(SocketConnect(tcp, to, sizeof(listen_addr)).error, Error::Intr);
    EXPECT_EQ(SocketConnect(tcp, to, sizeof(listen_addr)).error, Error::Ok);
    SocketClose(tcp);
    SocketClose(listener);
}

TEST_F(NetHostTest, EpollAbortPreservation) {
    const s32 ep = static_cast<s32>(EpollCreate().value);
    GuestEpollEvent events[4];

    // Nothing waiting, no flag: ENOTBLK, and no lasting effect.
    EXPECT_EQ(EpollAbort(ep, 0).error, Error::NotBlk);
    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(EpollWait(ep, events, 50'000).value, 0);
    EXPECT_GE(Millis(start), 45.0);

    // Preserved: the next wait fails at once, the one after waits normally.
    EXPECT_EQ(EpollAbort(ep, kEpollAbortPreserve).error, Error::Ok);
    EXPECT_EQ(EpollWait(ep, events, -1).error, Error::Intr);
    start = std::chrono::steady_clock::now();
    EXPECT_EQ(EpollWait(ep, events, 50'000).value, 0);
    EXPECT_GE(Millis(start), 45.0);

    // After waking a wait, the wake is drained: later waits are not woken by it.
    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        EpollAbort(ep, 0);
    });
    EXPECT_EQ(EpollWait(ep, events, 5'000'000).error, Error::Intr);
    aborter.join();
    start = std::chrono::steady_clock::now();
    EXPECT_EQ(EpollWait(ep, events, 50'000).value, 0);
    EXPECT_GE(Millis(start), 45.0);
    EpollDestroy(ep);
}

TEST_F(NetHostTest, CloseWakesBlockedAccept) {
    sockaddr_in addr{};
    const s32 listener = MakeListener(&addr);
    NetResult r{};
    std::thread t([&] { r = SocketAccept(listener, nullptr, nullptr); });
    std::this_thread::sleep_for(100ms);
    SocketClose(listener);
    t.join();
    EXPECT_EQ(r.error, Error::Intr);
}

TEST_F(NetHostTest, ConnectRefused) {
    // Bind a port, then close it, so nothing listens there.
    sockaddr_in addr{};
    SocketClose(MakeListener(&addr));
    const s32 client = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    EXPECT_EQ(SocketConnect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
              Error::ConnRefused);
    SocketClose(client);
}

TEST_F(NetHostTest, NonBlockingRecvWouldBlock) {
    const s32 sock = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    auto addr = Loopback();
    SocketBind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    char buf[4];
    // MSG_DONTWAIT on a blocking socket, then SO_NBIO.
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, true, nullptr, nullptr).error,
              Error::WouldBlock);
    SocketSetNonBlocking(sock, true);
    EXPECT_EQ(SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr).error,
              Error::WouldBlock);
    SocketClose(sock);
}

TEST_F(NetHostTest, EpollLevelTriggeredAndAbort) {
    const s32 sock = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    auto addr = Loopback();
    SocketBind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    socklen_t len = sizeof(addr);
    SocketGetName(sock, reinterpret_cast<sockaddr*>(&addr), &len);

    const s32 ep = static_cast<s32>(EpollCreate().value);
    ASSERT_EQ(EpollControl(ep, EpollOp::Add, sock, Host::EvIn, 0, 0xabcd).error, Error::Ok);
    EXPECT_EQ(EpollControl(ep, EpollOp::Add, sock, Host::EvIn, 0, 0).error, Error::Exist);

    GuestEpollEvent events[4];
    EXPECT_EQ(EpollWait(ep, events, 0).value, 0); // nothing pending

    const s32 sender = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    SocketSendTo(sender, "x", 1, 0, false, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ASSERT_EQ(EpollWait(ep, events, 2'000'000).value, 1);
    EXPECT_EQ(events[0].data, 0xabcdu);
    EXPECT_EQ(events[0].ident, sock);
    EXPECT_TRUE(events[0].events & Host::EvIn);
    EXPECT_EQ(EpollWait(ep, events, 0).value, 1); // level-triggered: still pending

    char buf[4];
    SocketRecvFrom(sock, buf, sizeof(buf), 0, false, nullptr, nullptr);
    EXPECT_EQ(EpollWait(ep, events, 0).value, 0);

    // Deleting, then closing: no stale events.
    EXPECT_EQ(EpollControl(ep, EpollOp::Delete, sock, 0, 0, 0).error, Error::Ok);
    EXPECT_EQ(EpollControl(ep, EpollOp::Delete, sock, 0, 0, 0).error, Error::NoEnt);

    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        EpollAbort(ep, kEpollAbortPreserve);
    });
    EXPECT_EQ(EpollWait(ep, events, -1).error, Error::Intr);
    aborter.join();

    SocketClose(sender);
    SocketClose(sock);
    EpollDestroy(ep);
}

TEST_F(NetHostTest, ClosedSocketLeavesEpoll) {
    const s32 sock = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    EpollControl(ep, EpollOp::Add, sock, Host::EvOut, 0, 1);
    GuestEpollEvent events[4];
    EXPECT_EQ(EpollWait(ep, events, 0).value, 1); // UDP is writable
    SocketClose(sock);
    EXPECT_EQ(EpollWait(ep, events, 0).value, 0);
    EXPECT_EQ(SocketRecvFrom(sock, nullptr, 0, 0, false, nullptr, nullptr).error, Error::BadF);
    EpollDestroy(ep);
}

TEST_F(NetHostTest, SocketsAndEpollsShareOneIdSpace) {
    const s32 sock = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    ASSERT_GE(sock, 0);
    ASSERT_GE(ep, 0);
    EXPECT_NE(sock, ep);
    EXPECT_EQ(GetObjectKind(sock), NetObjectKind::Socket);
    EXPECT_EQ(GetObjectKind(ep), NetObjectKind::Epoll);

    // An id of the wrong kind is a bad descriptor, and is left alone.
    EXPECT_EQ(SocketClose(ep).error, Error::BadF);
    EXPECT_EQ(EpollDestroy(sock).error, Error::BadF);
    EXPECT_EQ(EpollControl(ep, EpollOp::Add, ep, Host::EvIn, 0, 0).error, Error::BadF);
    EXPECT_EQ(GetObjectKind(ep), NetObjectKind::Epoll);

    // Lowest free id first, whatever kind takes it.
    ASSERT_EQ(CloseObject(sock).error, Error::Ok);
    EXPECT_FALSE(GetObjectKind(sock));
    const s32 ep2 = static_cast<s32>(EpollCreate().value);
    EXPECT_EQ(ep2, sock);
    EXPECT_EQ(CloseObject(ep2).error, Error::Ok);
    EXPECT_EQ(CloseObject(ep).error, Error::Ok);
    EXPECT_EQ(CloseObject(ep).error, Error::BadF);
}

TEST_F(NetHostTest, SocketLimitIs128) {
    std::vector<s32> sockets;
    for (int i = 0; i < MaxSockets + 1; ++i) {
        const auto r = SocketCreate(AF_INET, SOCK_DGRAM, 0);
        if (r.error != Error::Ok) {
            EXPECT_EQ(r.error, Error::MFile);
            break;
        }
        sockets.push_back(static_cast<s32>(r.value));
    }
    EXPECT_EQ(sockets.size(), static_cast<size_t>(MaxSockets)); // no other test leaks sockets
    // Epolls do not count against the limit.
    const auto ep = EpollCreate();
    EXPECT_EQ(ep.error, Error::Ok);
    // Closing one makes room for one.
    SocketClose(sockets.back());
    sockets.pop_back();
    const auto again = SocketCreate(AF_INET, SOCK_DGRAM, 0);
    ASSERT_EQ(again.error, Error::Ok);
    sockets.push_back(static_cast<s32>(again.value));
    EXPECT_EQ(SocketCreate(AF_INET, SOCK_DGRAM, 0).error, Error::MFile);

    for (const s32 id : sockets) {
        SocketClose(id);
    }
    EpollDestroy(static_cast<s32>(ep.value));
}

TEST_F(NetHostTest, IdAllocatorHook) {
    // Stands in for the kernel's file table.
    std::mutex mutex;
    s32 next = 1000;
    std::vector<s32> released;
    SetIdAllocator({
        [&] {
            std::scoped_lock lock{mutex};
            return next++;
        },
        [&](s32 id) {
            std::scoped_lock lock{mutex};
            released.push_back(id);
        },
    });
    const s32 sock = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    EXPECT_EQ(sock, 1000);
    EXPECT_EQ(ep, 1001);
    CloseObject(sock);
    CloseObject(ep);
    EXPECT_EQ(released, (std::vector<s32>{1000, 1001}));

    // A table that is full fails creation with ENFILE.
    SetIdAllocator({[] { return -1; }, [](s32) {}});
    EXPECT_EQ(SocketCreate(AF_INET, SOCK_DGRAM, 0).error, Error::NFile);
    EXPECT_EQ(EpollCreate().error, Error::NFile);

    SetIdAllocator({}); // back to the built-in allocator
    const s32 plain = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    EXPECT_LT(plain, 1000);
    SocketClose(plain);
}

TEST_F(NetHostTest, HostErrorsKeepTheirMeaning) {
    // Errors that used to fall through to EIO.
    const s32 udp = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    EXPECT_EQ(SocketAccept(udp, nullptr, nullptr).error, Error::OpNotSupp);
    auto broadcast = Loopback(9);
    broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    EXPECT_EQ(SocketSendTo(udp, "x", 1, 0, false, reinterpret_cast<sockaddr*>(&broadcast),
                           sizeof(broadcast))
                  .error,
              Error::Acces); // no SO_BROADCAST
    SocketClose(udp);
    EXPECT_EQ(SocketCreate(AF_INET, SOCK_DGRAM, IPPROTO_TCP).error, Error::ProtoNoSupport);

    // The enum carries FreeBSD's numbering, whatever the host's is.
#ifndef _WIN32
    EXPECT_EQ(Host::TranslateNative(EACCES), Error::Acces);
    EXPECT_EQ(Host::TranslateNative(EOPNOTSUPP), Error::OpNotSupp);
    EXPECT_EQ(Host::TranslateNative(EMFILE), Error::MFile);
    EXPECT_EQ(Host::TranslateNative(ENETRESET), Error::NetReset);
    EXPECT_EQ(Host::TranslateNative(EHOSTDOWN), Error::HostDown);
#endif
    EXPECT_EQ(static_cast<int>(Error::OpNotSupp), 45);
    EXPECT_EQ(static_cast<int>(Error::NetReset), 52);
    EXPECT_EQ(static_cast<int>(Error::NotEmpty), 66);
    // Anything unknown is the PS4's EINTERNAL.
    EXPECT_EQ(Host::TranslateNative(987654), Error::Internal);
    EXPECT_EQ(static_cast<int>(Error::Internal), 204);
}

TEST_F(NetHostTest, ExternalIdsReportCompletionOnce) {
    // What resolvers use: an id from the same space that an epoll can watch.
    const s32 ep = static_cast<s32>(EpollCreate().value);
    const auto reserved = ReserveExternalId();
    ASSERT_EQ(reserved.error, Error::Ok);
    const s32 id = static_cast<s32>(reserved.value);
    EXPECT_EQ(GetObjectKind(id), NetObjectKind::External);
    EXPECT_EQ(CloseObject(id).error, Error::BadF); // released by its owner only
    ASSERT_EQ(EpollControl(ep, EpollOp::Add, id, Host::EvIn, 0, 0xABC).error, Error::Ok);
    EXPECT_EQ(EpollControl(ep, EpollOp::Add, id, Host::EvIn, 0, 0).error, Error::Exist);

    GuestEpollEvent events[4];
    EXPECT_EQ(EpollWait(ep, events, 0).value, 0);
    std::thread signaller([&] {
        std::this_thread::sleep_for(50ms);
        SignalExternal(id);
    });
    ASSERT_EQ(EpollWait(ep, events, 2'000'000).value, 1); // woken by the signal
    signaller.join();
    EXPECT_EQ(events[0].events, Host::EvIn | Host::EvDescId);
    EXPECT_EQ(events[0].ident, id);
    EXPECT_EQ(events[0].data, 0xABCu);
    // Reported once: the next wait times out instead of spinning on the set flag.
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(EpollWait(ep, events, 50'000).value, 0);
    EXPECT_GE(Millis(start), 45.0);

    // Releasing removes it from the epoll; the id is gone.
    EXPECT_EQ(ReleaseExternalId(id).error, Error::Ok);
    EXPECT_FALSE(GetObjectKind(id));
    EXPECT_EQ(ReleaseExternalId(id).error, Error::BadF);
    EXPECT_EQ(EpollControl(ep, EpollOp::Delete, id, 0, 0, 0).error, Error::BadF);
    EpollDestroy(ep);
}

TEST_F(NetHostTest, ExternalIdsCountTowardsTheLimit) {
    std::vector<s32> ids;
    for (int i = 0; i < MaxSockets; ++i) {
        ids.push_back(static_cast<s32>(ReserveExternalId().value));
    }
    EXPECT_EQ(SocketCreate(AF_INET, SOCK_DGRAM, 0).error, Error::MFile);
    EXPECT_EQ(ReserveExternalId().error, Error::MFile);
    for (const s32 id : ids) {
        ReleaseExternalId(id);
    }
}

TEST_F(NetHostTest, SocketList) {
    const s32 a = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    const s32 b = static_cast<s32>(SocketCreate(AF_INET, SOCK_STREAM, 0).value);
    EXPECT_EQ(ListSockets(), (std::vector<s32>{a, b})); // sockets only, ascending
    SocketClose(a);
    SocketClose(b);
    EpollDestroy(ep);
    EXPECT_TRUE(ListSockets().empty());
}

TEST_F(NetHostTest, SelectOnNativeSockets) {
    const s32 udp = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(SocketBind(udp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error, Error::Ok);
    socklen_t len = sizeof(addr);
    SocketGetName(udp, reinterpret_cast<sockaddr*>(&addr), &len);

    // Writable at once, not readable; the same socket listed twice is fine.
    SelectEntry entries[] = {{udp, Host::EvIn, 0}, {udp, Host::EvOut, 0}};
    auto r = SocketSelect(entries, 0);
    ASSERT_EQ(r.error, Error::Ok);
    EXPECT_EQ(r.value, 1);
    EXPECT_EQ(entries[0].ready, 0u);
    EXPECT_EQ(entries[1].ready, Host::EvOut);

    // Readable once a datagram is queued; a wait with a timeout wakes for it.
    SelectEntry read_only[] = {{udp, Host::EvIn, 0}};
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(SocketSelect(read_only, 50'000).value, 0); // times out
    EXPECT_GE(std::chrono::steady_clock::now() - start, std::chrono::milliseconds{40});
    std::thread sender([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        Error e;
        const auto helper = Host::CreateSocket(AF_INET, SOCK_DGRAM, 0, &e);
        Host::SendTo(helper, "y", 1, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        Host::CloseSocket(helper);
    });
    r = SocketSelect(read_only, -1);
    sender.join();
    EXPECT_EQ(r.value, 1);
    EXPECT_EQ(read_only[0].ready, Host::EvIn);

    // Not a socket: EBADF. Nothing listed: nothing ready.
    const s32 ep = static_cast<s32>(EpollCreate().value);
    SelectEntry bad[] = {{ep, Host::EvIn, 0}};
    EXPECT_EQ(SocketSelect(bad, 0).error, Error::BadF);
    EXPECT_EQ(SocketSelect({}, 0).value, 0);
    EpollDestroy(ep);

    // Many sockets at once: every ready one is reported, beyond one host wait's 64.
    std::vector<s32> many;
    std::vector<SelectEntry> all;
    for (int i = 0; i < 100; ++i) {
        many.push_back(static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value));
        all.push_back({many.back(), Host::EvOut, 0});
    }
    EXPECT_EQ(SocketSelect(all, 0).value, 100);
    for (const s32 id : many) {
        SocketClose(id);
    }
    SocketClose(udp);
}

TEST_F(NetHostTest, SocketPair) {
    s32 ids[2];
#ifdef _WIN32
    const int family = AF_INET; // emulated with a loopback TCP connection
#else
    const int family = AF_UNIX;
#endif
    ASSERT_EQ(SocketCreatePair(family, SOCK_STREAM, 0, ids).error, Error::Ok);
    EXPECT_NE(ids[0], ids[1]);
    EXPECT_EQ(SocketSendTo(ids[0], "ping", 4, 0, false, nullptr, 0).value, 4);
    char buf[8]{};
    EXPECT_EQ(SocketRecvFrom(ids[1], buf, sizeof(buf), 0, false, nullptr, nullptr).value, 4);
    EXPECT_STREQ(buf, "ping");
    EXPECT_EQ(SocketSendTo(ids[1], "pong", 4, 0, false, nullptr, 0).value, 4);
    EXPECT_EQ(SocketRecvFrom(ids[0], buf, sizeof(buf), 0, false, nullptr, nullptr).value, 4);
    // Both ends are ordinary sockets: closing one gives the other EOF.
    SocketClose(ids[0]);
    EXPECT_EQ(SocketRecvFrom(ids[1], buf, sizeof(buf), 0, false, nullptr, nullptr).value, 0);
    SocketClose(ids[1]);
}

TEST_F(NetHostTest, OrbisReturnCodes) {
    EXPECT_EQ(ToOrbisReturn(NetResult::Ok(7)), 7);
    // ORBIS_NET_ERROR_EWOULDBLOCK / EINTR: 0x80410100 | FreeBSD errno.
    EXPECT_EQ(static_cast<u32>(ToOrbisReturn(NetResult::Fail(Error::WouldBlock))), 0x80410123u);
    EXPECT_EQ(static_cast<u32>(ToOrbisReturn(NetResult::Fail(Error::Intr))), 0x80410104u);
}

} // namespace
