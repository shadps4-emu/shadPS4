// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/np/np_types.h"

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
    Libraries::Kernel::PthreadT thread;
    Libraries::Kernel::PthreadMutexT mutex;
    Libraries::Kernel::PthreadCondT cond;
    s32 active;
    s32 stop_requested;
    OrbisNpCalloutEntry* head;
};

u32 PS4_SYSV_ABI sceNpMutexLock(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpMutexUnlock(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpMutexInit(Libraries::Kernel::PthreadMutexT* mutex, const char* name,
                                u64 flags);
s32 PS4_SYSV_ABI sceNpMutexDestroy(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpMutexTryLock(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexLock(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexUnlock(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexInit(Libraries::Kernel::PthreadMutexT* mutex, const char* name,
                                  u64 flags);
s32 PS4_SYSV_ABI sceNpLwMutexDestroy(Libraries::Kernel::PthreadMutexT* mutex);
u32 PS4_SYSV_ABI sceNpLwMutexTryLock(Libraries::Kernel::PthreadMutexT* mutex);
s32 PS4_SYSV_ABI sceNpCondInit(Libraries::Kernel::PthreadCondT* cond, const char* name, u64 flags);
s32 PS4_SYSV_ABI sceNpCondDestroy(Libraries::Kernel::PthreadCondT* cond);
s32 PS4_SYSV_ABI sceNpCondSignal(Libraries::Kernel::PthreadCondT* cond);
s32 PS4_SYSV_ABI sceNpCondTimedwait(Libraries::Kernel::PthreadCondT* cond,
                                    Libraries::Kernel::PthreadMutexT* mutex, u32 usec);
s32 PS4_SYSV_ABI sceNpCreateThread(Libraries::Kernel::PthreadT* thread, NpThreadEntry start_routine,
                                   void* arg, s32 priority, u64 stack_size, u64 affinity_mask,
                                   const char* name);
u32 PS4_SYSV_ABI sceNpJoinThread(Libraries::Kernel::PthreadT thread, void** ret);
s32 PS4_SYSV_ABI sceNpGetSystemClockUsec(s64* usec);
s32 PS4_SYSV_ABI sceNpGetPlatformType(const OrbisNpId* np_id);
s32 PS4_SYSV_ABI sceNpIntIsValidOnlineId(const OrbisNpOnlineId* id);
void PS4_SYSV_ABI sceNpGetSdkVersion(char* version_buf);
s32 PS4_SYSV_ABI sceNpCalloutInitCtx(OrbisNpCalloutContext* ctx, const char* name, u64 stack_size,
                                     s32 priority, u64 affinity_mask);
s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx(OrbisNpCalloutContext* ctx, OrbisNpCalloutEntry* callout,
                                        u64 delay_usec, u64 handler, u64 arg);
s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx64(OrbisNpCalloutContext* ctx, OrbisNpCalloutEntry* callout,
                                          s64 delay_usec, u64 handler, u64 arg);
s32 PS4_SYSV_ABI sceNpCalloutStopOnCtx(OrbisNpCalloutContext* ctx, OrbisNpCalloutEntry* callout,
                                       u32* removed);
void PS4_SYSV_ABI sceNpCalloutTermCtx(OrbisNpCalloutContext* ctx);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpCommon
