// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <future>
#include "common/alignment.h"
#include "common/arch.h"
#include "core/cpu_affinity.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "thread.h"
#ifdef _WIN64
#include <windows.h>
#include "common/ntapi.h"
#else
#include <csignal>
#include <pthread.h>
#include <unistd.h>
#ifdef ARCH_X86_64
#include <xmmintrin.h>
#endif
#endif

namespace Core {

static constexpr u32 ORBIS_MXCSR = 0x9fc0;
static constexpr u32 ORBIS_FPUCW = 0x037f;

NativeThread::NativeThread() : native_handle{0} {}

NativeThread::~NativeThread() {}

int NativeThread::Create(ThreadFunc func, void* arg, u64 affinity_mask,
                         std::vector<int> host_cpus) {
    cpu_affinity = CpuAffinity{std::move(host_cpus)};
    struct Startup {
        ThreadFunc func;
        void* arg;
        CpuAffinity& affinity;
        u64 mask;
        std::promise<int> ready;
        std::future<void> start;
    };
    std::promise<void> start;
    auto startup = std::make_unique<Startup>(func, arg, cpu_affinity, affinity_mask,
                                             std::promise<int>{}, start.get_future());
    auto ready = startup->ready.get_future();
    const auto entry = [](void* data)
#ifdef _WIN64
        -> DWORD
#else
        -> void*
#endif
    {
        std::unique_ptr<Startup> state{static_cast<Startup*>(data)};
        const auto func = state->func;
        const auto arg = state->arg;
        const int ret = state->affinity.SetThreadAffinity(0, state->mask);
        state->ready.set_value(ret);
        state->start.wait();
        state.reset();
        if (ret != 0) {
            return 0;
        }
        return func(arg);
    };
#ifndef _WIN64
    pthread_t* pthr = reinterpret_cast<pthread_t*>(&native_handle);
    const int ret = pthread_create(pthr, nullptr, entry, startup.get());
    if (ret != 0) {
        return POSIX_EAGAIN;
    }
#else
    native_handle = CreateThread(nullptr, 0, entry, startup.get(), 0, nullptr);
    if (native_handle == nullptr) {
        return POSIX_EAGAIN;
    }
#endif
    startup.release();
    const int error = ready.get();
    start.set_value();
    if (error != 0) {
#ifdef _WIN64
        WaitForSingleObject(native_handle, INFINITE);
        CloseHandle(native_handle);
#else
        pthread_join(reinterpret_cast<pthread_t>(native_handle), nullptr);
#endif
        native_handle = 0;
    }
    return error;
}

void NativeThread::Exit() {
    if (!native_handle) {
        return;
    }

    tid = 0;

#ifdef _WIN64
    native_handle = nullptr;
    ExitThread(0);
#else
    // Disable and free the signal stack.
    constexpr stack_t sig_stack = {
        .ss_flags = SS_DISABLE,
    };
    sigaltstack(&sig_stack, nullptr);

    if (sig_stack_ptr) {
        free(sig_stack_ptr);
        sig_stack_ptr = nullptr;
    }
    pthread_exit(nullptr);
#endif
}

void NativeThread::Initialize() {
#ifdef ARCH_X86_64
    // Set MXCSR and FPUCW registers to the values used by Orbis.
    _mm_setcsr(ORBIS_MXCSR);
    asm volatile("fldcw %0" : : "m"(ORBIS_FPUCW));
#endif
#if _WIN64
    tid = GetCurrentThreadId();
#else
    tid = (u64)pthread_self();

    // Set up an alternate signal handler stack to avoid overflowing small thread stacks.
    const size_t page_size = getpagesize();
    const size_t sig_stack_size = Common::AlignUp(std::max<size_t>(64_KB, MINSIGSTKSZ), page_size);
    ASSERT_MSG(posix_memalign(&sig_stack_ptr, page_size, sig_stack_size) == 0,
               "Failed to allocate signal stack: {}", errno);

    stack_t sig_stack;
    sig_stack.ss_sp = sig_stack_ptr;
    sig_stack.ss_size = sig_stack_size;
    sig_stack.ss_flags = 0;
    ASSERT_MSG(sigaltstack(&sig_stack, nullptr) == 0, "Failed to set signal stack: {}", errno);
#endif
}

} // namespace Core
