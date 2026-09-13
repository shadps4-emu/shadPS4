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
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/select.h>

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

// The state bit and epoch are one release/acquire publication. Pause callers are serialized so
// every call joins the participant snapshot for the current paused epoch. Resume never takes this
// mutex and can therefore wake targets while a Pause caller is waiting for acknowledgement.
class Protocol final {
public:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "Pause protocol requires lock-free 64-bit atomics");

    static constexpr std::uint64_t MaximumEpoch = std::numeric_limits<std::uint64_t>::max() >> 1;
    static constexpr std::uint64_t LastPausableRunningEpoch = MaximumEpoch - 1;

    Protocol() = default;

    // A narrow deterministic seam for the near-exhaustion test. Production always starts at
    // Running epoch zero.
    explicit Protocol(const Snapshot initial) : state(Encode(initial.state, initial.epoch)) {
        if (initial.epoch > MaximumEpoch) {
            std::abort();
        }
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
};

struct ParticipantTestAccess;

// One record belongs to one native POSIX thread. Signal-handler code uses only operations POSIX
// specifies as async-signal-safe plus C++ atomics statically required to be lock-free. The timed
// pselect is a bounded wake fallback: state, never timeout or signal count, decides whether the
// target remains paused.
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
    }

    Participant(const Participant&) = delete;
    Participant& operator=(const Participant&) = delete;

    ~Participant() {
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

    // Called by the target thread, never by a signal handler. Invalidation precedes signal
    // blocking and list removal. An in-flight controller shared_ptr remains a lifetime pin.
    void Unregister() noexcept {
        if (!registered.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        sigset_t signal_set{};
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGVTALRM);
        (void)pthread_sigmask(SIG_BLOCK, &signal_set, nullptr);
        PostAckWake();
    }

    // Used both by the actual SIGVTALRM handler and synchronously by registration. The latter is
    // what prevents insertion after a controller snapshot from crossing into guest execution
    // while the authoritative state is already Paused.
    StateWaitResult SynchronizeState() noexcept {
        sigset_t wait_mask{};
        const int mask_result = pthread_sigmask(SIG_SETMASK, nullptr, &wait_mask);
        if (mask_result != 0 || sigdelset(&wait_mask, SIGVTALRM) != 0) {
            return FailOpen(mask_result != 0 ? mask_result : errno);
        }

        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                return StateWaitResult::Unregistered;
            }

            const auto snapshot = protocol->Load();
            Acknowledge(snapshot.epoch);
            if (!registered.load(std::memory_order_acquire)) {
                return StateWaitResult::Unregistered;
            }
            if (snapshot.state == State::Running) {
                return StateWaitResult::Running;
            }

            // Close the state-change-before-wait race. If Resume changed the word and its signal
            // is delayed or failed, this acquire read avoids entering the wait at all.
            const auto before_wait = protocol->Load();
            if (before_wait.state != State::Paused || before_wait.epoch != snapshot.epoch) {
                continue;
            }

            timespec timeout = StateRecheckInterval;
            const int result = pselect(0, nullptr, nullptr, nullptr, &timeout, &wait_mask);
            if (result == 0 || (result < 0 && errno == EINTR)) {
                continue;
            }
            return FailOpen(errno, snapshot.epoch);
        }
    }

    void HandleNotification() noexcept {
        (void)SynchronizeState();
    }

    [[nodiscard]] AckWaitResult WaitForAckUntil(
        const std::uint64_t epoch, const std::chrono::steady_clock::time_point deadline) noexcept {
        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                return AckWaitResult::Unregistered;
            }
            if (wait_error.load(std::memory_order_acquire) != 0 ||
                ack_wake_error.load(std::memory_order_acquire) != 0) {
                return AckWaitResult::WaitError;
            }
            if (acknowledged_epoch.load(std::memory_order_acquire) >= epoch) {
                return AckWaitResult::Acknowledged;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return AckWaitResult::TimedOut;
            }

            timespec realtime_deadline{};
            if (clock_gettime(CLOCK_REALTIME, &realtime_deadline) != 0) {
                return AckWaitResult::WaitError;
            }
            const auto remaining =
                std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
            realtime_deadline.tv_sec += remaining / NanosecondsPerSecond;
            realtime_deadline.tv_nsec += remaining % NanosecondsPerSecond;
            if (realtime_deadline.tv_nsec >= NanosecondsPerSecond) {
                ++realtime_deadline.tv_sec;
                realtime_deadline.tv_nsec -= NanosecondsPerSecond;
            }

            if (sem_timedwait(&ack_semaphore, &realtime_deadline) == 0) {
                // A single producer token covers any number of epoch publications. Clearing after
                // consuming closes the producer/consumer handoff; the predicate is always reread.
                ack_wake_pending.store(false, std::memory_order_release);
                continue;
            }
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

    StateWaitResult FailOpen(const int error, const std::uint64_t paused_epoch) noexcept {
        wait_error.store(error != 0 ? error : EIO, std::memory_order_release);
        const auto rollback = protocol->RollbackPause(paused_epoch);
        Acknowledge(rollback.snapshot.epoch);
        return StateWaitResult::WaitError;
    }

    StateWaitResult FailOpen(const int error) noexcept {
        const auto snapshot = protocol->Load();
        if (snapshot.state == State::Paused) {
            return FailOpen(error, snapshot.epoch);
        }
        wait_error.store(error != 0 ? error : EIO, std::memory_order_release);
        Acknowledge(snapshot.epoch);
        return StateWaitResult::WaitError;
    }

    friend struct ParticipantTestAccess;

    Protocol* protocol;
    std::atomic_bool registered{true};
    std::atomic<std::uint64_t> acknowledged_epoch{};
    std::atomic_bool ack_wake_pending{};
    std::atomic_int ack_wake_error{};
    std::atomic_int wait_error{};
    sem_t ack_semaphore{};
};

inline thread_local Participant* CurrentParticipant{};

inline void BindCurrentParticipant(Participant* const participant) noexcept {
    CurrentParticipant = participant;
}

inline void ClearCurrentParticipant(const Participant* const participant) noexcept {
    if (CurrentParticipant == participant) {
        CurrentParticipant = nullptr;
    }
}

// Installed directly for SIGVTALRM. Keeping this entry point separate from the generic guest
// fault/signal translator ensures the pause path executes only the audited async-signal-safe core.
inline void PauseSignalHandler(int, siginfo_t*, void*) noexcept {
    if (CurrentParticipant != nullptr) {
        CurrentParticipant->HandleNotification();
    }
}

} // namespace Core::DebugPause

#endif // !_WIN32
