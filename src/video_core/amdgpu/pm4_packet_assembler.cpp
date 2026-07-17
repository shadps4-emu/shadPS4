// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/amdgpu/pm4_packet_assembler.h"

#include <algorithm>
#include <utility>

namespace AmdGpu {

Pm4PacketAssembler::Result Pm4PacketAssembler::Consume(std::span<const u32> fragment,
                                                       size_t packet_dwords) {
    if (packet_dwords == 0 || fragment.empty()) {
        return {Status::Invalid, 0, {}};
    }

    if (pending_packet.empty()) {
        if (packet_dwords <= fragment.size()) {
            return {Status::Complete, packet_dwords, {}};
        }

        // The PM4 header provides the final packet size up front. Reserving it avoids repeatedly
        // reallocating when DingDong exposes a large packet through several small ring fragments.
        pending_packet.reserve(packet_dwords);
        pending_packet.assign(fragment.begin(), fragment.end());
        return {Status::Incomplete, fragment.size(), {}};
    }

    if (pending_packet.size() >= packet_dwords) {
        return {Status::Invalid, 0, {}};
    }

    const size_t remaining_dwords = packet_dwords - pending_packet.size();
    const size_t consumed_dwords = std::min(remaining_dwords, fragment.size());
    pending_packet.insert(pending_packet.end(), fragment.begin(),
                          fragment.begin() + consumed_dwords);

    if (consumed_dwords < remaining_dwords) {
        return {Status::Incomplete, consumed_dwords, {}};
    }

    // Transfer ownership before the caller executes the packet. Queue-owned pending state must be
    // empty if the packet recursively enters an indirect buffer on the same virtual queue.
    return {Status::Complete, consumed_dwords, std::exchange(pending_packet, std::vector<u32>{})};
}

bool Pm4PacketAssembler::HasPendingPacket() const noexcept {
    return !pending_packet.empty();
}

std::span<const u32> Pm4PacketAssembler::PendingPacket() const noexcept {
    return pending_packet;
}

} // namespace AmdGpu
