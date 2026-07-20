// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "common/types.h"

namespace AmdGpu {

/// Reassembles direct ASC PM4 packets exposed through multiple DingDong ring fragments.
/// The state belongs to one virtual queue and must not be reused for indirect buffers.
class Pm4PacketAssembler {
public:
    enum class Status {
        Complete,
        Incomplete,
        Invalid,
    };

    struct Result {
        Status status{Status::Invalid};
        size_t consumed_dwords{};
        std::vector<u32> completed_packet;
    };

    [[nodiscard]] Result Consume(std::span<const u32> fragment, size_t packet_dwords);
    [[nodiscard]] bool HasPendingPacket() const noexcept;
    [[nodiscard]] std::span<const u32> PendingPacket() const noexcept;

private:
    std::vector<u32> pending_packet;
};

} // namespace AmdGpu
