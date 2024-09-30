// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "common/thread.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/tls.h"

namespace Libraries::Kernel {

thread_local ScePthread g_pthread_self{};
PThreadCxt* g_pthread_cxt = nullptr;

void init_pthreads() {
    g_pthread_cxt = new PThreadCxt{};
    // default attr init
    ScePthreadAttr default_attr = nullptr;
    scePthreadAttrInit(&default_attr);
    g_pthread_cxt->SetDefaultAttr(default_attr);
    g_pthread_cxt->SetPthreadPool(new PThreadPool);
}

void pthreadInitSelfMainThread() {
    const char* name = "Main_Thread";
    auto* pthread_pool = g_pthread_cxt->GetPthreadPool();
    g_pthread_self = pthread_pool->Create(name);
    scePthreadAttrInit(&g_pthread_self->attr);
    g_pthread_self->pth = pthread_self();
    g_pthread_self->name = name;
}

int PS4_SYSV_ABI scePthreadSetaffinity(ScePthread thread, const /*SceKernelCpumask*/ u64 mask) {
    LOG_DEBUG(Kernel_Pthread, "called");

    if (thread == nullptr) {
        return SCE_KERNEL_ERROR_ESRCH;
    }

    auto result = scePthreadAttrSetaffinity(&thread->attr, mask);

    return result;
}

int PS4_SYSV_ABI scePthreadGetaffinity(ScePthread thread, /*SceKernelCpumask*/ u64* mask) {
    LOG_INFO(Kernel_Pthread, "called");

    if (thread == nullptr) {
        return SCE_KERNEL_ERROR_ESRCH;
    }

    auto result = scePthreadAttrGetaffinity(&thread->attr, mask);

    return result;
}

int PS4_SYSV_ABI posix_pthread_once(pthread_once_t* once_control, void (*init_routine)(void)) {
    return pthread_once(once_control, init_routine);
}

static int pthread_copy_attributes(ScePthreadAttr* dst, const ScePthreadAttr* src) {
    if (dst == nullptr || *dst == nullptr || src == nullptr || *src == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    u64 mask = 0;
    int state = 0;
    size_t guard_size = 0;
    int inherit_sched = 0;
    SceKernelSchedParam param = {};
    int policy = 0;
    void* stack_addr = nullptr;
    size_t stack_size = 0;

    int result = 0;

    result = (result == 0 ? scePthreadAttrGetaffinity(src, &mask) : result);
    result = (result == 0 ? scePthreadAttrGetdetachstate(src, &state) : result);
    result = (result == 0 ? scePthreadAttrGetguardsize(src, &guard_size) : result);
    result = (result == 0 ? scePthreadAttrGetinheritsched(src, &inherit_sched) : result);
    result = (result == 0 ? scePthreadAttrGetschedparam(src, &param) : result);
    result = (result == 0 ? scePthreadAttrGetschedpolicy(src, &policy) : result);
    result = (result == 0 ? scePthreadAttrGetstackaddr(src, &stack_addr) : result);
    result = (result == 0 ? scePthreadAttrGetstacksize(src, &stack_size) : result);

    result = (result == 0 ? scePthreadAttrSetaffinity(dst, mask) : result);
    result = (result == 0 ? scePthreadAttrSetdetachstate(dst, state) : result);
    result = (result == 0 ? scePthreadAttrSetguardsize(dst, guard_size) : result);
    result = (result == 0 ? scePthreadAttrSetinheritsched(dst, inherit_sched) : result);
    result = (result == 0 ? scePthreadAttrSetschedparam(dst, &param) : result);
    result = (result == 0 ? scePthreadAttrSetschedpolicy(dst, policy) : result);
    if (stack_addr != nullptr) {
        result = (result == 0 ? scePthreadAttrSetstackaddr(dst, stack_addr) : result);
    }
    if (stack_size != 0) {
        result = (result == 0 ? scePthreadAttrSetstacksize(dst, stack_size) : result);
    }

    return result;
}

static void cleanup_thread(void* arg) {
    auto* thread = static_cast<ScePthread>(arg);
    for (const auto& [key, destructor] : thread->key_destructors) {
        if (void* value = pthread_getspecific(key); value != nullptr) {
            destructor(value);
        }
    }
    Core::SetTcbBase(nullptr);
    thread->is_almost_done = true;
}

static void* run_thread(void* arg) {
    auto* thread = static_cast<ScePthread>(arg);
    Common::SetCurrentThreadName(thread->name.c_str());
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    void* ret = nullptr;
    g_pthread_self = thread;
    pthread_cleanup_push(cleanup_thread, thread);
    thread->is_started = true;
    ret = linker->ExecuteGuest(thread->entry, thread->arg);
    pthread_cleanup_pop(1);
    return ret;
}

int PS4_SYSV_ABI scePthreadCreate(ScePthread* thread, const ScePthreadAttr* attr,
                                  PthreadEntryFunc start_routine, void* arg, const char* name) {
    if (thread == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    auto* pthread_pool = g_pthread_cxt->GetPthreadPool();

    if (attr == nullptr) {
        attr = g_pthread_cxt->GetDefaultAttr();
    }

    *thread = pthread_pool->Create(name);

    if ((*thread)->attr != nullptr) {
        scePthreadAttrDestroy(&(*thread)->attr);
    }
    scePthreadAttrInit(&(*thread)->attr);

    int result = pthread_copy_attributes(&(*thread)->attr, attr);
    ASSERT(result == 0);

    if (name != NULL) {
        (*thread)->name = name;
    } else {
        (*thread)->name = "no-name";
    }
    (*thread)->entry = start_routine;
    (*thread)->arg = arg;
    (*thread)->is_almost_done = false;
    (*thread)->is_detached = (*attr)->detached;
    (*thread)->is_started = false;

    pthread_attr_setstacksize(&(*attr)->pth_attr, 2_MB);
    result = pthread_create(&(*thread)->pth, &(*attr)->pth_attr, run_thread, *thread);

    LOG_INFO(Kernel_Pthread, "thread create name = {}", (*thread)->name);

    switch (result) {
    case 0:
        return SCE_OK;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    case EAGAIN:
        return SCE_KERNEL_ERROR_EAGAIN;
    case EDEADLK:
        return SCE_KERNEL_ERROR_EDEADLK;
    case EPERM:
        return SCE_KERNEL_ERROR_EPERM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

ScePthread PThreadPool::Create(const char* name) {
    std::scoped_lock lock{m_mutex};

    for (auto* p : m_threads) {
        if (p->is_free && name != nullptr && p->name == name) {
            p->is_free = false;
            return p;
        }
    }

    auto* ret = new PthreadInternal{};
    ret->is_free = false;
    ret->is_detached = false;
    ret->is_almost_done = false;
    ret->attr = nullptr;

    m_threads.push_back(ret);

    return ret;
}

void PS4_SYSV_ABI scePthreadYield() {
    sched_yield();
}

void PS4_SYSV_ABI posix_pthread_yield() {
    sched_yield();
}

int PS4_SYSV_ABI scePthreadJoin(ScePthread thread, void** res) {
    int result = pthread_join(thread->pth, res);
    LOG_INFO(Kernel_Pthread, "scePthreadJoin result = {}", result);
    thread->is_detached = false;
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_join(ScePthread thread, void** res) {
    int result = pthread_join(thread->pth, res);
    LOG_INFO(Kernel_Pthread, "posix_pthread_join result = {}", result);
    thread->is_detached = false;
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadDetach(ScePthread thread) {
    thread->is_detached = true;
    return ORBIS_OK;
}

ScePthread PS4_SYSV_ABI posix_pthread_self() {
    return g_pthread_self;
}

int PS4_SYSV_ABI scePthreadEqual(ScePthread thread1, ScePthread thread2) {
    return (thread1 == thread2 ? 1 : 0);
}

int PS4_SYSV_ABI posix_pthread_equal(ScePthread thread1, ScePthread thread2) {
    return (thread1 == thread2 ? 1 : 0);
}

struct TlsIndex {
    u64 ti_module;
    u64 ti_offset;
};

void* PS4_SYSV_ABI __tls_get_addr(TlsIndex* index) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    return linker->TlsGetAddr(index->ti_module, index->ti_offset);
}

int PS4_SYSV_ABI posix_sched_get_priority_max() {
    return ORBIS_KERNEL_PRIO_FIFO_HIGHEST;
}

int PS4_SYSV_ABI posix_sched_get_priority_min() {
    return ORBIS_KERNEL_PRIO_FIFO_LOWEST;
}

int PS4_SYSV_ABI posix_pthread_setprio(ScePthread thread, int prio) {
    int result = scePthreadSetprio(thread, prio);
    if (result < 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_create_name_np(ScePthread* thread, const ScePthreadAttr* attr,
                                              PthreadEntryFunc start_routine, void* arg,
                                              const char* name) {
    int result = scePthreadCreate(thread, attr, start_routine, arg, name);
    if (result != 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_create(ScePthread* thread, const ScePthreadAttr* attr,
                                      PthreadEntryFunc start_routine, void* arg) {
    return posix_pthread_create_name_np(thread, attr, start_routine, arg, "NoName");
}

int PS4_SYSV_ABI posix_pthread_setcancelstate(int state, int* oldstate) {
    return pthread_setcancelstate(state, oldstate);
}

int PS4_SYSV_ABI posix_pthread_detach(ScePthread thread) {
    return pthread_detach(thread->pth);
}

int PS4_SYSV_ABI scePthreadGetschedparam(ScePthread thread, int* policy,
                                         SceKernelSchedParam* param) {
    return pthread_getschedparam(thread->pth, policy, param);
}

int PS4_SYSV_ABI scePthreadSetschedparam(ScePthread thread, int policy,
                                         const SceKernelSchedParam* param) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called policy={}, sched_priority={}", policy,
              param->sched_priority);
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadOnce(int* once_control, void (*init_routine)(void)) {
    return pthread_once(reinterpret_cast<pthread_once_t*>(once_control), init_routine);
}

[[noreturn]] void PS4_SYSV_ABI scePthreadExit(void* value_ptr) {
    g_pthread_self->is_free = true;

    pthread_exit(value_ptr);
    UNREACHABLE();
}

[[noreturn]] void PS4_SYSV_ABI posix_pthread_exit(void* value_ptr) {
    pthread_exit(value_ptr);
    UNREACHABLE();
}

int PS4_SYSV_ABI scePthreadGetthreadid() {
    return (int)(size_t)g_pthread_self;
}

int PS4_SYSV_ABI scePthreadGetprio(ScePthread thread, int* prio) {
    *prio = thread->prio;
    return ORBIS_OK;
}
int PS4_SYSV_ABI scePthreadSetprio(ScePthread thread, int prio) {
    if (thread == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadSetprio: thread is nullptr");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    thread->prio = prio;
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_getschedparam(ScePthread thread, int* policy,
                                             SceKernelSchedParam* param) {
    return scePthreadGetschedparam(thread, policy, param);
}

int PS4_SYSV_ABI posix_pthread_setschedparam(ScePthread thread, int policy,
                                             const SceKernelSchedParam* param) {
    return scePthreadSetschedparam(thread, policy, param);
}

int PS4_SYSV_ABI scePthreadRename(ScePthread thread, const char* name) {
    thread->name = name;
    LOG_INFO(Kernel_Pthread, "scePthreadRename: name = {}", thread->name);
    return SCE_OK;
}

void RegisterThreads(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("lZzFeSxPl08", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setcancelstate);
    LIB_FUNCTION("4+h9EzwKF4I", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetschedpolicy);
    LIB_FUNCTION("-Wreprtu0Qs", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetdetachstate);
    LIB_FUNCTION("eXbUSpEaTsA", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetinheritsched);
    LIB_FUNCTION("DzES9hQF4f4", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetschedparam);
    LIB_FUNCTION("nsYoNRywwNg", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrInit);
    LIB_FUNCTION("onNY9Byn-W8", "libkernel", 1, "libkernel", 1, 1, scePthreadJoin);
    LIB_FUNCTION("4qGrR6eoP9Y", "libkernel", 1, "libkernel", 1, 1, scePthreadDetach);
    LIB_FUNCTION("3PtV6p3QNX4", "libkernel", 1, "libkernel", 1, 1, scePthreadEqual);
    LIB_FUNCTION("3kg7rT0NQIs", "libkernel", 1, "libkernel", 1, 1, scePthreadExit);
    LIB_FUNCTION("FJrT5LuUBAU", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_exit);
    LIB_FUNCTION("7Xl257M4VNI", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_equal);
    LIB_FUNCTION("h9CcP3J0oVM", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_join);
    LIB_FUNCTION("EI-5-jlq2dE", "libkernel", 1, "libkernel", 1, 1, scePthreadGetthreadid);
    LIB_FUNCTION("1tKyG7RlMJo", "libkernel", 1, "libkernel", 1, 1, scePthreadGetprio);
    LIB_FUNCTION("W0Hpm2X0uPE", "libkernel", 1, "libkernel", 1, 1, scePthreadSetprio);
    LIB_FUNCTION("GBUY7ywdULE", "libkernel", 1, "libkernel", 1, 1, scePthreadRename);

    LIB_FUNCTION("aI+OeCz8xrQ", "libkernel", 1, "libkernel", 1, 1, scePthreadSelf);
    LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("EotR8a3ASf4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("3qxgM4ezETA", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetaffinity);
    LIB_FUNCTION("P41kTWUS3EI", "libkernel", 1, "libkernel", 1, 1, scePthreadGetschedparam);
    LIB_FUNCTION("oIRFTjoILbg", "libkernel", 1, "libkernel", 1, 1, scePthreadSetschedparam);
    LIB_FUNCTION("vNe1w4diLCs", "libkernel", 1, "libkernel", 1, 1, __tls_get_addr);
    LIB_FUNCTION("OxhIB8LB-PQ", "libkernel", 1, "libkernel", 1, 1, posix_pthread_create);
    LIB_FUNCTION("OxhIB8LB-PQ", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_create);
    LIB_FUNCTION("bt3CTBKmGyI", "libkernel", 1, "libkernel", 1, 1, scePthreadSetaffinity);
    LIB_FUNCTION("rcrVFJsQWRY", "libkernel", 1, "libkernel", 1, 1, scePthreadGetaffinity);
    LIB_FUNCTION("6UgtwV+0zb4", "libkernel", 1, "libkernel", 1, 1, scePthreadCreate);
    LIB_FUNCTION("T72hz6ffq08", "libkernel", 1, "libkernel", 1, 1, scePthreadYield);
    LIB_FUNCTION("B5GmVDKwpn0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_yield);
    LIB_FUNCTION("14bOACANTBo", "libkernel", 1, "libkernel", 1, 1, scePthreadOnce);
    LIB_FUNCTION("Z4QosVuAsA0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_once);
    LIB_FUNCTION("a2P9wYGeZvc", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setprio);
    LIB_FUNCTION("Jmi+9w9u0E4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_create_name_np);
    LIB_FUNCTION("OxhIB8LB-PQ", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_create);
    LIB_FUNCTION("+U1R4WtXvoc", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_detach);
    LIB_FUNCTION("CBNtXOoef-E", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_max);
    LIB_FUNCTION("m0iS6jNsXds", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_min);
    LIB_FUNCTION("FIs3-UQT9sg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_getschedparam);
    LIB_FUNCTION("Xs9hdiD7sAA", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setschedparam);

    RegisterMutex(sym);
    RegisterCond(sym);
    RegisterRwlock(sym);
    RegisterSemaphore(sym);
    RegisterSpec(sym);
    RegisterThreadAttr(sym);
}

} // namespace Libraries::Kernel
