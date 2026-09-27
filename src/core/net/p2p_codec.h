// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The PS4 P2P wire format, ported from the reverse-engineered implementation.
//
//   Datagram:  [0xFF][flags|kind][src vport (2)][dst vport (2)][digest (4)?][payload]
//              vports in network byte order (p2p_vport, p2p_sin_vport).
//   Broadcast: [0xFF][flags|0x40|kind][port (2)][digest (4)?][payload] (datagrams only)
//   Stream:    [0xFF][flags|kind][digest (4)?][TCP segment]
//   Signaling: a plain datagram from and to vport 0xFFFF.
//
//   flags (t_p2p_proto_flags): 0x80 P2P_VALID, 0x40 P2P_BROADCAST, 0x20 P2P_DIGEST.
//   kind:  P2P_UDP_SIMPLE 3, _CRYPTO 4, _SIGNTR 5, _CRYPTO_SIGNTR 6,
//          P2P_TCP_SIMPLE 7, _CRYPTO 8, _SIGNTR 9, _CRYPTO_SIGNTR 10 (see p2p_crypto.h).
//   digest: the title's 4-byte tag, HMAC-SHA1 of the 16-byte value NP installs (ioctl 0xd8),
//           truncated; called the communication ID in this code.
//

#pragma once

#include <map>
#include <mutex>
#include <optional>

#include "core/net/p2p_crypto.h"
#include "core/net/p2p_transport.h"

namespace Core::Net::P2P {

namespace Wire {
constexpr u8 Marker = 0xFF;
constexpr u8 FlagValid = 0x80;     // P2P_VALID
constexpr u8 FlagBroadcast = 0x40; // P2P_BROADCAST
constexpr u8 FlagDigest = 0x20;    // P2P_DIGEST: the 4-byte communication ID follows
constexpr u8 KindDatagram = 3;
constexpr u8 KindStream = 7;
constexpr u16 SignalingVport = 0xFFFF;
} // namespace Wire

using CommunicationId = std::array<u8, 4>;

/// Keys for protected traffic and the title's communication ID.
struct KeyEntry {
    P2PKey key;
    u16 local_port = 0; // network byte order, as the ioctl carries it
    u8 flags = 0;
    u8 id = 0; // groups keys for removal
};

class Keyring {
public:
    /// Installs entry for peer, replacing any other key. When the same key value is
    /// already installed there, adds a reference instead (NP installs a key once per user).
    void AddPeerKey(const Endpoint& peer, const KeyEntry& entry);
    /// Drops one reference to the key installed for peer. It goes away with the last one.
    /// False when no key with that value and id is installed there.
    bool ReleasePeerKey(const Endpoint& peer, const P2PKey& key, u8 id);
    /// Removes every peer key installed with id.
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
    /// Sees a dropped P2P_BROADCAST packet (the whole UDP payload) and its sender.
    using BroadcastObserver = std::function<void(std::span<const u8>, const Endpoint&)>;
    static constexpr int MaxReportedBroadcasts = 16; // per codec, so per P2P session

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
    /// Applies the requested protection if a key exists,reports the kind offset used.
    std::vector<u8> Protect(std::span<const u8> data, const Endpoint& to, Protection protection,
                            u8& mode) const;

    std::shared_ptr<Keyring> keyring;
    BroadcastObserver broadcast_observer; // for debugging broadcasts
    std::atomic<int> reported_broadcasts{0};
    std::mutex pseudo_mutex;
    std::map<Endpoint, PseudoHeader> pseudo_cache;
};

} // namespace Core::Net::P2P
