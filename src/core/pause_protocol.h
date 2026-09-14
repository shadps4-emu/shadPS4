// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifndef _WIN32

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "common/rdtsc.h"

namespace Core::DebugPause {

enum class State : std::uint8_t {
    Running,
    Paused,
};

struct Snapshot {
    State state;
    std::uint64_t epoch;
};

struct Transition {
    Snapshot snapshot;
    bool changed;
    bool exhausted;
};

struct RollbackRecord {
    bool valid;
    std::uint64_t failed_pause_epoch;
    std::uint64_t running_epoch;
    std::uint64_t uptime;
};

enum class AckWaitResult : std::uint8_t {
    Acknowledged,
    Unregistered,
    TimedOut,
    WaitError,
};

enum class StateWaitResult : std::uint8_t {
    Running,
    Unregistered,
    WaitError,
};

#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
using FailOpenPublicationTestHook = void (*)() noexcept;
inline std::atomic<FailOpenPublicationTestHook> fail_open_publication_test_hook{};

using FailOpenRollbackTestHook = void (*)() noexcept;
inline std::atomic<FailOpenRollbackTestHook> fail_open_rollback_test_hook{};

using FailOpenRecordTestHook = void (*)() noexcept;
inline std::atomic<FailOpenRecordTestHook> fail_open_record_test_hook{};

using FailOpenTimestampTestHook = void (*)() noexcept;
inline std::atomic<FailOpenTimestampTestHook> fail_open_timestamp_test_hook{};

using FailOpenRunningTestHook = void (*)() noexcept;
inline std::atomic<FailOpenRunningTestHook> fail_open_running_test_hook{};

using FailOpenCompletionTestHook = void (*)() noexcept;
inline std::atomic<FailOpenCompletionTestHook> fail_open_completion_test_hook{};

inline void SetFailOpenPublicationTestHook(const FailOpenPublicationTestHook hook) noexcept {
    fail_open_publication_test_hook.store(hook, std::memory_order_release);
}

inline void SetFailOpenRollbackTestHook(const FailOpenRollbackTestHook hook) noexcept {
    fail_open_rollback_test_hook.store(hook, std::memory_order_release);
}

inline void SetFailOpenRecordTestHook(const FailOpenRecordTestHook hook) noexcept {
    fail_open_record_test_hook.store(hook, std::memory_order_release);
}

inline void SetFailOpenTimestampTestHook(const FailOpenTimestampTestHook hook) noexcept {
    fail_open_timestamp_test_hook.store(hook, std::memory_order_release);
}

inline void SetFailOpenRunningTestHook(const FailOpenRunningTestHook hook) noexcept {
    fail_open_running_test_hook.store(hook, std::memory_order_release);
}

inline void SetFailOpenCompletionTestHook(const FailOpenCompletionTestHook hook) noexcept {
    fail_open_completion_test_hook.store(hook, std::memory_order_release);
}
#endif

// The state bit and epoch are one release/acquire publication. Pause callers are serialized so
// every call joins the participant snapshot for the current paused epoch. Resume never takes this
// mutex and can therefore wake targets while a Pause caller is waiting for acknowledgement.
class Protocol final {
public:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "Pause protocol requires lock-free 64-bit atomics");
    static_assert(std::atomic_int::is_always_lock_free,
                  "Pause protocol requires a lock-free publication counter");

    static constexpr std::uint64_t MaximumEpoch = std::numeric_limits<std::uint64_t>::max() >> 1;
    static constexpr std::uint64_t LastPausableRunningEpoch = MaximumEpoch - 1;
    static constexpr std::uint64_t NoRollbackRecord = std::numeric_limits<std::uint64_t>::max();
    static constexpr long NanosecondsPerSecond = 1'000'000'000;

    Protocol() {
        if (sem_init(&rollback_completion_semaphore, 0, 0) != 0) {
            std::abort();
        }
    }

    // A narrow deterministic seam for the near-exhaustion test. Production always starts at
    // Running epoch zero.
    explicit Protocol(const Snapshot initial) : Protocol() {
        if (initial.epoch > MaximumEpoch ||
            (initial.state == State::Paused && initial.epoch == MaximumEpoch)) {
            std::abort();
        }
        state.store(Encode(initial.state, initial.epoch), std::memory_order_relaxed);
    }

    ~Protocol() {
        (void)sem_destroy(&rollback_completion_semaphore);
    }

    std::unique_lock<std::mutex> AcquirePauseRequest() {
        return std::unique_lock{pause_request_mutex};
    }

    std::unique_lock<std::mutex> TryAcquirePauseRequest() {
        return std::unique_lock{pause_request_mutex, std::try_to_lock};
    }

    Snapshot Load() const noexcept {
        return Decode(state.load(std::memory_order_acquire));
    }

    bool RollbackPublicationPending() const noexcept {
        return rollback_publication_count.load(std::memory_order_acquire) != 0;
    }

    RollbackRecord LoadRollbackRecord() const noexcept {
        const auto running_epoch = rollback_running_epoch.load(std::memory_order_acquire);
        if (running_epoch == NoRollbackRecord) {
            return {.valid = false,
                    .failed_pause_epoch = NoRollbackRecord,
                    .running_epoch = NoRollbackRecord,
                    .uptime = NoRollbackRecord};
        }
        return {.valid = true,
                .failed_pause_epoch = rollback_failed_pause_epoch.load(std::memory_order_relaxed),
                .running_epoch = running_epoch,
                .uptime = rollback_uptime.load(std::memory_order_relaxed)};
    }

    void BeginRollbackPublication() noexcept {
        rollback_publication_count.fetch_add(1, std::memory_order_acq_rel);
    }

    void PublishRollbackRecord(const std::uint64_t failed_pause_epoch,
                               const std::uint64_t running_epoch,
                               const std::uint64_t uptime) noexcept {
        rollback_failed_pause_epoch.store(failed_pause_epoch, std::memory_order_relaxed);
        rollback_uptime.store(uptime, std::memory_order_relaxed);
        rollback_running_epoch.store(running_epoch, std::memory_order_release);
    }

    void CompleteRollbackPublication() noexcept {
        const auto previous = rollback_publication_count.fetch_sub(1, std::memory_order_acq_rel);
        if (previous != 1 ||
            rollback_completion_wake_pending.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        if (sem_post(&rollback_completion_semaphore) != 0) {
            rollback_completion_wake_pending.store(false, std::memory_order_release);
        }
    }

    bool WaitForRollbackPublicationFor(const std::chrono::milliseconds timeout) noexcept {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        for (;;) {
            if (!RollbackPublicationPending()) {
                return true;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return false;
            }

            timespec monotonic_deadline{};
            if (clock_gettime(CLOCK_MONOTONIC, &monotonic_deadline) != 0) {
                return false;
            }
            const auto remaining =
                std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
            monotonic_deadline.tv_sec += remaining / NanosecondsPerSecond;
            monotonic_deadline.tv_nsec += remaining % NanosecondsPerSecond;
            if (monotonic_deadline.tv_nsec >= NanosecondsPerSecond) {
                ++monotonic_deadline.tv_sec;
                monotonic_deadline.tv_nsec -= NanosecondsPerSecond;
            }

#if defined(__linux__) && defined(__GLIBC__) && defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 30)
            if (sem_clockwait(&rollback_completion_semaphore, CLOCK_MONOTONIC,
                              &monotonic_deadline) == 0) {
                rollback_completion_wake_pending.store(false, std::memory_order_release);
                continue;
            }
#else
            if (sem_trywait(&rollback_completion_semaphore) == 0) {
                rollback_completion_wake_pending.store(false, std::memory_order_release);
                continue;
            }
            if (errno == EAGAIN) {
                const auto sleep_result =
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &monotonic_deadline, nullptr);
                if (sleep_result == 0 || sleep_result == EINTR) {
                    continue;
                }
                return false;
            }
#endif
            if (errno == EINTR || errno == ETIMEDOUT) {
                continue;
            }
            return false;
        }
    }

    Transition Request(const State desired) noexcept {
        auto current = state.load(std::memory_order_acquire);
        for (;;) {
            const auto snapshot = Decode(current);
            if (snapshot.state == desired) {
                return {.snapshot = snapshot, .changed = false, .exhausted = false};
            }

            // Never publish Paused at MaximumEpoch: every reachable Paused state retains one
            // representable transition to Running. Numeric acknowledgement comparison therefore
            // remains valid for the lifetime of the process and never crosses wrap.
            if ((desired == State::Paused && snapshot.epoch >= LastPausableRunningEpoch) ||
                snapshot.epoch == MaximumEpoch) {
                return {.snapshot = snapshot, .changed = false, .exhausted = true};
            }

            const auto next_epoch = snapshot.epoch + 1;
            const auto next = Encode(desired, next_epoch);
            if (state.compare_exchange_weak(current, next, std::memory_order_release,
                                            std::memory_order_acquire)) {
                return {.snapshot = {.state = desired, .epoch = next_epoch},
                        .changed = true,
                        .exhausted = false};
            }
        }
    }

    // Failure is allowed to roll back only the exact Paused epoch that encountered it. A
    // concurrent Resume (or any newer state) wins the compare-exchange and is never overwritten.
    Transition RollbackPause(const std::uint64_t paused_epoch) noexcept {
        if (paused_epoch >= MaximumEpoch) {
            return {.snapshot = Load(), .changed = false, .exhausted = true};
        }

        auto expected = Encode(State::Paused, paused_epoch);
        const auto running = Encode(State::Running, paused_epoch + 1);
        if (state.compare_exchange_strong(expected, running, std::memory_order_release,
                                          std::memory_order_acquire)) {
            return {.snapshot = {.state = State::Running, .epoch = paused_epoch + 1},
                    .changed = true,
                    .exhausted = false};
        }
        return {.snapshot = Decode(expected), .changed = false, .exhausted = false};
    }

    bool IsPaused() const noexcept {
        return Load().state == State::Paused;
    }

private:
    static constexpr std::uint64_t StateMask = 1;

    static std::uint64_t Encode(const State state, const std::uint64_t epoch) noexcept {
        return (epoch << 1) | (state == State::Paused ? StateMask : 0);
    }

    static Snapshot Decode(const std::uint64_t packed) noexcept {
        return {
            .state = (packed & StateMask) != 0 ? State::Paused : State::Running,
            .epoch = packed >> 1,
        };
    }

    std::mutex pause_request_mutex;
    std::atomic<std::uint64_t> state{};
    std::atomic_int rollback_publication_count{};
    std::atomic_bool rollback_completion_wake_pending{};
    std::atomic<std::uint64_t> rollback_failed_pause_epoch{NoRollbackRecord};
    std::atomic<std::uint64_t> rollback_running_epoch{NoRollbackRecord};
    std::atomic<std::uint64_t> rollback_uptime{NoRollbackRecord};
    sem_t rollback_completion_semaphore{};
};

struct ParticipantTestAccess;

// One record belongs to one native POSIX thread. SIGVTALRM is only an entry notification. Once in
// the handler it remains blocked and state transitions use a coalesced nonblocking pipe wake, so a
// persistent pause handler cannot recursively re-enter itself.
class Participant final {
public:
    static_assert(std::atomic_bool::is_always_lock_free,
                  "Pause handler requires lock-free bool atomics");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "Pause handler requires lock-free 64-bit atomics");
    static_assert(std::atomic_int::is_always_lock_free,
                  "Pause handler requires lock-free int atomics");

    explicit Participant(Protocol& protocol_) : protocol(&protocol_) {
        if (sem_init(&ack_semaphore, 0, 0) != 0) {
            std::abort();
        }
        if (pipe(state_wake_pipe) != 0 || state_wake_pipe[0] >= FD_SETSIZE ||
            state_wake_pipe[1] >= FD_SETSIZE || !SetNonBlocking(state_wake_pipe[0]) ||
            !SetNonBlocking(state_wake_pipe[1]) || !SetCloseOnExec(state_wake_pipe[0]) ||
            !SetCloseOnExec(state_wake_pipe[1])) {
            AbortConstruction();
        }
    }

    Participant(const Participant&) = delete;
    Participant& operator=(const Participant&) = delete;

    ~Participant() {
        CloseStateWakePipe();
        sem_destroy(&ack_semaphore);
    }

    bool IsRegistered() const noexcept {
        return registered.load(std::memory_order_acquire);
    }

    std::uint64_t AcknowledgedEpoch() const noexcept {
        return acknowledged_epoch.load(std::memory_order_acquire);
    }

    int WaitError() const noexcept {
        return wait_error.load(std::memory_order_acquire);
    }

    int StateWakeError() const noexcept {
        return state_wake_error.load(std::memory_order_acquire);
    }

    std::uint64_t FailedPauseEpoch() const noexcept {
        if (fail_open_running_epoch.load(std::memory_order_acquire) == MaximumRecordedEpoch) {
            return MaximumRecordedEpoch;
        }
        return failed_pause_epoch.load(std::memory_order_acquire);
    }

    std::uint64_t FailOpenRunningEpoch() const noexcept {
        return fail_open_running_epoch.load(std::memory_order_acquire);
    }

    bool FailOpenPublicationPending() const noexcept {
        return fail_open_publication_pending.load(std::memory_order_acquire);
    }

    std::uint64_t FailOpenUptime() const noexcept {
        if (fail_open_running_epoch.load(std::memory_order_acquire) == MaximumRecordedEpoch) {
            return MaximumRecordedEpoch;
        }
        return fail_open_uptime.load(std::memory_order_acquire);
    }

    int NotifyStateChange() noexcept {
        if (state_wake_pending.exchange(true, std::memory_order_acq_rel)) {
            return 0;
        }
        const char token = 1;
        const ssize_t result = write(state_wake_pipe[1], &token, sizeof(token));
        if (result == sizeof(token)) {
            return 0;
        }
        const int error = result < 0 ? errno : EIO;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            // EAGAIN means the pipe already contains a wake. Keep the pending bit set so later
            // callers cannot turn the coalescer into an unbounded token producer.
            return 0;
        }
        if (error == EINTR) {
            // The state word is authoritative and the handler has a finite recheck interval. A
            // transiently interrupted wake must not poison the participant or turn a coalesced
            // notification into a false delivery failure.
            state_wake_pending.store(false, std::memory_order_release);
            return 0;
        }
        state_wake_error.store(error != 0 ? error : EIO, std::memory_order_release);
        state_wake_pending.store(false, std::memory_order_release);
        return error != 0 ? error : EIO;
    }

#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
    void InjectStateWaitErrorForTest() noexcept {
        injected_wait_error.store(EBADF, std::memory_order_release);
    }

    void ClearStateWaitErrorForTest() noexcept {
        injected_wait_error.store(0, std::memory_order_release);
        wait_error.store(0, std::memory_order_release);
    }
#endif

    // Called by the target thread, never by a signal handler. Invalidation precedes signal
    // blocking and list removal. An in-flight controller shared_ptr remains a lifetime pin.
    void Unregister() noexcept {
        if (!registered.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        sigset_t signal_set{};
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGVTALRM);
        sigset_t previous_mask{};
        const int block_result = pthread_sigmask(SIG_BLOCK, &signal_set, &previous_mask);
        (void)NotifyStateChange();
        PostAckWake();
        if (block_result == 0) {
            (void)pthread_sigmask(SIG_SETMASK, &previous_mask, nullptr);
        }
    }

    // Used both by the actual SIGVTALRM handler and synchronously by registration. The latter is
    // what prevents insertion after a controller snapshot from crossing into guest execution
    // while the authoritative state is already Paused.
    StateWaitResult SynchronizeState() noexcept {
        sigset_t original_mask{};
        int result = pthread_sigmask(SIG_SETMASK, nullptr, &original_mask);
        if (result != 0) {
            return FailOpen(result);
        }
        auto wait_mask = original_mask;
        if (sigaddset(&wait_mask, SIGVTALRM) != 0) {
            return FinishWait(FailOpen(errno), original_mask);
        }
        result = pthread_sigmask(SIG_SETMASK, &wait_mask, nullptr);
        if (result != 0) {
            return FailOpen(result);
        }

        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                return FinishWait(StateWaitResult::Unregistered, original_mask);
            }

            const auto snapshot = protocol->Load();
            Acknowledge(snapshot.epoch);
            if (!registered.load(std::memory_order_acquire)) {
                return FinishWait(StateWaitResult::Unregistered, original_mask);
            }
            if (snapshot.state == State::Running && !protocol->RollbackPublicationPending()) {
                return FinishWait(StateWaitResult::Running, original_mask);
            }

            // Close the state-change-before-wait race. If Resume changed the word and its signal
            // is delayed or failed, this acquire read avoids entering the wait at all.
            const auto before_wait = protocol->Load();
            const bool publication_pending = protocol->RollbackPublicationPending();
            if (before_wait.state != snapshot.state || before_wait.epoch != snapshot.epoch ||
                (snapshot.state == State::Running && !publication_pending)) {
                continue;
            }

#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
            if (const int injected = injected_wait_error.load(std::memory_order_acquire);
                injected != 0) {
                return FinishWait(FailOpen(injected, snapshot.epoch), original_mask);
            }
#endif
            if (state_wake_pipe[0] < 0) {
                return FinishWait(FailOpen(EBADF, snapshot.epoch), original_mask);
            }
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(state_wake_pipe[0], &read_fds);
            timespec timeout = StateRecheckInterval;
            result =
                pselect(state_wake_pipe[0] + 1, &read_fds, nullptr, nullptr, &timeout, &wait_mask);
            if (result > 0) {
                const int drain_error = DrainStateWake();
                if (drain_error != 0 && drain_error != EINTR) {
                    return FinishWait(FailOpen(drain_error, snapshot.epoch), original_mask);
                }
                continue;
            }
            if (result == 0 || (result < 0 && errno == EINTR)) {
                continue;
            }
            return FinishWait(FailOpen(errno, snapshot.epoch), original_mask);
        }
    }

    void HandleNotification() noexcept {
        const auto depth = handler_depth.fetch_add(1, std::memory_order_acq_rel) + 1;
        auto maximum = maximum_handler_depth.load(std::memory_order_relaxed);
        while (maximum < depth &&
               !maximum_handler_depth.compare_exchange_weak(
                   maximum, depth, std::memory_order_release, std::memory_order_relaxed)) {
        }
        (void)SynchronizeState();
        handler_depth.fetch_sub(1, std::memory_order_release);
    }

    [[nodiscard]] AckWaitResult WaitForAckUntil(
        const std::uint64_t epoch, const std::chrono::steady_clock::time_point deadline) noexcept {
        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                return AckWaitResult::Unregistered;
            }
            if (protocol->RollbackPublicationPending() ||
                wait_error.load(std::memory_order_acquire) != 0 ||
                fail_open_running_epoch.load(std::memory_order_acquire) != MaximumRecordedEpoch ||
                ack_wake_error.load(std::memory_order_acquire) != 0 ||
                state_wake_error.load(std::memory_order_acquire) != 0) {
                return AckWaitResult::WaitError;
            }
            if (acknowledged_epoch.load(std::memory_order_acquire) >= epoch) {
                return AckWaitResult::Acknowledged;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return AckWaitResult::TimedOut;
            }

            timespec monotonic_deadline{};
            if (clock_gettime(CLOCK_MONOTONIC, &monotonic_deadline) != 0) {
                return AckWaitResult::WaitError;
            }
            const auto remaining =
                std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
            monotonic_deadline.tv_sec += remaining / NanosecondsPerSecond;
            monotonic_deadline.tv_nsec += remaining % NanosecondsPerSecond;
            if (monotonic_deadline.tv_nsec >= NanosecondsPerSecond) {
                ++monotonic_deadline.tv_sec;
                monotonic_deadline.tv_nsec -= NanosecondsPerSecond;
            }

#if defined(__linux__) && defined(__GLIBC__) && defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 30)
            if (sem_clockwait(&ack_semaphore, CLOCK_MONOTONIC, &monotonic_deadline) == 0) {
                // A single producer token covers any number of epoch publications. Clearing after
                // consuming closes the producer/consumer handoff; the predicate is always reread.
                ack_wake_pending.store(false, std::memory_order_release);
                continue;
            }
#else
            if (sem_trywait(&ack_semaphore) == 0) {
                ack_wake_pending.store(false, std::memory_order_release);
                continue;
            }
            if (errno == EAGAIN) {
                auto sleep_until = monotonic_deadline;
                const auto slice_ns = MonotonicFallbackSlice.count();
                timespec slice_until{};
                if (clock_gettime(CLOCK_MONOTONIC, &slice_until) != 0) {
                    return AckWaitResult::WaitError;
                }
                slice_until.tv_nsec += slice_ns;
                if (slice_until.tv_nsec >= NanosecondsPerSecond) {
                    ++slice_until.tv_sec;
                    slice_until.tv_nsec -= NanosecondsPerSecond;
                }
                if (slice_until.tv_sec < sleep_until.tv_sec ||
                    (slice_until.tv_sec == sleep_until.tv_sec &&
                     slice_until.tv_nsec < sleep_until.tv_nsec)) {
                    sleep_until = slice_until;
                }
                const int sleep_result =
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_until, nullptr);
                if (sleep_result == 0 || sleep_result == EINTR) {
                    continue;
                }
                return AckWaitResult::WaitError;
            }
#endif
            if (errno == EINTR) {
                continue;
            }
            if (errno == ETIMEDOUT) {
                continue;
            }
            return AckWaitResult::WaitError;
        }
    }

    [[nodiscard]] AckWaitResult WaitForAckFor(const std::uint64_t epoch,
                                              const std::chrono::milliseconds timeout) noexcept {
        return WaitForAckUntil(epoch, std::chrono::steady_clock::now() + timeout);
    }

private:
    static constexpr long NanosecondsPerSecond = 1'000'000'000;
    static constexpr timespec StateRecheckInterval{
        .tv_sec = 0,
        .tv_nsec = 100'000'000,
    };
    static constexpr auto MonotonicFallbackSlice = std::chrono::milliseconds{10};

    static bool SetNonBlocking(const int fd) noexcept {
        const int flags = fcntl(fd, F_GETFL);
        return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
    }

    static bool SetCloseOnExec(const int fd) noexcept {
        const int flags = fcntl(fd, F_GETFD);
        return flags >= 0 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
    }

    [[noreturn]] void AbortConstruction() noexcept {
        CloseStateWakePipe();
        (void)sem_destroy(&ack_semaphore);
        std::abort();
    }

    void CloseStateWakePipe() noexcept {
        if (state_wake_pipe[0] >= 0) {
            (void)close(state_wake_pipe[0]);
            state_wake_pipe[0] = -1;
        }
        if (state_wake_pipe[1] >= 0) {
            (void)close(state_wake_pipe[1]);
            state_wake_pipe[1] = -1;
        }
    }

    StateWaitResult FinishWait(const StateWaitResult result,
                               const sigset_t& original_mask) noexcept {
        const int restore_result = pthread_sigmask(SIG_SETMASK, &original_mask, nullptr);
        if (restore_result != 0 && result != StateWaitResult::WaitError) {
            return FailOpen(restore_result);
        }
        return result;
    }

    int DrainStateWake() noexcept {
        char token{};
        const ssize_t result = read(state_wake_pipe[0], &token, sizeof(token));
        if (result == sizeof(token)) {
            state_wake_pending.store(false, std::memory_order_release);
            return 0;
        }
        const int error = result < 0 ? errno : EIO;
        if (error == EINTR) {
            return EINTR;
        }
        if (error == EAGAIN || error == EWOULDBLOCK) {
            state_wake_pending.store(false, std::memory_order_release);
            return 0;
        }
        return error != 0 ? error : EIO;
    }

    void Acknowledge(const std::uint64_t epoch) noexcept {
        auto current = acknowledged_epoch.load(std::memory_order_relaxed);
        while (current < epoch) {
            if (acknowledged_epoch.compare_exchange_weak(current, epoch, std::memory_order_release,
                                                         std::memory_order_relaxed)) {
                PostAckWake();
                return;
            }
        }
    }

    void PostAckWake() noexcept {
        if (ack_wake_pending.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        if (sem_post(&ack_semaphore) != 0) {
            ack_wake_error.store(errno, std::memory_order_release);
            ack_wake_pending.store(false, std::memory_order_release);
        }
    }

    StateWaitResult FailOpen(const int error, const std::uint64_t /*paused_epoch*/) noexcept {
        const int recorded_error = error != 0 ? error : EIO;
        protocol->BeginRollbackPublication();
        fail_open_publication_pending.store(true, std::memory_order_release);
        auto snapshot = protocol->Load();
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
        if (const auto hook = fail_open_rollback_test_hook.load(std::memory_order_acquire);
            hook != nullptr) {
            hook();
        }
#endif
        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                wait_error.store(recorded_error, std::memory_order_release);
                fail_open_publication_pending.store(false, std::memory_order_release);
                protocol->CompleteRollbackPublication();
                return StateWaitResult::WaitError;
            }
            if (snapshot.state == State::Running) {
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
                if (const auto hook = fail_open_running_test_hook.load(std::memory_order_acquire);
                    hook != nullptr) {
                    hook();
                }
#endif
                wait_error.store(recorded_error, std::memory_order_release);
                Acknowledge(snapshot.epoch);
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
                if (const auto hook =
                        fail_open_completion_test_hook.load(std::memory_order_acquire);
                    hook != nullptr) {
                    hook();
                }
#endif
                fail_open_publication_pending.store(false, std::memory_order_release);
                protocol->CompleteRollbackPublication();
                return StateWaitResult::WaitError;
            }

            const auto failed_epoch = snapshot.epoch;
            const auto rollback = protocol->RollbackPause(failed_epoch);
            if (!rollback.changed) {
                // The error may have been detected from an older snapshot. Never acknowledge the
                // newer Paused state as complete; reread and either roll that epoch back or wait
                // until another actor has made the authoritative state Running/unregistered.
                snapshot = protocol->Load();
                continue;
            }

            // The protocol-owned count gates every handler that observes Running until this
            // release-published record is complete. The local fields remain diagnostic mirrors;
            // reconciliation reads the durable protocol record so participant removal cannot
            // erase the accounting boundary.
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
            if (const auto hook = fail_open_timestamp_test_hook.load(std::memory_order_acquire);
                hook != nullptr) {
                hook();
            }
#endif
            const auto fail_open_time = Common::FencedRDTSC();
            failed_pause_epoch.store(failed_epoch, std::memory_order_relaxed);
            fail_open_uptime.store(fail_open_time, std::memory_order_relaxed);
            protocol->PublishRollbackRecord(failed_epoch, rollback.snapshot.epoch, fail_open_time);
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
            if (const auto hook = fail_open_record_test_hook.load(std::memory_order_acquire);
                hook != nullptr) {
                hook();
            }
#endif
            fail_open_running_epoch.store(rollback.snapshot.epoch, std::memory_order_release);
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
            if (const auto hook = fail_open_publication_test_hook.load(std::memory_order_acquire);
                hook != nullptr) {
                hook();
            }
#endif
            wait_error.store(recorded_error, std::memory_order_release);
            if (registered.load(std::memory_order_acquire)) {
                Acknowledge(rollback.snapshot.epoch);
            }
            fail_open_publication_pending.store(false, std::memory_order_release);
            protocol->CompleteRollbackPublication();
            return StateWaitResult::WaitError;
        }
    }

    StateWaitResult FailOpen(const int error) noexcept {
        const auto snapshot = protocol->Load();
        return FailOpen(error, snapshot.epoch);
    }

    friend struct ParticipantTestAccess;

    Protocol* protocol;
    std::atomic_bool registered{true};
    std::atomic<std::uint64_t> acknowledged_epoch{};
    std::atomic_bool ack_wake_pending{};
    std::atomic_int ack_wake_error{};
    std::atomic_int wait_error{};
    std::atomic_int state_wake_error{};
    std::atomic_bool state_wake_pending{};
    std::atomic<std::uint64_t> failed_pause_epoch{MaximumRecordedEpoch};
    std::atomic<std::uint64_t> fail_open_running_epoch{MaximumRecordedEpoch};
    std::atomic<std::uint64_t> fail_open_uptime{MaximumRecordedEpoch};
    std::atomic_bool fail_open_publication_pending{};
    std::atomic_int handler_depth{};
    std::atomic_int maximum_handler_depth{};
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
    std::atomic_int injected_wait_error{};
#endif
    sem_t ack_semaphore{};
    int state_wake_pipe[2]{-1, -1};

    static constexpr std::uint64_t MaximumRecordedEpoch = std::numeric_limits<std::uint64_t>::max();
};

static_assert(std::atomic<Participant*>::is_always_lock_free,
              "Pause handler requires a lock-free participant pointer");

inline thread_local std::atomic<Participant*> CurrentParticipant{};

inline void BindCurrentParticipant(Participant* const participant) noexcept {
    CurrentParticipant.store(participant, std::memory_order_release);
}

inline void ClearCurrentParticipant(const Participant* const participant) noexcept {
    auto* expected = const_cast<Participant*>(participant);
    (void)CurrentParticipant.compare_exchange_strong(expected, nullptr, std::memory_order_release,
                                                     std::memory_order_relaxed);
}

#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
inline void InjectCurrentParticipantStateWaitErrorForTest() noexcept {
    if (auto* participant = CurrentParticipant.load(std::memory_order_acquire);
        participant != nullptr) {
        participant->InjectStateWaitErrorForTest();
    }
}
#endif

// Installed directly for SIGVTALRM. Keeping this entry point separate from the generic guest
// fault/signal translator ensures the pause path executes only the audited async-signal-safe core.
inline void PauseSignalHandler(int, siginfo_t*, void*) noexcept {
    const int saved_errno = errno;
    if (auto* participant = CurrentParticipant.load(std::memory_order_acquire);
        participant != nullptr) {
        participant->HandleNotification();
    }
    errno = saved_errno;
}

} // namespace Core::DebugPause

#endif // !_WIN32
