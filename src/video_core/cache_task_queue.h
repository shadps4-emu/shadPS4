// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>

#include "common/polyfill_thread.h"

namespace Storage::Detail {

class CacheTaskQueue {
public:
    using Task = std::packaged_task<void()>;

    void StartAccepting() {
        std::scoped_lock lock{mutex};
        accepting = true;
    }

    void StopAccepting() {
        {
            std::scoped_lock lock{mutex};
            accepting = false;
        }
        task_available.notify_all();
    }

    bool Submit(Task&& task) {
        {
            std::scoped_lock lock{mutex};
            if (!accepting) {
                return false;
            }
            tasks.emplace(std::move(task));
        }
        task_available.notify_one();
        return true;
    }

    std::optional<Task> Take(const std::stop_token& stop_token) {
        std::unique_lock lock{mutex};
        Common::CondvarWait(task_available, lock, stop_token, [this] { return !tasks.empty(); });
        if (tasks.empty()) {
            return std::nullopt;
        }

        auto task = std::move(tasks.front());
        tasks.pop();
        return task;
    }

private:
    std::condition_variable_any task_available{};
    std::queue<Task> tasks{};
    std::mutex mutex{};
    bool accepting{};
};

} // namespace Storage::Detail
