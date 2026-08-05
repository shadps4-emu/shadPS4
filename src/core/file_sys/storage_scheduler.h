// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <span>

#include <boost/container/small_vector.hpp>

#include "common/types.h"

namespace Core::FileSys {

struct File;

// Zero and values above 200 select native/unlimited speed. Small non-zero values are clamped to
// 50 MiB/s so a typo cannot make games appear broken. Every value in [50, 200] is accepted.
[[nodiscard]] constexpr u32 NormalizeReadBandwidth(u32 bandwidth_mibps) {
    if (bandwidth_mibps == 0 || bandwidth_mibps > 200) {
        return 0;
    }
    return std::max(bandwidth_mibps, 50u);
}

struct StorageSchedulerConfig {
    u32 bandwidth_mibps{};
    bool disable_time_stretching{};

    [[nodiscard]] constexpr bool IsEnabled() const {
        return bandwidth_mibps != 0;
    }
};

struct StorageReadSpan {
    void* data{};
    u64 size{};
};

using StorageReadSpans = boost::container::small_vector<StorageReadSpan, 4>;

class StorageRequest {
private:
    friend class StorageScheduler;
    [[nodiscard]] bool Start() {
        return !started_or_canceled.test_and_set(std::memory_order_acq_rel);
    }
    void Cancel() {
        started_or_canceled.test_and_set(std::memory_order_acq_rel);
    }
    std::atomic_flag started_or_canceled;
};

using StorageRequestHandle = std::shared_ptr<StorageRequest>;
using StorageCompletion = std::function<void(s64 result, bool canceled)>;

class StorageScheduler {
public:
    StorageScheduler();
    ~StorageScheduler();

    StorageScheduler(const StorageScheduler&) = delete;
    StorageScheduler& operator=(const StorageScheduler&) = delete;

    [[nodiscard]] StorageSchedulerConfig Configure(StorageSchedulerConfig config);
    [[nodiscard]] bool IsEnabled() const;

    StorageRequestHandle SubmitRead(std::shared_ptr<File> file, StorageReadSpans spans, u64 offset,
                                    StorageCompletion completion);
    s64 ReadBlocking(std::shared_ptr<File> file, std::span<const StorageReadSpan> spans,
                     u64 offset);
    void Cancel(const StorageRequestHandle& request);

    // Called from the present thread on every real guest flip. When the emulator runs slower
    // than the game's target flip cadence, modeled I/O time is stretched by the same ratio so
    // the guest-perceived delivery rate per frame stays close to a real PS4 under load.
    void ReportGuestFlip(std::chrono::nanoseconds expected_flip_period);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

StorageScheduler& GetApp0StorageScheduler();

// True when reads from this file must be routed through the app0 storage scheduler.
[[nodiscard]] bool ShouldScheduleAppRead(const File& file);

} // namespace Core::FileSys
