// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <cstring>
#include <random>
#include <thread>

#include <gtest/gtest.h>

#include "core/net/guest_net.h"
#include "core/net/p2p_codec.h"
#include "core/net/p2p_transport.h"

using namespace Core::Net;
using namespace Core::Net::P2P;
using namespace std::chrono_literals;
using Host::Error;

namespace {

class TestCodec final : public Codec {
public:
    Decoded Decode(std::span<const u8> packet, const Endpoint&) override {
        Decoded d;
        if (packet.size() < 5) {
            return d;
        }
        d.kind = packet[0] == 1   ? Kind::Stream
                 : packet[0] == 2 ? Kind::Datagram
                 : packet[0] == 3 ? Kind::Signaling
                                  : Kind::Invalid;
        d.src_vport = static_cast<u16>((packet[1] << 8) | packet[2]);
        d.dst_vport = static_cast<u16>((packet[3] << 8) | packet[4]);
        d.payload.assign(packet.begin() + 5, packet.end());
        return d;
    }
    std::vector<u8> EncodeStream(std::span<const u8> seg, const Endpoint&, Protection) override {
        return Frame(1, 0, 0, seg);
    }
    std::vector<u8> EncodeDatagram(u16 src, u16 dst, std::span<const u8> payload, const Endpoint&,
                                   Protection) override {
        return Frame(2, src, dst, payload);
    }
    std::vector<u8> EncodeSignaling(std::span<const u8> data, const Endpoint&) override {
        return Frame(3, 0, 0, data);
    }
    PseudoHeader StreamPseudoHeader(const Endpoint&) override {
        PseudoHeader p;
        p.local = p.remote = {127, 0, 0, 1};
        return p;
    }

private:
    static std::vector<u8> Frame(u8 kind, u16 src, u16 dst, std::span<const u8> payload) {
        std::vector<u8> v{kind, static_cast<u8>(src >> 8), static_cast<u8>(src),
                          static_cast<u8>(dst >> 8), static_cast<u8>(dst)};
        v.insert(v.end(), payload.begin(), payload.end());
        return v;
    }
};

std::shared_ptr<Transport> MakeTransport(std::unique_ptr<Codec> codec) {
    Error e;
    auto t = Transport::Create(AF_INET, Endpoint::IPv4("127.0.0.1", 0), std::move(codec), {}, &e);
    EXPECT_TRUE(t) << "transport creation failed: " << static_cast<int>(e);
    return t;
}

std::vector<u8> RandomBytes(size_t n, u32 seed) {
    std::mt19937 rng(seed);
    std::vector<u8> v(n);
    for (auto& b : v) {
        b = static_cast<u8>(rng());
    }
    return v;
}

double Millis(Clock::time_point since) {
    return std::chrono::duration<double, std::milli>(Clock::now() - since).count();
}

/// Drives a remote stream socket: send everything, read until `expect_in` bytes arrived.
bool RemotePump(StreamSocket& s, const std::vector<u8>& out, std::vector<u8>& in, size_t expect_in,
                std::chrono::seconds limit) {
    size_t off = 0;
    const auto end = Clock::now() + limit;
    u8 buf[8192];
    while (Clock::now() < end) {
        if (off < out.size()) {
            off += s.Send(std::span(out).subspan(off)).bytes;
        }
        for (;;) {
            const auto r = s.Recv(buf, false);
            if (r.bytes == 0) {
                break;
            }
            in.insert(in.end(), buf, buf + r.bytes);
        }
        if (off == out.size() && in.size() >= expect_in) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

class NetP2PGuest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_TRUE(Host::Initialize());
        local_ = MakeTransport(std::make_unique<TestCodec>());
        remote_ = MakeTransport(std::make_unique<TestCodec>());
        SetP2PTransport(AF_INET, local_);
    }
    static void TearDownTestSuite() {
        SetP2PTransport(AF_INET, nullptr);
        local_.reset();
        remote_.reset();
    }
    void TearDown() override {
        local_->SetOutgoingLossForTesting(0);
        remote_->SetOutgoingLossForTesting(0);
    }

    static Endpoint LocalAddress() {
        return Endpoint::IPv4("127.0.0.1", local_->BoundPort());
    }
    static Endpoint RemoteAddress() {
        return Endpoint::IPv4("127.0.0.1", remote_->BoundPort());
    }
    static s32 NewP2PSocket(bool stream) {
        const auto r = P2PSocketCreate(AF_INET, stream);
        EXPECT_EQ(r.error, Error::Ok);
        return static_cast<s32>(r.value);
    }

    static inline std::shared_ptr<Transport> local_;
    static inline std::shared_ptr<Transport> remote_;
};

TEST_F(NetP2PGuest, EpollInfiniteWaitWakesOnDatagram) {
    const s32 sock = NewP2PSocket(false);
    ASSERT_EQ(P2PSocketBind(sock, 3001).error, Error::Ok);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    ASSERT_EQ(EpollControl(ep, EpollOp::Add, sock, Host::EvIn, 0, 0xfeed).error, Error::Ok);

    auto remote = remote_->CreateDatagram();
    remote->Bind(4001);
    std::thread sender([&] {
        std::this_thread::sleep_for(200ms);
        const u8 msg[] = "ping";
        const auto to = LocalAddress();
        remote->SendTo(msg, &to, 3001);
    });
    const auto start = Clock::now();
    GuestEpollEvent events[4];
    const auto r = EpollWait(ep, events, -1); // infinite timeout
    const double waited = Millis(start);
    sender.join();

    ASSERT_EQ(r.value, 1);
    EXPECT_EQ(events[0].data, 0xfeedu);
    EXPECT_TRUE(events[0].events & Host::EvIn);
    EXPECT_LT(waited, 2000.0);

    u8 buf[16];
    Endpoint from;
    u16 from_vport = 0;
    EXPECT_EQ(P2PSocketRecvFrom(sock, buf, sizeof(buf), false, false, &from, &from_vport).value, 5);
    EXPECT_EQ(from_vport, 4001);
    EXPECT_EQ(from, RemoteAddress());
    SocketClose(sock);
    EpollDestroy(ep);
}

TEST_F(NetP2PGuest, MixedNativeAndP2PEpoll) {
    const s32 native = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(SocketBind(native, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)).error,
              Error::Ok);
    socklen_t len = sizeof(addr);
    SocketGetName(native, reinterpret_cast<sockaddr*>(&addr), &len);
    const s32 p2p = NewP2PSocket(false);
    P2PSocketBind(p2p, 3002);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    EpollControl(ep, EpollOp::Add, native, Host::EvIn, 0, 1);
    EpollControl(ep, EpollOp::Add, p2p, Host::EvIn, 0, 2);
    GuestEpollEvent events[4];

    auto remote = remote_->CreateDatagram();
    remote->Bind(4002);
    const u8 msg[] = "x";
    const auto to = LocalAddress();
    remote->SendTo(msg, &to, 3002);
    auto r = EpollWait(ep, events, 2'000'000);
    ASSERT_EQ(r.value, 1);
    EXPECT_EQ(events[0].data, 2u);
    EXPECT_EQ(EpollWait(ep, events, 0).value, 1); // level-triggered until read
    u8 buf[8];
    P2PSocketRecvFrom(p2p, buf, sizeof(buf), false, false, nullptr, nullptr);
    EXPECT_EQ(EpollWait(ep, events, 50'000).value, 0);

    Error e;
    const auto helper = Host::CreateSocket(AF_INET, SOCK_DGRAM, 0, &e);
    Host::SendTo(helper, "y", 1, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    r = EpollWait(ep, events, 2'000'000);
    ASSERT_EQ(r.value, 1);
    EXPECT_EQ(events[0].data, 1u);

    Host::CloseSocket(helper);
    SocketClose(native);
    SocketClose(p2p);
    EpollDestroy(ep);
}

TEST_F(NetP2PGuest, SelectWorksOnP2PSockets) {
    // select() needs no host socket: a P2P socket is waited on like a native one.
    const s32 p2p = NewP2PSocket(false);
    ASSERT_EQ(P2PSocketBind(p2p, 3009).error, Error::Ok);
    const s32 native = static_cast<s32>(SocketCreate(AF_INET, SOCK_DGRAM, 0).value);
    SelectEntry entries[] = {{p2p, Host::EvIn, 0}, {native, Host::EvIn, 0}};
    EXPECT_EQ(SocketSelect(entries, 20'000).value, 0);

    auto remote = remote_->CreateDatagram();
    remote->Bind(4009);
    std::thread sender([&] {
        std::this_thread::sleep_for(100ms);
        const u8 msg[] = "hi";
        const auto to = LocalAddress();
        remote->SendTo(msg, &to, 3009);
    });
    const auto r = SocketSelect(entries, 2'000'000);
    sender.join();
    ASSERT_EQ(r.error, Error::Ok);
    EXPECT_EQ(r.value, 1);
    EXPECT_EQ(entries[0].ready, Host::EvIn);
    EXPECT_EQ(entries[1].ready, 0u);

    // Selecting leaves nothing behind: an epoll on the same socket still works afterwards.
    const s32 ep = static_cast<s32>(EpollCreate().value);
    ASSERT_EQ(EpollControl(ep, EpollOp::Add, p2p, Host::EvIn, 0, 7).error, Error::Ok);
    GuestEpollEvent events[2];
    EXPECT_EQ(EpollWait(ep, events, 0).value, 1);
    EpollDestroy(ep);
    SocketClose(p2p);
    SocketClose(native);
}

TEST_F(NetP2PGuest, EpollWritableForDatagram) {
    const s32 sock = NewP2PSocket(false);
    P2PSocketBind(sock, 3003);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    EpollControl(ep, EpollOp::Add, sock, Host::EvIn | Host::EvOut, 0, 7);
    GuestEpollEvent events[4];
    ASSERT_EQ(EpollWait(ep, events, 0).value, 1); // reported once, not per handle
    EXPECT_EQ(events[0].events, static_cast<u32>(Host::EvOut));
    EpollControl(ep, EpollOp::Modify, sock, Host::EvIn, 0, 7);
    EXPECT_EQ(EpollWait(ep, events, 0).value, 0);
    EXPECT_EQ(EpollControl(ep, EpollOp::Add, sock, Host::EvIn, 0, 0).error, Error::Exist);
    // One-shot/edge-triggered are not emulated for P2P sockets yet (DISCUSS in guest_net).
    EpollControl(ep, EpollOp::Delete, sock, 0, 0, 0);
    EXPECT_EQ(EpollControl(ep, EpollOp::Add, sock, Host::EvIn, Host::OneShot, 0).error,
              Error::Inval);
    SocketClose(sock);
    EpollDestroy(ep);
}

TEST_F(NetP2PGuest, RecvTimeoutAndAbort) {
    const s32 sock = NewP2PSocket(false);
    P2PSocketBind(sock, 3004);
    u8 buf[8];

    SocketSetRecvTimeout(sock, 200ms);
    auto start = Clock::now();
    EXPECT_EQ(P2PSocketRecvFrom(sock, buf, sizeof(buf), false, false, nullptr, nullptr).error,
              Error::WouldBlock);
    EXPECT_GE(Millis(start), 190.0);
    EXPECT_LT(Millis(start), 2000.0);

    // Abort wakes a recv blocked without timeout (the old P2P abort only flushed queues).
    SocketSetRecvTimeout(sock, 0us);
    std::thread aborter([&] {
        std::this_thread::sleep_for(150ms);
        SocketAbort(sock, kSocketAbortPreserveRecv);
    });
    start = Clock::now();
    EXPECT_EQ(P2PSocketRecvFrom(sock, buf, sizeof(buf), false, false, nullptr, nullptr).error,
              Error::Intr);
    EXPECT_LT(Millis(start), 2000.0);
    aborter.join();
    SocketClose(sock);
}

TEST_F(NetP2PGuest, EpollAbortWakesInfiniteWait) {
    const s32 ep = static_cast<s32>(EpollCreate().value);
    std::thread aborter([&] {
        std::this_thread::sleep_for(100ms);
        EpollAbort(ep, kEpollAbortPreserve);
    });
    GuestEpollEvent events[1];
    EXPECT_EQ(EpollWait(ep, events, -1).error, Error::Intr);
    aborter.join();
    EpollDestroy(ep);
}

TEST_F(NetP2PGuest, StreamBlockingConnectAndRefused) {
    auto listener = remote_->CreateStream();
    ASSERT_EQ(listener->Bind(5000), Error::Ok);
    ASSERT_EQ(listener->Listen(4), Error::Ok);

    const s32 sock = NewP2PSocket(true);
    ASSERT_EQ(P2PSocketConnect(sock, RemoteAddress(), 5000).error, Error::Ok); // blocks
    Error e = Error::WouldBlock;
    std::shared_ptr<StreamSocket> accepted;
    for (int i = 0; i < 200 && !accepted; ++i) {
        accepted = listener->Accept(nullptr, nullptr, &e);
        std::this_thread::sleep_for(5ms);
    }
    ASSERT_TRUE(accepted);
    const char msg[] = "hello from the guest";
    EXPECT_EQ(P2PSocketSendTo(sock, msg, sizeof(msg), false, nullptr, 0).value,
              static_cast<s64>(sizeof(msg)));
    u8 buf[64]{};
    size_t got = 0;
    for (int i = 0; i < 200 && got < sizeof(msg); ++i) {
        got += accepted->Recv(std::span(buf).subspan(got), false).bytes;
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(std::memcmp(buf, msg, sizeof(msg)), 0);

    // Nothing listens on vport 5999: the remote transport answers with RST.
    const s32 refused = NewP2PSocket(true);
    EXPECT_EQ(P2PSocketConnect(refused, RemoteAddress(), 5999).error, Error::ConnRefused);

    // Non-blocking connect reports progress, then completion via writability.
    const s32 async = NewP2PSocket(true);
    SocketSetNonBlocking(async, true);
    EXPECT_EQ(P2PSocketConnect(async, RemoteAddress(), 5000).error, Error::InProgress);
    const s32 ep = static_cast<s32>(EpollCreate().value);
    EpollControl(ep, EpollOp::Add, async, Host::EvOut, 0, 0);
    GuestEpollEvent events[1];
    EXPECT_EQ(EpollWait(ep, events, 2'000'000).value, 1);
    EXPECT_EQ(P2PSocketConnect(async, RemoteAddress(), 5000).error, Error::IsConn);

    EpollDestroy(ep);
    SocketClose(async);
    SocketClose(sock);
    SocketClose(refused);
    listener->Close();
}

TEST_F(NetP2PGuest, CloseWakesBlockedAccept) {
    const s32 listener = NewP2PSocket(true);
    P2PSocketBind(listener, 3700);
    SocketListen(listener, 1);
    NetResult r{};
    std::thread t([&] { r = P2PSocketAccept(listener, nullptr, nullptr); });
    std::this_thread::sleep_for(100ms);
    SocketClose(listener);
    t.join();
    EXPECT_EQ(r.error, Error::Intr);
}

TEST_F(NetP2PGuest, DatagramQueueBoundedAndUnboundDropped) {
    const s32 sock = NewP2PSocket(false);
    P2PSocketBind(sock, 3800);
    EXPECT_EQ(P2PSocketBind(NewP2PSocket(false), 3800).error, Error::AddrInUse);
    auto remote = remote_->CreateDatagram();
    remote->Bind(4800);
    const auto to = LocalAddress();
    const std::vector<u8> big(1200, 0xab);
    for (int i = 0; i < 400; ++i) { // 480 KB into a 256 KB budget
        remote->SendTo(big, &to, 3800);
        remote->SendTo(big, &to, 3801); // nobody bound here
        if (i % 50 == 0) {
            std::this_thread::sleep_for(1ms); // don't overflow the host socket buffer
        }
    }
    std::this_thread::sleep_for(300ms);
    size_t received = 0;
    u8 buf[2048];
    SocketSetNonBlocking(sock, true);
    while (P2PSocketRecvFrom(sock, buf, sizeof(buf), false, false, nullptr, nullptr).value > 0) {
        received += 1200;
    }
    EXPECT_GT(received, 0u);
    EXPECT_LE(received, TransportConfig{}.datagram_queue_bytes);
    SocketClose(sock);
}

class NetP2PGuestStream : public NetP2PGuest, public ::testing::WithParamInterface<double> {};

TEST_P(NetP2PGuestStream, BlockingAcceptSendRecv) {
    const double loss = GetParam();
    const size_t bytes = loss > 0 ? (1u << 20) : (4u << 20);
    local_->SetOutgoingLossForTesting(loss);
    remote_->SetOutgoingLossForTesting(loss);
    const s32 listener = NewP2PSocket(true);
    ASSERT_EQ(P2PSocketBind(listener, 3658).error, Error::Ok);
    ASSERT_EQ(SocketListen(listener, 4).error, Error::Ok);

    const auto guest_data = RandomBytes(bytes, 11);
    const auto remote_data = RandomBytes(bytes, 12);
    std::vector<u8> guest_got;
    std::atomic<bool> guest_done{false};

    // Guest side: blocking accept, blocking send of everything, blocking recv to EOF.
    std::thread guest([&] {
        const auto a = P2PSocketAccept(listener, nullptr, nullptr);
        if (a.error != Error::Ok) {
            return;
        }
        const s32 conn = static_cast<s32>(a.value);
        std::thread writer([&] {
            EXPECT_EQ(P2PSocketSendTo(conn, guest_data.data(), guest_data.size(), false, nullptr, 0)
                          .value,
                      static_cast<s64>(guest_data.size()));
            SocketShutdown(conn, 1);
        });
        u8 buf[16384];
        for (;;) {
            const auto r =
                P2PSocketRecvFrom(conn, buf, sizeof(buf), false, false, nullptr, nullptr);
            if (r.error != Error::Ok || r.value == 0) {
                break;
            }
            guest_got.insert(guest_got.end(), buf, buf + r.value);
        }
        writer.join();
        SocketClose(conn);
        guest_done = true;
    });

    auto remote = remote_->CreateStream();
    EXPECT_EQ(remote->Connect(LocalAddress(), 3658), Error::InProgress);
    std::vector<u8> remote_got;
    const bool pumped = RemotePump(*remote, remote_data, remote_got, bytes, 120s);
    remote->Shutdown(1);
    if (!pumped) {
        SocketAbort(listener, kSocketAbortPreserveRecv); // unblock the guest thread
    }
    guest.join();
    remote->Close();
    SocketClose(listener);

    EXPECT_TRUE(pumped);
    EXPECT_TRUE(guest_done);
    EXPECT_TRUE(guest_got == remote_data) << "guest received " << guest_got.size();
    EXPECT_TRUE(remote_got == guest_data) << "remote received " << remote_got.size();
}

INSTANTIATE_TEST_SUITE_P(Loss, NetP2PGuestStream, ::testing::Values(0.0, 0.05),
                         [](const auto& info) {
                             return info.param > 0 ? std::string("Loss5") : std::string("Clean");
                         });

// ---------------------------------------------------------------------------------------------
// The real wire format end to end
// ---------------------------------------------------------------------------------------------

TEST(NetP2PFraming, ProtectedStreamWithCommunicationId) {
    ASSERT_TRUE(Host::Initialize());
    auto keys_a = std::make_shared<Keyring>();
    auto keys_b = std::make_shared<Keyring>();
    const auto a = MakeTransport(std::make_unique<FramingCodec>(keys_a));
    const auto b = MakeTransport(std::make_unique<FramingCodec>(keys_b));
    const auto a_addr = Endpoint::IPv4("127.0.0.1", a->BoundPort());
    const auto b_addr = Endpoint::IPv4("127.0.0.1", b->BoundPort());
    P2PKey key;
    key.value.fill(0x5a);
    for (auto* keys : {keys_a.get(), keys_b.get()}) {
        keys->SetCommunicationId(CommunicationId{'N', 'P', 'W', 'R'});
    }
    keys_a->SetPeerKey(b_addr, key);
    keys_b->SetPeerKey(a_addr, key);

    auto listener = b->CreateStream();
    listener->SetProtection({true, true});
    ASSERT_EQ(listener->Bind(3658), Error::Ok);
    ASSERT_EQ(listener->Listen(1), Error::Ok);
    auto client = a->CreateStream();
    client->SetProtection({true, true});
    ASSERT_EQ(client->Connect(b_addr, 3658), Error::InProgress);

    std::shared_ptr<StreamSocket> server;
    Error e;
    for (int i = 0; i < 400 && !server; ++i) {
        server = listener->Accept(nullptr, nullptr, &e);
        std::this_thread::sleep_for(5ms);
    }
    ASSERT_TRUE(server);
    const auto data = RandomBytes(256 * 1024, 5);
    std::vector<u8> got;
    std::vector<u8> nothing;
    std::thread reader([&] { RemotePump(*server, {}, got, data.size(), 30s); });
    EXPECT_TRUE(RemotePump(*client, data, nothing, 0, 30s));
    reader.join();
    EXPECT_TRUE(got == data) << "received " << got.size();

    // A transport of another title (different communication ID) cannot even connect.
    auto keys_c = std::make_shared<Keyring>();
    keys_c->SetCommunicationId(CommunicationId{'O', 'T', 'H', 'R'});
    const auto c = MakeTransport(std::make_unique<FramingCodec>(keys_c));
    auto stranger = c->CreateStream();
    stranger->Connect(b_addr, 3658);
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(stranger->ConnectResult(), Error::InProgress); // SYNs silently dropped
    stranger->Abort();
}

} // namespace
