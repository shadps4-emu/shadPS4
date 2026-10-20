// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// PS4 P2P wire format. vports are big endian.
//
// Datagram:  [0xFF][flags|kind][src vport 2][dst vport 2][digest 4?][payload]
// Broadcast: [0xFF][flags|0x40|kind][port 2][digest 4?][payload], datagrams only.
//            Broadcast layout is assumed, not confirmed yet.
// Stream:    [0xFF][flags|kind][digest 4?][TCP segment]
// Signaling: plain datagram from and to vport 0xFFFF.
//
// flags: 0x80 valid, 0x40 broadcast, 0x20 digest.
// kind: UDP 3-6, TCP 7-10. Base +1 for crypto, +2 for signature.
// digest: truncated HMAC-SHA1 of the 16-byte value NP installs (ioctl 0xd8).
// We call it the communication ID.

#pragma once

#include <map>
#include <mutex>
#include <optional>

#include "core/net/p2p_crypto.h"
#include "core/net/p2p_transport.h"

namespace Core::Net::P2P {

namespace Wire {
constexpr u8 Marker = 0xFF;
constexpr u8 FlagValid = 0x80;
constexpr u8 FlagBroadcast = 0x40;
constexpr u8 FlagDigest = 0x20; // communication ID follows
constexpr u8 KindDatagram = 3;
constexpr u8 KindStream = 7;
constexpr u16 SignalingVport = 0xFFFF;
} // namespace Wire

using CommunicationId = std::array<u8, 4>;

struct KeyEntry {
    P2PKey key;
    u16 local_port = 0; // network byte order
    u8 flags = 0;
    u8 id = 0; // groups keys for removal
};

class Keyring {
public:
    // Replaces the peer's key. The same key again just adds a reference,
    // since NP installs it once per user.
    void AddPeerKey(const Endpoint& peer, const KeyEntry& entry);
    // Drops a reference. False if that key and id aren't installed.
    bool ReleasePeerKey(const Endpoint& peer, const P2PKey& key, u8 id);
    void RemoveKeysWithId(u8 id);
    void SetPeerKey(const Endpoint& peer, const P2PKey& key);
    void RemovePeerKey(const Endpoint& peer);
    void SetWildcardKey(std::optional<P2PKey> key);
    std::optional<P2PKey> GetWildcardKey() const;
    void SetDefaultKey(std::optional<P2PKey> key);
    std::optional<P2PKey> GetDefaultKey() const;
    std::optional<P2PKey> Find(const Endpoint& peer) const;
    std::optional<KeyEntry> FindPeerEntry(const Endpoint& peer) const;
    void SetCommunicationId(std::optional<CommunicationId> id);
    std::optional<CommunicationId> GetCommunicationId() const;

private:
    struct Installed {
        KeyEntry entry;
        int references = 1;
    };

    mutable std::mutex mutex;
    std::map<Endpoint, Installed> peer_keys;
    std::optional<P2PKey> wildcard_key;
    std::optional<P2PKey> default_key;
    std::optional<CommunicationId> communication_id;
};

class FramingCodec final : public Codec {
public:
    // Gets dropped broadcast packets, for debugging.
    using BroadcastObserver = std::function<void(std::span<const u8>, const Endpoint&)>;
    static constexpr int MaxReportedBroadcasts = 16; // per session

    explicit FramingCodec(std::shared_ptr<Keyring> keyring,
                          BroadcastObserver broadcast_observer = {});

    Decoded Decode(std::span<const u8> packet, const Endpoint& from) override;
    std::vector<u8> EncodeStream(std::span<const u8> tcp_segment, const Endpoint& to,
                                 Protection protection) override;
    std::vector<u8> EncodeDatagram(u16 src_vport, u16 dst_vport, std::span<const u8> payload,
                                   const Endpoint& to, Protection protection) override;
    std::vector<u8> EncodeSignaling(std::span<const u8> data, const Endpoint& to) override;
    PseudoHeader StreamPseudoHeader(const Endpoint& peer) override;

private:
    // Sends plain if there is no key. mode gets the kind offset used.
    std::vector<u8> Protect(std::span<const u8> data, const Endpoint& to, Protection protection,
                            u8& mode) const;

    std::shared_ptr<Keyring> keyring;
    BroadcastObserver broadcast_observer;
    std::atomic<int> reported_broadcasts{0};
    std::mutex pseudo_mutex;
    std::map<Endpoint, PseudoHeader> pseudo_cache;
};

} // namespace Core::Net::P2P
