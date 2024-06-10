// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "kernel_threads.h"

namespace Libraries::Kernel {

extern PThreadCxt* g_pthread_cxt;

int PS4_SYSV_ABI posix_pthread_rwlock_destroy(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_destroy(&(*rwlock)->pth_rwlock);
    delete *rwlock;
    *rwlock = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_destroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlock_init(OrbisPthreadRwlock* rwlock,
                                           const OrbisPthreadRwlockattr* attr, const char* name) {
    *rwlock = new PthreadRwInternal{};
    if (attr == nullptr || *attr == nullptr) {
        attr = g_pthread_cxt->getDefaultRwattr();
    }
    int result = pthread_rwlock_init(&(*rwlock)->pth_rwlock, &(*attr)->attr_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_init: error = {}", result);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_rdlock(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_rdlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_rdlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlock_reltimedrdlock_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_reltimedwrlock_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_setname_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedrdlock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedwrlock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlock_tryrdlock(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_tryrdlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_tryrdlock: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlock_trywrlock(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_trywrlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_trywrlock: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlock_unlock(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_unlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_unlock: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlock_wrlock(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_wrlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlock_wrlock: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_destroy(OrbisPthreadRwlockattr* attr) {
    int result = pthread_rwlockattr_destroy(&(*attr)->attr_rwlock);
    delete *attr;
    *attr = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlockattr_destroy: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_getpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_gettype_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_init(OrbisPthreadRwlockattr* attr) {
    *attr = new PthreadRwLockAttrInernal{};
    int result = pthread_rwlockattr_init(&(*attr)->attr_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "posix_pthread_rwlockattr_init: error = {}", result);
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_setpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_settype_np() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockattrDestroy(OrbisPthreadRwlockattr* attr) {
    int result = pthread_rwlockattr_destroy(&(*attr)->attr_rwlock);
    delete *attr;
    *attr = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockattrDestroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockattrGetpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockattrGettype() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockattrInit(OrbisPthreadRwlockattr* attr) {
    *attr = new PthreadRwLockAttrInernal{};
    int result = pthread_rwlockattr_init(&(*attr)->attr_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockattrInit: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockattrSetpshared() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockattrSettype() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockDestroy(OrbisPthreadRwlock* rwlock) {
    int result = pthread_rwlock_destroy(&(*rwlock)->pth_rwlock);
    delete *rwlock;
    *rwlock = nullptr;
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockDestroy: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockInit(OrbisPthreadRwlock* rwlock,
                                      const OrbisPthreadRwlockattr* attr, const char* name) {
    *rwlock = new PthreadRwInternal{};
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (attr == nullptr || *attr == nullptr) {
        attr = g_pthread_cxt->getDefaultRwattr();
    }
    if (name != nullptr) {
        (*rwlock)->name = name;
    }
    int result = pthread_rwlock_init(&(*rwlock)->pth_rwlock, &(*attr)->attr_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockInit: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockRdlock(OrbisPthreadRwlock* rwlock) {
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_rwlock_rdlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockRdlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockTimedrdlock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockTimedwrlock() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePthreadRwlockTryrdlock(OrbisPthreadRwlock* rwlock) {
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_rwlock_tryrdlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockTryrdlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockTrywrlock(OrbisPthreadRwlock* rwlock) {
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_rwlock_trywrlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockTrywrlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockUnlock(OrbisPthreadRwlock* rwlock) {
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_rwlock_unlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockUnlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

int PS4_SYSV_ABI scePthreadRwlockWrlock(OrbisPthreadRwlock* rwlock) {
    if (rwlock == nullptr || *rwlock == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int result = pthread_rwlock_wrlock(&(*rwlock)->pth_rwlock);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadRwlockWrlock: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

void ThreadsRwlockSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1471ajPzxh0", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_destroy);
    LIB_FUNCTION("ytQULN-nhL4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_init);
    LIB_FUNCTION("iGjsr1WAtI0", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_rdlock);
    LIB_FUNCTION("dYv-+If2GPk", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_reltimedrdlock_np);
    LIB_FUNCTION("RRnSj8h8VR4", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_reltimedwrlock_np);
    LIB_FUNCTION("Uwxgnsi3xeM", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_setname_np);
    LIB_FUNCTION("lb8lnYo-o7k", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_timedrdlock);
    LIB_FUNCTION("9zklzAl9CGM", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_timedwrlock);
    LIB_FUNCTION("SFxTMOfuCkE", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_tryrdlock);
    LIB_FUNCTION("XhWHn6P5R7U", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_trywrlock);
    LIB_FUNCTION("EgmLo6EWgso", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_unlock);
    LIB_FUNCTION("sIlRvQqsN2Y", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlock_wrlock);
    LIB_FUNCTION("qsdmgXjqSgk", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_destroy);
    LIB_FUNCTION("VqEMuCv-qHY", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_getpshared);
    LIB_FUNCTION("l+bG5fsYkhg", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_gettype_np);
    LIB_FUNCTION("xFebsA4YsFI", "libkernel", 1, "libkernel", 1, 1, posix_pthread_rwlockattr_init);
    LIB_FUNCTION("OuKg+kRDD7U", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_setpshared);
    LIB_FUNCTION("8NuOHiTr1Vw", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_settype_np);
    LIB_FUNCTION("1471ajPzxh0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlock_destroy);
    LIB_FUNCTION("ytQULN-nhL4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlock_init);
    LIB_FUNCTION("iGjsr1WAtI0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlock_rdlock);
    LIB_FUNCTION("lb8lnYo-o7k", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_timedrdlock);
    LIB_FUNCTION("9zklzAl9CGM", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_timedwrlock);
    LIB_FUNCTION("SFxTMOfuCkE", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_tryrdlock);
    LIB_FUNCTION("XhWHn6P5R7U", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlock_trywrlock);
    LIB_FUNCTION("EgmLo6EWgso", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlock_unlock);
    LIB_FUNCTION("sIlRvQqsN2Y", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlock_wrlock);
    LIB_FUNCTION("qsdmgXjqSgk", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_destroy);
    LIB_FUNCTION("VqEMuCv-qHY", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_getpshared);
    LIB_FUNCTION("l+bG5fsYkhg", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_gettype_np);
    LIB_FUNCTION("xFebsA4YsFI", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rwlockattr_init);
    LIB_FUNCTION("OuKg+kRDD7U", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_setpshared);
    LIB_FUNCTION("8NuOHiTr1Vw", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_rwlockattr_settype_np);
    LIB_FUNCTION("i2ifZ3fS2fo", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrDestroy);
    LIB_FUNCTION("LcOZBHGqbFk", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrGetpshared);
    LIB_FUNCTION("Kyls1ChFyrc", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrGettype);
    LIB_FUNCTION("yOfGg-I1ZII", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrInit);
    LIB_FUNCTION("-ZvQH18j10c", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrSetpshared);
    LIB_FUNCTION("h-OifiouBd8", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockattrSettype);
    LIB_FUNCTION("BB+kb08Tl9A", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockDestroy);
    LIB_FUNCTION("6ULAa0fq4jA", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockInit);
    LIB_FUNCTION("Ox9i0c7L5w0", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockRdlock);
    LIB_FUNCTION("iPtZRWICjrM", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockTimedrdlock);
    LIB_FUNCTION("adh--6nIqTk", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockTimedwrlock);
    LIB_FUNCTION("XD3mDeybCnk", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockTryrdlock);
    LIB_FUNCTION("bIHoZCTomsI", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockTrywrlock);
    LIB_FUNCTION("+L98PIbGttk", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockUnlock);
    LIB_FUNCTION("mqdNorrB+gI", "libkernel", 1, "libkernel", 1, 1, scePthreadRwlockWrlock);
}
} // namespace Libraries::Kernel