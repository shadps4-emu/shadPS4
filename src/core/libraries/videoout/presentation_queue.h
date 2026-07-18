// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <utility>

namespace Libraries::VideoOut {

enum class PresentationQueuePolicy {
    Fifo,
    Mailbox,
};

/// Stages guest-latched frames for the host presentation thread.
/// FIFO preserves every frame in order, while Mailbox retains only the latest pending frame.
template <typename T>
class PresentationQueue {
public:
    explicit PresentationQueue(const PresentationQueuePolicy policy) : policy{policy} {}

    [[nodiscard]] std::optional<T> Push(T value) {
        std::optional<T> replaced;
        if (policy == PresentationQueuePolicy::Mailbox && !queue.empty()) {
            replaced = std::move(queue.front());
            queue.pop_front();
        }
        queue.push_back(std::move(value));
        return replaced;
    }

    [[nodiscard]] T Pop() {
        T value = std::move(queue.front());
        queue.pop_front();
        return value;
    }

    [[nodiscard]] std::deque<T> Drain() {
        return std::exchange(queue, {});
    }

    [[nodiscard]] bool Empty() const {
        return queue.empty();
    }

    [[nodiscard]] std::size_t Size() const {
        return queue.size();
    }

private:
    PresentationQueuePolicy policy;
    std::deque<T> queue;
};

} // namespace Libraries::VideoOut
