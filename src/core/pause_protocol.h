// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifndef _WIN32

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

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
};

// The state bit and epoch are one release/acquire publication. Notifications are only hints that
// cause a participant to reread this word; correctness never depends on standard-signal counts.
class Protocol final {
public:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "Pause protocol requires lock-free 64-bit atomics");

    Snapshot Load() const noexcept {
        return Decode(state.load(std::memory_order_acquire));
    }

    Transition Request(const State desired) noexcept {
        auto current = state.load(std::memory_order_acquire);
        for (;;) {
            const auto snapshot = Decode(current);
            if (snapshot.state == desired) {
                return {.snapshot = snapshot, .changed = false};
            }

            const auto next_epoch = snapshot.epoch + 1;
            const auto next = Encode(desired, next_epoch);
            if (state.compare_exchange_weak(current, next, std::memory_order_release,
                                            std::memory_order_acquire)) {
                return {.snapshot = {.state = desired, .epoch = next_epoch}, .changed = true};
            }
        }
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

    std::atomic<std::uint64_t> state{};
};

// One record belongs to one native POSIX thread. The record is shared with the debug controller
// only through a shared_ptr held by the list/waiter; signal-handler code uses only lock-free
// atomics, sem_post, and sigwait.
class Participant final {
public:
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

    // Called from the controller, never from a signal handler. Registration is invalidated before
    // the pause signal is blocked so a signal accepted in that small window returns immediately.
    void Unregister() noexcept {
        if (!registered.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        sigset_t signal_set{};
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGVTALRM);
        (void)pthread_sigmask(SIG_BLOCK, &signal_set, nullptr);

        // Wake a controller that is waiting for either an acknowledgement or terminal removal.
        (void)sem_post(&ack_semaphore);
    }

    // Called by the SIGVTALRM handler after the initial notification has entered the handler.
    // Every loop rereads the authoritative state, including after a coalesced notification.
    void HandleNotification() noexcept {
        if (!registered.load(std::memory_order_acquire)) {
            return;
        }

        sigset_t signal_set{};
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGVTALRM);

        for (;;) {
            if (!registered.load(std::memory_order_acquire)) {
                return;
            }

            const auto snapshot = protocol->Load();
            Acknowledge(snapshot.epoch);

            if (!registered.load(std::memory_order_acquire) || snapshot.state == State::Running) {
                return;
            }

            int signal{};
            const int result = sigwait(&signal_set, &signal);
            if (result != 0 && result != EINTR) {
                return;
            }
        }
    }

    // Returns false only if the semaphore itself reports an unexpected error. Registration
    // removal is a successful terminal outcome for the requested acknowledgement.
    [[nodiscard]] bool WaitForAck(const std::uint64_t epoch) noexcept {
        for (;;) {
            if (!registered.load(std::memory_order_acquire) ||
                acknowledged_epoch.load(std::memory_order_acquire) >= epoch) {
                return true;
            }

            if (sem_wait(&ack_semaphore) == 0) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
    }

    // The bounded form is used by focused tests as a watchdog-safe observation. Production debug
    // control uses WaitForAck(), because a live target is expected to acknowledge or unregister.
    [[nodiscard]] bool WaitForAckFor(const std::uint64_t epoch,
                                     const std::chrono::milliseconds timeout) noexcept {
        timespec deadline{};
        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            return false;
        }
        const auto timeout_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count();
        deadline.tv_sec += timeout_ns / 1'000'000'000;
        deadline.tv_nsec += timeout_ns % 1'000'000'000;
        if (deadline.tv_nsec >= 1'000'000'000) {
            ++deadline.tv_sec;
            deadline.tv_nsec -= 1'000'000'000;
        }

        for (;;) {
            if (!registered.load(std::memory_order_acquire) ||
                acknowledged_epoch.load(std::memory_order_acquire) >= epoch) {
                return true;
            }

            if (sem_timedwait(&ack_semaphore, &deadline) == 0) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == ETIMEDOUT) {
                return !registered.load(std::memory_order_acquire) ||
                       acknowledged_epoch.load(std::memory_order_acquire) >= epoch;
            }
            return false;
        }
    }

private:
    void Acknowledge(const std::uint64_t epoch) noexcept {
        auto current = acknowledged_epoch.load(std::memory_order_relaxed);
        while (current < epoch) {
            if (acknowledged_epoch.compare_exchange_weak(current, epoch, std::memory_order_release,
                                                         std::memory_order_relaxed)) {
                // POSIX specifies sem_post as async-signal-safe. The atomic publication above is
                // the predicate; this post is only the wake edge for a controller.
                (void)sem_post(&ack_semaphore);
                return;
            }
        }
    }

    Protocol* protocol;
    std::atomic_bool registered{true};
    std::atomic<std::uint64_t> acknowledged_epoch{};
    sem_t ack_semaphore{};
};

} // namespace Core::DebugPause

#endif // !_WIN32
