// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/ult/ult.h"
#include "core/libraries/ult/ult_error.h"

namespace Libraries::Ult {

// Internal thread-safe queue implementation
struct UltQueueInternal {
    std::mutex mtx;
    std::condition_variable cv_not_empty;
    std::condition_variable cv_not_full;
    std::deque<std::vector<u8>> items;
    u32 data_size;
    u32 max_items;
    bool destroyed = false;

    UltQueueInternal(u32 data_size_, u32 max_items_)
        : data_size(data_size_), max_items(max_items_) {}
};

s32 PS4_SYSV_ABI sceUltInitialize() {
    LOG_INFO(Lib_Ult, "called");
    return ORBIS_OK;
}

// --- Waiting Queue Resource Pool ---

s32 PS4_SYSV_ABI sceUltWaitingQueueResourcePoolGetWorkAreaSize(u32 numThreads) {
    LOG_DEBUG(Lib_Ult, "numThreads = {}", numThreads);
    // Return a reasonable work area size. The game allocates this much memory
    // and passes it to the create function. We don't actually use it since
    // we use std library synchronization primitives internally.
    return static_cast<s32>(sizeof(UltWaitingQueueResourcePool) + numThreads * 64);
}

s32 PS4_SYSV_ABI _sceUltWaitingQueueResourcePoolCreate(UltWaitingQueueResourcePool* pool,
                                                       const char* name, u32 numThreads,
                                                       void* workArea, void* optParam) {
    LOG_INFO(Lib_Ult, "name = {}, numThreads = {}", name ? name : "<null>", numThreads);
    if (!pool) {
        return SCE_ULT_ERROR_NULL;
    }
    pool->num_threads = numThreads;
    pool->initialized = true;
    return ORBIS_OK;
}

// --- Queue Data Resource Pool ---

s32 PS4_SYSV_ABI sceUltQueueDataResourcePoolGetWorkAreaSize(u32 numData, u32 dataSize) {
    LOG_DEBUG(Lib_Ult, "numData = {}, dataSize = {}", numData, dataSize);
    return static_cast<s32>(sizeof(UltQueueDataResourcePool) + numData * dataSize + 256);
}

s32 PS4_SYSV_ABI _sceUltQueueDataResourcePoolCreate(UltQueueDataResourcePool* pool,
                                                    const char* name, u32 numData, u32 dataSize,
                                                    void* workArea, void* optParam) {
    LOG_INFO(Lib_Ult, "name = {}, numData = {}, dataSize = {}", name ? name : "<null>", numData,
             dataSize);
    if (!pool) {
        return SCE_ULT_ERROR_NULL;
    }
    pool->num_data = numData;
    pool->data_size = dataSize;
    pool->initialized = true;
    return ORBIS_OK;
}

// --- Queue ---

s32 PS4_SYSV_ABI _sceUltQueueCreate(UltQueue* queue, const char* name, u32 dataSize, u32 numData,
                                    UltWaitingQueueResourcePool* waitPool,
                                    UltQueueDataResourcePool* dataPool, void* optParam) {
    LOG_INFO(Lib_Ult, "name = {}, dataSize = {}, numData = {}", name ? name : "<null>", dataSize,
             numData);
    if (!queue) {
        return SCE_ULT_ERROR_NULL;
    }
    queue->internal = new UltQueueInternal(dataSize, numData);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUltQueuePush(UltQueue* queue, const void* data) {
    if (!queue || !queue->internal || !data) {
        LOG_ERROR(Lib_Ult, "null parameter");
        return SCE_ULT_ERROR_NULL;
    }

    auto* q = queue->internal;
    std::unique_lock lock(q->mtx);

    // Wait until there's space or queue is destroyed
    q->cv_not_full.wait(lock, [&] { return q->items.size() < q->max_items || q->destroyed; });

    if (q->destroyed) {
        return SCE_ULT_ERROR_STATE;
    }

    // Copy data into the queue
    std::vector<u8> item(q->data_size);
    std::memcpy(item.data(), data, q->data_size);
    q->items.push_back(std::move(item));

    lock.unlock();
    q->cv_not_empty.notify_one();

    LOG_TRACE(Lib_Ult, "pushed item, queue size = {}", q->items.size());
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUltQueuePop(UltQueue* queue, void* data) {
    if (!queue || !queue->internal || !data) {
        LOG_ERROR(Lib_Ult, "null parameter");
        return SCE_ULT_ERROR_NULL;
    }

    auto* q = queue->internal;
    std::unique_lock lock(q->mtx);

    // Wait until there's data or queue is destroyed
    q->cv_not_empty.wait(lock, [&] { return !q->items.empty() || q->destroyed; });

    if (q->destroyed && q->items.empty()) {
        return SCE_ULT_ERROR_STATE;
    }

    // Copy data out of the queue
    auto& item = q->items.front();
    std::memcpy(data, item.data(), q->data_size);
    q->items.pop_front();

    lock.unlock();
    q->cv_not_full.notify_one();

    LOG_TRACE(Lib_Ult, "popped item, queue size = {}", q->items.size());
    return ORBIS_OK;
}

// --- Ulthread Runtime ---

s32 PS4_SYSV_ABI _sceUltUlthreadRuntimeOptParamInitialize(UltUlthreadRuntimeOptParam* param) {
    LOG_DEBUG(Lib_Ult, "called");
    if (!param) {
        return SCE_ULT_ERROR_NULL;
    }
    std::memset(param, 0, sizeof(UltUlthreadRuntimeOptParam));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUltUlthreadRuntimeGetWorkAreaSize(u32 numThread, u32 stackSize) {
    LOG_DEBUG(Lib_Ult, "numThread = {}, stackSize = {}", numThread, stackSize);
    return static_cast<s32>(sizeof(UltUlthreadRuntime) + numThread * 128);
}

s32 PS4_SYSV_ABI _sceUltUlthreadRuntimeCreate(UltUlthreadRuntime* runtime, const char* name,
                                              u32 numThread, u32 stackSize, void* workArea,
                                              void* optParam) {
    LOG_INFO(Lib_Ult, "name = {}, numThread = {}, stackSize = {}", name ? name : "<null>",
             numThread, stackSize);
    if (!runtime) {
        return SCE_ULT_ERROR_NULL;
    }
    runtime->num_threads = numThread;
    runtime->stack_size = stackSize;
    runtime->initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUltUlthreadRuntimeDestroy(UltUlthreadRuntime* runtime) {
    LOG_INFO(Lib_Ult, "called");
    if (!runtime) {
        return SCE_ULT_ERROR_NULL;
    }
    runtime->initialized = false;
    return ORBIS_OK;
}

// --- Ulthread ---

s32 PS4_SYSV_ABI _sceUltUlthreadCreate(UltUlthread* thread, const char* name, void (*entry)(void*),
                                       void* arg, void* context, u32 stackSize,
                                       UltUlthreadRuntime* runtime, void* optParam) {
    LOG_INFO(Lib_Ult, "name = {}, stackSize = {}", name ? name : "<null>", stackSize);
    if (!thread || !entry) {
        return SCE_ULT_ERROR_NULL;
    }

    // Launch as a real host thread. ULT threads on PS4 are cooperative lightweight
    // threads, but for now a real thread should work for compatibility.
    auto* t = new std::thread(entry, arg);
    thread->handle = t;
    thread->joined = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceUltUlthreadJoin(UltUlthread* thread, s32* status) {
    LOG_INFO(Lib_Ult, "called");
    if (!thread) {
        return SCE_ULT_ERROR_NULL;
    }
    if (thread->handle && !thread->joined) {
        auto* t = static_cast<std::thread*>(thread->handle);
        if (t->joinable()) {
            t->join();
        }
        delete t;
        thread->handle = nullptr;
        thread->joined = true;
    }
    if (status) {
        *status = 0;
    }
    return ORBIS_OK;
}

void RegisterlibSceUlt(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hZIg1EWGsHM", "libSceUlt", 1, "libSceUlt", sceUltInitialize);

    LIB_FUNCTION("WIWV1Qd7PFU", "libSceUlt", 1, "libSceUlt",
                 sceUltWaitingQueueResourcePoolGetWorkAreaSize);
    LIB_FUNCTION("YiHujOG9vXY", "libSceUlt", 1, "libSceUlt", _sceUltWaitingQueueResourcePoolCreate);

    LIB_FUNCTION("evj9YPkS8s4", "libSceUlt", 1, "libSceUlt",
                 sceUltQueueDataResourcePoolGetWorkAreaSize);
    LIB_FUNCTION("TFHm6-N6vks", "libSceUlt", 1, "libSceUlt", _sceUltQueueDataResourcePoolCreate);

    LIB_FUNCTION("9Y5keOvb6ok", "libSceUlt", 1, "libSceUlt", _sceUltQueueCreate);
    LIB_FUNCTION("dUwpX3e5NDE", "libSceUlt", 1, "libSceUlt", sceUltQueuePush);
    LIB_FUNCTION("RVSq2tsm2yw", "libSceUlt", 1, "libSceUlt", sceUltQueuePop);

    LIB_FUNCTION("V2u3WLrwh64", "libSceUlt", 1, "libSceUlt",
                 _sceUltUlthreadRuntimeOptParamInitialize);
    LIB_FUNCTION("grs2pbc2awM", "libSceUlt", 1, "libSceUlt", sceUltUlthreadRuntimeGetWorkAreaSize);
    LIB_FUNCTION("jw9FkZBXo-g", "libSceUlt", 1, "libSceUlt", _sceUltUlthreadRuntimeCreate);
    LIB_FUNCTION("-gxcs521SvA", "libSceUlt", 1, "libSceUlt", sceUltUlthreadRuntimeDestroy);

    LIB_FUNCTION("znI3q8S7KQ4", "libSceUlt", 1, "libSceUlt", _sceUltUlthreadCreate);
    LIB_FUNCTION("gCeAI57LGgI", "libSceUlt", 1, "libSceUlt", sceUltUlthreadJoin);
}

} // namespace Libraries::Ult
