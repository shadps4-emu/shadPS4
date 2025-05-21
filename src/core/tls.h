// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include "common/types.h"

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

/// Swaps the TCB in or out of the fs register, if required by the platform.
void SwapTcb();

/// Makes sure TLS is initialized for the thread before entering guest.
void EnsureThreadInitialized();

template <size_t size>
__attribute__((optnone)) void ClearStack() {
    volatile void* buf = alloca(size);
    memset(const_cast<void*>(buf), 0, size);
    buf = nullptr;
}

template <class ReturnType, class... FuncArgs, class... CallArgs>
ReturnType ExecuteGuest(PS4_SYSV_ABI ReturnType (*func)(FuncArgs...), CallArgs&&... args) {
    EnsureThreadInitialized();
    // clear stack to avoid trash from EnsureThreadInitialized
    ClearStack<13_KB>();

    if constexpr (std::is_same_v<ReturnType, void>) {
        SwapTcb();
        func(std::forward<CallArgs>(args)...);
        SwapTcb();
    } else {
        SwapTcb();
        auto ret = func(std::forward<CallArgs>(args)...);
        SwapTcb();
        return ret;
    }
}

template <class F, F f>
struct HostCallWrapperImpl;

template <class ReturnType, class... Args, PS4_SYSV_ABI ReturnType (*func)(Args...)>
struct HostCallWrapperImpl<PS4_SYSV_ABI ReturnType (*)(Args...), func> {
    static ReturnType PS4_SYSV_ABI wrap(Args... args) {
        if constexpr (std::is_same_v<ReturnType, void>) {
            SwapTcb();
            func(args...);
            SwapTcb();
        } else {
            SwapTcb();
            auto ret = func(args...);
            SwapTcb();
            return ret;
        }
    }
};

#define HOST_CALL(func) (Core::HostCallWrapperImpl<decltype(&(func)), func>::wrap)

} // namespace Core
