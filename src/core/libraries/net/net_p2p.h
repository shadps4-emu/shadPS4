// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/types.h"

namespace Core::Net::P2P {
class Keyring;
}

namespace Libraries::Net {
void ConfigureP2P(u16 udp_port, u32 advertised_addr);
u16 StartP2P(u16 udp_port);
void StopP2P();
std::shared_ptr<Core::Net::P2P::Keyring> GetP2PKeyring();

// Used by NP signaling/matching.
bool EnsureP2PTransport();
bool P2PTransportIsReady();
u16 GetP2PConfiguredPort(); // host order
u16 GetP2PBoundPort();      // host order, 0 when not running
u32 GetP2PAdvertisedAddr(); // Network order, 0 if unknown.
u16 GetP2PAdvertisedPort();
int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);

} // namespace Libraries::Net
