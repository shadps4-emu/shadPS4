// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/types.h"

namespace Core::Net::P2P {
class Keyring;
}

namespace Libraries::Net {

// P2P runs over one shared UDP socket, started lazily on first use.

// udp_port is host order, 0 = any. advertised_addr is network order, 0 = find via STUN.
// Defaults are 3658 and 0.
void ConfigureP2P(u16 udp_port, u32 advertised_addr);
// Restarts the transport. Returns the bound port, 0 on failure.
u16 StartP2P(u16 udp_port);
// Open P2P sockets keep the old transport until closed.
void StopP2P();
// Filled by sceNetIoctl from the NP libraries.
std::shared_ptr<Core::Net::P2P::Keyring> GetP2PKeyring();

// Used by NP signaling/matching.
bool EnsureP2PTransport();
bool P2PTransportIsReady();
u16 GetP2PConfiguredPort(); // host order
u16 GetP2PBoundPort();      // host order, 0 when not running
// Network order, 0 if unknown.
u32 GetP2PAdvertisedAddr();
// Host order. Bound port once running, configured port before.
u16 GetP2PAdvertisedPort();

// Private channels between shadPS4 instances on the signaling vport. Packets start with "SHAD"
// and a type byte: 0x10 control, 0x21 matching2, anything else signaling. Addresses/ports are
// network order. Recv doesn't block and returns -1 when empty.
int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);

} // namespace Libraries::Net
