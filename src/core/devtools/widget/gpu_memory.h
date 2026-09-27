//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "common/types.h"

namespace Core::Devtools::Widget {

/// Memory used by one memory type within a group.
struct GpuMemoryRow {
    std::string name;
    u64 bytes{};
};

/// Stats published by one subsystem.
struct GpuMemoryGroup {
    std::string name;
    std::vector<GpuMemoryRow> rows;
};

class GpuMemoryViewer {
public:
    bool open = false;

    void Draw();

    static bool IsEnabled() {
        const s64 last = last_drawn_ms.load(std::memory_order_relaxed);
        return last != 0 && NowMs() - last < ENABLED_TIMEOUT_MS;
    }

    static void Publish(GpuMemoryGroup group) {
        std::string name = group.name;
        std::scoped_lock lock{mutex};
        groups.insert_or_assign(std::move(name), std::move(group));
    }

private:
    static constexpr s64 ENABLED_TIMEOUT_MS = 500;

    static s64 NowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static inline std::atomic<s64> last_drawn_ms{0};
    static inline std::mutex mutex;
    static inline std::map<std::string, GpuMemoryGroup> groups;
};

} // namespace Core::Devtools::Widget
