// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <deque>
#include <list>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
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

constexpr u64 MaxChunkSize = StorageTimingModel::MaxChunkSize;
constexpr size_t HostWorkerCount = 2;
constexpr size_t StagingBufferCount = 8;

struct ContinuationKey {
    const File* file{};
    u64 offset{};

    bool operator==(const ContinuationKey&) const = default;
};

struct ContinuationKeyHash {
    std::size_t operator()(const ContinuationKey& key) const noexcept {
        const auto pointer = reinterpret_cast<std::uintptr_t>(key.file);
        return std::hash<std::uintptr_t>{}(pointer) ^ (std::hash<u64>{}(key.offset) << 1);
    }
};

} // namespace

void StorageRequest::Cancel() {
    canceled.store(true, std::memory_order_release);
}

bool StorageRequest::IsCanceled() const {
    return canceled.load(std::memory_order_acquire);
}

struct StorageScheduler::Impl {
    struct Pending final : StorageRequest {
        enum class HostState {
            Idle,
            Queued,
            Reading,
            Ready,
        };

        std::shared_ptr<File> file;
        StorageReadSpans spans;
        u64 offset{};
        u64 requested{};
        u64 processed{};
        s32 priority{};
        StorageCompletion completion;
        std::list<std::shared_ptr<Pending>>::iterator queue_iterator;
        HostState host_state{HostState::Idle};
        size_t staging_index{};
        u64 staged_offset{};
        u64 staged_size{};
        size_t staged_read{};
        bool scheduled{};
    };

    struct HostTask {
        std::shared_ptr<Pending> pending;
        bool urgent{};
        u64 sequence{};
    };

    using Queue = std::list<std::shared_ptr<Pending>>;

    void EnsureWorkersStarted() {
        if (worker.joinable()) {
            return;
        }
        for (size_t index = 0; index < StagingBufferCount; ++index) {
            staging_buffers[index].resize(MaxChunkSize);
            free_staging.push_back(index);
        }
        for (auto& host_worker : host_workers) {
            host_worker =
                std::jthread{[this](std::stop_token stop_token) { RunHostWorker(stop_token); }};
        }
        worker = std::jthread{[this](std::stop_token stop_token) { Run(stop_token); }};
    }

    ~Impl() {
        worker.request_stop();
        for (auto& host_worker : host_workers) {
            host_worker.request_stop();
        }
        timer.Notify();
        cv.notify_all();
        host_cv.notify_all();
    }

    void SetBucket(u8 index) {
        active_buckets[index / 64] |= u64{1} << (index % 64);
    }

    void ClearBucketIfEmpty(u8 index) {
        if (queues[index].empty()) {
            active_buckets[index / 64] &= ~(u64{1} << (index % 64));
        }
    }

    std::optional<u8> HighestPriority() const {
        for (s32 word_index = 3; word_index >= 0; --word_index) {
            const u64 word = active_buckets[word_index];
            if (word != 0) {
                return static_cast<u8>(word_index * 64 + 63 - std::countl_zero(word));
            }
        }
        return std::nullopt;
    }

    void Enqueue(const std::shared_ptr<Pending>& pending) {
        const u8 bucket = StoragePriorityIndex(pending->priority);
        auto& queue = queues[bucket];
        queue.push_back(pending);
        pending->scheduled = true;
        pending->queue_iterator = std::prev(queue.end());
        continuations.try_emplace({pending->file.get(), pending->offset + pending->processed},
                                  pending);
        SetBucket(bucket);
        ++queue_depth;
        stats.max_queue_depth = std::max(stats.max_queue_depth, queue_depth);
        QueueHostRead(pending, false);
    }

    void Remove(const std::shared_ptr<Pending>& pending, u8 bucket) {
        const ContinuationKey key{pending->file.get(), pending->offset + pending->processed};
        if (const auto it = continuations.find(key);
            it != continuations.end() && it->second.lock() == pending) {
            continuations.erase(it);
        }
        queues[bucket].erase(pending->queue_iterator);
        pending->scheduled = false;
        ClearBucketIfEmpty(bucket);
        --queue_depth;
    }

    std::shared_ptr<Pending> SelectNext() {
        const auto bucket = HighestPriority();
        if (!bucket) {
            return {};
        }

        auto& queue = queues[*bucket];
        std::shared_ptr<Pending> selected;
        if (head_file != nullptr) {
            const ContinuationKey key{head_file, head_offset};
            if (const auto it = continuations.find(key); it != continuations.end()) {
                auto candidate = it->second.lock();
                if (candidate && StoragePriorityIndex(candidate->priority) == *bucket) {
                    selected = std::move(candidate);
                }
            }
        }
        if (!selected) {
            selected = queue.front();
        }
        Remove(selected, *bucket);
        return selected;
    }

    bool QueueHostRead(const std::shared_ptr<Pending>& pending, bool urgent) {
        if (pending->host_state == Pending::HostState::Queued) {
            if (urgent) {
                for (auto& task : host_tasks) {
                    if (task.pending == pending) {
                        task.urgent = true;
                        break;
                    }
                }
                host_cv.notify_one();
            }
            return true;
        }
        if (pending->host_state != Pending::HostState::Idle) {
            return true;
        }

        // Keep one buffer in reserve so a newly selected high-priority request cannot be
        // blocked by speculative reads for lower-priority work.
        if (free_staging.empty() || (!urgent && free_staging.size() == 1)) {
            return false;
        }

        pending->staging_index = free_staging.back();
        free_staging.pop_back();
        pending->staged_offset = pending->offset + pending->processed;
        pending->staged_size = std::min(MaxChunkSize, pending->requested - pending->processed);
        pending->staged_read = 0;
        pending->host_state = Pending::HostState::Queued;
        host_tasks.push_back({pending, urgent, host_sequence++});
        stats.max_staging_buffers =
            std::max<u64>(stats.max_staging_buffers, StagingBufferCount - free_staging.size());
        host_cv.notify_one();
        return true;
    }

    void PrimeHostReads() {
        while (free_staging.size() > 1) {
            bool queued{};
            for (s32 word_index = 3; word_index >= 0 && !queued; --word_index) {
                u64 word = active_buckets[word_index];
                while (word != 0 && !queued) {
                    const s32 bit = 63 - std::countl_zero(word);
                    word &= ~(u64{1} << bit);
                    for (const auto& pending : queues[word_index * 64 + bit]) {
                        if (!pending->IsCanceled() &&
                            pending->host_state == Pending::HostState::Idle) {
                            queued = QueueHostRead(pending, false);
                            break;
                        }
                    }
                }
            }
            if (!queued) {
                break;
            }
        }
    }

    void ReleaseStaging(const std::shared_ptr<Pending>& pending) {
        free_staging.push_back(pending->staging_index);
        pending->host_state = Pending::HostState::Idle;
        pending->staged_read = 0;
        PrimeHostReads();
    }

    void DiscardPrefetch(const std::shared_ptr<Pending>& pending) {
        if (!pending->scheduled) {
            return;
        }
        if (pending->host_state == Pending::HostState::Queued) {
            const auto task = std::ranges::find(host_tasks, pending, &HostTask::pending);
            if (task != host_tasks.end()) {
                host_tasks.erase(task);
                ReleaseStaging(pending);
            }
        } else if (pending->host_state == Pending::HostState::Ready) {
            ReleaseStaging(pending);
        }
    }

    void RunHostWorker(std::stop_token stop_token) {
        Common::SetCurrentThreadName("shadPS4:HddHost");
        while (!stop_token.stop_requested()) {
            HostTask task;
            {
                std::unique_lock lock{mutex};
                host_cv.wait(lock,
                             [&] { return stop_token.stop_requested() || !host_tasks.empty(); });
                if (stop_token.stop_requested()) {
                    break;
                }
                const auto best = std::max_element(
                    host_tasks.begin(), host_tasks.end(),
                    [](const HostTask& left, const HostTask& right) {
                        if (left.urgent != right.urgent) {
                            return !left.urgent && right.urgent;
                        }
                        if (left.pending->priority != right.pending->priority) {
                            return left.pending->priority < right.pending->priority;
                        }
                        return left.sequence > right.sequence;
                    });
                task = std::move(*best);
                host_tasks.erase(best);
                task.pending->host_state = Pending::HostState::Reading;
            }

            auto& buffer = staging_buffers[task.pending->staging_index];
            const s64 result = task.pending->file->PRead(
                buffer.data(), static_cast<size_t>(task.pending->staged_size),
                task.pending->staged_offset);
            const size_t read = result > 0 ? static_cast<size_t>(result) : 0;

            {
                std::scoped_lock lock{mutex};
                task.pending->staged_read = read;
                task.pending->host_state = Pending::HostState::Ready;
                if (task.pending->IsCanceled()) {
                    DiscardPrefetch(task.pending);
                }
            }
            cv.notify_all();
        }
    }

    void Publish(const Pending& pending, u64 request_offset, std::span<const u8> data) {
        u64 source_offset{};
        u64 logical_offset{};
        for (const auto& span : pending.spans) {
            const u64 span_end = logical_offset + span.size;
            if (request_offset < span_end && source_offset < data.size()) {
                const u64 in_span =
                    request_offset > logical_offset ? request_offset - logical_offset : 0;
                const u64 copy_size =
                    std::min<u64>(span.size - in_span, data.size() - source_offset);
                auto* destination = static_cast<u8*>(span.data) + in_span;
                Core::Memory::Instance()->InvalidateMemory(reinterpret_cast<VAddr>(destination),
                                                           copy_size);
                std::memcpy(destination, data.data() + source_offset, copy_size);
                source_offset += copy_size;
                request_offset += copy_size;
            }
            logical_offset = span_end;
        }
    }

    void Complete(const std::shared_ptr<Pending>& pending, s64 result, bool canceled) {
        auto completion = std::move(pending->completion);
        if (completion) {
            completion(result, canceled);
        }
    }

    void Run(std::stop_token stop_token) {
        Common::SetCurrentThreadName("shadPS4:HddSch");
        while (!stop_token.stop_requested()) {
            std::shared_ptr<Pending> pending;
            bool prefetched{};
            bool host_read_queued{};
            bool canceled_without_host{};
            {
                std::unique_lock lock{mutex};
                cv.wait(lock, [&] { return stop_token.stop_requested() || queue_depth != 0; });
                if (stop_token.stop_requested()) {
                    break;
                }
                pending = SelectNext();
                prefetched = pending->host_state == Pending::HostState::Ready;
                canceled_without_host =
                    pending->IsCanceled() && pending->host_state == Pending::HostState::Idle;
                if (!canceled_without_host) {
                    host_read_queued = QueueHostRead(pending, true);
                }
            }
            if (canceled_without_host) {
                Complete(pending, -1, true);
                continue;
            }
            if (!host_read_queued) {
                LOG_CRITICAL(Kernel_Fs, "Failed to reserve an app0 staging buffer");
                Complete(pending, -1, false);
                continue;
            }

            const u64 chunk_offset = pending->offset + pending->processed;
            const u64 chunk_size = std::min(MaxChunkSize, pending->requested - pending->processed);
            const auto current_config = config.load(std::memory_order_acquire);
            const StorageTimingModel timing{current_config};
            const auto transfer = timing.TransferDuration(chunk_size);
            const u32 slowdown = current_config.disable_time_stretching
                                     ? 100
                                     : slowdown_percent.load(std::memory_order_relaxed);
            const auto credit_lifetime = timing.TransferDuration(MaxChunkSize);

            bool sequential{};
            Nanoseconds applied_credit{};
            {
                std::scoped_lock lock{mutex};
                sequential = head_file == pending->file.get() && head_offset == chunk_offset;
                if (sequential && !current_config.unlimited_sequential_read_speed &&
                    Clock::now() - credit_timestamp <= credit_lifetime) {
                    applied_credit = std::min(timer_credit, transfer);
                    timer_credit -= applied_credit;
                } else {
                    timer_credit = Nanoseconds::zero();
                }
            }

            const auto service_start = Clock::now();
            const auto modeled_duration =
                timing.ServiceDuration(chunk_size, sequential, slowdown) - applied_credit;
            const auto deadline = service_start + modeled_duration;

            size_t staging_index{};
            size_t read{};
            bool stopped{};
            {
                std::unique_lock lock{mutex};
                cv.wait(lock, [&] {
                    return stop_token.stop_requested() ||
                           pending->host_state == Pending::HostState::Ready;
                });
                if (stop_token.stop_requested()) {
                    stopped = true;
                } else {
                    staging_index = pending->staging_index;
                    read = pending->staged_read;
                }
            }
            if (stopped) {
                Complete(pending, -1, true);
                break;
            }
            const auto host_ready = Clock::now();

            while (Clock::now() < deadline && !stop_token.stop_requested() &&
                   !pending->IsCanceled()) {
                timer.WaitUntil(deadline);
            }
            if (stop_token.stop_requested()) {
                Complete(pending, -1, true);
                break;
            }
            if (pending->IsCanceled()) {
                {
                    std::scoped_lock lock{mutex};
                    ReleaseStaging(pending);
                }
                Complete(pending, -1, true);
                continue;
            }

            const auto completed_at = Clock::now();
            Nanoseconds oversleep{};
            // Only compensate timer imprecision. Host latency and positioning time must not
            // make the next sequential transfer artificially faster.
            if (sequential && !current_config.unlimited_sequential_read_speed &&
                host_ready <= deadline && completed_at > deadline) {
                oversleep = std::chrono::duration_cast<Nanoseconds>(completed_at - deadline);
            }
            const Nanoseconds host_overrun =
                host_ready > deadline
                    ? std::chrono::duration_cast<Nanoseconds>(host_ready - deadline)
                    : Nanoseconds::zero();

            const auto& staging = staging_buffers[staging_index];
            Publish(*pending, pending->processed, std::span<const u8>{staging.data(), read});
            pending->processed += read;

            bool emit_window{};
            u64 window_bytes{};
            u64 window_sequential{};
            u64 window_positioned{};
            u64 window_depth{};
            Nanoseconds window_span{};
            {
                std::scoped_lock lock{mutex};
                head_file = pending->file.get();
                head_offset = chunk_offset + read;
                if (oversleep != Nanoseconds::zero()) {
                    timer_credit = std::min(timer_credit + oversleep, credit_lifetime);
                    credit_timestamp = completed_at;
                    stats.timer_oversleep_ns += oversleep.count();
                }
                stats.host_overrun_ns += host_overrun.count();
                stats.host_wait_ns +=
                    std::chrono::duration_cast<Nanoseconds>(host_ready - service_start).count();
                if (prefetched) {
                    ++stats.prefetched_chunks;
                } else {
                    ++stats.demand_chunks;
                }
                ++stats.chunks;
                stats.bytes_read += read;
                stats.modeled_wait_ns += modeled_duration.count();
                if (sequential) {
                    ++stats.sequential_chunks;
                } else {
                    ++stats.positioned_chunks;
                }
                ReleaseStaging(pending);

                if (stats_window_start == Clock::time_point{}) {
                    stats_window_start = completed_at;
                    stats_window_base = stats;
                } else if (completed_at - stats_window_start >= StatsWindowPeriod) {
                    emit_window = true;
                    window_span = completed_at - stats_window_start;
                    window_bytes = stats.bytes_read - stats_window_base.bytes_read;
                    window_sequential =
                        stats.sequential_chunks - stats_window_base.sequential_chunks;
                    window_positioned =
                        stats.positioned_chunks - stats_window_base.positioned_chunks;
                    window_depth = queue_depth;
                    stats_window_start = completed_at;
                    stats_window_base = stats;
                }
            }
            if (emit_window) {
                const double window_seconds = window_span.count() / 1e9;
                LOG_DEBUG(Kernel_Fs,
                          "app0 HDD window: {:.1f} MiB/s sequential={} positioned={} depth={} "
                          "slowdown={}%",
                          window_bytes / (1024.0 * 1024.0) / window_seconds, window_sequential,
                          window_positioned, window_depth, slowdown);
            }

            if (read != chunk_size || pending->processed == pending->requested) {
                Complete(pending, static_cast<s64>(pending->processed), false);
            } else {
                std::scoped_lock lock{mutex};
                Enqueue(pending);
                PrimeHostReads();
                cv.notify_all();
            }
        }

        std::vector<std::shared_ptr<Pending>> canceled;
        {
            std::scoped_lock lock{mutex};
            for (auto& queue : queues) {
                canceled.insert(canceled.end(), queue.begin(), queue.end());
                queue.clear();
            }
            queue_depth = 0;
        }
        for (auto& pending : canceled) {
            Complete(pending, -1, true);
        }
    }

    // Only touched by the present thread; the resulting factor is published through
    // slowdown_percent for the scheduler worker.
    void ReportGuestFlip(Nanoseconds expected_period) {
        const auto now = Clock::now();
        if (expected_period <= Nanoseconds::zero()) {
            return;
        }
        if (last_flip_time == Clock::time_point{} ||
            now - last_flip_time > std::chrono::seconds{2}) {
            // First flip or a long gap (scene change, frozen loading screen): restart the
            // estimate instead of treating the gap as one gigantic frame.
            last_flip_time = now;
            flip_period_ema = Nanoseconds::zero();
            slowdown_percent.store(100, std::memory_order_relaxed);
            return;
        }
        const auto period = std::chrono::duration_cast<Nanoseconds>(now - last_flip_time);
        last_flip_time = now;
        flip_period_ema =
            flip_period_ema == Nanoseconds::zero() ? period : (flip_period_ema * 9 + period) / 10;
        // factor = observed / expected, clamped to [1x, 4x] and quantized to 25% steps so the
        // modeled schedule stays stable within a scene. Never below 1x: running faster than
        // the target cadence must not shrink the modeled HDD time.
        const u64 raw_percent = static_cast<u64>(flip_period_ema.count()) * 100 /
                                static_cast<u64>(expected_period.count());
        const u64 clamped = std::clamp<u64>(raw_percent, 100, 400);
        const u64 quantized = (clamped + 12) / 25 * 25;
        slowdown_percent.store(static_cast<u32>(quantized), std::memory_order_relaxed);
    }

    static constexpr auto StatsWindowPeriod = std::chrono::seconds{5};

    std::atomic<StorageSchedulerConfig> config{};
    std::atomic<u32> slowdown_percent{100};
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::condition_variable host_cv;
    Common::InterruptibleTimer timer;
    std::array<std::vector<u8>, StagingBufferCount> staging_buffers;
    std::vector<size_t> free_staging;
    std::deque<HostTask> host_tasks;
    std::array<Queue, 256> queues;
    std::array<u64, 4> active_buckets{};
    std::unordered_map<ContinuationKey, std::weak_ptr<Pending>, ContinuationKeyHash> continuations;
    u64 queue_depth{};
    u64 host_sequence{};
    const File* head_file{};
    u64 head_offset{};
    Nanoseconds timer_credit{};
    Clock::time_point credit_timestamp{};
    StorageSchedulerStats stats{};
    Clock::time_point stats_window_start{};
    StorageSchedulerStats stats_window_base{};
    Clock::time_point last_flip_time{};
    Nanoseconds flip_period_ema{};
    // Declared last so their implicit joins happen before the state they access is destroyed.
    std::array<std::jthread, HostWorkerCount> host_workers;
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
    if (config.IsEnabled()) {
        impl->EnsureWorkersStarted();
    }
    impl->head_file = nullptr;
    impl->timer_credit = Nanoseconds::zero();
    impl->stats = {};
    impl->stats_window_start = {};
    impl->stats_window_base = {};
    impl->slowdown_percent.store(100, std::memory_order_relaxed);
    impl->last_flip_time = {};
    impl->flip_period_ema = Nanoseconds::zero();
    return config;
}

bool StorageScheduler::IsEnabled() const {
    return impl->config.load(std::memory_order_acquire).IsEnabled();
}

StorageRequestHandle StorageScheduler::SubmitRead(std::shared_ptr<File> file,
                                                  StorageReadSpans spans, u64 offset, s32 priority,
                                                  StorageCompletion completion) {
    auto pending = std::make_shared<Impl::Pending>();
    pending->file = std::move(file);
    pending->spans = std::move(spans);
    pending->offset = offset;
    pending->priority = priority;
    pending->completion = std::move(completion);
    for (const auto& span : pending->spans) {
        pending->requested += span.size;
    }

    if (pending->requested == 0) {
        auto result = std::static_pointer_cast<StorageRequest>(pending);
        auto callback = std::move(pending->completion);
        if (callback) {
            callback(0, false);
        }
        return result;
    }

    // No submission cap here: the 512-record limit is an AIO contract enforced by the
    // AioManager, and synchronous guest reads must never fail because of it.
    {
        std::scoped_lock lock{impl->mutex};
        impl->Enqueue(pending);
    }
    impl->cv.notify_one();
    return std::static_pointer_cast<StorageRequest>(pending);
}

s64 StorageScheduler::ReadBlocking(std::shared_ptr<File> file,
                                   std::span<const StorageReadSpan> spans, u64 offset,
                                   s32 priority) {
    std::mutex result_mutex;
    std::condition_variable result_cv;
    bool complete{};
    s64 result{-1};
    SubmitRead(std::move(file), {spans.begin(), spans.end()}, offset, priority,
               [&](s64 read, bool canceled) {
                   std::scoped_lock lock{result_mutex};
                   result = canceled ? -1 : read;
                   complete = true;
                   result_cv.notify_one();
               });
    std::unique_lock lock{result_mutex};
    result_cv.wait(lock, [&] { return complete; });
    return result;
}

void StorageScheduler::Cancel(const StorageRequestHandle& request) {
    if (request) {
        request->Cancel();
        {
            std::scoped_lock lock{impl->mutex};
            impl->DiscardPrefetch(std::static_pointer_cast<Impl::Pending>(request));
        }
        impl->timer.Notify();
        impl->cv.notify_all();
    }
}

StorageSchedulerStats StorageScheduler::GetStats() const {
    std::scoped_lock lock{impl->mutex};
    return impl->stats;
}

void StorageScheduler::ReportGuestFlip(std::chrono::nanoseconds expected_flip_period) {
    if (!IsEnabled()) {
        return;
    }
    impl->ReportGuestFlip(expected_flip_period);
}

StorageScheduler& GetApp0StorageScheduler() {
    static StorageScheduler scheduler;
    return scheduler;
}

bool ShouldScheduleAppRead(const File& file) {
    return file.storage_class == StorageClass::App0 && GetApp0StorageScheduler().IsEnabled();
}

} // namespace Core::FileSys
