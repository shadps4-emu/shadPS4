// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <atomic>

namespace Common {
template <typename CounterType>
class IncrementalIdProvider {
public:
    IncrementalIdProvider() : counter(0) {}
    ~IncrementalIdProvider() = default;

    CounterType Next() {
        return counter.fetch_add(1, std::memory_order_relaxed) + 1;
    }

private:
    std::atomic<CounterType> counter;
};

} // namespace Common