// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include "video_core/amdgpu/pm4_packet_assembler.h"

namespace AmdGpu {
namespace {

using Status = Pm4PacketAssembler::Status;

TEST(Pm4PacketAssemblerTest, CompletesPacketWithoutBuffering) {
    Pm4PacketAssembler assembler;
    const std::array<u32, 5> input{1, 2, 3, 4, 99};

    const auto result = assembler.Consume(input, 4);

    EXPECT_EQ(result.status, Status::Complete);
    EXPECT_EQ(result.consumed_dwords, 4u);
    EXPECT_TRUE(result.completed_packet.empty());
    EXPECT_FALSE(assembler.HasPendingPacket());
}

TEST(Pm4PacketAssemblerTest, CompletesAcrossThreeFragmentsAndLeavesTrailingData) {
    Pm4PacketAssembler assembler;
    const std::vector<u32> packet{10, 11, 12, 13, 14, 15};
    const std::array<u32, 2> first{10, 11};
    const std::array<u32, 1> second{12};
    const std::array<u32, 5> last_with_trailing{13, 14, 15, 90, 91};

    const auto first_result = assembler.Consume(first, packet.size());
    ASSERT_EQ(first_result.status, Status::Incomplete);
    EXPECT_EQ(first_result.consumed_dwords, first.size());

    const auto second_result = assembler.Consume(second, packet.size());
    ASSERT_EQ(second_result.status, Status::Incomplete);
    EXPECT_EQ(second_result.consumed_dwords, second.size());

    const auto final_result = assembler.Consume(last_with_trailing, packet.size());
    EXPECT_EQ(final_result.status, Status::Complete);
    EXPECT_EQ(final_result.consumed_dwords, 3u);
    EXPECT_EQ(final_result.completed_packet, packet);
    EXPECT_FALSE(assembler.HasPendingPacket());
}

TEST(Pm4PacketAssemblerTest, HandlesPacketLargerThanLegacyScratchBuffer) {
    constexpr size_t PacketDwords = 2048;
    Pm4PacketAssembler assembler;
    std::vector<u32> packet(PacketDwords);
    std::iota(packet.begin(), packet.end(), 0u);

    const auto first_result = assembler.Consume(std::span{packet}.first(700), packet.size());
    ASSERT_EQ(first_result.status, Status::Incomplete);

    const auto second_result =
        assembler.Consume(std::span{packet}.subspan(700, 700), packet.size());
    ASSERT_EQ(second_result.status, Status::Incomplete);

    const auto final_result = assembler.Consume(std::span{packet}.subspan(1400), packet.size());
    EXPECT_EQ(final_result.status, Status::Complete);
    EXPECT_EQ(final_result.completed_packet, packet);
    EXPECT_FALSE(assembler.HasPendingPacket());
}

TEST(Pm4PacketAssemblerTest, ReleasesCompletedPacketBeforeNestedDispatch) {
    Pm4PacketAssembler assembler;
    const std::array<u32, 2> outer_start{20, 21};
    const std::array<u32, 2> outer_end{22, 23};
    const std::array<u32, 2> nested_packet{30, 31};

    ASSERT_EQ(assembler.Consume(outer_start, 4).status, Status::Incomplete);
    const auto outer_result = assembler.Consume(outer_end, 4);
    ASSERT_EQ(outer_result.status, Status::Complete);
    ASSERT_FALSE(assembler.HasPendingPacket());

    const auto nested_result = assembler.Consume(nested_packet, nested_packet.size());
    EXPECT_EQ(nested_result.status, Status::Complete);
    EXPECT_EQ(nested_result.consumed_dwords, nested_packet.size());
    EXPECT_FALSE(assembler.HasPendingPacket());
}

TEST(Pm4PacketAssemblerTest, KeepsPendingDataIsolatedPerQueue) {
    Pm4PacketAssembler first_queue;
    Pm4PacketAssembler second_queue;
    const std::array<u32, 2> first_fragment{1, 2};
    const std::array<u32, 3> second_fragment{7, 8, 9};

    ASSERT_EQ(first_queue.Consume(first_fragment, 4).status, Status::Incomplete);
    ASSERT_EQ(second_queue.Consume(second_fragment, 5).status, Status::Incomplete);

    EXPECT_TRUE(std::ranges::equal(first_queue.PendingPacket(), first_fragment));
    EXPECT_TRUE(std::ranges::equal(second_queue.PendingPacket(), second_fragment));
}

TEST(Pm4PacketAssemblerTest, CompletesSingleDwordPacketWithoutPendingState) {
    Pm4PacketAssembler assembler;
    const std::array<u32, 1> type_two_padding{0x80000000u};

    const auto result = assembler.Consume(type_two_padding, 1);

    EXPECT_EQ(result.status, Status::Complete);
    EXPECT_EQ(result.consumed_dwords, 1u);
    EXPECT_FALSE(assembler.HasPendingPacket());
}

} // namespace
} // namespace AmdGpu
