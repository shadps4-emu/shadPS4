// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The P2P wire format: exact bytes (pinning the reverse-engineered layout), communication-ID
// filtering, and payload protection (AES-128-CFB + truncated HMAC-SHA1).

#include <gtest/gtest.h>

#include "core/net/p2p_codec.h"

using namespace Core::Net;
using namespace Core::Net::P2P;

namespace {

const Endpoint kPeer = Endpoint::IPv4("192.0.2.10", 3658);
const Endpoint kOther = Endpoint::IPv4("192.0.2.11", 3658);
constexpr CommunicationId kComId{'N', 'P', 'W', 'R'};

std::vector<u8> Bytes(std::string_view s) {
    return {s.begin(), s.end()};
}

P2PKey Key(u8 fill) {
    P2PKey k;
    k.value.fill(fill);
    return k;
}

class NetP2PCodec : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_TRUE(Host::Initialize());
    }

    std::shared_ptr<Keyring> keys = std::make_shared<Keyring>();
    FramingCodec codec{keys};
};

// ---------------------------------------------------------------------------------------------
// Exact bytes
// ---------------------------------------------------------------------------------------------

TEST_F(NetP2PCodec, DatagramHeader) {
    // Vports are always two bytes, network byte order, even when small (0x40 is
    // P2P_BROADCAST, not a short-vport flag).
    EXPECT_EQ(codec.EncodeDatagram(3, 4, Bytes("ab"), kPeer, {}),
              (std::vector<u8>{0xFF, 0x80 | 3, 0x00, 0x03, 0x00, 0x04, 'a', 'b'}));
    EXPECT_EQ(codec.EncodeDatagram(0x0E4A, 4, Bytes("ab"), kPeer, {}),
              (std::vector<u8>{0xFF, 0x80 | 3, 0x0E, 0x4A, 0x00, 0x04, 'a', 'b'}));
}

TEST_F(NetP2PCodec, DatagramWithDigest) {
    keys->SetCommunicationId(kComId);
    EXPECT_EQ(codec.EncodeDatagram(1, 2, Bytes("z"), kPeer, {}),
              (std::vector<u8>{0xFF, 0x80 | 0x20 | 3, 0, 1, 0, 2, 'N', 'P', 'W', 'R', 'z'}));
}

TEST_F(NetP2PCodec, StreamHeader) {
    const auto segment = Bytes("tcp-segment");
    auto expected = std::vector<u8>{0xFF, 0x80 | 7};
    expected.insert(expected.end(), segment.begin(), segment.end());
    EXPECT_EQ(codec.EncodeStream(segment, kPeer, {}), expected);

    keys->SetCommunicationId(kComId);
    expected = {0xFF, 0x80 | 0x20 | 7, 'N', 'P', 'W', 'R'};
    expected.insert(expected.end(), segment.begin(), segment.end());
    EXPECT_EQ(codec.EncodeStream(segment, kPeer, {}), expected);
}

TEST_F(NetP2PCodec, SignalingHeader) {
    EXPECT_EQ(codec.EncodeSignaling(Bytes("SHAD"), kPeer),
              (std::vector<u8>{0xFF, 0x80 | 3, 0xFF, 0xFF, 0xFF, 0xFF, 'S', 'H', 'A', 'D'}));
}

// ---------------------------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------------------------

TEST_F(NetP2PCodec, DecodeRoundTrips) {
    for (const auto& [src, dst] : {std::pair<u16, u16>{3, 4}, {3658, 49152}, {255, 256}}) {
        const auto d =
            codec.Decode(codec.EncodeDatagram(src, dst, Bytes("payload"), kPeer, {}), kPeer);
        EXPECT_EQ(d.kind, Codec::Kind::Datagram);
        EXPECT_EQ(d.src_vport, src); // host byte order
        EXPECT_EQ(d.dst_vport, dst);
        EXPECT_EQ(d.payload, Bytes("payload"));
    }
    const auto s = codec.Decode(codec.EncodeStream(Bytes("seg"), kPeer, {}), kPeer);
    EXPECT_EQ(s.kind, Codec::Kind::Stream);
    EXPECT_EQ(s.payload, Bytes("seg"));
    const auto g = codec.Decode(codec.EncodeSignaling(Bytes("hi"), kPeer), kPeer);
    EXPECT_EQ(g.kind, Codec::Kind::Signaling);
    EXPECT_EQ(g.payload, Bytes("hi"));
}

TEST_F(NetP2PCodec, DecodeRejectsMalformed) {
    const auto invalid = [&](std::vector<u8> p) {
        return codec.Decode(p, kPeer).kind == Codec::Kind::Invalid;
    };
    EXPECT_TRUE(invalid({}));
    EXPECT_TRUE(invalid({0xFF}));
    EXPECT_TRUE(invalid({0xFE, 0x80 | 3, 0, 1, 0, 2}));      // bad marker
    EXPECT_TRUE(invalid({0xFF, 3, 0, 1, 0, 2}));             // P2P_VALID missing
    EXPECT_TRUE(invalid({0xFF, 0x80 | 2, 0, 1, 0, 2}));      // kind below P2P_UDP_SIMPLE
    EXPECT_TRUE(invalid({0xFF, 0x80 | 11}));                 // kind above P2P_TCP_CRYPTO_SIGNTR
    EXPECT_TRUE(invalid({0xFF, 0x80 | 0x40 | 7, 0, 1}));     // P2P_BROADCAST on TCP
    EXPECT_TRUE(invalid({0xFF, 0x80 | 3, 0, 1, 0}));         // truncated header
    EXPECT_TRUE(invalid({0xFF, 0x80 | 0x20 | 7, 'N', 'P'})); // truncated communication ID
}

TEST_F(NetP2PCodec, BroadcastPacketsAreDecodedAndReported) {
    int reported = 0;
    std::vector<u8> seen;
    FramingCodec observed{keys, [&](std::span<const u8> packet, const Endpoint& from) {
                              ++reported;
                              seen.assign(packet.begin(), packet.end());
                              EXPECT_EQ(from, kPeer);
                          }};
    // [FF][valid|broadcast|digest|UDP_SIMPLE][port 0x0E4A][digest][payload]
    const std::vector<u8> broadcast{
        0xFF, 0x80 | 0x40 | 0x20 | 3, 0x0E, 0x4A, 'N', 'P', 'W', 'R', 'x', 'y'};
    for (int i = 0; i < FramingCodec::MaxReportedBroadcasts + 5; ++i) {
        const auto d = observed.Decode(broadcast, kPeer);
        ASSERT_EQ(d.kind, Codec::Kind::Datagram);
        EXPECT_EQ(d.dst_vport, 0x0E4A);
        EXPECT_EQ(d.src_vport, 0x0E4A); // no source vport on the wire: the same port
        EXPECT_EQ(d.payload, Bytes("xy"));
    }
    EXPECT_EQ(reported, FramingCodec::MaxReportedBroadcasts); // only the first few
    EXPECT_EQ(seen, broadcast);                                // the whole packet

    // Without a digest the payload follows the port.
    const auto small = observed.Decode(std::vector<u8>{0xFF, 0x80 | 0x40 | 3, 0, 3, 'z'}, kPeer);
    EXPECT_EQ(small.kind, Codec::Kind::Datagram);
    EXPECT_EQ(small.dst_vport, 3);
    EXPECT_EQ(small.payload, Bytes("z"));

    // The digest filters broadcasts like everything else; no broadcast TCP; truncated.
    keys->SetCommunicationId(CommunicationId{'O', 'T', 'H', 'R'});
    EXPECT_EQ(observed.Decode(broadcast, kPeer).kind, Codec::Kind::Invalid);
    keys->SetCommunicationId(std::nullopt);
    EXPECT_EQ(observed.Decode(std::vector<u8>{0xFF, 0x80 | 0x40 | 7, 0, 3}, kPeer).kind,
              Codec::Kind::Invalid);
    EXPECT_EQ(observed.Decode(std::vector<u8>{0xFF, 0x80 | 0x40 | 3, 0}, kPeer).kind,
              Codec::Kind::Invalid);

    // Ordinary packets are not reported.
    const int before = reported;
    observed.Decode(codec.EncodeDatagram(1, 2, Bytes("x"), kPeer, {}), kPeer);
    EXPECT_EQ(reported, before);
}

TEST_F(NetP2PCodec, CommunicationIdFilter) {
    FramingCodec sender{std::make_shared<Keyring>()};
    const auto plain = sender.EncodeDatagram(1, 2, Bytes("x"), kPeer, {});
    auto sender_keys = std::make_shared<Keyring>();
    sender_keys->SetCommunicationId(CommunicationId{'O', 'T', 'H', 'R'});
    FramingCodec other_title{sender_keys};
    const auto foreign = other_title.EncodeDatagram(1, 2, Bytes("x"), kPeer, {});
    const auto signaling = other_title.EncodeSignaling(Bytes("s"), kPeer);

    // No ID configured: everything is accepted.
    EXPECT_EQ(codec.Decode(plain, kPeer).kind, Codec::Kind::Datagram);
    EXPECT_EQ(codec.Decode(foreign, kPeer).kind, Codec::Kind::Datagram);

    keys->SetCommunicationId(kComId);
    EXPECT_EQ(codec.Decode(plain, kPeer).kind, Codec::Kind::Invalid);   // missing ID
    EXPECT_EQ(codec.Decode(foreign, kPeer).kind, Codec::Kind::Invalid); // other title
    EXPECT_EQ(codec.Decode(codec.EncodeDatagram(1, 2, Bytes("x"), kPeer, {}), kPeer).kind,
              Codec::Kind::Datagram);
    EXPECT_EQ(codec.Decode(signaling, kPeer).kind, Codec::Kind::Signaling); // exempt
}

// ---------------------------------------------------------------------------------------------
// Protection
// ---------------------------------------------------------------------------------------------

TEST(NetP2PCrypto, LayoutAndRoundTrip) {
    const auto data = Bytes("the quick brown fox");
    for (const bool encrypt : {false, true}) {
        for (const bool sign : {false, true}) {
            std::vector<u8> protected_data, restored;
            ASSERT_TRUE(ProtectPayload(data, encrypt, sign, Key(7), protected_data));
            EXPECT_EQ(protected_data.size(), data.size() + (encrypt ? 4 : 0) + (sign ? 4 : 0));
            if (encrypt) {
                EXPECT_NE(std::vector<u8>(protected_data.end() - data.size(), protected_data.end()),
                          data);
            }
            ASSERT_TRUE(UnprotectPayload(protected_data, encrypt, sign, Key(7), restored));
            EXPECT_EQ(restored, data);
        }
    }
}

TEST(NetP2PCrypto, SignatureDetectsTamperingAndWrongKey) {
    std::vector<u8> protected_data, restored;
    ASSERT_TRUE(ProtectPayload(Bytes("payload"), true, true, Key(7), protected_data));
    EXPECT_FALSE(UnprotectPayload(protected_data, true, true, Key(8), restored));
    protected_data.back() ^= 0x01;
    EXPECT_FALSE(UnprotectPayload(protected_data, true, true, Key(7), restored));
    EXPECT_FALSE(UnprotectPayload(std::vector<u8>(7, 0), true, true, Key(7), restored)); // short
}

TEST(NetP2PCrypto, RandomIvPerPacket) {
    std::vector<u8> a, b;
    ProtectPayload(Bytes("same"), true, false, Key(1), a);
    ProtectPayload(Bytes("same"), true, false, Key(1), b);
    EXPECT_NE(a, b);
}

TEST_F(NetP2PCodec, ProtectedTrafficNeedsMatchingKey) {
    auto sender_keys = std::make_shared<Keyring>();
    FramingCodec sender{sender_keys};
    const Protection both{true, true};

    // Sender without a key falls back to a plain packet (kind 3).
    auto packet = sender.EncodeDatagram(1, 2, Bytes("secret"), kPeer, both);
    EXPECT_EQ(packet[1] & 0x0F, 3);

    sender_keys->SetPeerKey(kPeer, Key(9));
    packet = sender.EncodeDatagram(1, 2, Bytes("secret"), kPeer, both);
    EXPECT_EQ(packet[1] & 0x0F, 3 + 1 + 2);
    const auto stream = sender.EncodeStream(Bytes("segment"), kPeer, both);
    EXPECT_EQ(stream[1] & 0x0F, 7 + 1 + 2);

    // The receiver looks up the key of whoever sent it (the sender is kPeer from its view).
    EXPECT_EQ(codec.Decode(packet, kPeer).kind, Codec::Kind::Invalid); // no key: dropped
    keys->SetPeerKey(kOther, Key(9));
    EXPECT_EQ(codec.Decode(packet, kPeer).kind, Codec::Kind::Invalid); // key for someone else
    keys->SetWildcardKey(Key(1));
    EXPECT_EQ(codec.Decode(packet, kPeer).kind, Codec::Kind::Invalid); // wrong wildcard key
    keys->SetPeerKey(kPeer, Key(9));
    const auto d = codec.Decode(packet, kPeer);
    EXPECT_EQ(d.kind, Codec::Kind::Datagram);
    EXPECT_EQ(d.payload, Bytes("secret"));
    EXPECT_EQ(codec.Decode(stream, kPeer).payload, Bytes("segment"));
}

TEST(NetP2PKeyring, LookupOrder) {
    Keyring keys;
    EXPECT_FALSE(keys.Find(kPeer));
    keys.SetDefaultKey(Key(1));
    EXPECT_EQ(keys.Find(kPeer), Key(1));
    keys.SetWildcardKey(Key(2));
    EXPECT_EQ(keys.Find(kPeer), Key(2));
    keys.SetPeerKey(kPeer, Key(3));
    EXPECT_EQ(keys.Find(kPeer), Key(3));
    EXPECT_EQ(keys.Find(kOther), Key(2));
    keys.RemovePeerKey(kPeer);
    EXPECT_EQ(keys.Find(kPeer), Key(2));
}

TEST_F(NetP2PCodec, PseudoHeaderUsesInterfaceAddress) {
    const auto p = codec.StreamPseudoHeader(Endpoint::IPv4("127.0.0.1", 9));
    EXPECT_EQ(p.family, AF_INET);
    EXPECT_EQ((std::array<u8, 4>{p.local[0], p.local[1], p.local[2], p.local[3]}),
              (std::array<u8, 4>{127, 0, 0, 1}));
    EXPECT_EQ((std::array<u8, 4>{p.remote[0], p.remote[1], p.remote[2], p.remote[3]}),
              (std::array<u8, 4>{127, 0, 0, 1}));
}

} // namespace
