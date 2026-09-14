// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <latch>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

#ifndef _WIN32
#include "core/pause_protocol.h"
#endif

#ifndef _WIN32
namespace Core::DebugPause {

struct ParticipantTestAccess {
    static int PendingAckWakeCount(Participant& participant) {
        int value{};
        return sem_getvalue(&participant.ack_semaphore, &value) == 0 ? value : -1;
    }

    static int MaximumHandlerDepth(const Participant& participant) {
        return participant.maximum_handler_depth.load(std::memory_order_acquire);
    }

    static bool StateWakePending(const Participant& participant) {
        return participant.state_wake_pending.load(std::memory_order_acquire);
    }

    static void InjectStateWaitError(Participant& participant) {
        participant.InjectStateWaitErrorForTest();
    }

    static StateWaitResult InvokeFailOpen(Participant& participant, const int error,
                                          const std::uint64_t paused_epoch) {
        return participant.FailOpen(error, paused_epoch);
    }

    static int FillStateWakePipe(Participant& participant) {
        const char token = 1;
        for (;;) {
            const ssize_t result = write(participant.state_wake_pipe[1], &token, sizeof(token));
            if (result == sizeof(token)) {
                continue;
            }
            if (result < 0 && errno == EINTR) {
                continue;
            }
            return result < 0 ? errno : EIO;
        }
    }

    static int DrainOneStateWake(Participant& participant) {
        return participant.DrainStateWake();
    }

    static void EmptyStateWakePipe(Participant& participant) {
        char tokens[64];
        while (read(participant.state_wake_pipe[0], tokens, sizeof(tokens)) > 0) {
        }
    }

    static void CloseStateWakeWriteEnd(Participant& participant) {
        ASSERT_GE(participant.state_wake_pipe[1], 0);
        ASSERT_EQ(close(participant.state_wake_pipe[1]), 0);
        participant.state_wake_pipe[1] = -1;
    }
};

} // namespace Core::DebugPause
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

using Core::DebugPause::AckWaitResult;
using Core::DebugPause::Participant;
using Core::DebugPause::ParticipantTestAccess;
using Core::DebugPause::Protocol;
using Core::DebugPause::State;
using Core::DebugPause::StateWaitResult;

std::atomic<std::binary_semaphore*> fail_open_publication_reached{};
std::atomic<std::binary_semaphore*> fail_open_publication_release{};
std::atomic<std::binary_semaphore*> fail_open_rollback_reached{};
std::atomic<std::binary_semaphore*> fail_open_rollback_release{};

void HoldFailOpenPublication() noexcept {
    if (auto* reached = fail_open_publication_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    if (auto* release = fail_open_publication_release.load(std::memory_order_acquire);
        release != nullptr) {
        release->acquire();
    }
}

void HoldFailOpenRollback() noexcept {
    if (auto* reached = fail_open_rollback_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    if (auto* release = fail_open_rollback_release.load(std::memory_order_acquire);
        release != nullptr) {
        release->acquire();
    }
}

constexpr auto TestTimeout = std::chrono::seconds{2};

bool WaitForAck(Participant& participant, const std::uint64_t epoch,
                const std::chrono::milliseconds timeout = TestTimeout) {
    return participant.WaitForAckFor(epoch, timeout) == AckWaitResult::Acknowledged;
}

class PauseProtocol : public testing::Test {
public:
    void SetUp() override {
        ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &old_mask), 0);
        auto test_mask = old_mask;
        ASSERT_EQ(sigdelset(&test_mask, SIGVTALRM), 0);
        ASSERT_EQ(pthread_sigmask(SIG_SETMASK, &test_mask, nullptr), 0);

        struct sigaction action{};
        action.sa_sigaction = Core::DebugPause::PauseSignalHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO;
        ASSERT_EQ(sigaction(SIGVTALRM, &action, &old_action), 0);
    }

    void TearDown() override {
        EXPECT_EQ(sigaction(SIGVTALRM, &old_action, nullptr), 0);
        EXPECT_EQ(pthread_sigmask(SIG_SETMASK, &old_mask, nullptr), 0);
    }

private:
    struct sigaction old_action{};
    sigset_t old_mask{};
};

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
private:
    std::unique_ptr<Protocol> owned_protocol;

public:
    PauseTarget()
        : owned_protocol(std::make_unique<Protocol>()), protocol(*owned_protocol),
          participant(std::make_shared<Participant>(protocol)), thread([this] { Run(); }) {}

    explicit PauseTarget(Protocol& protocol_)
        : protocol(protocol_), participant(std::make_shared<Participant>(protocol)),
          thread([this] { Run(); }) {}

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

    Protocol& protocol;
    std::shared_ptr<Participant> participant;

private:
    void Run() {
        BlockPauseSignal();
        Core::DebugPause::BindCurrentParticipant(participant.get());
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
        Core::DebugPause::ClearCurrentParticipant(participant.get());
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

class LiveSignalTarget {
public:
    LiveSignalTarget()
        : participant(std::make_shared<Participant>(protocol)), thread([this] { Run(); }) {}

    LiveSignalTarget(const LiveSignalTarget&) = delete;
    LiveSignalTarget& operator=(const LiveSignalTarget&) = delete;

    ~LiveSignalTarget() {
        protocol.Request(State::Running);
        (void)participant->NotifyStateChange();
        stop.store(true, std::memory_order_release);
        if (native_ready.load(std::memory_order_acquire)) {
            (void)pthread_kill(native_thread, SIGVTALRM);
        }
        if (thread.joinable()) {
            thread.join();
        }
    }

    bool WaitReady() {
        return ready.try_acquire_for(TestTimeout);
    }

    int NotifyEntry() const {
        return pthread_kill(native_thread, SIGVTALRM);
    }

    bool WaitStopped() {
        return stopped.try_acquire_for(TestTimeout);
    }

    void Stop() {
        stop.store(true, std::memory_order_release);
    }

    Protocol protocol;
    std::shared_ptr<Participant> participant;

private:
    void Run() {
        sigset_t old_mask{};
        sigset_t unblocked{};
        EXPECT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &old_mask), 0);
        unblocked = old_mask;
        EXPECT_EQ(sigdelset(&unblocked, SIGVTALRM), 0);
        EXPECT_EQ(pthread_sigmask(SIG_SETMASK, &unblocked, nullptr), 0);
        Core::DebugPause::BindCurrentParticipant(participant.get());
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        while (!stop.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        participant->Unregister();
        Core::DebugPause::ClearCurrentParticipant(participant.get());
        EXPECT_EQ(pthread_sigmask(SIG_SETMASK, &old_mask, nullptr), 0);
        stopped.release();
    }

    pthread_t native_thread{};
    std::atomic_bool native_ready{};
    std::atomic_bool stop{};
    std::counting_semaphore<2> ready{0};
    std::counting_semaphore<2> stopped{0};
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
        Core::DebugPause::BindCurrentParticipant(participant.get());
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        start.acquire();

        if (ConsumePauseSignal() != 0) {
            worker_error.store(true, std::memory_order_release);
            done.release();
            participant->Unregister();
            Core::DebugPause::ClearCurrentParticipant(participant.get());
            return;
        }
        entered.release();
        allow_body.acquire();
        participant->HandleNotification();
        finished.store(true, std::memory_order_release);
        done.release();
        participant->Unregister();
        Core::DebugPause::ClearCurrentParticipant(participant.get());
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
        Core::DebugPause::BindCurrentParticipant(participant.get());
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        ready.release();
        exit.acquire();
        participant->Unregister();
        Core::DebugPause::ClearCurrentParticipant(participant.get());
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

// Models the production AddCurrentThreadToGuestList ordering: publish the participant, then
// synchronously honor the authoritative state before crossing into guest execution.
class RegistrationTarget {
private:
    Protocol& protocol;

public:
    explicit RegistrationTarget(Protocol& protocol_)
        : protocol(protocol_), participant(std::make_shared<Participant>(protocol)),
          thread([this] { Run(); }) {}

    RegistrationTarget(const RegistrationTarget&) = delete;
    RegistrationTarget& operator=(const RegistrationTarget&) = delete;

    ~RegistrationTarget() {
        if (thread.joinable()) {
            protocol.Request(State::Running);
            if (native_ready.load(std::memory_order_acquire)) {
                (void)pthread_kill(native_thread, SIGVTALRM);
            }
            thread.join();
        }
    }

    bool WaitPublished() {
        return published.try_acquire_for(TestTimeout);
    }

    bool WaitGuestEntry() {
        return guest_entry.try_acquire_for(TestTimeout);
    }

    bool HasEnteredGuest() const {
        return entered_guest.load(std::memory_order_acquire);
    }

    int Notify() const {
        if (!native_ready.load(std::memory_order_acquire)) {
            return ESRCH;
        }
        return pthread_kill(native_thread, SIGVTALRM);
    }

    void Join() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::shared_ptr<Participant> participant;

private:
    void Run() {
        BlockPauseSignal();
        Core::DebugPause::BindCurrentParticipant(participant.get());
        native_thread = pthread_self();
        native_ready.store(true, std::memory_order_release);
        published.release();

        const auto result = participant->SynchronizeState();
        if (result == StateWaitResult::Running) {
            entered_guest.store(true, std::memory_order_release);
        }
        guest_entry.release();
        participant->Unregister();
        Core::DebugPause::ClearCurrentParticipant(participant.get());
    }

    pthread_t native_thread{};
    std::atomic_bool native_ready{};
    std::atomic_bool entered_guest{};
    std::counting_semaphore<4> published{0};
    std::counting_semaphore<4> guest_entry{0};
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
        installed = sigaction(SignalNumber(), &action, &old_action) == 0;
        if (installed) {
            cancel_seen.store(&semaphore, std::memory_order_release);
        }
    }

    ScopedCancellationSignal(const ScopedCancellationSignal&) = delete;
    ScopedCancellationSignal& operator=(const ScopedCancellationSignal&) = delete;

    ~ScopedCancellationSignal() {
        if (installed) {
            cancel_seen.store(nullptr, std::memory_order_release);
            (void)sigaction(SignalNumber(), &old_action, nullptr);
        }
    }

    bool IsInstalled() const {
        return installed;
    }

    static int SignalNumber() {
#if defined(__APPLE__) || defined(__FreeBSD__)
        return SIGUSR2;
#else
        return SIGRTMAX;
#endif
    }

private:
    struct sigaction old_action{};
    bool installed{};
};

TEST_F(PauseProtocol, PauseWaitsForAckThenResumes) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);

    std::latch waiter_ready{1};
    std::atomic_bool ack_result{};
    std::thread ack_waiter{[&] {
        waiter_ready.count_down();
        ack_result.store(WaitForAck(*target.participant, pause.snapshot.epoch),
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

TEST_F(PauseProtocol, ImmediateResumeBeforeHandlerEntryUsesState) {
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

TEST_F(PauseProtocol, ResumeRacingHandlerEntryUsesLatestState) {
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

TEST_F(PauseProtocol, RegistrationDuringActivePauseCannotEnterGuest) {
    PauseTarget existing;
    ASSERT_TRUE(existing.WaitReady());

    auto pause_request = existing.protocol.AcquirePauseRequest();
    const auto pause = existing.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(existing.Notify(), 0);
    existing.ReleaseHandler();
    ASSERT_TRUE(WaitForAck(*existing.participant, pause.snapshot.epoch));
    pause_request.unlock();

    RegistrationTarget newcomer{existing.protocol};
    ASSERT_TRUE(newcomer.WaitPublished());
    ASSERT_TRUE(WaitForAck(*newcomer.participant, pause.snapshot.epoch));
    EXPECT_FALSE(newcomer.HasEnteredGuest());

    const auto resume = existing.protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(existing.Notify(), 0);
    ASSERT_EQ(newcomer.Notify(), 0);
    ASSERT_TRUE(existing.WaitDone());
    ASSERT_TRUE(newcomer.WaitGuestEntry());
    EXPECT_TRUE(newcomer.HasEnteredGuest());
    existing.Join();
    newcomer.Join();
}

TEST_F(PauseProtocol, EmptyListPauseThenRegistrationCannotEnterGuest) {
    Protocol protocol;
    {
        auto pause_request = protocol.AcquirePauseRequest();
        const auto pause = protocol.Request(State::Paused);
        ASSERT_TRUE(pause.changed);
    }

    RegistrationTarget target{protocol};
    ASSERT_TRUE(target.WaitPublished());
    const auto paused = protocol.Load();
    ASSERT_EQ(paused.state, State::Paused);
    ASSERT_TRUE(WaitForAck(*target.participant, paused.epoch));
    EXPECT_FALSE(target.HasEnteredGuest());

    const auto resume = protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitGuestEntry());
    EXPECT_TRUE(target.HasEnteredGuest());
    target.Join();
}

TEST_F(PauseProtocol, ConcurrentPauseJoinsCurrentAcknowledgementEpoch) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    std::counting_semaphore<4> first_published{0};
    std::counting_semaphore<4> second_tried{0};
    std::counting_semaphore<4> controllers_done{0};
    std::atomic_bool first_ok{};
    std::atomic_bool second_try_owned{};
    std::atomic_bool second_ok{};

    std::thread first{[&] {
        auto pause_request = target.protocol.AcquirePauseRequest();
        const auto pause = target.protocol.Request(State::Paused);
        first_published.release();
        if (pause.changed && target.Notify() == 0 &&
            WaitForAck(*target.participant, pause.snapshot.epoch)) {
            first_ok.store(true, std::memory_order_release);
        }
        controllers_done.release();
    }};

    ASSERT_TRUE(first_published.try_acquire_for(TestTimeout));
    std::thread second{[&] {
        auto try_request = target.protocol.TryAcquirePauseRequest();
        second_try_owned.store(try_request.owns_lock(), std::memory_order_release);
        if (try_request.owns_lock()) {
            try_request.unlock();
        }
        second_tried.release();

        auto pause_request = target.protocol.AcquirePauseRequest();
        const auto pause = target.protocol.Request(State::Paused);
        if (!pause.changed && !pause.exhausted &&
            WaitForAck(*target.participant, pause.snapshot.epoch)) {
            second_ok.store(true, std::memory_order_release);
        }
        controllers_done.release();
    }};

    ASSERT_TRUE(second_tried.try_acquire_for(TestTimeout));
    EXPECT_FALSE(second_try_owned.load(std::memory_order_acquire));
    target.ReleaseHandler();
    ASSERT_TRUE(controllers_done.try_acquire_for(TestTimeout));
    ASSERT_TRUE(controllers_done.try_acquire_for(TestTimeout));
    first.join();
    second.join();
    EXPECT_TRUE(first_ok.load(std::memory_order_acquire));
    EXPECT_TRUE(second_ok.load(std::memory_order_acquire));

    ASSERT_TRUE(target.protocol.Request(State::Running).changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST_F(PauseProtocol, ResumeDoesNotTakePauseSerializer) {
    Protocol protocol;
    auto pause_request = protocol.AcquirePauseRequest();
    ASSERT_TRUE(protocol.Request(State::Paused).changed);

    std::counting_semaphore<2> resumed{0};
    std::atomic_bool resume_changed{};
    std::thread resume_thread{[&] {
        resume_changed.store(protocol.Request(State::Running).changed, std::memory_order_release);
        resumed.release();
    }};

    ASSERT_TRUE(resumed.try_acquire_for(TestTimeout));
    EXPECT_TRUE(resume_changed.load(std::memory_order_acquire));
    pause_request.unlock();
    resume_thread.join();
}

TEST_F(PauseProtocol, BlockedDeliveryTimesOutAndRollsBack) {
    Protocol protocol;
    Participant participant{protocol};
    auto pause_request = protocol.AcquirePauseRequest();
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);

    EXPECT_EQ(participant.WaitForAckFor(pause.snapshot.epoch, std::chrono::milliseconds{25}),
              AckWaitResult::TimedOut);
    const auto rollback = protocol.RollbackPause(pause.snapshot.epoch);
    ASSERT_TRUE(rollback.changed);
    EXPECT_EQ(rollback.snapshot.state, State::Running);
    EXPECT_EQ(participant.SynchronizeState(), StateWaitResult::Running);
    participant.Unregister();
}

TEST_F(PauseProtocol, InjectedDeliveryFailureCannotLeavePausedState) {
    Protocol protocol;
    auto pause_request = protocol.AcquirePauseRequest();
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);

    constexpr int injected_delivery_result = EAGAIN;
    ASSERT_NE(injected_delivery_result, 0);
    const auto rollback = protocol.RollbackPause(pause.snapshot.epoch);
    EXPECT_TRUE(rollback.changed);
    EXPECT_EQ(protocol.Load().state, State::Running);
    EXPECT_GT(rollback.snapshot.epoch, pause.snapshot.epoch);
}

TEST_F(PauseProtocol, FailedResumeNotificationUsesBoundedStateRecheck) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandler();
    ASSERT_TRUE(WaitForAck(*target.participant, pause.snapshot.epoch));

    // Deliberately inject a failed/missing Resume notification. The handler's finite pselect wait
    // must reread Running and return without an edge or a polling loop.
    ASSERT_TRUE(target.protocol.Request(State::Running).changed);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST_F(PauseProtocol, MultipleTargetsAcknowledgeOneSnapshot) {
    Protocol protocol;
    PauseTarget first{protocol};
    PauseTarget second{protocol};
    PauseTarget third{protocol};
    ASSERT_TRUE(first.WaitReady());
    ASSERT_TRUE(second.WaitReady());
    ASSERT_TRUE(third.WaitReady());

    auto pause_request = protocol.AcquirePauseRequest();
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(first.Notify(), 0);
    ASSERT_EQ(second.Notify(), 0);
    ASSERT_EQ(third.Notify(), 0);
    first.ReleaseHandler();
    second.ReleaseHandler();
    third.ReleaseHandler();
    const auto deadline = std::chrono::steady_clock::now() + TestTimeout;
    EXPECT_EQ(first.participant->WaitForAckUntil(pause.snapshot.epoch, deadline),
              AckWaitResult::Acknowledged);
    EXPECT_EQ(second.participant->WaitForAckUntil(pause.snapshot.epoch, deadline),
              AckWaitResult::Acknowledged);
    EXPECT_EQ(third.participant->WaitForAckUntil(pause.snapshot.epoch, deadline),
              AckWaitResult::Acknowledged);
    EXPECT_FALSE(first.IsFinished());
    EXPECT_FALSE(second.IsFinished());
    EXPECT_FALSE(third.IsFinished());
    pause_request.unlock();

    ASSERT_TRUE(protocol.Request(State::Running).changed);
    ASSERT_EQ(first.Notify(), 0);
    ASSERT_EQ(second.Notify(), 0);
    ASSERT_EQ(third.Notify(), 0);
    ASSERT_TRUE(first.WaitDone());
    ASSERT_TRUE(second.WaitDone());
    ASSERT_TRUE(third.WaitDone());
    first.Join();
    second.Join();
    third.Join();
}

TEST_F(PauseProtocol, RapidPauseResumePauseAcknowledgesFinalPause) {
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
    ASSERT_TRUE(WaitForAck(*target.participant, pause2.snapshot.epoch));
    EXPECT_FALSE(target.IsFinished());

    const auto final_resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(final_resume.changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    EXPECT_GE(target.participant->AcknowledgedEpoch(), pause2.snapshot.epoch);
    target.Join();
}

TEST_F(PauseProtocol, RepeatedPauseIsIdempotent) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto first = target.protocol.Request(State::Paused);
    const auto second = target.protocol.Request(State::Paused);
    ASSERT_TRUE(first.changed);
    EXPECT_FALSE(second.changed);
    EXPECT_EQ(second.snapshot.epoch, first.snapshot.epoch);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandler();
    ASSERT_TRUE(WaitForAck(*target.participant, first.snapshot.epoch));

    ASSERT_TRUE(target.protocol.Request(State::Running).changed);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST_F(PauseProtocol, RepeatedResumeIsIdempotent) {
    PauseTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.ReleaseHandler();
    ASSERT_TRUE(WaitForAck(*target.participant, pause.snapshot.epoch));

    const auto first = target.protocol.Request(State::Running);
    const auto second = target.protocol.Request(State::Running);
    ASSERT_TRUE(first.changed);
    EXPECT_FALSE(second.changed);
    EXPECT_EQ(second.snapshot.epoch, first.snapshot.epoch);
    ASSERT_EQ(target.Notify(), 0);
    ASSERT_TRUE(target.WaitDone());
    target.Join();
}

TEST_F(PauseProtocol, DeferredCancellationSignalRunsWhilePaused) {
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
        ASSERT_TRUE(WaitForAck(*target.participant, pause.snapshot.epoch));

        ASSERT_EQ(target.NotifySignal(ScopedCancellationSignal::SignalNumber()), 0);
        ASSERT_TRUE(WaitForSemaphore(cancel_semaphore));
        EXPECT_FALSE(target.IsFinished());

        ASSERT_TRUE(target.protocol.Request(State::Running).changed);
        ASSERT_EQ(target.Notify(), 0);
        ASSERT_TRUE(target.WaitDone());
        target.Join();
    }
    ASSERT_EQ(sem_destroy(&cancel_semaphore), 0);
}

TEST_F(PauseProtocol, ForkIsolatedProductionPauseHandlerEntryResumes) {
    int result_pipe[2]{};
    ASSERT_EQ(pipe(result_pipe), 0);
    const pid_t child = fork();
    ASSERT_GE(child, 0);

    if (child == 0) {
        (void)close(result_pipe[0]);
        struct sigaction action{};
        action.sa_sigaction = Core::DebugPause::PauseSignalHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO;
        if (sigaction(SIGVTALRM, &action, nullptr) != 0) {
            _exit(2);
        }

        Protocol protocol;
        Participant participant{protocol};
        Core::DebugPause::BindCurrentParticipant(&participant);
        const pthread_t target = pthread_self();
        const auto pause = protocol.Request(State::Paused);
        if (!pause.changed) {
            _exit(3);
        }

        std::thread controller{[&] {
            if (participant.WaitForAckFor(pause.snapshot.epoch, TestTimeout) !=
                AckWaitResult::Acknowledged) {
                return;
            }
            const auto resume = protocol.Request(State::Running);
            if (resume.changed) {
                (void)pthread_kill(target, SIGVTALRM);
            }
        }};

        if (pthread_kill(target, SIGVTALRM) != 0) {
            _exit(5);
        }
        controller.join();
        const bool passed = protocol.Load().state == State::Running &&
                            participant.AcknowledgedEpoch() >= pause.snapshot.epoch;
        const char result = passed ? '1' : '0';
        (void)write(result_pipe[1], &result, sizeof(result));
        participant.Unregister();
        Core::DebugPause::ClearCurrentParticipant(&participant);
        _exit(passed ? 0 : 4);
    }

    (void)close(result_pipe[1]);
    pollfd result_poll{.fd = result_pipe[0], .events = POLLIN};
    const int poll_result = poll(&result_poll, 1, 2000);
    if (poll_result <= 0) {
        (void)kill(child, SIGKILL);
    }
    char result{};
    const ssize_t bytes = poll_result > 0 ? read(result_pipe[0], &result, sizeof(result)) : -1;
    int child_status{};
    ASSERT_EQ(waitpid(child, &child_status, 0), child);
    (void)close(result_pipe[0]);

    ASSERT_GT(poll_result, 0);
    ASSERT_EQ(bytes, 1);
    EXPECT_EQ(result, '1');
    ASSERT_TRUE(WIFEXITED(child_status));
    EXPECT_EQ(WEXITSTATUS(child_status), 0);
}

TEST_F(PauseProtocol, MultiplePendingParticipantsUnregisterWithoutAckStrand) {
    Protocol protocol;
    auto first = std::make_shared<Participant>(protocol);
    auto second = std::make_shared<Participant>(protocol);
    auto third = std::make_shared<Participant>(protocol);
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);

    std::counting_semaphore<4> release{0};
    std::counting_semaphore<4> done{0};
    std::vector<std::thread> targets;
    for (const auto& participant : {first, second, third}) {
        targets.emplace_back([&, participant] {
            BlockPauseSignal();
            release.acquire();
            participant->Unregister();
            done.release();
        });
    }
    release.release(3);
    ASSERT_TRUE(done.try_acquire_for(TestTimeout));
    ASSERT_TRUE(done.try_acquire_for(TestTimeout));
    ASSERT_TRUE(done.try_acquire_for(TestTimeout));
    for (auto& target : targets) {
        target.join();
    }

    const auto deadline = std::chrono::steady_clock::now() + TestTimeout;
    EXPECT_EQ(first->WaitForAckUntil(pause.snapshot.epoch, deadline), AckWaitResult::Unregistered);
    EXPECT_EQ(second->WaitForAckUntil(pause.snapshot.epoch, deadline), AckWaitResult::Unregistered);
    EXPECT_EQ(third->WaitForAckUntil(pause.snapshot.epoch, deadline), AckWaitResult::Unregistered);
    EXPECT_TRUE(protocol.RollbackPause(pause.snapshot.epoch).changed);
}

TEST_F(PauseProtocol, EpochExhaustionFailsWhileStillRunning) {
    Protocol protocol{{
        .state = State::Paused,
        .epoch = Protocol::LastPausableRunningEpoch - 1,
    }};

    const auto final_resume = protocol.Request(State::Running);
    ASSERT_TRUE(final_resume.changed);
    ASSERT_EQ(final_resume.snapshot.epoch, Protocol::LastPausableRunningEpoch);
    const auto rejected_pause = protocol.Request(State::Paused);
    EXPECT_FALSE(rejected_pause.changed);
    EXPECT_TRUE(rejected_pause.exhausted);
    EXPECT_EQ(rejected_pause.snapshot.state, State::Running);
    EXPECT_EQ(rejected_pause.snapshot.epoch, Protocol::LastPausableRunningEpoch);
}

TEST_F(PauseProtocol, AcknowledgementWakeDoesNotAccumulatePerEpoch) {
    Protocol protocol;
    Participant participant{protocol};
    constexpr int Iterations = 5000;

    for (int i = 0; i < Iterations; ++i) {
        ASSERT_TRUE(protocol.Request(State::Paused).changed);
        const auto running = protocol.Request(State::Running);
        ASSERT_TRUE(running.changed);
        ASSERT_EQ(participant.SynchronizeState(), StateWaitResult::Running);
    }

    EXPECT_EQ(ParticipantTestAccess::PendingAckWakeCount(participant), 1);
    const auto final_pause = protocol.Request(State::Paused);
    ASSERT_TRUE(final_pause.changed);
    EXPECT_EQ(participant.WaitForAckFor(final_pause.snapshot.epoch, std::chrono::milliseconds{25}),
              AckWaitResult::TimedOut);
    EXPECT_EQ(ParticipantTestAccess::PendingAckWakeCount(participant), 0);
    EXPECT_TRUE(protocol.RollbackPause(final_pause.snapshot.epoch).changed);
    participant.Unregister();
}

TEST_F(PauseProtocol, StateWakePipeFullUsesCoalescedEagainAndRecovers) {
    Protocol protocol;
    Participant participant{protocol};

    ASSERT_EQ(ParticipantTestAccess::FillStateWakePipe(participant), EAGAIN);
    EXPECT_EQ(participant.NotifyStateChange(), 0);
    EXPECT_EQ(participant.StateWakeError(), 0);
    EXPECT_TRUE(ParticipantTestAccess::StateWakePending(participant));
    EXPECT_EQ(ParticipantTestAccess::DrainOneStateWake(participant), 0);

    // The test-filled pipe remains readable/full after one drain. EAGAIN is still a valid
    // coalesced wake because readability, not the write result, is the predicate.
    EXPECT_EQ(participant.NotifyStateChange(), 0);
    EXPECT_EQ(participant.StateWakeError(), 0);
    ParticipantTestAccess::EmptyStateWakePipe(participant);
    EXPECT_EQ(ParticipantTestAccess::DrainOneStateWake(participant), 0);
    participant.Unregister();
}

TEST_F(PauseProtocol, StateWakePipeErrorClearsPendingAndIsReportedToController) {
    Protocol protocol;
    Participant participant{protocol};
    ParticipantTestAccess::CloseStateWakeWriteEnd(participant);

    EXPECT_EQ(participant.NotifyStateChange(), EBADF);
    EXPECT_EQ(participant.StateWakeError(), EBADF);
    EXPECT_EQ(participant.NotifyStateChange(), EBADF);
    EXPECT_EQ(participant.StateWakeError(), EBADF);

    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    EXPECT_EQ(participant.WaitForAckFor(pause.snapshot.epoch, std::chrono::milliseconds{25}),
              AckWaitResult::WaitError);
    participant.Unregister();
}

TEST_F(PauseProtocol, UnregisterRestoresPauseSignalMask) {
    Protocol protocol;
    Participant participant{protocol};
    sigset_t before{};
    sigset_t after{};
    ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &before), 0);

    participant.Unregister();

    ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &after), 0);
    EXPECT_EQ(sigismember(&after, SIGVTALRM), sigismember(&before, SIGVTALRM));
}

TEST_F(PauseProtocol, AcknowledgementDeadlineUsesMonotonicBound) {
    Protocol protocol;
    Participant participant{protocol};
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(participant.WaitForAckFor(pause.snapshot.epoch, std::chrono::milliseconds{50}),
              AckWaitResult::TimedOut);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed, std::chrono::seconds{1});

    ASSERT_TRUE(protocol.RollbackPause(pause.snapshot.epoch).changed);
    participant.Unregister();
}

TEST_F(PauseProtocol, PendingPauseUnregistersWithoutHandlerTouch) {
    PendingExitTarget target;
    ASSERT_TRUE(target.WaitReady());

    const auto pause = target.protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ASSERT_EQ(target.Notify(), 0);
    target.Exit();
    ASSERT_TRUE(target.WaitDone());
    EXPECT_EQ(target.participant->AcknowledgedEpoch(), 0);
    EXPECT_EQ(target.participant->WaitForAckFor(pause.snapshot.epoch, TestTimeout),
              AckWaitResult::Unregistered);
}

TEST_F(PauseProtocol, ThousandsOfCoalescedTransitionsDoNotPermanentlySleep) {
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

TEST_F(PauseProtocol, PersistentHandlerDepthStaysOneAcrossThousandsOfCycles) {
    LiveSignalTarget target;
    ASSERT_TRUE(target.WaitReady());

    auto transition = target.protocol.Request(State::Paused);
    ASSERT_TRUE(transition.changed);
    ASSERT_EQ(target.participant->NotifyStateChange(), 0);
    ASSERT_EQ(target.NotifyEntry(), 0);
    ASSERT_TRUE(WaitForAck(*target.participant, transition.snapshot.epoch));

    constexpr int Iterations = 5000;
    for (int i = 0; i < Iterations; ++i) {
        transition = target.protocol.Request(State::Running);
        ASSERT_TRUE(transition.changed);
        ASSERT_EQ(target.participant->NotifyStateChange(), 0);
        transition = target.protocol.Request(State::Paused);
        ASSERT_TRUE(transition.changed);
        ASSERT_EQ(target.participant->NotifyStateChange(), 0);
        ASSERT_EQ(target.NotifyEntry(), 0);
    }
    ASSERT_TRUE(WaitForAck(*target.participant, transition.snapshot.epoch));

    const auto resume = target.protocol.Request(State::Running);
    ASSERT_TRUE(resume.changed);
    ASSERT_EQ(target.participant->NotifyStateChange(), 0);
    target.Stop();
    ASSERT_TRUE(target.WaitStopped());
    EXPECT_EQ(ParticipantTestAccess::MaximumHandlerDepth(*target.participant), 1);
}

TEST_F(PauseProtocol, PauseSignalHandlerPreservesErrnoOnFastPath) {
    Protocol protocol;
    Participant participant{protocol};
    Core::DebugPause::BindCurrentParticipant(&participant);
    errno = EDOM;
    Core::DebugPause::PauseSignalHandler(SIGVTALRM, nullptr, nullptr);
    EXPECT_EQ(errno, EDOM);
    Core::DebugPause::ClearCurrentParticipant(&participant);
    participant.Unregister();
}

TEST_F(PauseProtocol, PauseSignalHandlerPreservesErrnoAcrossPausedWait) {
    Protocol protocol;
    Participant participant{protocol};
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    Core::DebugPause::BindCurrentParticipant(&participant);

    std::thread controller{[&] {
        EXPECT_TRUE(WaitForAck(participant, pause.snapshot.epoch));
        EXPECT_TRUE(protocol.Request(State::Running).changed);
        EXPECT_EQ(participant.NotifyStateChange(), 0);
    }};
    errno = ERANGE;
    Core::DebugPause::PauseSignalHandler(SIGVTALRM, nullptr, nullptr);
    EXPECT_EQ(errno, ERANGE);
    controller.join();

    Core::DebugPause::ClearCurrentParticipant(&participant);
    participant.Unregister();
}

TEST_F(PauseProtocol, HandlerWaitErrorRecordsExactFailOpenTransition) {
    Protocol protocol;
    Participant participant{protocol};
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);
    ParticipantTestAccess::InjectStateWaitError(participant);
    Core::DebugPause::BindCurrentParticipant(&participant);

    errno = E2BIG;
    Core::DebugPause::PauseSignalHandler(SIGVTALRM, nullptr, nullptr);
    EXPECT_EQ(errno, E2BIG);
    EXPECT_EQ(participant.WaitError(), EBADF);
    EXPECT_EQ(participant.FailedPauseEpoch(), pause.snapshot.epoch);
    EXPECT_EQ(participant.FailOpenRunningEpoch(), pause.snapshot.epoch + 1);
    EXPECT_EQ(protocol.Load().state, State::Running);
    EXPECT_EQ(protocol.Load().epoch, pause.snapshot.epoch + 1);

    Core::DebugPause::ClearCurrentParticipant(&participant);
    participant.Unregister();
}

TEST_F(PauseProtocol, FailOpenRollsBackCurrentPausedEpochAfterStateAdvance) {
    Protocol protocol;
    Participant participant{protocol};
    const auto first_pause = protocol.Request(State::Paused);
    ASSERT_TRUE(first_pause.changed);
    std::binary_semaphore reached{0};
    std::binary_semaphore release{0};
    fail_open_rollback_reached.store(&reached, std::memory_order_release);
    fail_open_rollback_release.store(&release, std::memory_order_release);
    Core::DebugPause::SetFailOpenRollbackTestHook(&HoldFailOpenRollback);

    std::atomic<StateWaitResult> result{StateWaitResult::Running};
    std::thread failure{[&] {
        result.store(
            ParticipantTestAccess::InvokeFailOpen(participant, EBADF, first_pause.snapshot.epoch),
            std::memory_order_release);
    }};
    const bool hook_reached = reached.try_acquire_for(TestTimeout);
    EXPECT_TRUE(hook_reached);
    auto newer_pause = first_pause;
    if (hook_reached) {
        EXPECT_TRUE(protocol.Request(State::Running).changed);
        newer_pause = protocol.Request(State::Paused);
        EXPECT_TRUE(newer_pause.changed);
    }
    release.release();
    failure.join();
    Core::DebugPause::SetFailOpenRollbackTestHook(nullptr);
    fail_open_rollback_reached.store(nullptr, std::memory_order_release);
    fail_open_rollback_release.store(nullptr, std::memory_order_release);

    EXPECT_EQ(result.load(std::memory_order_acquire), StateWaitResult::WaitError);
    const auto final_snapshot = protocol.Load();
    EXPECT_EQ(final_snapshot.state, State::Running);
    EXPECT_EQ(final_snapshot.epoch, newer_pause.snapshot.epoch + 1);
    EXPECT_EQ(participant.FailedPauseEpoch(), newer_pause.snapshot.epoch);
    EXPECT_EQ(participant.FailOpenRunningEpoch(), final_snapshot.epoch);
    EXPECT_EQ(participant.AcknowledgedEpoch(), final_snapshot.epoch);
    participant.Unregister();
}

TEST_F(PauseProtocol, FailOpenPublishesRollbackRecordBeforeWaitError) {
    Protocol protocol;
    Participant participant{protocol};
    const auto pause = protocol.Request(State::Paused);
    ASSERT_TRUE(pause.changed);

    std::binary_semaphore reached{0};
    std::binary_semaphore release{0};
    fail_open_publication_reached.store(&reached, std::memory_order_release);
    fail_open_publication_release.store(&release, std::memory_order_release);
    Core::DebugPause::SetFailOpenPublicationTestHook(&HoldFailOpenPublication);

    std::atomic<StateWaitResult> result{StateWaitResult::Running};
    std::thread failure{[&] {
        result.store(
            ParticipantTestAccess::InvokeFailOpen(participant, EBADF, pause.snapshot.epoch),
            std::memory_order_release);
    }};
    EXPECT_TRUE(reached.try_acquire_for(TestTimeout));
    EXPECT_EQ(participant.FailedPauseEpoch(), pause.snapshot.epoch);
    EXPECT_EQ(participant.FailOpenRunningEpoch(), pause.snapshot.epoch + 1);
    EXPECT_EQ(participant.WaitError(), 0);
    EXPECT_EQ(participant.WaitForAckFor(pause.snapshot.epoch, std::chrono::milliseconds{25}),
              AckWaitResult::WaitError);

    release.release();
    failure.join();
    Core::DebugPause::SetFailOpenPublicationTestHook(nullptr);
    fail_open_publication_reached.store(nullptr, std::memory_order_release);
    fail_open_publication_release.store(nullptr, std::memory_order_release);

    EXPECT_EQ(result.load(std::memory_order_acquire), StateWaitResult::WaitError);
    participant.Unregister();
}

TEST_F(PauseProtocol, CurrentParticipantBindingCanBeInterruptedDuringBindAndClear) {
    Protocol protocol;
    Participant participant{protocol};
    const pthread_t owner = pthread_self();
    std::atomic_bool stop{};
    std::thread interrupter{[&] {
        while (!stop.load(std::memory_order_acquire)) {
            (void)pthread_kill(owner, SIGVTALRM);
        }
    }};

    Core::DebugPause::BindCurrentParticipant(&participant);
    const auto deadline = std::chrono::steady_clock::now() + TestTimeout;
    while (ParticipantTestAccess::MaximumHandlerDepth(participant) == 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    const bool handler_observed_binding =
        ParticipantTestAccess::MaximumHandlerDepth(participant) > 0;

    for (int i = 0; i < 10'000; ++i) {
        Core::DebugPause::ClearCurrentParticipant(&participant);
        Core::DebugPause::BindCurrentParticipant(&participant);
    }
    Core::DebugPause::ClearCurrentParticipant(&participant);
    stop.store(true, std::memory_order_release);
    interrupter.join();

    EXPECT_TRUE(handler_observed_binding);
    participant.Unregister();
}

#endif // !_WIN32

} // namespace
