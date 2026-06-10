// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <thread>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpCommon {

namespace {
static_assert(sizeof(OrbisNpCalloutEntry) == 0x20);
static_assert(offsetof(OrbisNpCalloutContext, thread) == 0x00);
static_assert(offsetof(OrbisNpCalloutContext, mutex) == 0x08);
static_assert(offsetof(OrbisNpCalloutContext, cond) == 0x10);
static_assert(offsetof(OrbisNpCalloutContext, active) == 0x18);
static_assert(offsetof(OrbisNpCalloutContext, stop_requested) == 0x1c);
static_assert(offsetof(OrbisNpCalloutContext, head) == 0x20);
static_assert(sizeof(OrbisNpCalloutContext) == 0x28);

s32 NormalizeNpCommonResult(int rc) {
    if (rc <= 0) {
        return rc;
    }
    return Libraries::Kernel::ErrnoToSceKernelError(rc);
}

u32 NormalizeNpTryLockResult(int rc) {
    const s32 sce_rc = NormalizeNpCommonResult(rc);
    if (sce_rc == ORBIS_KERNEL_ERROR_EBUSY) {
        return ORBIS_NP_LW_MUTEX_ERROR_BUSY;
    }
    return static_cast<u32>(sce_rc);
}

void* PS4_SYSV_ABI NpCalloutThreadMain(void* arg) {
    auto* ctx = static_cast<OrbisNpCalloutContext*>(arg);
    const auto mutex = &ctx->mutex;
    (void)sceNpMutexLock(mutex);

    for (;;) {
        s64 now_usec = 0;
        sceNpGetSystemClockUsec(&now_usec);

        auto* entry = ctx->head;
        u32 wait_usec = 0;

        if (entry == nullptr) {
            if (ctx->stop_requested > 0) {
                break;
            }
        } else {
            const u64 deadline = static_cast<u64>(entry->deadline_usec);
            const u64 now = static_cast<u64>(now_usec);
            const u64 remaining = deadline - now;
            if (deadline < now || remaining == 0) {
                ctx->head = entry->next;
                (void)sceNpMutexUnlock(mutex);
                if (entry->handler != nullptr) {
                    entry->handler(entry->arg);
                }
                (void)sceNpMutexLock(mutex);
                continue;
            }

            if (ctx->stop_requested > 0) {
                break;
            }

            wait_usec = static_cast<u32>(remaining > 0xffffffffULL ? 0xffffffffULL : remaining);
        }

        (void)sceNpCondTimedwait(&ctx->cond, mutex, wait_usec);
        if (ctx->stop_requested > 0) {
            break;
        }
    }

    (void)sceNpMutexUnlock(mutex);
    return nullptr;
}
} // namespace

s32 PS4_SYSV_ABI sceNpCmpNpId(OrbisNpId* np_id1, OrbisNpId* np_id2) {
    if (np_id1 == nullptr || np_id2 == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (std::strncmp(np_id1->handle.data, np_id2->handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH) != 0) {
        return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
    }

    for (u32 i = 0; i < 8; i++) {
        if (np_id1->opt[i] != np_id2->opt[i]) {
            return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
        }
    }

    for (u32 i = 0; i < 8; i++) {
        if (np_id1->reserved[i] != np_id2->reserved[i]) {
            return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCmpNpIdInOrder(OrbisNpId* np_id1, OrbisNpId* np_id2, u32* out_result) {
    if (np_id1 == nullptr || np_id2 == nullptr || out_result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    u32 compare =
        std::strncmp(np_id1->handle.data, np_id2->handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    if (compare < 0) {
        *out_result = -1;
        return ORBIS_OK;
    } else if (compare > 0) {
        *out_result = 1;
        return ORBIS_OK;
    }

    for (u32 i = 0; i < 8; i++) {
        if (np_id1->opt[i] < np_id2->opt[i]) {
            *out_result = -1;
            return ORBIS_OK;
        } else if (np_id1->opt[i] > np_id2->opt[i]) {
            *out_result = 1;
            return ORBIS_OK;
        }
    }

    for (u32 i = 0; i < 8; i++) {
        if (np_id1->reserved[i] < np_id2->reserved[i]) {
            *out_result = -1;
            return ORBIS_OK;
        } else if (np_id1->reserved[i] > np_id2->reserved[i]) {
            *out_result = 1;
            return ORBIS_OK;
        }
    }

    *out_result = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCmpOnlineId(OrbisNpOnlineId* online_id1, OrbisNpOnlineId* online_id2) {
    if (online_id1 == nullptr || online_id2 == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (std::strncmp(online_id1->data, online_id2->data, ORBIS_NP_ONLINEID_MAX_LENGTH) != 0) {
        return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
    }
    return ORBIS_OK;
}

using NpMutexStorage = std::recursive_mutex;
using NpCondStorage = std::condition_variable_any;
struct NpThreadStorage {
    std::thread t;
};

u32 PS4_SYSV_ABI sceNpMutexLock(void* mutex) {
    // scepthread should be used here
    LOG_TRACE(Lib_NpCommon, "sceNpMutexLock: mutex={:p}", fmt::ptr(mutex));
    auto* m = *static_cast<NpMutexStorage**>(mutex);
    if (m != nullptr) {
        m->lock();
    }
    return 0;
}

u32 PS4_SYSV_ABI sceNpMutexUnlock(void* mutex) {
    // scepthread should be used here
    LOG_TRACE(Lib_NpCommon, "sceNpMutexUnlock: mutex={:p}", fmt::ptr(mutex));
    auto* m = *static_cast<NpMutexStorage**>(mutex);
    if (m != nullptr) {
        m->unlock();
    }
    return 0;
}

u32 PS4_SYSV_ABI sceNpMutexInit(void* mutex, void* mutex_name, u64 flags) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpMutexInit: mutex={:p} name={:p} flags={:#x}", fmt::ptr(mutex),
              fmt::ptr(mutex_name), flags);
    *static_cast<NpMutexStorage**>(mutex) = new NpMutexStorage();
    return 0;
}

void PS4_SYSV_ABI sceNpMutexDestroy(void* mutex) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpMutexDestroy: mutex={:p}", fmt::ptr(mutex));
    auto** slot = static_cast<NpMutexStorage**>(mutex);
    delete *slot;
    *slot = nullptr;
}

u32 PS4_SYSV_ABI sceNpMutexTryLock(void* mutex) {
    // scepthread should be used here
    LOG_TRACE(Lib_NpCommon, "sceNpMutexTryLock: mutex={:p}", fmt::ptr(mutex));
    auto* m = *static_cast<NpMutexStorage**>(mutex);
    if (m != nullptr && !m->try_lock()) {
        return ORBIS_NP_LW_MUTEX_ERROR_BUSY;
    }
    return 0;
}

u32 PS4_SYSV_ABI sceNpLwMutexLock(void* mutex) {
    LOG_TRACE(Lib_NpCommon, "sceNpLwMutexLock: mutex={:p}", fmt::ptr(mutex));
    return sceNpMutexLock(mutex);
}

u32 PS4_SYSV_ABI sceNpLwMutexUnlock(void* mutex) {
    LOG_TRACE(Lib_NpCommon, "sceNpLwMutexUnlock: mutex={:p}", fmt::ptr(mutex));
    return sceNpMutexUnlock(mutex);
}

u32 PS4_SYSV_ABI sceNpLwMutexInit(void* mutex, void* mutex_name, u64 flags) {
    LOG_DEBUG(Lib_NpCommon, "sceNpLwMutexInit: mutex={:p} name={:p} flags={:#x}", fmt::ptr(mutex),
              fmt::ptr(mutex_name), flags);
    return sceNpMutexInit(mutex, mutex_name, flags);
}

void PS4_SYSV_ABI sceNpLwMutexDestroy(void* mutex) {
    LOG_DEBUG(Lib_NpCommon, "sceNpLwMutexDestroy: mutex={:p}", fmt::ptr(mutex));
    sceNpMutexDestroy(mutex);
}

u32 PS4_SYSV_ABI sceNpLwMutexTryLock(void* mutex) {
    LOG_TRACE(Lib_NpCommon, "sceNpLwMutexTryLock: mutex={:p}", fmt::ptr(mutex));
    return sceNpMutexTryLock(mutex);
}

s32 PS4_SYSV_ABI sceNpCondInit(void* cond, void* cond_name, u64 flags) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpCondInit: cond={:p} name={:p} flags={:#x}", fmt::ptr(cond),
              fmt::ptr(cond_name), flags);
    *static_cast<NpCondStorage**>(cond) = new NpCondStorage();
    return 0;
}

void PS4_SYSV_ABI sceNpCondDestroy(void* cond) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpCondDestroy: cond={:p}", fmt::ptr(cond));
    auto** slot = static_cast<NpCondStorage**>(cond);
    delete *slot;
    *slot = nullptr;
}

void PS4_SYSV_ABI sceNpCondSignal(void* cond) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpCondSignal: cond={:p}", fmt::ptr(cond));
    auto* c = *static_cast<NpCondStorage**>(cond);
    if (c != nullptr) {
        c->notify_one();
    }
}

s32 PS4_SYSV_ABI sceNpCondTimedwait(void* cond, void* mutex, u32 usec) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpCondTimedwait: cond={:p} mutex={:p} usec={:#x}", fmt::ptr(cond),
              fmt::ptr(mutex), usec);
    auto* c = *static_cast<NpCondStorage**>(cond);
    auto* m = *static_cast<NpMutexStorage**>(mutex);
    if (c == nullptr || m == nullptr) {
        return 0;
    }
    std::unique_lock<NpMutexStorage> lock(*m, std::adopt_lock);
    if (usec == 0) {
        c->wait(lock);
    } else {
        c->wait_for(lock, std::chrono::microseconds(usec));
    }
    lock.release();
    return 0;
}

s32 PS4_SYSV_ABI sceNpCreateThread(void* thread, NpThreadEntry start_routine, void* arg,
                                   s32 priority, u64 stack_size, u64 affinity_mask,
                                   void* thread_name) {
    LOG_DEBUG(Lib_NpCommon,
              "sceNpCreateThread: thread={:p} start={:#x} arg={:p} priority={} stack={:#x} "
              "affinity={:#x} name={:p}",
              fmt::ptr(thread), reinterpret_cast<u64>(start_routine), fmt::ptr(arg), priority,
              stack_size, affinity_mask, fmt::ptr(thread_name));

    // scepthread should be used here
    (void)priority;
    (void)stack_size;
    (void)affinity_mask;
    (void)thread_name;
    auto* storage = new NpThreadStorage();
    storage->t = std::thread([start_routine, arg]() { (void)start_routine(arg); });
    *static_cast<NpThreadStorage**>(thread) = storage;
    return 0;
}

u32 PS4_SYSV_ABI sceNpJoinThread(void* thread, void** ret) {
    // scepthread should be used here
    LOG_DEBUG(Lib_NpCommon, "sceNpJoinThread: thread={:p} ret={:p}", fmt::ptr(thread),
              fmt::ptr(ret));
    auto** slot = static_cast<NpThreadStorage**>(thread);
    if (*slot != nullptr) {
        if ((*slot)->t.joinable()) {
            (*slot)->t.join();
        }
        delete *slot;
        *slot = nullptr;
    }
    if (ret != nullptr) {
        *ret = nullptr;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceNpGetSystemClockUsec(s64* usec) {
    LOG_DEBUG(Lib_NpCommon, "sceNpGetSystemClockUsec: usec={:p}", fmt::ptr(usec));
    Libraries::Kernel::OrbisKernelTimespec ts{};
    const s32 rc =
        Libraries::Kernel::sceKernelClockGettime(Libraries::Kernel::ORBIS_CLOCK_MONOTONIC, &ts);
    if (rc == ORBIS_OK) {
        *usec = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    }
    return rc;
}

s32 PS4_SYSV_ABI sceNpGetPlatformType(const void* np_id) {
    const auto* npid = static_cast<const OrbisNpId*>(np_id);
    LOG_DEBUG(Lib_NpCommon, "sceNpGetPlatformType: np_id={:p}", fmt::ptr(np_id));

    if (npid == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (npid->opt[4] == 0) {
        return 0;
    }
    u32 tag = 0;
    std::memcpy(&tag, npid->opt + 4, sizeof(tag));
    if (tag == 0x00337370) {
        return 1;
    }
    if (tag == 0x32707370) {
        return 2;
    }
    if (tag == 0x00347370) {
        return 3;
    }
    return ORBIS_NP_ERROR_INVALID_PLATFORM_TYPE;
}

s32 PS4_SYSV_ABI sceNpIntIsValidOnlineId(const void* online_id) {
    const auto* id = static_cast<const OrbisNpOnlineId*>(online_id);
    LOG_DEBUG(Lib_NpCommon, "sceNpIntIsValidOnlineId: online_id={:p}", fmt::ptr(online_id));

    if (id == nullptr || id->term != 0) {
        return 0;
    }

    const size_t len = strnlen(id->data, ORBIS_NP_ONLINEID_MAX_LENGTH + 1);
    if (len < 3 || len > ORBIS_NP_ONLINEID_MAX_LENGTH) {
        return 0;
    }

    const unsigned char first = static_cast<unsigned char>(id->data[0]);
    if (!std::isalnum(first)) {
        return 0;
    }

    for (size_t i = 1; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(id->data[i]);
        if (std::isalnum(c) || c == '_' || c == '-') {
            continue;
        }
        return 0;
    }

    return 1;
}

s32 PS4_SYSV_ABI sceNpCalloutInitCtx(void* ctx, void* name, u64 stack_size, s32 priority,
                                     u64 affinity_mask) {
    auto* callout_ctx = static_cast<OrbisNpCalloutContext*>(ctx);

    LOG_DEBUG(Lib_NpCommon,
              "sceNpCalloutInitCtx: ctx={:p} name={:p} stack={:#x} priority={} affinity={:#x}",
              fmt::ptr(ctx), fmt::ptr(name), stack_size, priority, affinity_mask);

    s32 rc = ORBIS_NP_CALLOUT_ERROR_ALREADY_INITIALIZED;
    if (callout_ctx->active < 1) {
        callout_ctx->stop_requested = 0;
        callout_ctx->head = nullptr;
        rc = static_cast<s32>(sceNpMutexInit(&callout_ctx->mutex, name, 1));
        if (rc >= 0) {
            rc = sceNpCondInit(&callout_ctx->cond, name, 0);
            if (rc >= 0) {
                rc = sceNpCreateThread(&callout_ctx->thread, NpCalloutThreadMain, callout_ctx,
                                       priority, stack_size, affinity_mask, name);
                if (rc >= 0) {
                    callout_ctx->active = 1;
                    return ORBIS_OK;
                }
                sceNpCondDestroy(&callout_ctx->cond);
            }
            sceNpMutexDestroy(&callout_ctx->mutex);
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx(void* ctx, void* callout, u64 delay_usec, u64 handler,
                                        u64 arg) {
    auto* callout_ctx = static_cast<OrbisNpCalloutContext*>(ctx);
    auto* entry = static_cast<OrbisNpCalloutEntry*>(callout);

    LOG_DEBUG(Lib_NpCommon,
              "sceNpCalloutStartOnCtx: ctx={:p} callout={:p} delay={:#x} handler={:p} arg={:#x}",
              fmt::ptr(ctx), fmt::ptr(callout), delay_usec,
              fmt::ptr(reinterpret_cast<void*>(static_cast<uintptr_t>(handler))), arg);

    if (callout_ctx->active == 0) {
        return ORBIS_NP_CALLOUT_ERROR_NOT_INITIALIZED;
    }

    entry->next = nullptr;
    entry->handler = reinterpret_cast<OrbisNpCalloutHandler>(handler);
    entry->arg = arg;
    sceNpGetSystemClockUsec(&entry->deadline_usec);
    entry->deadline_usec += static_cast<s64>(static_cast<u32>(delay_usec));

    (void)sceNpMutexLock(&callout_ctx->mutex);

    for (auto* cur = callout_ctx->head; cur != nullptr; cur = cur->next) {
        if (cur == entry) {
            (void)sceNpMutexUnlock(&callout_ctx->mutex);
            return ORBIS_NP_CALLOUT_ERROR_DUPLICATE_ENTRY;
        }
    }

    OrbisNpCalloutEntry** prev_next = &callout_ctx->head;
    for (auto* cur = callout_ctx->head; cur != nullptr; cur = cur->next) {
        if (cur->deadline_usec >= entry->deadline_usec) {
            break;
        }
        prev_next = &cur->next;
    }

    entry->next = *prev_next;
    *prev_next = entry;
    sceNpCondSignal(&callout_ctx->cond);
    (void)sceNpMutexUnlock(&callout_ctx->mutex);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCalloutStartOnCtx64(void* ctx, void* callout, s64 delay_usec, u64 handler,
                                          u64 arg) {
    auto* callout_ctx = static_cast<OrbisNpCalloutContext*>(ctx);
    auto* entry = static_cast<OrbisNpCalloutEntry*>(callout);

    LOG_DEBUG(Lib_NpCommon,
              "sceNpCalloutStartOnCtx64: ctx={:p} callout={:p} delay={} handler={:p} arg={:#x}",
              fmt::ptr(ctx), fmt::ptr(callout), delay_usec,
              fmt::ptr(reinterpret_cast<void*>(static_cast<uintptr_t>(handler))), arg);

    if (callout_ctx->active == 0) {
        return ORBIS_NP_CALLOUT_ERROR_NOT_INITIALIZED;
    }

    entry->next = nullptr;
    entry->handler = reinterpret_cast<OrbisNpCalloutHandler>(handler);
    entry->arg = arg;
    sceNpGetSystemClockUsec(&entry->deadline_usec);
    entry->deadline_usec += delay_usec;

    (void)sceNpMutexLock(&callout_ctx->mutex);

    for (auto* cur = callout_ctx->head; cur != nullptr; cur = cur->next) {
        if (cur == entry) {
            (void)sceNpMutexUnlock(&callout_ctx->mutex);
            return ORBIS_NP_CALLOUT_ERROR_DUPLICATE_ENTRY;
        }
    }

    OrbisNpCalloutEntry** prev_next = &callout_ctx->head;
    for (auto* cur = callout_ctx->head; cur != nullptr; cur = cur->next) {
        if (cur->deadline_usec >= entry->deadline_usec) {
            break;
        }
        prev_next = &cur->next;
    }

    entry->next = *prev_next;
    *prev_next = entry;
    sceNpCondSignal(&callout_ctx->cond);
    (void)sceNpMutexUnlock(&callout_ctx->mutex);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCalloutStopOnCtx(void* ctx, void* callout, u32* removed) {
    auto* callout_ctx = static_cast<OrbisNpCalloutContext*>(ctx);
    auto* entry = static_cast<OrbisNpCalloutEntry*>(callout);

    LOG_DEBUG(Lib_NpCommon, "sceNpCalloutStopOnCtx: ctx={:p} callout={:p} removed={:p}",
              fmt::ptr(ctx), fmt::ptr(callout), fmt::ptr(removed));

    if (callout_ctx->active == 0) {
        return ORBIS_NP_CALLOUT_ERROR_NOT_INITIALIZED;
    }

    (void)sceNpMutexLock(&callout_ctx->mutex);
    u32 found = 0;
    OrbisNpCalloutEntry** prev_next = &callout_ctx->head;
    for (auto* cur = callout_ctx->head; cur != nullptr; cur = cur->next) {
        if (cur == entry) {
            *prev_next = cur->next;
            found = 1;
            break;
        }
        prev_next = &cur->next;
    }
    if (removed != nullptr) {
        *removed = found;
    }
    (void)sceNpMutexUnlock(&callout_ctx->mutex);
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceNpCalloutTermCtx(void* ctx) {
    auto* callout_ctx = static_cast<OrbisNpCalloutContext*>(ctx);

    LOG_DEBUG(Lib_NpCommon, "sceNpCalloutTermCtx: ctx={:p}", fmt::ptr(ctx));

    if (callout_ctx->active != 0) {
        (void)sceNpMutexLock(&callout_ctx->mutex);
        callout_ctx->stop_requested = 1;
        sceNpCondSignal(&callout_ctx->cond);
        (void)sceNpMutexUnlock(&callout_ctx->mutex);
        (void)sceNpJoinThread(&callout_ctx->thread, nullptr);
        sceNpCondDestroy(&callout_ctx->cond);
        sceNpMutexDestroy(&callout_ctx->mutex);
        callout_ctx->active = 0;
    }
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpOnlineId);
    LIB_FUNCTION("1a+iY5YUJcI", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCondDestroy);
    LIB_FUNCTION("9+m5nRdJ-wQ", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCalloutInitCtx);
    LIB_FUNCTION("AqJ4xkWsV+I", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCalloutTermCtx);
    LIB_FUNCTION("18j+qk6dRwk", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpLwMutexLock);
    LIB_FUNCTION("1CiXI-MyEKs", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpLwMutexInit);
    LIB_FUNCTION("4zxevggtYrQ", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpLwMutexDestroy);
    LIB_FUNCTION("CQG2oyx1-nM", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpLwMutexUnlock);
    LIB_FUNCTION("DuslmoqQ+nk", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpMutexTryLock);
    LIB_FUNCTION("EjMsfO3GCIA", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpJoinThread);
    LIB_FUNCTION("fClnlkZmA6k", "libSceNpCommonCompat", 1, "libSceNpCommon",
                 sceNpCalloutStartOnCtx);
    LIB_FUNCTION("fhJ5uKzcn0w", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCreateThread);
    LIB_FUNCTION("hkeX9iuCwlI", "libSceNpCommonCompat", 1, "libSceNpCommon",
                 sceNpIntIsValidOnlineId);
    LIB_FUNCTION("hp0kVgu5Fxw", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpLwMutexTryLock);
    LIB_FUNCTION("in19gH7G040", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCalloutStopOnCtx);
    LIB_FUNCTION("lQ11BpMM4LU", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpMutexDestroy);
    LIB_FUNCTION("lpr66Gby8dQ", "libSceNpCommonCompat", 1, "libSceNpCommon",
                 sceNpCalloutStartOnCtx64);
    LIB_FUNCTION("oZyb9ktuCpA", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpMutexUnlock);
    LIB_FUNCTION("PVVsRmMkO1g", "libSceNpCommonCompat", 1, "libSceNpCommon",
                 sceNpGetSystemClockUsec);
    LIB_FUNCTION("q2tsVO3lM4A", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCondInit);
    LIB_FUNCTION("r9Bet+s6fKc", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpMutexLock);
    LIB_FUNCTION("sXVQUIGmk2U", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpGetPlatformType);
    LIB_FUNCTION("ss2xO9IJxKQ", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCondTimedwait);
    LIB_FUNCTION("uEwag-0YZPc", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpMutexInit);
    LIB_FUNCTION("uMJFOA62mVU", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCondSignal);
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpOnlineId);
    LIB_FUNCTION("1a+iY5YUJcI", "libSceNpCommon", 1, "libSceNpCommon", sceNpCondDestroy);
    LIB_FUNCTION("9+m5nRdJ-wQ", "libSceNpCommon", 1, "libSceNpCommon", sceNpCalloutInitCtx);
    LIB_FUNCTION("AqJ4xkWsV+I", "libSceNpCommon", 1, "libSceNpCommon", sceNpCalloutTermCtx);
    LIB_FUNCTION("18j+qk6dRwk", "libSceNpCommon", 1, "libSceNpCommon", sceNpLwMutexLock);
    LIB_FUNCTION("1CiXI-MyEKs", "libSceNpCommon", 1, "libSceNpCommon", sceNpLwMutexInit);
    LIB_FUNCTION("4zxevggtYrQ", "libSceNpCommon", 1, "libSceNpCommon", sceNpLwMutexDestroy);
    LIB_FUNCTION("CQG2oyx1-nM", "libSceNpCommon", 1, "libSceNpCommon", sceNpLwMutexUnlock);
    LIB_FUNCTION("DuslmoqQ+nk", "libSceNpCommon", 1, "libSceNpCommon", sceNpMutexTryLock);
    LIB_FUNCTION("EjMsfO3GCIA", "libSceNpCommon", 1, "libSceNpCommon", sceNpJoinThread);
    LIB_FUNCTION("fClnlkZmA6k", "libSceNpCommon", 1, "libSceNpCommon", sceNpCalloutStartOnCtx);
    LIB_FUNCTION("fhJ5uKzcn0w", "libSceNpCommon", 1, "libSceNpCommon", sceNpCreateThread);
    LIB_FUNCTION("hkeX9iuCwlI", "libSceNpCommon", 1, "libSceNpCommon", sceNpIntIsValidOnlineId);
    LIB_FUNCTION("hp0kVgu5Fxw", "libSceNpCommon", 1, "libSceNpCommon", sceNpLwMutexTryLock);
    LIB_FUNCTION("in19gH7G040", "libSceNpCommon", 1, "libSceNpCommon", sceNpCalloutStopOnCtx);
    LIB_FUNCTION("lQ11BpMM4LU", "libSceNpCommon", 1, "libSceNpCommon", sceNpMutexDestroy);
    LIB_FUNCTION("lpr66Gby8dQ", "libSceNpCommon", 1, "libSceNpCommon", sceNpCalloutStartOnCtx64);
    LIB_FUNCTION("oZyb9ktuCpA", "libSceNpCommon", 1, "libSceNpCommon", sceNpMutexUnlock);
    LIB_FUNCTION("PVVsRmMkO1g", "libSceNpCommon", 1, "libSceNpCommon", sceNpGetSystemClockUsec);
    LIB_FUNCTION("q2tsVO3lM4A", "libSceNpCommon", 1, "libSceNpCommon", sceNpCondInit);
    LIB_FUNCTION("r9Bet+s6fKc", "libSceNpCommon", 1, "libSceNpCommon", sceNpMutexLock);
    LIB_FUNCTION("sXVQUIGmk2U", "libSceNpCommon", 1, "libSceNpCommon", sceNpGetPlatformType);
    LIB_FUNCTION("ss2xO9IJxKQ", "libSceNpCommon", 1, "libSceNpCommon", sceNpCondTimedwait);
    LIB_FUNCTION("uEwag-0YZPc", "libSceNpCommon", 1, "libSceNpCommon", sceNpMutexInit);
    LIB_FUNCTION("uMJFOA62mVU", "libSceNpCommon", 1, "libSceNpCommon", sceNpCondSignal);
};

} // namespace Libraries::Np::NpCommon
