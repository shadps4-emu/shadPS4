// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"

#include <atomic>
#include <cstddef>

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

struct alignas(8) OrbisFiber {
    u32 magic_start;                              // 0x00
    u32 state;                                    // 0x04
    u64 entry_xor;                                // 0x08
    u64 arg_on_initialize_xor;                    // 0x10
    void* addr_context;                           // 0x18
    u64 size_context;                             // 0x20
    u8 name_xor[ORBIS_FIBER_MAX_NAME_LENGTH + 1]; // 0x28
    OrbisFiberContext* context;                   // 0x48
    u64 owner_thread;                             // 0x50
    u32 flags;                                    // 0x58
    u32 razor_id_xor;                             // 0x5c
    u64 switch_cookie;                            // 0x60
    void* asan_fake_stack;                        // 0x68
    u8 random_pad[0x78];                          // 0x70
    void* context_start;                          // 0xe8
    void* context_end;                            // 0xf0
    u32 reserved;                                 // 0xf8
    u32 magic_end;                                // 0xfc
};
static_assert(sizeof(OrbisFiber) == 0x100);
static_assert(offsetof(OrbisFiber, context_start) == 0xe8);
static_assert(offsetof(OrbisFiber, context_end) == 0xf0);
static_assert(offsetof(OrbisFiber, magic_end) == 0xfc);

struct alignas(8) OrbisFiberInfo {
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

struct alignas(8) OrbisFiberOptParam {
    u32 magic;
    u32 option_flags;
    u8 reserved[0x80 - 8];
};
static_assert(sizeof(OrbisFiberOptParam) == 0x80);

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
