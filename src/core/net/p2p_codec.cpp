// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "core/net/p2p_codec.h"

namespace Core::Net::P2P {

using namespace Wire;

void Keyring::AddPeerKey(const Endpoint& peer, const KeyEntry& entry) {
    std::scoped_lock lock{mutex};
    const auto it = peer_keys.find(peer);
    if (it != peer_keys.end() && it->second.entry.key == entry.key) {
        ++it->second.references;
    } else {
        peer_keys[peer] = {entry, 1};
    }
}

bool Keyring::ReleasePeerKey(const Endpoint& peer, const P2PKey& key, u8 id) {
    std::scoped_lock lock{mutex};
    const auto it = peer_keys.find(peer);
    if (it == peer_keys.end() || !(it->second.entry.key == key) || it->second.entry.id != id) {
        return false;
    }
    if (--it->second.references == 0) {
        peer_keys.erase(it);
    }
    return true;
}

void Keyring::RemoveKeysWithId(u8 id) {
    std::scoped_lock lock{mutex};
    std::erase_if(peer_keys, [id](const auto& entry) { return entry.second.entry.id == id; });
}

void Keyring::SetPeerKey(const Endpoint& peer, const P2PKey& key) {
    std::scoped_lock lock{mutex};
    peer_keys[peer] = {KeyEntry{key}, 1};
}

void Keyring::RemovePeerKey(const Endpoint& peer) {
    std::scoped_lock lock{mutex};
    peer_keys.erase(peer);
}

void Keyring::SetWildcardKey(std::optional<P2PKey> key) {
    std::scoped_lock lock{mutex};
    wildcard_key = key;
}

std::optional<P2PKey> Keyring::GetWildcardKey() const {
    std::scoped_lock lock{mutex};
    return wildcard_key;
}

void Keyring::SetDefaultKey(std::optional<P2PKey> key) {
    std::scoped_lock lock{mutex};
    default_key = key;
}

std::optional<P2PKey> Keyring::GetDefaultKey() const {
    std::scoped_lock lock{mutex};
    return default_key;
}

std::optional<P2PKey> Keyring::Find(const Endpoint& peer) const {
    std::scoped_lock lock{mutex};
    if (const auto it = peer_keys.find(peer); it != peer_keys.end()) {
        return it->second.entry.key;
    }
    return wildcard_key ? wildcard_key : default_key;
}

std::optional<KeyEntry> Keyring::FindPeerEntry(const Endpoint& peer) const {
    std::scoped_lock lock{mutex};
    if (const auto it = peer_keys.find(peer); it != peer_keys.end()) {
        return it->second.entry;
    }
    return std::nullopt;
}

void Keyring::SetCommunicationId(std::optional<CommunicationId> id) {
    std::scoped_lock lock{mutex};
    communication_id = id;
}

std::optional<CommunicationId> Keyring::GetCommunicationId() const {
    std::scoped_lock lock{mutex};
    return communication_id;
}

FramingCodec::FramingCodec(std::shared_ptr<Keyring> keyring, BroadcastObserver broadcast_observer)
    : keyring(std::move(keyring)), broadcast_observer(std::move(broadcast_observer)) {}

Codec::Decoded FramingCodec::Decode(std::span<const u8> packet, const Endpoint& from) {
    Decoded out;
    if (packet.size() < 2 || packet[0] != Marker || (packet[1] & FlagValid) == 0) {
        return out;
    }
    const u8 flags = packet[1];
    const u8 kind = flags & 0x0F;
    if (kind < KindDatagram || kind > KindStream + 3) {
        return out;
    }
    const bool stream = kind >= KindStream;

    size_t offset = 2;
    u16 src_vport = 0;
    u16 dst_vport = 0;
    if (flags & FlagBroadcast) {
        // Layout unconfirmed, report a few so we can check
        if (broadcast_observer &&
            reported_broadcasts.fetch_add(1, std::memory_order_relaxed) < MaxReportedBroadcasts) {
            broadcast_observer(packet, from);
        }
        if (stream) {
            return out; // no broadcast TCP
        }
        offset = 4;
        if (packet.size() < offset) {
            return out;
        }
        dst_vport = static_cast<u16>((packet[2] << 8) | packet[3]);
        src_vport = dst_vport;
    } else if (!stream) {
        offset = 6;
        if (packet.size() < offset) {
            return out;
        }
        src_vport = static_cast<u16>((packet[2] << 8) | packet[3]);
        dst_vport = static_cast<u16>((packet[4] << 8) | packet[5]);
    }

    std::optional<CommunicationId> communication_id;
    if (flags & FlagDigest) {
        if (packet.size() < offset + 4) {
            return out;
        }
        communication_id.emplace();
        std::memcpy(communication_id->data(), packet.data() + offset, 4);
        offset += 4;
    }

    const bool signaling = !stream && dst_vport == SignalingVport;
    // TODO: drop signaling once the communication ID is set?
    if (!signaling) {
        if (const auto expected = keyring->GetCommunicationId();
            expected && communication_id != expected) {
            return out;
        }
    }

    const u8 mode = kind - (stream ? KindStream : KindDatagram);
    const auto body = packet.subspan(offset);
    if (mode != 0) {
        const auto key = keyring->Find(from);
        if (!key || !UnprotectPayload(body, (mode & 1) != 0, (mode & 2) != 0, *key, out.payload)) {
            return out; // no key or bad signature
        }
    } else {
        out.payload.assign(body.begin(), body.end());
    }

    out.kind = stream ? Kind::Stream : signaling ? Kind::Signaling : Kind::Datagram;
    out.src_vport = src_vport;
    out.dst_vport = dst_vport;
    return out;
}

std::vector<u8> FramingCodec::Protect(std::span<const u8> data, const Endpoint& to,
                                      Protection protection, u8& mode) const {
    mode = 0;
    if (protection.crypto || protection.signature) {
        if (const auto key = keyring->Find(to)) {
            std::vector<u8> out;
            if (ProtectPayload(data, protection.crypto, protection.signature, *key, out)) {
                mode =
                    static_cast<u8>((protection.crypto ? 1 : 0) | (protection.signature ? 2 : 0));
                return out;
            }
        }
        // TODO: this sends plain. Should we drop instead?
    }
    return {data.begin(), data.end()};
}

std::vector<u8> FramingCodec::EncodeStream(std::span<const u8> tcp_segment, const Endpoint& to,
                                           Protection protection) {
    u8 mode = 0;
    const auto body = Protect(tcp_segment, to, protection, mode);
    const auto communication_id = keyring->GetCommunicationId();
    std::vector<u8> packet{Marker, static_cast<u8>(FlagValid | (KindStream + mode) |
                                                   (communication_id ? FlagDigest : 0))};
    if (communication_id) {
        packet.insert(packet.end(), communication_id->begin(), communication_id->end());
    }
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}

std::vector<u8> FrameDatagram(u16 src_vport, u16 dst_vport, u8 kind,
                              const std::optional<CommunicationId>& communication_id,
                              std::span<const u8> body) {
    std::vector<u8> packet{Marker,
                           static_cast<u8>(FlagValid | kind | (communication_id ? FlagDigest : 0)),
                           static_cast<u8>(src_vport >> 8),
                           static_cast<u8>(src_vport),
                           static_cast<u8>(dst_vport >> 8),
                           static_cast<u8>(dst_vport)};
    if (communication_id) {
        packet.insert(packet.end(), communication_id->begin(), communication_id->end());
    }
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}

std::vector<u8> FramingCodec::EncodeDatagram(u16 src_vport, u16 dst_vport,
                                             std::span<const u8> payload, const Endpoint& to,
                                             Protection protection) {
    u8 mode = 0;
    const auto body = Protect(payload, to, protection, mode);
    return FrameDatagram(src_vport, dst_vport, KindDatagram + mode, keyring->GetCommunicationId(),
                         body);
}

std::vector<u8> FramingCodec::EncodeSignaling(std::span<const u8> data, const Endpoint&) {
    return FrameDatagram(SignalingVport, SignalingVport, KindDatagram, std::nullopt, data);
}

PseudoHeader FramingCodec::StreamPseudoHeader(const Endpoint& peer) {
    std::scoped_lock lock{pseudo_mutex};
    if (const auto it = pseudo_cache.find(peer); it != pseudo_cache.end()) {
        return it->second;
    }
    PseudoHeader pseudo;
    pseudo.family = peer.Family();
    const bool v6 = peer.Family() == AF_INET6;
    const size_t size = v6 ? 16 : 4;
    const auto address_of = [v6](const sockaddr_storage& ss) -> const void* {
        return v6 ? static_cast<const void*>(&reinterpret_cast<const sockaddr_in6*>(&ss)->sin6_addr)
                  : static_cast<const void*>(&reinterpret_cast<const sockaddr_in*>(&ss)->sin_addr);
    };
    std::memcpy(pseudo.remote.data(), address_of(peer.addr), size);

    Host::Error error;
    const auto probe = Host::CreateSocket(peer.Family(), SOCK_DGRAM, 0, &error);
    if (probe != Host::InvalidSocket) {
        sockaddr_storage local{};
        socklen_t local_len = sizeof(local);
        if (Host::Connect(probe, peer.Sockaddr(), peer.Length()) == Host::Error::Ok &&
            getsockname(probe, reinterpret_cast<sockaddr*>(&local), &local_len) == 0) {
            std::memcpy(pseudo.local.data(), address_of(local), size);
        }
        Host::CloseSocket(probe);
    }
    if (pseudo_cache.size() >= 256) {
        pseudo_cache.clear();
    }
    pseudo_cache.emplace(peer, pseudo);
    return pseudo;
}

} // namespace Core::Net::P2P
