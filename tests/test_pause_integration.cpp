// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <limits>
#include <semaphore>
#include <thread>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include "common/native_clock.h"
#include "core/debug_state.h"
#include "core/libraries/kernel/time.h"
#include "core/pause_protocol.h"
#include "core/signals.h"

namespace Libraries::Kernel::Dev {
namespace {
Common::NativeClock integration_clock;
u64 integration_initial_ptc{};
} // namespace

u64& GetInitialPtc() {
    return integration_initial_ptc;
}

Common::NativeClock* GetClock() {
    return &integration_clock;
}
} // namespace Libraries::Kernel::Dev

namespace {

using namespace std::chrono_literals;
constexpr auto TestTimeout = 5s;

std::atomic<std::binary_semaphore*> registration_barrier{};
std::atomic_bool fail_open_record_reached{};
std::atomic_bool fail_open_record_release{};
std::atomic<std::binary_semaphore*> fail_open_timestamp_reached{};
std::atomic_bool fail_open_timestamp_release{};
std::atomic<std::binary_semaphore*> fail_open_running_reached{};
std::atomic_bool fail_open_running_release{};
std::atomic<std::binary_semaphore*> fail_open_completion_reached{};
std::atomic_bool fail_open_completion_release{};
std::atomic<std::binary_semaphore*> state_wait_reached{};
std::atomic_bool state_wait_release{};
std::atomic<std::binary_semaphore*> rollback_completion_wait_reached{};
std::atomic_bool rollback_completion_wait_release{};

void RegistrationPublished() noexcept {
    if (auto* barrier = registration_barrier.load(std::memory_order_acquire); barrier != nullptr) {
        barrier->release();
    }
}

void HoldFailOpenRecord() noexcept {
    // This hook runs in the real pause handler. Keep the barrier to lock-free atomics only.
    fail_open_record_reached.store(true, std::memory_order_release);
    while (!fail_open_record_release.load(std::memory_order_acquire)) {
    }
}

void HoldFailOpenTimestamp() noexcept {
    if (auto* reached = fail_open_timestamp_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    while (!fail_open_timestamp_release.load(std::memory_order_acquire)) {
    }
}

void HoldFailOpenRunning() noexcept {
    if (auto* reached = fail_open_running_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    while (!fail_open_running_release.load(std::memory_order_acquire)) {
    }
}

void HoldFailOpenCompletion() noexcept {
    if (auto* reached = fail_open_completion_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    while (!fail_open_completion_release.load(std::memory_order_acquire)) {
    }
}

void HoldStateWait() noexcept {
    if (auto* reached = state_wait_reached.load(std::memory_order_acquire); reached != nullptr) {
        reached->release();
    }
    while (!state_wait_release.load(std::memory_order_acquire)) {
    }
}

void HoldRollbackCompletionWait() noexcept {
    if (auto* reached = rollback_completion_wait_reached.load(std::memory_order_acquire);
        reached != nullptr) {
        reached->release();
    }
    while (!rollback_completion_wait_release.load(std::memory_order_acquire)) {
    }
}

class ScopedStateWaitHook {
public:
    explicit ScopedStateWaitHook(std::binary_semaphore& reached) : reached{reached} {
        state_wait_reached.store(&this->reached, std::memory_order_release);
        state_wait_release.store(false, std::memory_order_release);
        Core::DebugPause::SetStateWaitTestHook(&HoldStateWait);
    }

    ScopedStateWaitHook(const ScopedStateWaitHook&) = delete;
    ScopedStateWaitHook& operator=(const ScopedStateWaitHook&) = delete;

    ~ScopedStateWaitHook() {
        Release();
        Core::DebugPause::SetStateWaitTestHook(nullptr);
        state_wait_reached.store(nullptr, std::memory_order_release);
    }

    void Release() {
        state_wait_release.store(true, std::memory_order_release);
    }

private:
    std::binary_semaphore& reached;
};

class ScopedRollbackCompletionWaitHook {
public:
    explicit ScopedRollbackCompletionWaitHook(std::binary_semaphore& reached) : reached{reached} {
        rollback_completion_wait_reached.store(&this->reached, std::memory_order_release);
        rollback_completion_wait_release.store(false, std::memory_order_release);
        Core::DebugPause::SetRollbackCompletionWaitTestHook(&HoldRollbackCompletionWait);
    }

    ScopedRollbackCompletionWaitHook(const ScopedRollbackCompletionWaitHook&) = delete;
    ScopedRollbackCompletionWaitHook& operator=(const ScopedRollbackCompletionWaitHook&) = delete;

    ~ScopedRollbackCompletionWaitHook() {
        Release();
        Core::DebugPause::SetRollbackCompletionWaitTestHook(nullptr);
        rollback_completion_wait_reached.store(nullptr, std::memory_order_release);
    }

    void Release() {
        rollback_completion_wait_release.store(true, std::memory_order_release);
    }

private:
    std::binary_semaphore& reached;
};

class ScopedFailOpenTimestampHook {
public:
    explicit ScopedFailOpenTimestampHook(std::binary_semaphore& reached) : reached{reached} {
        fail_open_timestamp_reached.store(&this->reached, std::memory_order_release);
        fail_open_timestamp_release.store(false, std::memory_order_release);
        Core::DebugPause::SetFailOpenTimestampTestHook(&HoldFailOpenTimestamp);
    }

    ScopedFailOpenTimestampHook(const ScopedFailOpenTimestampHook&) = delete;
    ScopedFailOpenTimestampHook& operator=(const ScopedFailOpenTimestampHook&) = delete;

    ~ScopedFailOpenTimestampHook() {
        Release();
        Core::DebugPause::SetFailOpenTimestampTestHook(nullptr);
        fail_open_timestamp_reached.store(nullptr, std::memory_order_release);
    }

    void Release() {
        fail_open_timestamp_release.store(true, std::memory_order_release);
    }

private:
    std::binary_semaphore& reached;
};

class ScopedFailOpenRecordHook {
public:
    ScopedFailOpenRecordHook() {
        fail_open_record_reached.store(false, std::memory_order_release);
        fail_open_record_release.store(false, std::memory_order_release);
        Core::DebugPause::SetFailOpenRecordTestHook(&HoldFailOpenRecord);
    }

    ScopedFailOpenRecordHook(const ScopedFailOpenRecordHook&) = delete;
    ScopedFailOpenRecordHook& operator=(const ScopedFailOpenRecordHook&) = delete;

    ~ScopedFailOpenRecordHook() {
        Release();
        Core::DebugPause::SetFailOpenRecordTestHook(nullptr);
    }

    bool Reached() const {
        return fail_open_record_reached.load(std::memory_order_acquire);
    }

    void Release() {
        fail_open_record_release.store(true, std::memory_order_release);
    }
};

class ScopedFailOpenTestHooks {
public:
    ScopedFailOpenTestHooks(std::binary_semaphore& timestamp_reached,
                            std::binary_semaphore& running_reached,
                            std::binary_semaphore& completion_reached)
        : timestamp_reached{timestamp_reached}, running_reached{running_reached},
          completion_reached{completion_reached} {
        fail_open_timestamp_reached.store(&this->timestamp_reached, std::memory_order_release);
        fail_open_running_reached.store(&this->running_reached, std::memory_order_release);
        fail_open_completion_reached.store(&this->completion_reached, std::memory_order_release);
        fail_open_timestamp_release.store(false, std::memory_order_release);
        fail_open_running_release.store(false, std::memory_order_release);
        fail_open_completion_release.store(false, std::memory_order_release);
        Core::DebugPause::SetFailOpenTimestampTestHook(&HoldFailOpenTimestamp);
        Core::DebugPause::SetFailOpenRunningTestHook(&HoldFailOpenRunning);
        Core::DebugPause::SetFailOpenCompletionTestHook(&HoldFailOpenCompletion);
    }

    ScopedFailOpenTestHooks(const ScopedFailOpenTestHooks&) = delete;
    ScopedFailOpenTestHooks& operator=(const ScopedFailOpenTestHooks&) = delete;

    ~ScopedFailOpenTestHooks() {
        fail_open_timestamp_release.store(true, std::memory_order_release);
        fail_open_running_release.store(true, std::memory_order_release);
        fail_open_completion_release.store(true, std::memory_order_release);
        Core::DebugPause::SetFailOpenTimestampTestHook(nullptr);
        Core::DebugPause::SetFailOpenRunningTestHook(nullptr);
        Core::DebugPause::SetFailOpenCompletionTestHook(nullptr);
        fail_open_timestamp_reached.store(nullptr, std::memory_order_release);
        fail_open_running_reached.store(nullptr, std::memory_order_release);
        fail_open_completion_reached.store(nullptr, std::memory_order_release);
    }

    void ReleaseTimestamp() {
        fail_open_timestamp_release.store(true, std::memory_order_release);
    }

    void ReleaseRunning() {
        fail_open_running_release.store(true, std::memory_order_release);
    }

    void ReleaseCompletion() {
        fail_open_completion_release.store(true, std::memory_order_release);
    }

private:
    std::binary_semaphore& timestamp_reached;
    std::binary_semaphore& running_reached;
    std::binary_semaphore& completion_reached;
};

class ScopedRegistrationHook {
public:
    explicit ScopedRegistrationHook(std::binary_semaphore& barrier) : barrier{barrier} {
        registration_barrier.store(&this->barrier, std::memory_order_release);
        DebugStateType::SetPauseRegistrationTestHook(&RegistrationPublished);
    }

    ScopedRegistrationHook(const ScopedRegistrationHook&) = delete;
    ScopedRegistrationHook& operator=(const ScopedRegistrationHook&) = delete;

    ~ScopedRegistrationHook() {
        DebugStateType::SetPauseRegistrationTestHook(nullptr);
        registration_barrier.store(nullptr, std::memory_order_release);
    }

private:
    std::binary_semaphore& barrier;
};

class RegisteredTarget {
public:
    explicit RegisteredTarget(bool block_pause = false, bool inject_wait_error = false)
        : block_pause_signal{block_pause}, inject_error{inject_wait_error},
          thread{[this] { Run(); }} {}

    ~RegisteredTarget() {
        if (DebugState.IsGuestThreadsPaused()) {
            DebugState.ResumeGuestThreads();
        }
        Stop();
        Join();
    }

    bool WaitEntered() {
        return entered.try_acquire_for(TestTimeout);
    }

    bool HasEntered() const {
        return entered_guest.load(std::memory_order_acquire);
    }

    bool SetupFailed() const {
        return setup_failed.load(std::memory_order_acquire);
    }

    ThreadID Id() {
        return thread.native_handle();
    }

    int Notify() {
        return pthread_kill(thread.native_handle(), SIGVTALRM);
    }

    void Stop() {
        stop.store(true, std::memory_order_release);
    }

    void Join() {
        if (thread.joinable()) {
            thread.join();
        }
    }

private:
    void Run() {
        sigset_t old_mask{};
        sigset_t run_mask{};
        if (pthread_sigmask(SIG_SETMASK, nullptr, &old_mask) != 0) {
            setup_failed.store(true, std::memory_order_release);
            entered.release();
            return;
        }
        run_mask = old_mask;
        const int mask_result =
            block_pause_signal ? sigaddset(&run_mask, SIGVTALRM) : sigdelset(&run_mask, SIGVTALRM);
        if (mask_result != 0 || pthread_sigmask(SIG_SETMASK, &run_mask, nullptr) != 0) {
            setup_failed.store(true, std::memory_order_release);
            entered.release();
            return;
        }

        DebugState.AddCurrentThreadToGuestList();
        if (inject_error) {
            Core::DebugPause::InjectCurrentParticipantStateWaitErrorForTest();
        }
        entered_guest.store(true, std::memory_order_release);
        entered.release();
        while (!stop.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        DebugState.RemoveCurrentThreadFromGuestList();

        (void)pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
    }

    bool block_pause_signal{};
    bool inject_error{};
    std::atomic_bool entered_guest{};
    std::atomic_bool setup_failed{};
    std::atomic_bool stop{};
    std::binary_semaphore entered{0};
    std::thread thread;
};

bool WaitUntilPaused() {
    const auto deadline = std::chrono::steady_clock::now() + TestTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (DebugState.TestPauseSnapshot().state == Core::DebugPause::State::Paused) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

bool RunIntegrationChild() {
    Core::SignalDispatch dispatch{Core::SignalDispatch::PauseOnlyTestTag{}};

    {
        RegisteredTarget first;
        RegisteredTarget second;
        if (!first.WaitEntered() || !second.WaitEntered() || first.SetupFailed() ||
            second.SetupFailed() || DebugState.TestGuestThreadCount() != 2) {
            return false;
        }
        DebugState.PauseGuestThreads();
        if (!DebugState.IsGuestThreadsPaused() || !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }
        DebugState.ResumeGuestThreads();
        first.Stop();
        second.Stop();
        first.Join();
        second.Join();
        if (DebugState.TestGuestThreadCount() != 0) {
            return false;
        }
    }

    {
        RegisteredTarget target;
        if (!target.WaitEntered() || target.SetupFailed()) {
            return false;
        }
        std::counting_semaphore<2> start{0};
        std::atomic_bool first_returned{};
        std::atomic_bool second_returned{};
        std::thread first{[&] {
            start.acquire();
            DebugState.PauseGuestThreads();
            first_returned.store(true, std::memory_order_release);
        }};
        std::thread second{[&] {
            start.acquire();
            DebugState.PauseGuestThreads();
            second_returned.store(true, std::memory_order_release);
        }};
        start.release(2);
        first.join();
        second.join();
        if (!first_returned.load(std::memory_order_acquire) ||
            !second_returned.load(std::memory_order_acquire) ||
            DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Paused ||
            !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }
        DebugState.ResumeGuestThreads();
        target.Stop();
        target.Join();
    }

    {
        DebugState.PauseGuestThreads();
        std::binary_semaphore published{0};
        ScopedRegistrationHook hook{published};
        RegisteredTarget newcomer;
        if (!published.try_acquire_for(TestTimeout) || newcomer.HasEntered()) {
            return false;
        }
        DebugState.ResumeGuestThreads();
        if (!newcomer.WaitEntered()) {
            return false;
        }
        newcomer.Stop();
        newcomer.Join();
    }

    {
        RegisteredTarget failing;
        if (!failing.WaitEntered() || failing.SetupFailed()) {
            return false;
        }
        DebugState.PauseGuestThreads();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Paused ||
            !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }

        std::binary_semaphore timestamp_reached{0};
        std::binary_semaphore state_wait_reached_signal{0};
        ScopedFailOpenTimestampHook timestamp_hook{timestamp_reached};
        ScopedFailOpenRecordHook record_hook;
        ScopedStateWaitHook state_wait_hook{state_wait_reached_signal};
        std::binary_semaphore registration_published{0};
        ScopedRegistrationHook registration_hook{registration_published};
        DebugState.TestTriggerWaitError();

        bool scenario_ok = timestamp_reached.try_acquire_for(TestTimeout);
        scenario_ok = scenario_ok &&
                      DebugState.TestPauseSnapshot().state == Core::DebugPause::State::Running &&
                      DebugState.TestFailOpenPublicationPending();

        RegisteredTarget newcomer;
        scenario_ok = scenario_ok && registration_published.try_acquire_for(TestTimeout);
        const bool state_wait_hook_reached = state_wait_reached_signal.try_acquire_for(TestTimeout);
        scenario_ok = scenario_ok && state_wait_hook_reached && !newcomer.HasEntered();

        timestamp_hook.Release();
        const auto record_deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (!record_hook.Reached() && std::chrono::steady_clock::now() < record_deadline) {
            std::this_thread::yield();
        }
        scenario_ok = scenario_ok && record_hook.Reached();
        record_hook.Release();

        const auto publication_deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (DebugState.TestFailOpenPublicationPending() &&
               std::chrono::steady_clock::now() < publication_deadline) {
            std::this_thread::yield();
        }
        scenario_ok = scenario_ok && !DebugState.TestFailOpenPublicationPending();
        scenario_ok = scenario_ok && !newcomer.HasEntered();

        state_wait_hook.Release();
        if (state_wait_hook_reached) {
            scenario_ok = scenario_ok && newcomer.Notify() == 0;
        }
        scenario_ok = scenario_ok && newcomer.WaitEntered();

        newcomer.Stop();
        newcomer.Join();
        failing.Stop();
        failing.Join();
        DebugState.ResumeGuestThreads();
        if (!scenario_ok || DebugState.TestGuestThreadCount() != 0) {
            return false;
        }
    }

    {
        RegisteredTarget unregistering{true};
        if (!unregistering.WaitEntered()) {
            return false;
        }
        std::atomic_bool pause_returned{};
        std::thread controller{[&] {
            DebugState.PauseGuestThreads();
            pause_returned.store(true, std::memory_order_release);
        }};
        if (!WaitUntilPaused()) {
            return false;
        }
        unregistering.Stop();
        unregistering.Join();
        controller.join();
        if (!pause_returned.load(std::memory_order_acquire)) {
            return false;
        }
        DebugState.ResumeGuestThreads();
    }

    {
        RegisteredTarget first;
        RegisteredTarget second;
        if (!first.WaitEntered() || !second.WaitEntered() || first.SetupFailed() ||
            second.SetupFailed()) {
            return false;
        }
        DebugState.PauseGuestThreads();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Paused ||
            !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }

        const u64 pause_time = DebugState.TestPauseTime();
        const u64 paused_epoch = DebugState.TestPauseSnapshot().epoch;
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        std::binary_semaphore timestamp_reached{0};
        std::binary_semaphore running_reached{0};
        std::binary_semaphore completion_reached{0};
        ScopedFailOpenTestHooks hooks{timestamp_reached, running_reached, completion_reached};
        const auto first_id = first.Id();
        const auto second_id = second.Id();
        DebugState.TestTriggerWaitError();
        const bool timestamp_hook_reached = timestamp_reached.try_acquire_for(TestTimeout);
        const bool running_hook_reached = running_reached.try_acquire_for(TestTimeout);
        if (!timestamp_hook_reached || !running_hook_reached ||
            DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            !DebugState.TestPauseBookkeepingSet() || !DebugState.TestFailOpenPublicationPending()) {
            return false;
        }

        hooks.ReleaseTimestamp();
        const u64 running_epoch = paused_epoch + 1;
        const auto winner_deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (DebugState.TestFailOpenRunningEpoch(first_id) != running_epoch &&
               DebugState.TestFailOpenRunningEpoch(second_id) != running_epoch &&
               std::chrono::steady_clock::now() < winner_deadline) {
            std::this_thread::yield();
        }
        const bool first_won = DebugState.TestFailOpenRunningEpoch(first_id) == running_epoch;
        const bool second_won = DebugState.TestFailOpenRunningEpoch(second_id) == running_epoch;
        if (!first_won && !second_won) {
            return false;
        }

        RegisteredTarget* winner = first_won ? &first : &second;
        RegisteredTarget* loser = first_won ? &second : &first;
        winner->Stop();
        winner->Join();
        if (DebugState.TestGuestThreadCount() != 1 ||
            !DebugState.TestFailOpenPublicationPending()) {
            return false;
        }
        const u64 fail_open_time = DebugState.TestFailOpenUptime();
        const u64 expected = fail_open_time > pause_time ? fail_open_time - pause_time : 0;
        if (fail_open_time == std::numeric_limits<u64>::max() ||
            Libraries::Kernel::Dev::GetInitialPtc() != initial_ptc) {
            return false;
        }

        DebugState.TestClearWaitErrors();
        std::binary_semaphore completion_wait_reached{0};
        ScopedRollbackCompletionWaitHook completion_wait_hook{completion_wait_reached};
        std::atomic_bool pause_returned{};
        std::thread controller{[&] {
            DebugState.PauseGuestThreads();
            pause_returned.store(true, std::memory_order_release);
        }};
        const bool completion_wait_hook_reached =
            completion_wait_reached.try_acquire_for(TestTimeout);
        if (!completion_wait_hook_reached) {
            hooks.ReleaseRunning();
            completion_wait_hook.Release();
            hooks.ReleaseCompletion();
            controller.join();
            return false;
        }
        const bool pause_waited_for_last_completion =
            !pause_returned.load(std::memory_order_acquire);
        const bool no_early_adjustment = Libraries::Kernel::Dev::GetInitialPtc() == initial_ptc;

        hooks.ReleaseRunning();
        if (!completion_reached.try_acquire_for(TestTimeout)) {
            hooks.ReleaseCompletion();
            completion_wait_hook.Release();
            controller.join();
            return false;
        }
        DebugState.TestClearWaitErrors();
        loser->Stop();
        completion_wait_hook.Release();
        hooks.ReleaseCompletion();
        controller.join();
        if (!pause_waited_for_last_completion || !no_early_adjustment ||
            Libraries::Kernel::Dev::GetInitialPtc() - initial_ptc != expected ||
            DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Paused ||
            !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }

        DebugState.ResumeGuestThreads();
        loser->Join();
        const u64 after_resume_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        DebugState.PauseGuestThreads();
        if (Libraries::Kernel::Dev::GetInitialPtc() != after_resume_ptc) {
            return false;
        }
    }

    {
        RegisteredTarget failing;
        if (!failing.WaitEntered() || failing.SetupFailed()) {
            return false;
        }
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        DebugStateType::SetPauseSignalDeliveryFailureForTest(true);
        DebugState.PauseGuestThreads();
        DebugStateType::SetPauseSignalDeliveryFailureForTest(false);
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            DebugState.TestPauseBookkeepingSet() ||
            Libraries::Kernel::Dev::GetInitialPtc() < initial_ptc) {
            return false;
        }

        // The failed edge left the target registered with a coalesced pipe token. A subsequent
        // real pause/resume must still be able to consume that token and complete normally.
        DebugState.PauseGuestThreads();
        if (!DebugState.IsGuestThreadsPaused()) {
            return false;
        }
        DebugState.ResumeGuestThreads();
        failing.Stop();
        failing.Join();
    }

    {
        RegisteredTarget failing{false, true};
        if (!failing.WaitEntered()) {
            return false;
        }
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        DebugState.PauseGuestThreads();
        const auto snapshot = DebugState.TestPauseSnapshot();
        if (snapshot.state != Core::DebugPause::State::Running ||
            DebugState.TestPauseBookkeepingSet() ||
            Libraries::Kernel::Dev::GetInitialPtc() <= initial_ptc) {
            return false;
        }
        failing.Stop();
        failing.Join();
    }

    {
        RegisteredTarget failing;
        if (!failing.WaitEntered() || failing.SetupFailed()) {
            return false;
        }
        DebugState.PauseGuestThreads();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Paused ||
            !DebugState.TestPauseBookkeepingSet()) {
            return false;
        }
        const u64 pause_time = DebugState.TestPauseTime();
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        fail_open_record_reached.store(false, std::memory_order_release);
        fail_open_record_release.store(false, std::memory_order_release);
        Core::DebugPause::SetFailOpenRecordTestHook(&HoldFailOpenRecord);
        DebugState.TestTriggerWaitError();
        const auto record_deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (!fail_open_record_reached.load(std::memory_order_acquire) &&
               std::chrono::steady_clock::now() < record_deadline) {
            std::this_thread::yield();
        }
        if (!fail_open_record_reached.load(std::memory_order_acquire) ||
            DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            !DebugState.TestPauseBookkeepingSet()) {
            fail_open_record_release.store(true, std::memory_order_release);
            return false;
        }
        fail_open_record_release.store(true, std::memory_order_release);
        Core::DebugPause::SetFailOpenRecordTestHook(nullptr);
        const auto deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (std::chrono::steady_clock::now() < deadline &&
               (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
                DebugState.TestFailOpenRunningEpoch() == std::numeric_limits<u64>::max())) {
            std::this_thread::yield();
        }
        const u64 fail_open_time = DebugState.TestFailOpenUptime();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            fail_open_time == std::numeric_limits<u64>::max()) {
            return false;
        }
        std::this_thread::sleep_for(50ms);
        failing.Stop();
        failing.Join();
        DebugState.ResumeGuestThreads();
        const u64 accounted = Libraries::Kernel::Dev::GetInitialPtc() - initial_ptc;
        const u64 expected = fail_open_time > pause_time ? fail_open_time - pause_time : 0;
        if (accounted > expected + 20'000'000 || DebugState.TestPauseBookkeepingSet()) {
            return false;
        }
    }

    {
        RegisteredTarget failed{false, true};
        RegisteredTarget leaving{true};
        if (!failed.WaitEntered() || !leaving.WaitEntered() || failed.SetupFailed() ||
            leaving.SetupFailed()) {
            return false;
        }
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        std::binary_semaphore timestamp_reached{0};
        ScopedFailOpenTimestampHook timestamp_hook{timestamp_reached};
        std::thread controller{[&] { DebugState.PauseGuestThreads(); }};
        if (!timestamp_reached.try_acquire_for(TestTimeout)) {
            timestamp_hook.Release();
            leaving.Stop();
            leaving.Join();
            controller.join();
            return false;
        }
        const auto bookkeeping_deadline = std::chrono::steady_clock::now() + TestTimeout;
        while (!DebugState.TestPauseBookkeepingSet() &&
               std::chrono::steady_clock::now() < bookkeeping_deadline) {
            std::this_thread::yield();
        }
        if (!DebugState.TestPauseBookkeepingSet()) {
            timestamp_hook.Release();
            leaving.Stop();
            leaving.Join();
            controller.join();
            return false;
        }
        leaving.Stop();
        leaving.Join();
        timestamp_hook.Release();
        controller.join();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            DebugState.TestPauseBookkeepingSet() ||
            Libraries::Kernel::Dev::GetInitialPtc() <= initial_ptc) {
            return false;
        }
        failed.Stop();
        failed.Join();
    }

    {
        DebugState.AddCurrentThreadToGuestList();
        Core::DebugPause::InjectCurrentParticipantStateWaitErrorForTest();
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        DebugState.PauseGuestThreads();
        if (DebugState.TestPauseSnapshot().state != Core::DebugPause::State::Running ||
            DebugState.TestPauseBookkeepingSet() ||
            Libraries::Kernel::Dev::GetInitialPtc() <= initial_ptc) {
            return false;
        }
        DebugState.RemoveCurrentThreadFromGuestList();
    }

    return DebugState.TestGuestThreadCount() == 0;
}

TEST(PauseProductionIntegration, SignalDispatchDebugStateLifecycleAndRollback) {
    const pid_t child = fork();
    ASSERT_GE(child, 0);
    if (child == 0) {
        _exit(RunIntegrationChild() ? 0 : 1);
    }

    int status{};
    bool reaped = false;
    bool timed_out = false;
    const auto deadline = std::chrono::steady_clock::now() + 15s;
    while (!reaped) {
        const pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            reaped = true;
            break;
        }
        if (result < 0 && errno != EINTR) {
            break;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            timed_out = true;
            (void)kill(child, SIGKILL);
            break;
        }
        std::this_thread::sleep_for(10ms);
    }
    if (!reaped) {
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
        }
        reaped = true;
    }
    ASSERT_TRUE(reaped);
    ASSERT_FALSE(timed_out);
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

} // namespace
