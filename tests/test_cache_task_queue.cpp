// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <cstdint>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "video_core/cache_task_queue.h"

namespace {

using Storage::Detail::CacheTaskQueue;

TEST(CacheTaskQueue, StopDrainsPendingTasksAndRejectsNewWork) {
    CacheTaskQueue queue{};
    queue.StartAccepting();

    std::atomic<std::uint32_t> completed{};
    std::jthread worker{[&](const std::stop_token& stop_token) {
        while (auto task = queue.Take(stop_token)) {
            (*task)();
        }
    }};

    constexpr std::uint32_t NumTasks = 256;
    for (std::uint32_t i = 0; i < NumTasks; ++i) {
        ASSERT_TRUE(queue.Submit(std::packaged_task<void()>{[&] { ++completed; }}));
    }

    queue.StopAccepting();
    worker.request_stop();
    worker.join();

    EXPECT_EQ(completed.load(), NumTasks);
    EXPECT_FALSE(queue.Submit(std::packaged_task<void()>{[] {}}));
}

TEST(CacheTaskQueue, CanRestartAfterDraining) {
    CacheTaskQueue queue{};
    std::atomic<std::uint32_t> completed{};

    for (std::uint32_t cycle = 0; cycle < 2; ++cycle) {
        queue.StartAccepting();
        std::jthread worker{[&](const std::stop_token& stop_token) {
            while (auto task = queue.Take(stop_token)) {
                (*task)();
            }
        }};

        ASSERT_TRUE(queue.Submit(std::packaged_task<void()>{[&] { ++completed; }}));
        queue.StopAccepting();
        worker.request_stop();
        worker.join();
    }

    EXPECT_EQ(completed.load(), 2u);
}

} // namespace
