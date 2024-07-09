// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ult {

typedef int32_t (*OrbisUltUlthreadEntry)(uint64_t arg);

struct OrbisUltQueue {
	//private
};

struct OrbisUltUlthreadRuntimeOptParam {
    uint64_t oneShotThreadStackSize;
    u64 workerThreadCpuAffinityMask;
    int32_t workerThreadPriority;
    int inheritSched;
    /* other members are private */
};

struct OrbisUltUlthreadRuntime {
    // private
};

struct OrbisUltUlthread {
    // private
};

struct OrbisUltUlthreadOptParam {
    uint32_t attribute;
    // rest private
};

struct OrbisUltWaitingQueueResourcePool {
    char unknown_padding1[30];
    char unknown_char;
    u16 field32_0x20;
    char field33_0x22;
    char unknkown_char3;
    u32 numThreads;
    u32 numSyncObjects;
    char unkown_padding2[4];
    void* workArea;
    // More unkowns...
};

struct OrbisUltWaitingQueueResourcePoolOptParam {
    // private
};

struct OrbisUltQueueDataResourcePool {
    // private
};

struct OrbisUltQueueDataResourcePoolOptParam {
    // private
};

int PS4_SYSV_ABI sceUltInitialize();

int PS4_SYSV_ABI _sceUltUlthreadRuntimeCreate(OrbisUltUlthreadRuntime* runtime, const char* name,
                                              uint32_t maxNumUlthread, uint32_t numWorkerThread,
                                              void* workArea,
                                              OrbisUltUlthreadRuntimeOptParam* optParam);

int PS4_SYSV_ABI _sceUltUlthreadCreate(OrbisUltUlthread* ulthread, const char* name,
                                       OrbisUltUlthreadEntry entry, uint64_t arg, void* context,
                                       uint64_t sizeContext, OrbisUltUlthreadRuntime* runtime,
                                       OrbisUltUlthreadOptParam* optParam);

int PS4_SYSV_ABI _sceUltWaitingQueueResourcePoolCreate(
    OrbisUltWaitingQueueResourcePool* pool, const char* name, uint32_t numThreads,
    uint32_t numSyncObjects, void* workArea, OrbisUltWaitingQueueResourcePoolOptParam* optParam);

int PS4_SYSV_ABI _sceUltQueueDataResourcePoolCreate(
    OrbisUltQueueDataResourcePool* pool, const char* name, uint32_t numData, uint64_t dataSize,
    uint32_t numQueueObjects, OrbisUltWaitingQueueResourcePool* waitingQueueResourcePool,
    void* workArea, OrbisUltQueueDataResourcePoolOptParam* optParam);

int PS4_SYSV_ABI sceUltQueueTryPush(OrbisUltQueue* queue, void* data);
int PS4_SYSV_ABI sceUltQueuePush(OrbisUltQueue* queue, void* data);

int PS4_SYSV_ABI sceUltQueueTryPop(OrbisUltQueue* queue, void* data);
int PS4_SYSV_ABI sceUltQueuePop(OrbisUltQueue* queue, void* data);

void RegisterlibSceUlt(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ult