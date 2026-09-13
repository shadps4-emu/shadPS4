// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <latch>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <semaphore.h>
#include <signal.h>

#include <gtest/gtest.h>

#ifndef _WIN32
#include "core/pause_protocol.h"
#endif

namespace {

// This is the old SIGSLEEP shape reduced to a deterministic one-slot signal model. A standard
// signal can represent at most one pending edge here; the handler consumes the first edge and then
// waits for another edge to leave the pause. The test deliberately observes the resulting strand
// before providing a test-only cleanup wake.
class LegacySignalSlot {
public:
    void Notify() {
        {
            std::scoped_lock lock{mutex};
            pending = true;
        }
        cv.notify_all();
    }

    void AllowHandlerEntry() {
        {
            std::scoped_lock lock{mutex};
            entry_allowed = true;
        }
        cv.notify_all();
    }

    void HandleNotification(std::latch& handler_ready, std::latch& handler_waiting) {
        {
            std::unique_lock lock{mutex};
            handler_ready.count_down();
            cv.wait(lock, [&] { return entry_allowed; });
            ASSERT_TRUE(pending);
            pending = false;
        }

        handler_waiting.count_down();
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return pending; });
        pending = false;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool entry_allowed{};
    bool pending{};
};

TEST(PauseProtocolLegacy, PauseThenImmediateResumeCoalescesAndStrands) {
    LegacySignalSlot signal_slot;
    std::latch handler_ready{1};
    std::latch handler_waiting{1};
    std::atomic_bool handler_done{};

    std::jthread target{[&] {
        signal_slot.HandleNotification(handler_ready, handler_waiting);
        handler_done.store(true, std::memory_order_release);
    }};

    handler_ready.wait();

    // Pause and Resume use the same standard signal. The target has not entered the handler, so
    // both edges occupy the single pending slot and the second edge coalesces.
    signal_slot.Notify();
    signal_slot.Notify();
    signal_slot.AllowHandlerEntry();
    handler_waiting.wait();

    // The old handler has consumed the coalesced slot and is now waiting for an edge that Resume
    // already attempted to send. The assertion is bounded by the barrier, not by a hanging join.
    ASSERT_FALSE(handler_done.load(std::memory_order_acquire));

    // Test-only cleanup makes the intentionally stranded model joinable and cannot mask the
    // bounded observation above.
    signal_slot.Notify();
    target.join();
    EXPECT_TRUE(handler_done.load(std::memory_order_acquire));
}

#ifndef _WIN32

using Core::DebugPause::Participant;
using Core::DebugPause::Protocol;
using Core::DebugPause::State;

constexpr auto TestTimeout = std::chrono::seconds{2};

void BlockPauseSignal() {
    sigset_t signal_set{};
    ASSERT_EQ(sigemptyset(&signal_set), 0);
    ASSERT_EQ(sigaddset(&signal_set, SIGVTALRM), 0);
    ASSERT_EQ(pthread_sigmask(SIG_BLOCK, &signal_set, nullptr), 0);
}

int ConsumePauseSignal() {
    sigset_t signal_set{};
    if (sigemptyset(&signal_set) != 0 || sigaddset(&signal_set, SIGVTALRM) != 0) {
        return -1;
    }
    int signal{};
    return sigwait(&signal_set, &signal);
}

class PauseTarget {
public:
    PauseTarget()
        : participant(std::make_shared<Participant>(protocol)), thread([this] { Run(); }) {}

    PauseTarget(const PauseTarget&) = delete;
    PauseTarget& operator=(const PauseTarget&) = delete;

    ~PauseTarget() {
        Cleanup();
    }

    bool WaitReady() {
        return ready.try_acquire_for(TestTimeout);
    }

    void ReleaseHandler() {
        start.release();
    }

    bool WaitDone() {
        return done.try_acquire_for(TestTimeout);
    }

    bool IsFinished() const {
        return finished.load(std::memory_order_acquire);
    }

    int Notify() const {
        if (!native_ready.load(std::memory_order_acquire)) {
            return ESRCH;
        }
        return pthread_kill(native_thread, SIGVTALRM);
    }

    int NotifySignal(const int signal) const {
        if (!native_ready.load(std::memory_order_acquire)) {
            return ESRCH;
        }
        return pthread_kill(native_thread, signal);
    }

    void Join() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    Protocol protocol;
    std::shared_ptr<Participant> participant;

private:
    void Run() {
        BlockPauseSignal();
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        start.acquire();

        if (ConsumePauseSignal() == 0) {
            participant->HandleNotification();
        } else {
            worker_error.store(true, std::memory_order_release);
        }
        finished.store(true, std::memory_order_release);
        done.release();
        participant->Unregister();
    }

    void Cleanup() {
        if (!thread.joinable()) {
            return;
        }

        protocol.Request(State::Running);
        if (native_ready.load(std::memory_order_acquire)) {
            (void)pthread_kill(native_thread, SIGVTALRM);
        }
        start.release();
        thread.join();
    }

    pthread_t native_thread{};
    std::atomic_bool native_ready{};
    std::atomic_bool worker_error{};
    std::atomic_bool finished{};
    std::counting_semaphore<4> ready{0};
    std::counting_semaphore<4> start{0};
    std::counting_semaphore<4> done{0};
    std::thread thread;
};

class DelayedEntryTarget {
public:
    DelayedEntryTarget()
        : participant(std::make_shared<Participant>(protocol)), thread([this] { Run(); }) {}

    DelayedEntryTarget(const DelayedEntryTarget&) = delete;
    DelayedEntryTarget& operator=(const DelayedEntryTarget&) = delete;

    ~DelayedEntryTarget() {
        Cleanup();
    }

    bool WaitReady() {
        return ready.try_acquire_for(TestTimeout);
    }

    void ReleaseHandlerEntry() {
        start.release();
    }

    bool WaitHandlerEntered() {
        return entered.try_acquire_for(TestTimeout);
    }

    void ReleaseHandlerBody() {
        allow_body.release();
    }

    bool WaitDone() {
        return done.try_acquire_for(TestTimeout);
    }

    void Join() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    int Notify() const {
        if (!native_ready.load(std::memory_order_acquire)) {
            return ESRCH;
        }
        return pthread_kill(native_thread, SIGVTALRM);
    }

    Protocol protocol;
    std::shared_ptr<Participant> participant;

private:
    void Run() {
        BlockPauseSignal();
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        start.acquire();

        if (ConsumePauseSignal() != 0) {
            worker_error.store(true, std::memory_order_release);
            done.release();
            participant->Unregister();
            return;
        }
        entered.release();
        allow_body.acquire();
        participant->HandleNotification();

        // Resume is sent while the first handler body is held at entry. Consume that notification
        // as the next handler entry so a pending standard signal cannot escape the test thread.
        if (ConsumePauseSignal() != 0) {
            worker_error.store(true, std::memory_order_release);
        }
        finished.store(true, std::memory_order_release);
        done.release();
        participant->Unregister();
    }

    void Cleanup() {
        if (!thread.joinable()) {
            return;
        }

        protocol.Request(State::Running);
        if (native_ready.load(std::memory_order_acquire)) {
            (void)pthread_kill(native_thread, SIGVTALRM);
        }
        start.release();
        allow_body.release();
        thread.join();
    }

    pthread_t native_thread{};
    std::atomic_bool native_ready{};
    std::atomic_bool worker_error{};
    std::atomic_bool finished{};
    std::counting_semaphore<4> ready{0};
    std::counting_semaphore<4> start{0};
    std::counting_semaphore<4> entered{0};
    std::counting_semaphore<4> allow_body{0};
    std::counting_semaphore<4> done{0};
    std::thread thread;
};

class PendingExitTarget {
public:
    PendingExitTarget()
        : participant(std::make_shared<Participant>(protocol)), thread([this] { Run(); }) {}

    PendingExitTarget(const PendingExitTarget&) = delete;
    PendingExitTarget& operator=(const PendingExitTarget&) = delete;

    ~PendingExitTarget() {
        if (thread.joinable()) {
            exit.release();
            thread.join();
        }
    }

    bool WaitReady() {
        return ready.try_acquire_for(TestTimeout);
    }

    void Exit() {
        exit.release();
    }

    bool WaitDone() {
        return done.try_acquire_for(TestTimeout);
    }

    int Notify() const {
        if (!native_ready.load(std::memory_order_acquire)) {
            return ESRCH;
        }
        return pthread_kill(native_thread, SIGVTALRM);
    }

    Protocol protocol;
    std::shared_ptr<Participant> participant;

private:
    void Run() {
        BlockPauseSignal();
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        exit.acquire();
        participant->Unregister();
        finished.store(true, std::memory_order_release);
        done.release();
    }

    pthread_t native_thread{};
    std::atomic_bool native_ready{};
    std::atomic_bool finished{};
    std::counting_semaphore<4> ready{0};
    std::counting_semaphore<4> exit{0};
    std::counting_semaphore<4> done{0};
    std::thread thread;
};

std::atomic<sem_t*> cancel_seen{};

extern "C" void TestCancellationSignalHandler(int) noexcept {
    if (auto* semaphore = cancel_seen.load(std::memory_order_relaxed); semaphore != nullptr) {
        (void)sem_post(semaphore);
    }
}

bool WaitForSemaphore(sem_t& semaphore) {
    timespec deadline{};
    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
        return false;
    }
    deadline.tv_sec += TestTimeout.count();
    for (;;) {
        if (sem_timedwait(&semaphore, &deadline) == 0) {
            return true;
        }
        if (errno == EINTR) {
            continue;
        }
        return false;
    }
}

class ScopedCancellationSignal {
public:
    explicit ScopedCancellationSignal(sem_t& semaphore) {
        struct sigaction action{};
        action.sa_handler = TestCancellationSignalHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        installed = sigaction(SIGUSR1, &action, &old_action) == 0;
        if (installed) {
            cancel_seen.store(&semaphore, std::memory_order_release);
        }
    }

    ScopedCancellationSignal(const ScopedCancellationSignal&) = delete;
    ScopedCancellationSignal& operator=(const ScopedCancellationSignal&) = delete;

    ~ScopedCancellationSignal() {
        if (installed) {
            cancel_seen.store(nullptr, std::memory_order_release);
            (void)sigaction(SIGUSR1, &old_action, nullptr);
        }
    }

    bool IsInstalled() const {
        return installed;
    }

private:
    struct sigaction old_action{};
    bool installed{};
};

TEST(PauseProtocol, PauseWaitsForAckThenResumes) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);

    std::latch waiter_ready{1};
    std::atomic_bool ack_result{};
    std::thread ack_waiter{[&] {
        waiter_ready.count_down();
        ack_result.store(target.participant->WaitForAckFor(pause.snapshot.epoch, TestTimeout),
                         std::memory_order_release);
    }};
    waiter_ready.wait();
    EXPECT_FALSE(ack_result.load(std::memory_order_acquire));

    target.ReleaseHandler();
    ack_waiter.join();
    ASSERT_TRUE(ack_result.load(std::memory_order_acquire));
    EXPECT_GE(target.participant->AcknowledgedEpoch(), pause.snapshot.epoch);
    EXPECT_FALSE(target.IsFinished());

    const auto resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST(PauseProtocol, ImmediateResumeBeforeHandlerEntryUsesState) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    const auto resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(target.Notify(), 0);

    target.ReleaseHandler();
    ASSERT_TRUE(target.WaitDone());
    EXPECT_GE(target.participant->AcknowledgedEpoch(), pause.snapshot.epoch);
    EXPECT_GE(target.participant->AcknowledgedEpoch(), resume.snapshot.epoch);
    target.Join();
}

TEST(PauseProtocol, ResumeRacingHandlerEntryUsesLatestState) {
    DelayedEntryTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandlerEntry();
    ASSERT_TRUE(target.WaitHandlerEntered());

    const auto resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandlerBody();
    ASSERT_TRUE(target.WaitDone());
    EXPECT_GE(target.participant->AcknowledgedEpoch(), resume.snapshot.epoch);
    target.Join();
}

TEST(PauseProtocol, RapidPauseResumePauseAcknowledgesFinalPause) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause1 = target.protocol.Request(State::Paused);
    const auto resume = target.protocol.Request(State::Running);
    const auto pause2 = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause1.changed);
    ASSERT_TRUE(resume.changed);
    ASSERT_TRUE(pause2.changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_EQ(target.Notify(), 0);

    target.ReleaseHandler();
    ASSERT_TRUE(target.participant->WaitForAckFor(pause2.snapshot.epoch, TestTimeout));
    EXPECT_FALSE(target.IsFinished());

    const auto final_resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(final_resume.changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    EXPECT_GE(target.participant->AcknowledgedEpoch(), pause2.snapshot.epoch);
    target.Join();
}

TEST(PauseProtocol, RepeatedPauseIsIdempotent) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto first = target.protocol.Request(State::Paused);
    const auto second = target.protocol.Request(State::Paused);
    ASSERT_TRUE(first.changed);
    EXPECT_FALSE(second.changed);
    EXPECT_EQ(second.snapshot.epoch, first.snapshot.epoch);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandler();
    ASSERT_TRUE(target.participant->WaitForAckFor(first.snapshot.epoch, TestTimeout));

    ASSERT_TRUE(target.protocol.Request(State::Running).changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST(PauseProtocol, RepeatedResumeIsIdempotent) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandler();
    ASSERT_TRUE(target.participant->WaitForAckFor(pause.snapshot.epoch, TestTimeout));

    const auto first = target.protocol.Request(State::Running);
    const auto second = target.protocol.Request(State::Running);
    ASSERT_TRUE(first.changed);
    EXPECT_FALSE(second.changed);
    EXPECT_EQ(second.snapshot.epoch, first.snapshot.epoch);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST(PauseProtocol, IndependentCancellationRunsWhilePaused) {
    sem_t cancel_semaphore{};
    ASSERT_EQ(sem_init(&cancel_semaphore, 0, 0), 0);
    {
        ScopedCancellationSignal cancellation_signal{cancel_semaphore};
        ASSERT_TRUE(cancellation_signal.IsInstalled());

        PauseTarget target;
        ASSERT_TRUE(target.WaitReady());
        const auto pause = target.protocol.Request(State::Paused);
        ASSERT_TRUE(pause.changed);
        ASSERT_EQ(target.Notify(), 0);
        target.ReleaseHandler();
        ASSERT_TRUE(target.participant->WaitForAckFor(pause.snapshot.epoch, TestTimeout));

        ASSERT_EQ(target.NotifySignal(SIGUSR1), 0);
        ASSERT_TRUE(WaitForSemaphore(cancel_semaphore));
        EXPECT_FALSE(target.IsFinished());

        ASSERT_TRUE(target.protocol.Request(State::Running).changed);
        ASSERT_EQ(target.Notify(), 0);
        ASSERT_TRUE(target.WaitDone());
        target.Join();
    }
    ASSERT_EQ(sem_destroy(&cancel_semaphore), 0);
}

TEST(PauseProtocol, PendingPauseUnregistersWithoutHandlerTouch) {
    PendingExitTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.Exit();
    ASSERT_TRUE(target.WaitDone());
    EXPECT_EQ(target.participant->AcknowledgedEpoch(), 0);
    EXPECT_TRUE(target.participant->WaitForAckFor(pause.snapshot.epoch, TestTimeout));
}

TEST(PauseProtocol, ThousandsOfCoalescedTransitionsDoNotPermanentlySleep) {
    Protocol protocol;
    auto participant = std::make_shared<Participant>(protocol);
    std::counting_semaphore<4> ready{0};
    std::counting_semaphore<4> start{0};
    std::counting_semaphore<4> done{0};
    std::atomic_bool native_ready{};
    std::atomic_bool worker_error{};
    pthread_t native_thread{};
    constexpr int Iterations = 5000;

    std::thread target{[&] {
        BlockPauseSignal();
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        for (int i = 0; i < Iterations; ++i) {
            ready.release();
            start.acquire();
            if (ConsumePauseSignal() != 0) {
                worker_error.store(true, std::memory_order_release);
                break;
            }
            participant->HandleNotification();
            done.release();
        }
        participant->Unregister();
    }};

    bool completed = true;
    for (int i = 0; i < Iterations; ++i) {
        if (!ready.try_acquire_for(TestTimeout)) {
            completed = false;
            break;
        }
        const auto pause = protocol.Request(State::Paused);
        const auto resume = protocol.Request(State::Running);
        if (!pause.changed || !resume.changed || !native_ready.load(std::memory_order_acquire) ||
            pthread_kill(native_thread, SIGVTALRM) != 0) {
            completed = false;
            break;
        }
        start.release();
        if (!done.try_acquire_for(TestTimeout) ||
            participant->AcknowledgedEpoch() < resume.snapshot.epoch) {
            completed = false;
            break;
        }
    }

    protocol.Request(State::Running);
    if (native_ready.load(std::memory_order_acquire)) {
        (void)pthread_kill(native_thread, SIGVTALRM);
    }
    start.release();
    if (target.joinable()) {
        target.join();
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(worker_error.load(std::memory_order_acquire));
}

#endif // !_WIN32

} // namespace
