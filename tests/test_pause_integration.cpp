// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
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

void RegistrationPublished() noexcept {
    if (auto* barrier = registration_barrier.load(std::memory_order_acquire); barrier != nullptr) {
        barrier->release();
    }
}

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
        RegisteredTarget failed{false, true};
        RegisteredTarget leaving{true};
        if (!failed.WaitEntered() || !leaving.WaitEntered() || failed.SetupFailed() ||
            leaving.SetupFailed()) {
            return false;
        }
        const u64 initial_ptc = Libraries::Kernel::Dev::GetInitialPtc();
        std::thread controller{[&] { DebugState.PauseGuestThreads(); }};
        if (!WaitUntilPaused()) {
            return false;
        }
        leaving.Stop();
        leaving.Join();
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
