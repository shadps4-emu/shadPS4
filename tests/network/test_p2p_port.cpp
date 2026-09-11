// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include "core/libraries/network/p2p_port.h"

using namespace Libraries::Net;

namespace {

// Delivery goes through the host stack twice (peer socket, then the inbox hop), so every
// expectation waits instead of assuming the datagram is already there.
constexpr s64 kDeliveryTimeoutUs = 2'000'000;
// Long enough to catch a delivery that should not happen, short enough not to slow the suite.
constexpr s64 kNoDeliveryTimeoutUs = 200'000;

sockaddr_in Loopback(u16 port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = port;
    return addr;
}

// Stands in for the inbox a guest P2PSocket owns: the port forwards into it and the guest reads
// from it, which is exactly what makes the guest-visible fd pollable.
struct Inbox {
    net_socket sock{};
    sockaddr_in addr{};

    Inbox() {
        EXPECT_TRUE(CreateP2PInbox(sock, addr));
    }
    ~Inbox() {
        CloseP2PSocket(sock);
    }

    bool Receive(P2PInboxHeader& header, std::string& payload, s64 timeout_us) {
        if (!WaitP2PSocketReadable(sock, timeout_us)) {
            return false;
        }
        char buffer[2048];
        const int received = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (received < static_cast<int>(sizeof(P2PInboxHeader))) {
            return false;
        }
        std::memcpy(&header, buffer, sizeof(header));
        payload.assign(buffer + sizeof(header), received - sizeof(header));
        return true;
    }
};

class P2PPortTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
#ifdef _WIN32
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }
};

// The load-bearing case: LittleBigPlanet 3 sends to its own bound (address, port, vport) and polls
// for the datagram to come back. With a real socket this is an OS round-trip, not an emulated one.
TEST_F(P2PPortTest, SelfProbeRoundTrips) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox inbox;
    const u16 vport = htons(3658);
    ASSERT_EQ(port->Claim(vport, false, &inbox, inbox.addr), vport);

    const std::string probe = "self-probe";
    ASSERT_EQ(port->Send(probe.data(), static_cast<u32>(probe.size()), vport, vport,
                         Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(probe.size()));

    P2PInboxHeader header{};
    std::string payload;
    ASSERT_TRUE(inbox.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_EQ(payload, probe);
    EXPECT_EQ(header.from_addr, htonl(INADDR_LOOPBACK));
    EXPECT_EQ(header.from_port, port->BoundPort());
    EXPECT_EQ(header.from_vport, vport);
}

TEST_F(P2PPortTest, DemultiplexesByVport) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox first, second;
    const u16 first_vport = htons(100);
    const u16 second_vport = htons(200);
    ASSERT_EQ(port->Claim(first_vport, false, &first, first.addr), first_vport);
    ASSERT_EQ(port->Claim(second_vport, false, &second, second.addr), second_vport);

    const std::string message = "for the second endpoint";
    ASSERT_EQ(port->Send(message.data(), static_cast<u32>(message.size()), first_vport,
                         second_vport, Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(message.size()));

    P2PInboxHeader header{};
    std::string payload;
    ASSERT_TRUE(second.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_EQ(payload, message);
    EXPECT_EQ(header.from_vport, first_vport);
    EXPECT_FALSE(first.Receive(header, payload, kNoDeliveryTimeoutUs));
}

TEST_F(P2PPortTest, RejectsVportInUseUnlessBothShare) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox owner, intruder;
    const u16 vport = htons(4242);
    ASSERT_EQ(port->Claim(vport, false, &owner, owner.addr), vport);
    // The holder did not opt into sharing, so nobody else gets the vport.
    EXPECT_EQ(port->Claim(vport, true, &intruder, intruder.addr), 0);

    port->Release(&owner);
    ASSERT_EQ(port->Claim(vport, true, &owner, owner.addr), vport);
    ASSERT_EQ(port->Claim(vport, true, &intruder, intruder.addr), vport);

    // Both opted in, so both receive the datagram, as a real stack would deliver it.
    const std::string message = "shared";
    ASSERT_EQ(port->Send(message.data(), static_cast<u32>(message.size()), vport, vport,
                         Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(message.size()));

    P2PInboxHeader header{};
    std::string payload;
    ASSERT_TRUE(owner.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_EQ(payload, message);
    ASSERT_TRUE(intruder.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_EQ(payload, message);
}

TEST_F(P2PPortTest, AllocatesEphemeralVportAndKeepsZeroReserved) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox first, second;
    const u16 first_vport = port->Claim(kP2PSignalingVport, false, &first, first.addr);
    const u16 second_vport = port->Claim(kP2PSignalingVport, false, &second, second.addr);
    EXPECT_NE(first_vport, kP2PSignalingVport);
    EXPECT_NE(second_vport, kP2PSignalingVport);
    EXPECT_NE(first_vport, second_vport);
    EXPECT_GE(ntohs(first_vport), kP2PFirstEphemeralVport);
    EXPECT_GE(ntohs(second_vport), kP2PFirstEphemeralVport);
}

// Two guest sockets bound to different vports of one host port must end up on the same OS socket,
// or the second bind would collide with the first.
TEST_F(P2PPortTest, SharesOneSocketPerHostPort) {
    auto first = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(first, nullptr);
    auto second = P2PPort::Acquire(htonl(INADDR_LOOPBACK), first->BoundPort());
    EXPECT_EQ(first.get(), second.get());

    auto other = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(other, nullptr);
    EXPECT_NE(first.get(), other.get());
    EXPECT_NE(first->BoundPort(), other->BoundPort());
}

TEST_F(P2PPortTest, DropsDatagramForUnclaimedVport) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox inbox;
    const u16 vport = htons(700);
    ASSERT_EQ(port->Claim(vport, false, &inbox, inbox.addr), vport);

    const std::string message = "nobody is listening";
    ASSERT_EQ(port->Send(message.data(), static_cast<u32>(message.size()), vport, htons(701),
                         Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(message.size()));

    P2PInboxHeader header{};
    std::string payload;
    EXPECT_FALSE(inbox.Receive(header, payload, kNoDeliveryTimeoutUs));
}

// A zero-length datagram is a legal datagram: it must arrive, not be mistaken for "no data".
TEST_F(P2PPortTest, DeliversEmptyDatagram) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox inbox;
    const u16 vport = htons(900);
    ASSERT_EQ(port->Claim(vport, false, &inbox, inbox.addr), vport);
    ASSERT_EQ(port->Send(nullptr, 0, vport, vport, Loopback(port->BoundPort()), kP2PFlagDgram), 0);

    P2PInboxHeader header{};
    std::string payload;
    ASSERT_TRUE(inbox.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_TRUE(payload.empty());
    EXPECT_EQ(header.from_vport, vport);
}

// The largest payload LittleBigPlanet 3 was measured sending is 918 bytes; check that a datagram
// well past that survives encapsulation intact.
TEST_F(P2PPortTest, DeliversLargePayloadIntact) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox inbox;
    const u16 vport = htons(1500);
    ASSERT_EQ(port->Claim(vport, false, &inbox, inbox.addr), vport);

    std::string message(1400, '\0');
    for (size_t i = 0; i < message.size(); ++i) {
        message[i] = static_cast<char>(i & 0xff);
    }
    ASSERT_EQ(port->Send(message.data(), static_cast<u32>(message.size()), vport, vport,
                         Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(message.size()));

    P2PInboxHeader header{};
    std::string payload;
    ASSERT_TRUE(inbox.Receive(header, payload, kDeliveryTimeoutUs));
    EXPECT_EQ(payload, message);
}

// Releasing an endpoint must stop its delivery, or a closed guest socket would keep a stale inbox
// alive in the port's routing table.
TEST_F(P2PPortTest, ReleaseStopsDelivery) {
    auto port = P2PPort::Acquire(htonl(INADDR_LOOPBACK), 0);
    ASSERT_NE(port, nullptr);

    Inbox inbox;
    const u16 vport = htons(2600);
    ASSERT_EQ(port->Claim(vport, false, &inbox, inbox.addr), vport);
    port->Release(&inbox);

    const std::string message = "after release";
    ASSERT_EQ(port->Send(message.data(), static_cast<u32>(message.size()), vport, vport,
                         Loopback(port->BoundPort()), kP2PFlagDgram),
              static_cast<int>(message.size()));

    P2PInboxHeader header{};
    std::string payload;
    EXPECT_FALSE(inbox.Receive(header, payload, kNoDeliveryTimeoutUs));
}

} // namespace
