// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpCommon {

struct OrbisNpFreeKernelMemoryArgs {
    u64 length;
    u64 unk;
    void* addr;
};

using NpThreadEntry = void* PS4_SYSV_ABI (*)(void*);
using OrbisNpCalloutHandler = void PS4_SYSV_ABI (*)(u64);

struct OrbisNpCalloutEntry {
    OrbisNpCalloutEntry* next;
    OrbisNpCalloutHandler handler;
    s64 deadline_usec;
    u64 arg;
};

struct OrbisNpCalloutContext {
    // scepthread should be used here
    void* thread;
    void* mutex;
    void* cond;
    s32 active;
    s32 stop_requested;
    OrbisNpCalloutEntry* head;
};

u32 PS4_SYSV_ABI sceNpMutexLock(void* mutex);
u32 PS4_SYSV_ABI sceNpMutexUnlock(void* mutex);
u32 PS4_SYSV_ABI sceNpMutexInit(void* mutex, void* mutex_name, u64 flags);
void PS4_SYSV_ABI sceNpMutexDestroy(void* mutex);
u32 PS4_SYSV_ABI sceNpMutexTryLock(void* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexLock(void* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexUnlock(void* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexInit(void* mutex, void* mutex_name, u64 flags);
void PS4_SYSV_ABI sceNpLwMutexDestroy(void* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexTryLock(void* mutex);
s32 PS4_SYSV_ABI sceNpCondInit(void* cond, void* cond_name, u64 flags);
void PS4_SYSV_ABI sceNpCondDestroy(void* cond);
void PS4_SYSV_ABI sceNpCondSignal(void* cond);
s32 PS4_SYSV_ABI sceNpCondTimedwait(void* cond, void* mutex, u32 usec);
s32 PS4_SYSV_ABI sceNpCreateThread(void* thread, NpThreadEntry start_routine, void* arg,
                                   s32 priority, u64 stack_size, u64 affinity_mask,
                                   void* thread_name);
u32 PS4_SYSV_ABI sceNpJoinThread(void* thread, void** ret);
s32 PS4_SYSV_ABI sceNpGetSystemClockUsec(s64* usec);
s32 PS4_SYSV_ABI sceNpGetPlatformType(const void* np_id);
s32 PS4_SYSV_ABI sceNpIntIsValidOnlineId(const void* online_id);
s32 PS4_SYSV_ABI sceNpCalloutInitCtx(void* ctx, void* name, u64 stack_size, s32 priority,
                                     u64 affinity_mask);
s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx(void* ctx, void* callout, u64 delay_usec, u64 handler,
                                        u64 arg);
s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx64(void* ctx, void* callout, s64 delay_usec, u64 handler,
                                          u64 arg);
s32 PS4_SYSV_ABI sceNpCalloutStopOnCtx(void* ctx, void* callout, u32* removed);
void PS4_SYSV_ABI sceNpCalloutTermCtx(void* ctx);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpCommon
