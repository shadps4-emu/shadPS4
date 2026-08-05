// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <semaphore>
#include <thread>
#include <utility>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/file_sys/fs.h"
#include "core/file_sys/storage_scheduler.h"
#include "core/memory.h"

namespace Core::FileSys {
namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

Nanoseconds TransferDuration(StorageSchedulerConfig config, u64 bytes, u32 slowdown_percent) {
    if (!config.IsEnabled() || bytes == 0) {
        return {};
    }
    const u64 bytes_per_second = static_cast<u64>(config.bandwidth_mibps) * 1024ULL * 1024ULL;
    const auto duration =
        std::chrono::seconds{bytes / bytes_per_second} +
        Nanoseconds{bytes % bytes_per_second * 1'000'000'000ULL / bytes_per_second};
    return duration * slowdown_percent / 100;
}

} // namespace

struct StorageScheduler::Impl {
    struct Pending final : StorageRequest {
        std::shared_ptr<File> file;
        StorageReadSpans spans;
        u64 offset{};
        u64 requested{};
        StorageCompletion completion;
    };

    ~Impl() {
        worker.request_stop();
        cv.notify_all();
    }

    Clock::time_point ScheduleTransfer(u64 bytes, StorageSchedulerConfig current_config) {
        const auto now = Clock::now();
        const u32 slowdown = current_config.disable_time_stretching
                                 ? 100
                                 : slowdown_percent.load(std::memory_order_relaxed);
        transfer_cursor = std::max(transfer_cursor, now - BurstDuration);
        transfer_cursor += TransferDuration(current_config, bytes, slowdown);
        return transfer_cursor;
    }

    static void Publish(const Pending& pending, std::span<const u8> data) {
        u64 source_offset{};
        for (const auto& span : pending.spans) {
            const u64 copy_size = std::min<u64>(span.size, data.size() - source_offset);
            if (copy_size == 0) {
                break;
            }
            Core::Memory::Instance()->InvalidateMemory(reinterpret_cast<VAddr>(span.data),
                                                       copy_size);
            std::memcpy(span.data, data.data() + source_offset, copy_size);
            source_offset += copy_size;
        }
    }

    static void Complete(Pending& pending, s64 result, bool canceled) {
        if (auto completion = std::move(pending.completion)) {
            completion(result, canceled);
        }
    }

    void Run(std::stop_token stop_token) {
        Common::SetCurrentThreadName("shadPS4:Hdd");
        std::vector<u8> staging;
        while (!stop_token.stop_requested()) {
            std::shared_ptr<Pending> pending;
            {
                std::unique_lock lock{mutex};
                cv.wait(lock, [&] { return stop_token.stop_requested() || !queue.empty(); });
                if (stop_token.stop_requested()) {
                    break;
                }
                // FIOS2 already scheduled its chunks; preserve that submission order.
                pending = std::move(queue.front());
                queue.pop_front();
            }

            if (!pending->Start()) {
                Complete(*pending, -1, true);
                continue;
            }

            Clock::time_point deadline;
            {
                std::scoped_lock lock{mutex};
                const auto current_config = config.load(std::memory_order_acquire);
                deadline = ScheduleTransfer(pending->requested, current_config);
            }

            staging.resize(pending->requested);
            const s64 result =
                pending->file->PRead(staging.data(), staging.size(), pending->offset);
            const size_t read = result > 0 ? static_cast<size_t>(result) : 0;
            {
                std::unique_lock lock{mutex};
                cv.wait_until(lock, deadline, [&] { return stop_token.stop_requested(); });
            }
            if (stop_token.stop_requested()) {
                Complete(*pending, -1, true);
                break;
            }

            if (result >= 0) {
                Publish(*pending, std::span<const u8>{staging.data(), read});
            }
            Complete(*pending, result, false);
        }

        std::deque<std::shared_ptr<Pending>> canceled;
        {
            std::scoped_lock lock{mutex};
            canceled.swap(queue);
        }
        for (auto& pending : canceled) {
            Complete(*pending, -1, true);
        }
    }

    void ReportGuestFlip(Nanoseconds expected_period) {
        const auto now = Clock::now();
        if (expected_period <= Nanoseconds::zero()) {
            return;
        }
        if (last_flip_time == Clock::time_point{} ||
            now - last_flip_time > std::chrono::seconds{2}) {
            last_flip_time = now;
            flip_period_ema = Nanoseconds::zero();
            slowdown_percent.store(100, std::memory_order_relaxed);
            return;
        }

        const auto period = std::chrono::duration_cast<Nanoseconds>(now - last_flip_time);
        last_flip_time = now;
        flip_period_ema =
            flip_period_ema == Nanoseconds::zero() ? period : (flip_period_ema * 9 + period) / 10;
        const u64 percent = std::clamp<u64>(static_cast<u64>(flip_period_ema.count()) * 100 /
                                                static_cast<u64>(expected_period.count()),
                                            100, 400);
        slowdown_percent.store(static_cast<u32>((percent + 12) / 25 * 25),
                               std::memory_order_relaxed);
    }

    std::atomic<StorageSchedulerConfig> config{};
    std::atomic<u32> slowdown_percent{100};
    static constexpr auto BurstDuration = std::chrono::milliseconds{5};
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<std::shared_ptr<Pending>> queue;
    Clock::time_point transfer_cursor{};
    Clock::time_point last_flip_time{};
    Nanoseconds flip_period_ema{};
    std::jthread worker;
};

StorageScheduler::StorageScheduler() : impl{std::make_unique<Impl>()} {}

StorageScheduler::~StorageScheduler() = default;

StorageSchedulerConfig StorageScheduler::Configure(StorageSchedulerConfig config) {
    const u32 requested_bandwidth = config.bandwidth_mibps;
    config.bandwidth_mibps = NormalizeReadBandwidth(requested_bandwidth);
    if (config.bandwidth_mibps != requested_bandwidth) {
        LOG_WARNING(Config, "App0 HDD bandwidth {} MiB/s normalized to {} MiB/s",
                    requested_bandwidth, config.bandwidth_mibps);
    }
    impl->config.store(config, std::memory_order_release);
    std::scoped_lock lock{impl->mutex};
    if (config.IsEnabled() && !impl->worker.joinable()) {
        impl->worker =
            std::jthread{[scheduler = impl.get()](std::stop_token stop) { scheduler->Run(stop); }};
    }
    impl->transfer_cursor = {};
    impl->slowdown_percent.store(100, std::memory_order_relaxed);
    impl->last_flip_time = {};
    impl->flip_period_ema = Nanoseconds::zero();
    return config;
}

bool StorageScheduler::IsEnabled() const {
    return impl->config.load(std::memory_order_acquire).IsEnabled();
}

StorageRequestHandle StorageScheduler::SubmitRead(std::shared_ptr<File> file,
                                                  StorageReadSpans spans, u64 offset,
                                                  StorageCompletion completion) {
    auto pending = std::make_shared<Impl::Pending>();
    pending->file = std::move(file);
    pending->spans = std::move(spans);
    pending->offset = offset;
    pending->completion = std::move(completion);
    for (const auto& span : pending->spans) {
        pending->requested += span.size;
    }

    if (pending->requested == 0) {
        static_cast<void>(pending->Start());
        Impl::Complete(*pending, 0, false);
        return pending;
    }

    {
        std::scoped_lock lock{impl->mutex};
        impl->queue.push_back(pending);
    }
    impl->cv.notify_one();
    return pending;
}

s64 StorageScheduler::ReadBlocking(std::shared_ptr<File> file,
                                   std::span<const StorageReadSpan> spans, u64 offset) {
    std::binary_semaphore complete{0};
    s64 result{-1};
    SubmitRead(std::move(file), {spans.begin(), spans.end()}, offset, [&](s64 read, bool canceled) {
        result = canceled ? -1 : read;
        complete.release();
    });
    complete.acquire();
    return result;
}

void StorageScheduler::Cancel(const StorageRequestHandle& request) {
    if (request) {
        request->Cancel();
    }
}

void StorageScheduler::ReportGuestFlip(std::chrono::nanoseconds expected_flip_period) {
    if (IsEnabled()) {
        impl->ReportGuestFlip(expected_flip_period);
    }
}

StorageScheduler& GetApp0StorageScheduler() {
    static StorageScheduler scheduler;
    return scheduler;
}

bool ShouldScheduleAppRead(const File& file) {
    return (file.m_guest_name == "/app0" || file.m_guest_name.starts_with("/app0/")) &&
           GetApp0StorageScheduler().IsEnabled();
}

} // namespace Core::FileSys
