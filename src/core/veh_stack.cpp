// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/veh_stack.h"

#ifdef _WIN64

#include <windows.h>

namespace Core {
namespace {

struct VehStackState {
    void* main_fiber{};
    void* handler_fiber{};

    VehHandlerFn pending_fn{};
    _EXCEPTION_POINTERS* pending_exp{};
    long result{};

    bool initialized{};
    bool converted_thread{};
};

thread_local VehStackState g_veh_stack_state{};

VOID WINAPI VehStackFiberProc(LPVOID param) {
    auto* state = static_cast<VehStackState*>(param);
    for (;;) {
        const auto fn = state->pending_fn;
        auto* exp = state->pending_exp;

        // If something went wrong, fail closed by returning "not handled".
        state->result = fn ? fn(exp) : EXCEPTION_CONTINUE_SEARCH;

        state->pending_fn = nullptr;
        state->pending_exp = nullptr;

        SwitchToFiber(state->main_fiber);
    }
}

} // namespace

void InitializeVehStackForCurrentThread() noexcept {
    auto& state = g_veh_stack_state;
    if (state.initialized) {
        return;
    }

    void* main_fiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
    if (main_fiber == nullptr) {
        const DWORD err = GetLastError();
        if (err == ERROR_ALREADY_FIBER) {
            main_fiber = GetCurrentFiber();
        } else {
            // Unable to set up a dedicated handler stack for this thread. Keep going without it.
            state.initialized = true;
            return;
        }
    } else {
        state.converted_thread = true;
    }

    state.main_fiber = main_fiber;

    // Match the non-Windows alt-stack intent (avoid running heavy handler code on small stacks),
    // while keeping per-thread overhead reasonable.
    constexpr SIZE_T commit_size = 64 * 1024;
    constexpr SIZE_T reserve_size = 256 * 1024;

    state.handler_fiber = CreateFiberEx(commit_size, reserve_size, FIBER_FLAG_FLOAT_SWITCH,
                                        VehStackFiberProc, &state);
    state.initialized = true;
}

void CleanupVehStackForCurrentThread() noexcept {
    auto& state = g_veh_stack_state;
    if (state.handler_fiber) {
        // Don't attempt to delete the currently executing fiber.
        if (state.main_fiber && GetCurrentFiber() != state.handler_fiber) {
            DeleteFiber(state.handler_fiber);
            state.handler_fiber = nullptr;
        }
    }

    if (state.converted_thread && state.main_fiber && GetCurrentFiber() == state.main_fiber) {
        ConvertFiberToThread();
        state.main_fiber = nullptr;
        state.converted_thread = false;
    }
}

long RunOnVehStack(VehHandlerFn fn, _EXCEPTION_POINTERS* exp) noexcept {
    auto& state = g_veh_stack_state;
    if (!state.initialized) {
        InitializeVehStackForCurrentThread();
    }

    if (!state.main_fiber || !state.handler_fiber) {
        return fn(exp);
    }

    // If we're already on the handler fiber, just run directly.
    if (GetCurrentFiber() == state.handler_fiber) {
        return fn(exp);
    }

    state.pending_fn = fn;
    state.pending_exp = exp;
    SwitchToFiber(state.handler_fiber);
    return state.result;
}

} // namespace Core

#endif
