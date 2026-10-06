// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The P2P key ioctl (sceNetIoctl on P2P sockets): how the NP libraries install keys and the
// communication ID. Tested on a Keyring directly, without sockets.

#include <array>
#include <cstring>

#include <gtest/gtest.h>

#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_p2p_ioctl.h"
#include "core/libraries/net/net_types.h"
#include "core/net/p2p_codec.h"

using namespace Libraries::Net;
namespace P2P = Core::Net::P2P;

namespace {

constexpr u16 NpPort = 0x4a0e; // 3658 in network byte order

// ioctl numbers: group 'P' (0x50), 36-byte argument written by the caller (IOC_IN).
constexpr u64 SetDefault = 0x802450c9;
constexpr u64 GetDefault = 0xc02450ca;
constexpr u64 AddKey = 0x802450cb;
constexpr u64 ReleaseKey = 0x802450cc;
constexpr u64 SetWildcard = 0x802450cd;
constexpr u64 GetWildcard = 0xc02450ce;
constexpr u64 GetNpPort = 0xc00250cf;
constexpr u64 RemoveById = 0x802450c8;
constexpr u64 SetCommunicationIdCmd = 0x801050d8;
constexpr u64 ClearCommunicationId = 0x200050d9;

/// The 36-byte argument: IPv4 P2P sockaddr, key value, local port, flags, id.
struct Record {
    std::array<u8, 36> bytes{};

    Record(const char* ip, u16 port, u8 key_fill, u8 id, u16 local_port = 0) {
        OrbisNetSockaddrIn in{};
        in.sin_len = sizeof(in);
        in.sin_family = ORBIS_NET_AF_INET;
        in.sin_port = htons(port);
        inet_pton(AF_INET, ip, &in.sin_addr);
        std::memcpy(bytes.data(), &in, sizeof(in));
        std::memset(bytes.data() + 16, key_fill, 16);
        std::memcpy(bytes.data() + 32, &local_port, 2);
        bytes[34] = 0x01; // flags
        bytes[35] = id;
    }
    void* data() {
        return bytes.data();
    }
};

P2P::P2PKey Key(u8 fill) {
    P2P::P2PKey key;
    key.value.fill(fill);
    return key;
}

const P2P::Endpoint Peer = P2P::Endpoint::IPv4("198.51.100.4", 3658);

TEST(NetLibP2PIoctl, CommunicationIdIsDerived) {
    P2P::Keyring keys;
    std::array<u8, 16> value;
    for (u8 i = 0; i < 16; ++i) {
        value[i] = i;
    }
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, SetCommunicationIdCmd, value.data()), 0);
    // HMAC-SHA1(key = value, message = "")[0..4], from an independent implementation (Python).
    EXPECT_EQ(keys.GetCommunicationId(), (P2P::CommunicationId{0x54, 0x33, 0x12, 0x2f}));
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, ClearCommunicationId, nullptr), 0);
    EXPECT_FALSE(keys.GetCommunicationId());
}

TEST(NetLibP2PIoctl, PeerKeysAreReferenceCounted) {
    P2P::Keyring keys;
    Record record("198.51.100.4", 3658, 0x11, 7);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, record.data()), 0);
    const auto entry = keys.FindPeerEntry(Peer);
    ASSERT_TRUE(entry);
    EXPECT_EQ(entry->key, Key(0x11));
    EXPECT_EQ(entry->id, 7);
    EXPECT_EQ(entry->flags, 1);
    EXPECT_EQ(entry->local_port, NpPort); // 0 in the record means the NP port

    // Installed twice, so it takes two releases to go away.
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, record.data()), 0);
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, ReleaseKey, record.data()), 0);
    EXPECT_TRUE(keys.FindPeerEntry(Peer));
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, ReleaseKey, record.data()), 0);
    EXPECT_FALSE(keys.FindPeerEntry(Peer));
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, ReleaseKey, record.data()), ORBIS_NET_ENOENT);

    // A release must name the same key value and id.
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, record.data()), 0);
    Record wrong_id("198.51.100.4", 3658, 0x11, 8);
    Record wrong_key("198.51.100.4", 3658, 0x22, 7);
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, ReleaseKey, wrong_id.data()), ORBIS_NET_ENOENT);
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, ReleaseKey, wrong_key.data()), ORBIS_NET_ENOENT);

    // A different value replaces the key; an explicit local port is kept.
    Record other("198.51.100.4", 3658, 0x33, 7, 0x1234);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, other.data()), 0);
    EXPECT_EQ(keys.FindPeerEntry(Peer)->key, Key(0x33));
    EXPECT_EQ(keys.FindPeerEntry(Peer)->local_port, 0x1234);
}

TEST(NetLibP2PIoctl, RemoveKeysById) {
    P2P::Keyring keys;
    Record a("198.51.100.4", 3658, 0x11, 5);
    Record b("198.51.100.5", 3658, 0x22, 5);
    Record c("198.51.100.6", 3658, 0x33, 6);
    for (auto* r : {&a, &b, &c}) {
        ASSERT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, r->data()), 0);
    }
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, RemoveById, a.data()), 0); // id 5
    EXPECT_FALSE(keys.FindPeerEntry(P2P::Endpoint::IPv4("198.51.100.4", 3658)));
    EXPECT_FALSE(keys.FindPeerEntry(P2P::Endpoint::IPv4("198.51.100.5", 3658)));
    EXPECT_TRUE(keys.FindPeerEntry(P2P::Endpoint::IPv4("198.51.100.6", 3658)));
}

TEST(NetLibP2PIoctl, DefaultAndWildcardKeys) {
    P2P::Keyring keys;
    Record default_key("0.0.0.0", 0, 0x44, 1);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, SetDefault, default_key.data()), 0);
    EXPECT_EQ(keys.Find(Peer), Key(0x44)); // falls back to the default key
    EXPECT_EQ(keys.FindPeerEntry(P2P::Endpoint::IPv4("127.0.0.1", 3658))->key, Key(0x44));

    Record out("0.0.0.0", 0, 0x00, 0);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, GetDefault, out.data()), 0);
    EXPECT_EQ(out.bytes[16], 0x44);
    EXPECT_EQ(out.bytes[31], 0x44);

    // The wildcard key wins over the default key; reading it back works too.
    Record out_wild("0.0.0.0", 0, 0xee, 0);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, GetWildcard, out_wild.data()), 0);
    EXPECT_EQ(out_wild.bytes[16], 0x00); // none yet: zeros
    Record wildcard("0.0.0.0", 0, 0x55, 1);
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, SetWildcard, wildcard.data()), 0);
    EXPECT_EQ(keys.Find(Peer), Key(0x55));
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, GetWildcard, out_wild.data()), 0);
    EXPECT_EQ(out_wild.bytes[16], 0x55);
}

TEST(NetLibP2PIoctl, NpPortAndErrors) {
    P2P::Keyring keys;
    u16 port = 0;
    ASSERT_EQ(P2PKeyIoctl(keys, NpPort, GetNpPort, &port), 0);
    EXPECT_EQ(port, NpPort);

    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, 0x8004667e /* FIONBIO */, &port), ORBIS_NET_EINVAL);
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, nullptr), ORBIS_NET_EFAULT);
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, 0x200050c8, nullptr), 0); // the no-op command
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, 0x802450ff, &port), ORBIS_NET_EINVAL);

    // Adding a key needs an IPv4 P2P sockaddr.
    Record bad_family("198.51.100.4", 3658, 0x11, 7);
    bad_family.bytes[1] = ORBIS_NET_AF_INET6;
    EXPECT_EQ(P2PKeyIoctl(keys, NpPort, AddKey, bad_family.data()), ORBIS_NET_EINVAL);
}

} // namespace
