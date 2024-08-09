// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "ult.h"

namespace Libraries::Ult {

bool isUltInitialized = false;
int PS4_SYSV_ABI sceUltInitialize() {
    LOG_INFO(Lib_Ult, "called");
    if (isUltInitialized) {
        return ORBIS_ULT_ERROR_STATE;
    }

    isUltInitialized = true;
    return ORBIS_OK;
}
int PS4_SYSV_ABI _sceUltUlthreadRuntimeCreate(OrbisUltUlthreadRuntime* runtime, const char* name,
                                              uint32_t maxNumUlthread, uint32_t numWorkerThread,
                                              void* workArea,
                                              OrbisUltUlthreadRuntimeOptParam* optParam) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceUltUlthreadCreate(OrbisUltUlthread* ulthread, const char* name,
                                       OrbisUltUlthreadEntry entry, uint64_t arg, void* context,
                                       uint64_t sizeContext, OrbisUltUlthreadRuntime* runtime,
                                       OrbisUltUlthreadOptParam* optParam) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceUltWaitingQueueResourcePoolCreate(
    OrbisUltWaitingQueueResourcePool* pool, const char* name, uint32_t numThreads,
    uint32_t numSyncObjects, void* workArea, OrbisUltWaitingQueueResourcePoolOptParam* optParam) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");

    if (pool == nullptr)
        return ORBIS_ULT_ERROR_NULL;

    if (name != nullptr)
        LOG_INFO(Lib_Ult, "Creating WaitingQueueResourcePool for {}", name);

    // TODO: Check memory alignment
    // TODO: Set ORBIS_ULT_ERROR_NOT_INITIALIZE

    if (optParam != nullptr) {
        // TODO: Check memory alignment
        // TODO: FUN_0100678(optParam)
    }

    // TODO: FUN_01011b10(&pool->field41_0x30,numThreads,numSyncObjects,(long)workArea);

    if (numThreads > 0 && numSyncObjects > 0 && workArea != nullptr) {
        pool->workArea = workArea;
    }

    // IF NO ERROR
    strncpy((char*)pool, name, 0x1f);
    pool->field32_0x20 = 0x100;  // ??
    pool->field33_0x22 = '\x06'; // ??
    pool->numThreads = numThreads * 2;
    pool->numSyncObjects = numSyncObjects;
    // BG26hBGiNlw(pool, 0x16, &pool->field178_0xc0);
    //  ENDIF

    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceUltQueueDataResourcePoolCreate(
    OrbisUltQueueDataResourcePool* pool, const char* name, uint32_t numData, uint64_t dataSize,
    uint32_t numQueueObjects, OrbisUltWaitingQueueResourcePool* waitingQueueResourcePool,
    void* workArea, OrbisUltQueueDataResourcePoolOptParam* optParam) {

    LOG_ERROR(Lib_Ult, "(STUBBED) called");

    if (pool == nullptr)
        return ORBIS_ULT_ERROR_NULL;

    // TODO: Check 8 bit alignment

    strncpy((char*)pool, name, 0x1f);

    pool->uk_200 = 0x200; // TODO: Why?
    pool->uk_a = '\a';    // TODO: Why?
    pool->numData = numData;
    pool->numQueueObjects = numQueueObjects;
    pool->waitingPool = waitingQueueResourcePool;
    pool->workArea = workArea;

    // TODO: BG26hBGiNlw(pool,0x17,&pool->field347_0x170);

    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceUltQueueCreate(OrbisUltQueue* queue, const char* name, uint64_t dataSize,
                                    OrbisUltQueueDataResourcePool* queueDataResourcePool,
                                    OrbisUltWaitingQueueResourcePool* waitingQueueResourcePool,
                                    OrbisUltQueueOptParam* optParam) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");

    // TODO: Put data pool + Waiting Pool into Queue

    if (queue != nullptr && name != nullptr && waitingQueueResourcePool->workArea != nullptr) {
        // TODO: Check 8bit alignment
        if (optParam == nullptr) {
            // TODO: Check 8bit alignment
            // TODO: Handle default args. It looks like it just sets everything to 0.
        } else {
            // TODO: Handle optional params
        }

        // TODO: Do this in a more readable way
        strncpy((char*)queue, name, 0x1f);

        queue->waitingWorkArea = waitingQueueResourcePool->workArea;
        queue->dataWorkArea = queueDataResourcePool->workArea;
        queue->datasize = dataSize;

    } else {
        return ORBIS_ULT_ERROR_NULL;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceUltQueueOptParamInitialize(OrbisUltQueueOptParam* optParam) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUltQueueTryPush(OrbisUltQueue* queue, void* data) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUltQueuePush(OrbisUltQueue* queue, void* data) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");

    if (queue == nullptr || data == nullptr)
        return ORBIS_ULT_ERROR_NULL;
    
    // If there is no data in the queue when sceUltQueuePop() is executed, the thread is in the wait
    // state until data is added to the queue.
    void* addr = (char*)queue->waitingWorkArea + (queue->datasize) * (queue->uk5);

    if (!addr) // Empty
        return ORBIS_OK;

    memcpy(addr, data, queue->datasize);

    queue->uk5++;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUltQueueTryPop(OrbisUltQueue* queue, void* data) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUltQueuePop(OrbisUltQueue* queue, void* data) {
    LOG_ERROR(Lib_Ult, "(STUBBED) called");

    if (queue == nullptr || data == nullptr)
        return ORBIS_ULT_ERROR_NULL;

    if (queue->uk5 < 1) // Thread should wait
        return ORBIS_OK;

    // If there is no data in the queue when sceUltQueuePop() is executed, the thread is in the wait state until data is added to the queue.
    void* addr = (char*)queue->waitingWorkArea + (queue->datasize) * (queue->uk5 - 1);

    if (!addr) // Empty
        return ORBIS_OK;

    memcpy(data, addr, queue->datasize);

    queue->uk5--;

    return ORBIS_OK;
}

u64 PS4_SYSV_ABI sceUltWaitingQueueResourcePoolGetWorkAreaSize(uint32_t numThreads,
                                                               uint32_t numSyncObjects) {
    u64 size = (numSyncObjects + 2 + numThreads * 2) << 5;
    LOG_INFO(Lib_Ult, "WaitingQueueResourcePoolSize: {}", size);
    return size;
}

u64 PS4_SYSV_ABI sceUltQueueDataResourcePoolGetWorkAreaSize(uint32_t numData, uint64_t dataSize,
                                                            uint32_t numQueueObjects) {
    u64 size = numData * dataSize + (numQueueObjects + 3 + numData * 2) * 0x20;
    LOG_INFO(Lib_Ult, "QueueDataResourcePoolSize: {}", size);
    return size;
} 

void RegisterlibSceUlt(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hZIg1EWGsHM", "libSceUlt", 1, "libSceUlt", 1, 1, sceUltInitialize);
    LIB_FUNCTION("jw9FkZBXo-g", "libSceUlt", 1, "libSceUlt", 1, 1, _sceUltUlthreadRuntimeCreate);
    LIB_FUNCTION("uZz3ci7XYqc", "libSceUlt", 1, "libSceUlt", 1, 1, sceUltQueueTryPop);
    LIB_FUNCTION("RVSq2tsm2yw", "libSceUlt", 1, "libSceUlt", 1, 1, sceUltQueuePop);
    LIB_FUNCTION("znI3q8S7KQ4", "libSceUlt", 1, "libSceUlt", 1, 1, _sceUltUlthreadCreate);
    LIB_FUNCTION("6Mc2Xs7pI1I", "libSceUlt", 1, "libSceUlt", 1, 1, sceUltQueueTryPush);
    LIB_FUNCTION("dUwpX3e5NDE", "libSceUlt", 1, "libSceUlt", 1, 1, sceUltQueuePush);
    LIB_FUNCTION("YiHujOG9vXY", "libSceUlt", 1, "libSceUlt", 1, 1,
                 _sceUltWaitingQueueResourcePoolCreate);
    LIB_FUNCTION("TFHm6-N6vks", "libSceUlt", 1, "libSceUlt", 1, 1,
                 _sceUltQueueDataResourcePoolCreate);
    LIB_FUNCTION("9Y5keOvb6ok", "libSceUlt", 1, "libSceUlt", 1, 1, _sceUltQueueCreate);
    LIB_FUNCTION("TkASc9I-xX0", "libSceUlt", 1, "libSceUlt", 1, 1, _sceUltQueueOptParamInitialize);
    LIB_FUNCTION("WIWV1Qd7PFU", "libSceUlt", 1, "libSceUlt", 1, 1,
                 sceUltWaitingQueueResourcePoolGetWorkAreaSize);
    LIB_FUNCTION("evj9YPkS8s4", "libSceUlt", 1, "libSceUlt", 1, 1,
                 sceUltQueueDataResourcePoolGetWorkAreaSize);
};

} // namespace Libraries::Ult