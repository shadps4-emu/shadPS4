// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ult {

// Opaque handles - the game allocates these and passes pointers to them.
// We store our internal state at the start of the game-provided memory.

struct UltWaitingQueueResourcePool {
    u32 num_threads;
    bool initialized;
};

struct UltQueueDataResourcePool {
    u32 num_data;
    u32 data_size;
    bool initialized;
};

// Forward declaration of internal queue implementation
struct UltQueueInternal;

struct UltQueue {
    UltQueueInternal* internal;
};

struct UltUlthreadRuntimeOptParam {
    u64 reserved[8];
};

struct UltUlthreadRuntime {
    u32 num_threads;
    u32 stack_size;
    bool initialized;
};

struct UltUlthread {
    void* handle; // platform thread handle
    bool joined;
};

// Library initialization
s32 PS4_SYSV_ABI sceUltInitialize();

// Waiting queue resource pool
s32 PS4_SYSV_ABI sceUltWaitingQueueResourcePoolGetWorkAreaSize(u32 numThreads);
s32 PS4_SYSV_ABI _sceUltWaitingQueueResourcePoolCreate(UltWaitingQueueResourcePool* pool,
                                                       const char* name, u32 numThreads,
                                                       void* workArea, void* optParam);

// Queue data resource pool
s32 PS4_SYSV_ABI sceUltQueueDataResourcePoolGetWorkAreaSize(u32 numData, u32 dataSize);
s32 PS4_SYSV_ABI _sceUltQueueDataResourcePoolCreate(UltQueueDataResourcePool* pool,
                                                    const char* name, u32 numData, u32 dataSize,
                                                    void* workArea, void* optParam);

// Queue operations
s32 PS4_SYSV_ABI _sceUltQueueCreate(UltQueue* queue, const char* name, u32 dataSize, u32 numData,
                                    UltWaitingQueueResourcePool* waitPool,
                                    UltQueueDataResourcePool* dataPool, void* optParam);
s32 PS4_SYSV_ABI sceUltQueuePush(UltQueue* queue, const void* data);
s32 PS4_SYSV_ABI sceUltQueuePop(UltQueue* queue, void* data);

// Ulthread runtime
s32 PS4_SYSV_ABI _sceUltUlthreadRuntimeOptParamInitialize(UltUlthreadRuntimeOptParam* param);
s32 PS4_SYSV_ABI sceUltUlthreadRuntimeGetWorkAreaSize(u32 numThread, u32 stackSize);
s32 PS4_SYSV_ABI _sceUltUlthreadRuntimeCreate(UltUlthreadRuntime* runtime, const char* name,
                                              u32 numThread, u32 stackSize, void* workArea,
                                              void* optParam);
s32 PS4_SYSV_ABI sceUltUlthreadRuntimeDestroy(UltUlthreadRuntime* runtime);

// Ulthread
s32 PS4_SYSV_ABI _sceUltUlthreadCreate(UltUlthread* thread, const char* name, void (*entry)(void*),
                                       void* arg, void* context, u32 stackSize,
                                       UltUlthreadRuntime* runtime, void* optParam);
s32 PS4_SYSV_ABI sceUltUlthreadJoin(UltUlthread* thread, s32* status);

void RegisterlibSceUlt(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Ult
