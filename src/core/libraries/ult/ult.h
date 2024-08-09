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
    char queue_name[31];
    char uk[8];
    u64 uk2;
    u64 uk3;
    u64 uk4;
    void* ukP;
    u32 uk5;
    char uk6[131];
    void* waitingWorkArea;
    void* dataWorkArea;
    char unknown2[24];
    size_t datasize;
    // private
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
    char queue_name[31];
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
    char queue_name[31];
    short uk_200;
    char uk_a;
    char unknown_char;
    u32 numData;
    u32 numQueueObjects;
    char unknkown_char3[4];
    void* workArea;
    char padd[130];
    char unkown_padding2[4];
    OrbisUltWaitingQueueResourcePool* waitingPool;
    // private
};

struct OrbisUltQueueDataResourcePoolOptParam {
    // private
};

struct OrbisUltQueueOptParam {
    // Private
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

int PS4_SYSV_ABI _sceUltQueueCreate(OrbisUltQueue* queue, const char* name, uint64_t dataSize,
                                    OrbisUltQueueDataResourcePool* queueDataResourcePool,
                                    OrbisUltWaitingQueueResourcePool* waitingQueueResourcePool,
                                    OrbisUltQueueOptParam* optParam);

int PS4_SYSV_ABI _sceUltQueueOptParamInitialize(OrbisUltQueueOptParam* optParam);

int PS4_SYSV_ABI sceUltQueueTryPush(OrbisUltQueue* queue, void* data);
int PS4_SYSV_ABI sceUltQueuePush(OrbisUltQueue* queue, void* data);

int PS4_SYSV_ABI sceUltQueueTryPop(OrbisUltQueue* queue, void* data);
int PS4_SYSV_ABI sceUltQueuePop(OrbisUltQueue* queue, void* data);

u64 PS4_SYSV_ABI sceUltWaitingQueueResourcePoolGetWorkAreaSize(uint32_t numThreads,
                                                               uint32_t numSyncObjects);

u64 PS4_SYSV_ABI sceUltQueueDataResourcePoolGetWorkAreaSize(uint32_t numData, uint64_t dataSize,
                                                            uint32_t numQueueObjects);

void RegisterlibSceUlt(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ult