// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/logging/log.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_p2p_ioctl.h"
#include "core/libraries/net/net_types.h"
#include "core/net/p2p_codec.h"

namespace Libraries::Net {

namespace P2P = Core::Net::P2P;

namespace {

constexpr size_t AddressSize = sizeof(OrbisNetSockaddrIn); // IPv4 only
constexpr size_t RecordSize = AddressSize + 20;
constexpr size_t KeyOffset = AddressSize;
constexpr size_t LocalPortOffset = RecordSize - 4;
constexpr size_t FlagsOffset = RecordSize - 2;
constexpr size_t IdOffset = RecordSize - 1;
constexpr u16 DefaultKeyPort = 3658; // loopback port that also gets the default key

P2P::KeyEntry ReadEntry(const u8* bytes) {
    P2P::KeyEntry entry;
    std::memcpy(entry.key.value.data(), bytes + KeyOffset, entry.key.value.size());
    std::memcpy(&entry.local_port, bytes + LocalPortOffset, sizeof(entry.local_port));
    entry.flags = bytes[FlagsOffset];
    entry.id = bytes[IdOffset];
    return entry;
}

bool ReadPeer(const u8* bytes, P2P::Endpoint* peer) {
    OrbisNetSockaddrIn in;
    std::memcpy(&in, bytes, sizeof(in));
    if (in.sin_family != ORBIS_NET_AF_INET) {
        return false;
    }
    sockaddr_in host{};
    host.sin_family = AF_INET;
    host.sin_port = in.sin_port;
    std::memcpy(&host.sin_addr, &in.sin_addr, sizeof(in.sin_addr));
    *peer = P2P::Endpoint::FromSockaddr(reinterpret_cast<const sockaddr*>(&host), sizeof(host));
    return true;
}

void WriteKey(u8* bytes, const std::optional<P2P::P2PKey>& key) {
    if (key) {
        std::memcpy(bytes + KeyOffset, key->value.data(), key->value.size());
    } else {
        std::memset(bytes + KeyOffset, 0, sizeof(P2P::P2PKey::value));
    }
}

} // namespace

int P2PKeyIoctl(P2P::Keyring& keyring, u16 np_port, u64 cmd, void* data) {
    const u32 control = static_cast<u32>(cmd);
    if ((control & 0xff00) != 0x5000) {
        return ORBIS_NET_EINVAL;
    }
    if (control == 0x200050c8) {
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: enable (no-op)", control);
        return 0;
    }
    if ((control & 0xff) == 0xd9) {
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: communication ID cleared", control);
        keyring.SetCommunicationId(std::nullopt);
        return 0;
    }
    if (data == nullptr) {
        return ORBIS_NET_EFAULT;
    }
    auto* bytes = static_cast<u8*>(data);
    if (control == 0x802450c8 || control == 0x803450c8) {
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: keys with id {} removed", control, bytes[IdOffset]);
        keyring.RemoveKeysWithId(bytes[IdOffset]);
        return 0;
    }
    // Never log key values.
    const auto peer_text = [&] {
        return FormatSockaddr(reinterpret_cast<const OrbisNetSockaddr*>(bytes), AddressSize);
    };

    switch (control & 0xff) {
    case 0xc9: {
        const auto entry = ReadEntry(bytes);
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: default key set (id {})", control, entry.id);
        keyring.SetDefaultKey(entry.key);
        keyring.AddPeerKey(P2P::Endpoint::IPv4("127.0.0.1", DefaultKeyPort), entry);
        return 0;
    }
    case 0xca:
        WriteKey(bytes, keyring.GetDefaultKey());
        return 0;
    case 0xcb: {
        P2P::Endpoint peer;
        if (!ReadPeer(bytes, &peer)) {
            return ORBIS_NET_EINVAL;
        }
        auto entry = ReadEntry(bytes);
        if (entry.local_port == 0) {
            entry.local_port = np_port;
        }
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: key for peer {} added (id {}, flags {:#x})", control,
                  peer_text(), entry.id, entry.flags);
        keyring.AddPeerKey(peer, entry);
        return 0;
    }
    case 0xcc: {
        P2P::Endpoint peer;
        if (!ReadPeer(bytes, &peer)) {
            return ORBIS_NET_EINVAL;
        }
        const auto entry = ReadEntry(bytes);
        const bool released = keyring.ReleasePeerKey(peer, entry.key, entry.id);
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: key for peer {} (id {}) {}", control, peer_text(),
                  entry.id, released ? "released" : "not installed");
        return released ? 0 : ORBIS_NET_ENOENT;
    }
    case 0xcd:
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: wildcard key set", control);
        keyring.SetWildcardKey(ReadEntry(bytes).key);
        return 0;
    case 0xce:
        WriteKey(bytes, keyring.GetWildcardKey());
        return 0;
    case 0xcf:
    case 0xd0:
        std::memcpy(bytes, &np_port, sizeof(np_port));
        return 0;
    case 0xd8:
        LOG_DEBUG(Lib_Net, "P2P ioctl {:#x}: communication ID set", control);
        keyring.SetCommunicationId(P2P::DeriveCommunicationId(std::span<const u8, 16>(bytes, 16)));
        return 0;
    default:
        LOG_WARNING(Lib_Net, "P2P ioctl {:#x}: unknown command", control);
        return ORBIS_NET_EINVAL;
    }
}

} // namespace Libraries::Net
