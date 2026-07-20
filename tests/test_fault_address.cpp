// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <limits>

#include <gtest/gtest.h>

#include "video_core/buffer_cache/fault_address.h"

namespace VideoCore {

TEST(FaultAddress, RejectsNullAddressEvenWhenReportedMapped) {
    EXPECT_FALSE(IsFaultAddressCacheable(0, true));
}

TEST(FaultAddress, RejectsUnmappedAddress) {
    EXPECT_FALSE(IsFaultAddressCacheable(0x3F800000, false));
}

TEST(FaultAddress, AcceptsMappedGuestAddress) {
    EXPECT_TRUE(IsFaultAddressCacheable(0x200000000, true));
}

TEST(FaultAddress, FaultCountUsesAvailablePayloadSlots) {
    EXPECT_EQ(ClampFaultCount(7, 1024), 7);
    EXPECT_EQ(ClampFaultCount(1024, 1024), 1023);
    EXPECT_EQ(ClampFaultCount(std::numeric_limits<u64>::max(), 1024), 1023);
}

TEST(FaultAddress, FaultCountHandlesEmptyArea) {
    EXPECT_EQ(ClampFaultCount(1, 0), 0);
}

} // namespace VideoCore
