// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include "common/scope_exit.h"
#include "common/types.h"
#ifdef _WIN32
#include <malloc.h>
#endif

namespace Xbyak {
class CodeGenerator;
}

namespace Libraries::Fiber {
struct OrbisFiberContext;
}

namespace Core {

union DtvEntry {
    std::size_t counter;
    u8* pointer;
};

struct Tcb {
    Tcb* tcb_self;
    DtvEntry* tcb_dtv;
    void* tcb_thread;
    void* tcb_spare[2];
    u64 tcb_canary;
    ::Libraries::Fiber::OrbisFiberContext* tcb_fiber;
};

#ifdef _WIN32
/// Gets the thread local storage key for the TCB block.
u32 GetTcbKey();
#endif

/// Sets the data pointer to the TCB block.
void SetTcbBase(void* image_address);

/// Retrieves Tcb structure for the calling thread.
Tcb* GetTcbBase();

/// Makes sure TLS is initialized for the thread before entering guest.
void InitializeTLS();

/// Return address of the most recent guest call into an HLE function on this thread.
inline thread_local VAddr g_guest_caller_address{};

template <auto f>
struct HostCallWrapperImpl;

template <class ReturnType, class... Args, PS4_SYSV_ABI ReturnType (*func)(Args...)>
struct HostCallWrapperImpl<func> {
    static ReturnType PS4_SYSV_ABI wrap(Args... args) {
        const VAddr previous_caller = g_guest_caller_address;
        g_guest_caller_address = reinterpret_cast<VAddr>(__builtin_return_address(0));
        SCOPE_EXIT {
            g_guest_caller_address = previous_caller;
        };
        return func(args...);
    }
};

#define HOST_CALL(func) (Core::HostCallWrapperImpl<func>::wrap)

} // namespace Core
