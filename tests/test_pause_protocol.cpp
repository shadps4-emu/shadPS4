// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <condition_variable>
#include <latch>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

namespace {

// This is the old SIGSLEEP shape reduced to a deterministic one-slot signal model. A standard
// signal can represent at most one pending edge here; the handler consumes the first edge and then
// waits for another edge to leave the pause. The test deliberately observes the resulting strand
// before providing a test-only cleanup wake.
class LegacySignalSlot {
public:
    void Notify() {
        {
            std::scoped_lock lock{mutex};
            pending = true;
        }
        cv.notify_all();
    }

    void AllowHandlerEntry() {
        {
            std::scoped_lock lock{mutex};
            entry_allowed = true;
        }
        cv.notify_all();
    }

    void HandleNotification(std::latch& handler_ready, std::latch& handler_waiting) {
        {
            std::unique_lock lock{mutex};
            handler_ready.count_down();
            cv.wait(lock, [&] { return entry_allowed; });
            ASSERT_TRUE(pending);
            pending = false;
        }

        handler_waiting.count_down();
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return pending; });
        pending = false;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool entry_allowed{};
    bool pending{};
};

TEST(PauseProtocolLegacy, PauseThenImmediateResumeCoalescesAndStrands) {
    LegacySignalSlot signal_slot;
    std::latch handler_ready{1};
    std::latch handler_waiting{1};
    std::atomic_bool handler_done{};

    std::jthread target{[&] {
        signal_slot.HandleNotification(handler_ready, handler_waiting);
        handler_done.store(true, std::memory_order_release);
    }};

    handler_ready.wait();

    // Pause and Resume use the same standard signal. The target has not entered the handler, so
    // both edges occupy the single pending slot and the second edge coalesces.
    signal_slot.Notify();
    signal_slot.Notify();
    signal_slot.AllowHandlerEntry();
    handler_waiting.wait();

    // The old handler has consumed the coalesced slot and is now waiting for an edge that Resume
    // already attempted to send. The assertion is bounded by the barrier, not by a hanging join.
    ASSERT_FALSE(handler_done.load(std::memory_order_acquire));

    // Test-only cleanup makes the intentionally stranded model joinable and cannot mask the
    // bounded observation above.
    signal_slot.Notify();
    target.join();
    EXPECT_TRUE(handler_done.load(std::memory_order_acquire));
}

} // namespace
