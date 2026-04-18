// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"

#include <atomic>

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Fiber {

#define ORBIS_FIBER_MAX_NAME_LENGTH (31)
#define ORBIS_FIBER_CONTEXT_MINIMUM_SIZE (512)

typedef void PS4_SYSV_ABI (*OrbisFiberEntry)(u64 arg_on_initialize, u64 arg_on_run);

enum FiberState : u32 {
    Run = 1u,
    Idle = 2u,
    Terminated = 3u,
};

enum FiberFlags : u32 {
    None = 0x0,
    NoUlobjmgr = 0x1,
    ContextSizeCheck = 0x10,
    SetFpuRegs = 0x100,
};

struct OrbisFiber;

struct OrbisFiberContext {
    struct {
        u64 rax, rcx, rdx, rbx, rsp, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
        u16 fpucw;
        u32 mxcsr;
    };
    OrbisFiber* current_fiber;
    OrbisFiber* prev_fiber;
    u64 arg_on_run_to;
    u64 arg_on_return;
    u64 return_val;
};

struct OrbisFiberData {
    OrbisFiberEntry entry;
    u64 arg_on_initialize;
    u64 arg_on_run_to;
    void* stack_addr;
    u32* state;
    u16 fpucw;
    s8 pad[2];
    u32 mxcsr;
};

struct OrbisFiber {
    u32 magic_start;
    std::atomic<FiberState> state;
    OrbisFiberEntry entry;
    u64 arg_on_initialize;
    void* addr_context;
    u64 size_context;
    char name[ORBIS_FIBER_MAX_NAME_LENGTH + 1];
    OrbisFiberContext* context;
    u32 flags;
    void* context_start;
    void* context_end;
    u32 magic_end;
};
static_assert(sizeof(OrbisFiber) <= 256);

struct OrbisFiberInfo {
    u64 size;
    OrbisFiberEntry entry;
    u64 arg_on_initialize;
    void* addr_context;
    u64 size_context;
    char name[ORBIS_FIBER_MAX_NAME_LENGTH + 1];
    u64 size_context_margin;
    u8 pad[48];
};
static_assert(sizeof(OrbisFiberInfo) == 128);

struct OrbisFiberOptParam {
    u32 magic;
};
static_assert(sizeof(OrbisFiberOptParam) <= 128);

s32 PS4_SYSV_ABI sceFiberInitialize(OrbisFiber* fiber, const char* name, OrbisFiberEntry entry,
                                    u64 arg_on_initialize, void* addr_context, u64 size_context,
                                    const OrbisFiberOptParam* opt_param, u32 build_version);

s32 PS4_SYSV_ABI sceFiberOptParamInitialize(OrbisFiberOptParam* opt_param);

s32 PS4_SYSV_ABI sceFiberFinalize(OrbisFiber* fiber);

s32 PS4_SYSV_ABI sceFiberRun(OrbisFiber* fiber, u64 arg_on_run_to, u64* arg_on_return);

s32 PS4_SYSV_ABI sceFiberSwitch(OrbisFiber* fiber, u64 arg_on_run_to, u64* arg_on_run);

s32 PS4_SYSV_ABI sceFiberGetSelf(OrbisFiber** fiber);

s32 PS4_SYSV_ABI sceFiberReturnToThread(u64 arg_on_return, u64* arg_on_run);

s32 PS4_SYSV_ABI sceFiberGetInfo(OrbisFiber* fiber, OrbisFiberInfo* fiber_info);

s32 PS4_SYSV_ABI sceFiberStartContextSizeCheck(u32 flags);

s32 PS4_SYSV_ABI sceFiberStopContextSizeCheck(void);

s32 PS4_SYSV_ABI sceFiberRename(OrbisFiber* fiber, const char* name);

s32 PS4_SYSV_ABI sceFiberGetThreadFramePointerAddress(u64* addr_frame_pointer);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Fiber