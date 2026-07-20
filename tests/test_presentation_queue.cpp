// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "common/thread.h"
#include "core/libraries/videoout/presentation_queue.h"

namespace Libraries::VideoOut {
namespace {

TEST(PresentationQueue, FifoPreservesSubmissionOrder) {
    PresentationQueue<int> queue{PresentationQueuePolicy::Fifo};

    EXPECT_FALSE(queue.Push(1).has_value());
    EXPECT_FALSE(queue.Push(2).has_value());
    EXPECT_FALSE(queue.Push(3).has_value());
    ASSERT_EQ(queue.Size(), 3);

    EXPECT_EQ(queue.Pop(), 1);
    EXPECT_EQ(queue.Pop(), 2);
    EXPECT_EQ(queue.Pop(), 3);
    EXPECT_TRUE(queue.Empty());
}

TEST(PresentationQueue, MailboxReplacesPendingFrame) {
    PresentationQueue<int> queue{PresentationQueuePolicy::Mailbox};

    EXPECT_FALSE(queue.Push(1).has_value());
    const auto first_replaced = queue.Push(2);
    ASSERT_TRUE(first_replaced.has_value());
    EXPECT_EQ(*first_replaced, 1);

    const auto second_replaced = queue.Push(3);
    ASSERT_TRUE(second_replaced.has_value());
    EXPECT_EQ(*second_replaced, 2);
    ASSERT_EQ(queue.Size(), 1);
    EXPECT_EQ(queue.Pop(), 3);
}

TEST(PresentationQueue, DrainReturnsAllFifoFrames) {
    PresentationQueue<int> queue{PresentationQueuePolicy::Fifo};
    EXPECT_FALSE(queue.Push(4).has_value());
    EXPECT_FALSE(queue.Push(5).has_value());

    const auto drained = queue.Drain();
    ASSERT_EQ(drained.size(), 2);
    EXPECT_EQ(drained[0], 4);
    EXPECT_EQ(drained[1], 5);
    EXPECT_TRUE(queue.Empty());
}

TEST(AccurateTimer, CatchUpPreservesBoundedTimingDebt) {
    using namespace std::chrono_literals;

    EXPECT_EQ(
        Common::Detail::NormalizePeriodicWait(-35ms, 10ms, 20ms, Common::MissedTickPolicy::CatchUp),
        -20ms);
}

TEST(AccurateTimer, SkipMissedNeverShortensTheRecoveryInterval) {
    using namespace std::chrono_literals;

    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(-1ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::SkipMissed),
              10ms);
    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(-21ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::SkipMissed),
              10ms);
    EXPECT_EQ(
        Common::Detail::NormalizePeriodicWait(0ms, 10ms, 0ms, Common::MissedTickPolicy::SkipMissed),
        10ms);
}

TEST(AccurateTimer, PreservePhaseDropsMissedMediaTicksWithoutReplayingThem) {
    using namespace std::chrono_literals;

    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(-1ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::PreservePhase),
              9ms);
    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(-21ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::PreservePhase),
              9ms);
    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(-20ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::PreservePhase),
              10ms);
    EXPECT_EQ(Common::Detail::NormalizePeriodicWait(0ms, 10ms, 0ms,
                                                    Common::MissedTickPolicy::PreservePhase),
              10ms);
}

} // namespace
} // namespace Libraries::VideoOut
