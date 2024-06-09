// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "kernel_threads.h"

namespace Libraries::Kernel {

extern PThreadCxt* g_pthread_cxt;

void* CreatePosixCond(void* addr) {
    if (addr == nullptr || *static_cast<OrbisPthreadCond*>(addr) != nullptr) {
        return addr;
    }
    posix_pthread_cond_init(static_cast<OrbisPthreadCond*>(addr), nullptr);
    return addr;
}

int PS4_SYSV_ABI posix_pthread_cond_broadcast(OrbisPthreadCond* cond) {
    if (*cond == nullptr) {
        cond = static_cast<OrbisPthreadCond*>(CreatePosixCond(cond));
    }
    int result = pthread_cond_broadcast(&(*cond)->cond);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_cond_broadcast: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_destroy(OrbisPthreadCond* cond) {
    int result = pthread_cond_destroy(&(*cond)->cond);
    delete *cond;
    *cond = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_cond_destroy: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_init(OrbisPthreadCond* cond, const OrbisPthreadCondattr* attr) {
    *cond = new PthreadCondInternal{};
    if (attr == nullptr) {
        attr = g_pthread_cxt->getDefaultCondattr();
    }
    int result = pthread_cond_init(&(*cond)->cond, &(*attr)->cond_attr);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_cond_init: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_reltimedwait_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_cond_setname_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_cond_signal(OrbisPthreadCond* cond) {
    if (*cond == nullptr) {
        cond = static_cast<OrbisPthreadCond*>(CreatePosixCond(cond));
    }
    int result = pthread_cond_signal(&(*cond)->cond);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondSignal: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_signalto_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_cond_timedwait(OrbisPthreadCond* cond, ScePthreadMutex* mutex,
                                              u64 usec) {
    if (*cond == nullptr) {
        cond = static_cast<OrbisPthreadCond*>(CreatePosixCond(cond));
    }
    timespec time{};
    time.tv_sec = usec / 1000000;
    time.tv_nsec = ((usec % 1000000) * 1000);
    int result = pthread_cond_timedwait(&(*cond)->cond, &(*mutex)->pth_mutex, &time);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_cond_timedwait: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_wait(OrbisPthreadCond* cond, ScePthreadMutex* mutex) {
    if (*cond == nullptr) {
        cond = static_cast<OrbisPthreadCond*>(CreatePosixCond(cond));
    }
    int result = pthread_cond_wait(&(*cond)->cond, &(*mutex)->pth_mutex);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_cond_wait: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_condattr_destroy(OrbisPthreadCondattr* attr) {
    int result = pthread_condattr_destroy(&(*attr)->cond_attr);
    delete *attr;
    *attr = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_condattr_destroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_condattr_getclock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_condattr_getpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_condattr_init(OrbisPthreadCondattr* attr) {
    *attr = new PthreadCondAttrInternal{};
    int result = pthread_condattr_init(&(*attr)->cond_attr);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_condattr_init: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_condattr_setclock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_condattr_setpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondattrDestroy(OrbisPthreadCondattr* attr) {
    int result = pthread_condattr_destroy(&(*attr)->cond_attr);
    delete *attr;
    *attr = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondattrDestroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondattrGetclock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondattrGetpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondattrInit(OrbisPthreadCondattr* attr) {
    *attr = new PthreadCondAttrInternal{};
    int result = pthread_condattr_init(&(*attr)->cond_attr);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondattrInit: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondattrSetclock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondattrSetpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondBroadcast(OrbisPthreadCond* cond) {
    if (cond == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondBroadcast: cond = nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_cond_broadcast(&(*cond)->cond);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondBroadcast: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondDestroy(OrbisPthreadCond* cond) {
    if (cond == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondDestroy: cond = nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_cond_destroy(&(*cond)->cond);
    delete *cond;
    *cond = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondDestroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondInit(OrbisPthreadCond* cond, const OrbisPthreadCondattr* attr,
                                    const char* name) {
    *cond = new PthreadCondInternal{};
    if (cond == nullptr || *cond == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (attr == nullptr) {
        attr = g_pthread_cxt->getDefaultCondattr();
    }
    if (name != nullptr) {
        (*cond)->name = name;
    }

    int result = pthread_cond_init(&(*cond)->cond, &(*attr)->cond_attr);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondInit: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondSignal(OrbisPthreadCond* cond) {
    if (cond == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondSignal cond==nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_cond_signal(&(*cond)->cond);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondSignal: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondSignalto() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadCondTimedwait(OrbisPthreadCond* cond, ScePthreadMutex* mutex, u64 usec) {
    if (cond == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondTimedwait cond==nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (mutex == nullptr || *mutex == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondTimedwait mutex==nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    timespec time{};
    time.tv_sec = usec / 1000000;
    time.tv_nsec = ((usec % 1000000) * 1000);
    int result = pthread_cond_timedwait(&(*cond)->cond, &(*mutex)->pth_mutex, &time);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondTimedwait: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadCondWait(OrbisPthreadCond* cond, ScePthreadMutex* mutex) {
    if (cond == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondWait cond==nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (mutex == nullptr || *mutex == nullptr) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondWait mutex==nullptr");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_cond_wait(&(*cond)->cond, &(*mutex)->pth_mutex);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadCondWait: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

void ThreadsCondSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);
    LIB_FUNCTION("RXXqi4CtF8w", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_destroy);
    LIB_FUNCTION("0TyVk4MSLt0", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_init);
    LIB_FUNCTION("K953PF5u6Pc", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_cond_reltimedwait_np);
    LIB_FUNCTION("EZ8h70dtFLg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_setname_np);
    LIB_FUNCTION("2MOy+rUfuhQ", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_signal);
    LIB_FUNCTION("CI6Qy73ae10", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_signalto_np);
    LIB_FUNCTION("27bAgiJmOh0", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_timedwait);
    LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_wait);
    LIB_FUNCTION("dJcuQVn6-Iw", "libkernel", 1, "libkernel", 1, 1, posix_pthread_condattr_destroy);
    LIB_FUNCTION("cTDYxTUNPhM", "libkernel", 1, "libkernel", 1, 1, posix_pthread_condattr_getclock);
    LIB_FUNCTION("h0qUqSuOmC8", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_getpshared);
    LIB_FUNCTION("mKoTx03HRWA", "libkernel", 1, "libkernel", 1, 1, posix_pthread_condattr_init);
    LIB_FUNCTION("EjllaAqAPZo", "libkernel", 1, "libkernel", 1, 1, posix_pthread_condattr_setclock);
    LIB_FUNCTION("3BpP850hBT4", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_setpshared);
    LIB_FUNCTION("waPcxYiR3WA", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrDestroy);
    LIB_FUNCTION("6qM3kO5S3Oo", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrGetclock);
    LIB_FUNCTION("Dn-DRWi9t54", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrGetpshared);
    LIB_FUNCTION("m5-2bsNfv7s", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrInit);
    LIB_FUNCTION("c-bxj027czs", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrSetclock);
    LIB_FUNCTION("6xMew9+rZwI", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrSetpshared);
    LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel", 1, 1, scePthreadCondBroadcast);
    LIB_FUNCTION("g+PZd2hiacg", "libkernel", 1, "libkernel", 1, 1, scePthreadCondDestroy);
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", 1, 1, scePthreadCondInit);
    LIB_FUNCTION("kDh-NfxgMtE", "libkernel", 1, "libkernel", 1, 1, scePthreadCondSignal);
    LIB_FUNCTION("o69RpYO-Mu0", "libkernel", 1, "libkernel", 1, 1, scePthreadCondSignalto);
    LIB_FUNCTION("BmMjYxmew1w", "libkernel", 1, "libkernel", 1, 1, scePthreadCondTimedwait);
    LIB_FUNCTION("WKAXJ4XBPQ4", "libkernel", 1, "libkernel", 1, 1, scePthreadCondWait);
    LIB_FUNCTION("EZ8h70dtFLg", "libkernel_psmkit", 1, "libkernel", 1, 1,
                 posix_pthread_cond_setname_np);
    LIB_FUNCTION("mkx2fVhNMsg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);
    LIB_FUNCTION("RXXqi4CtF8w", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_destroy);
    LIB_FUNCTION("0TyVk4MSLt0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_init);
    LIB_FUNCTION("2MOy+rUfuhQ", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_signal);
    LIB_FUNCTION("CI6Qy73ae10", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_cond_signalto_np);
    LIB_FUNCTION("27bAgiJmOh0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_timedwait);
    LIB_FUNCTION("Op8TBGY5KHg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_wait);
    LIB_FUNCTION("dJcuQVn6-Iw", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_destroy);
    LIB_FUNCTION("cTDYxTUNPhM", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_getclock);
    LIB_FUNCTION("h0qUqSuOmC8", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_getpshared);
    LIB_FUNCTION("mKoTx03HRWA", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_condattr_init);
    LIB_FUNCTION("EjllaAqAPZo", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_setclock);
    LIB_FUNCTION("3BpP850hBT4", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_condattr_setpshared);
}
} // namespace Libraries::Kernel