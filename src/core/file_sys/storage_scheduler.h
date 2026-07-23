// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <span>
#include <vector>

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
    bool unlimited_sequential_read_speed{};

    [[nodiscard]] constexpr bool IsEnabled() const {
        return bandwidth_mibps != 0;
    }
};

[[nodiscard]] constexpr u8 StoragePriorityIndex(s32 priority) {
    return static_cast<u8>(std::clamp(priority, -128, 127) + 128);
}

class StorageTimingModel {
public:
    static constexpr u64 MaxChunkSize = 512 * 1024;
    static constexpr auto AverageSeek = std::chrono::milliseconds{13};
    static constexpr auto AverageRotation = std::chrono::microseconds{5'556};

    explicit constexpr StorageTimingModel(StorageSchedulerConfig config) : config{config} {}

    [[nodiscard]] constexpr std::chrono::nanoseconds TransferDuration(u64 bytes) const {
        if (!config.IsEnabled() || bytes == 0) {
            return {};
        }
        const u64 bytes_per_second = static_cast<u64>(config.bandwidth_mibps) * 1024ULL * 1024ULL;
        const u64 whole_seconds = bytes / bytes_per_second;
        const u64 remainder = bytes % bytes_per_second;
        return std::chrono::seconds{whole_seconds} +
               std::chrono::nanoseconds{remainder * 1'000'000'000ULL / bytes_per_second};
    }

    [[nodiscard]] constexpr std::chrono::nanoseconds ServiceDuration(
        u64 bytes, bool sequential, u32 slowdown_percent = 100) const {
        const auto transfer = sequential && config.unlimited_sequential_read_speed
                                  ? std::chrono::nanoseconds::zero()
                                  : TransferDuration(bytes);
        const auto positioning =
            sequential ? std::chrono::nanoseconds::zero() : AverageSeek + AverageRotation;
        const auto duration = transfer + positioning;
        return duration * (config.disable_time_stretching ? 100 : slowdown_percent) / 100;
    }

private:
    StorageSchedulerConfig config;
};

struct StorageReadSpan {
    void* data{};
    u64 size{};
};

using StorageReadSpans = std::vector<StorageReadSpan>;

class StorageRequest {
public:
    [[nodiscard]] bool IsCanceled() const;

private:
    friend class StorageScheduler;
    void Cancel();
    std::atomic_bool canceled{};
};

using StorageRequestHandle = std::shared_ptr<StorageRequest>;
using StorageCompletion = std::function<void(s64 result, bool canceled)>;

struct StorageSchedulerStats {
    u64 bytes_read{};
    u64 chunks{};
    u64 sequential_chunks{};
    u64 positioned_chunks{};
    u64 modeled_wait_ns{};
    u64 timer_oversleep_ns{};
    u64 host_overrun_ns{};
    u64 host_wait_ns{};
    u64 prefetched_chunks{};
    u64 demand_chunks{};
    u64 max_staging_buffers{};
    u64 max_queue_depth{};
};

class StorageScheduler {
public:
    StorageScheduler();
    ~StorageScheduler();

    StorageScheduler(const StorageScheduler&) = delete;
    StorageScheduler& operator=(const StorageScheduler&) = delete;

    [[nodiscard]] StorageSchedulerConfig Configure(StorageSchedulerConfig config);
    [[nodiscard]] bool IsEnabled() const;

    StorageRequestHandle SubmitRead(std::shared_ptr<File> file, StorageReadSpans spans, u64 offset,
                                    s32 priority, StorageCompletion completion);
    s64 ReadBlocking(std::shared_ptr<File> file, std::span<const StorageReadSpan> spans, u64 offset,
                     s32 priority = 0);
    void Cancel(const StorageRequestHandle& request);
    [[nodiscard]] StorageSchedulerStats GetStats() const;

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
