// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/thread_management.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {
/****
 * rwlock calls
 */
int PS4_SYSV_ABI posix_pthread_rwlock_destroy(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlock_init(OrbisPthreadRwlock* rwlock,
                                           const OrbisPthreadRwlockattr* attr, const char* name);
int PS4_SYSV_ABI posix_pthread_rwlock_rdlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlock_reltimedrdlock_np();
int PS4_SYSV_ABI posix_pthread_rwlock_reltimedwrlock_np();
int PS4_SYSV_ABI posix_pthread_rwlock_setname_np();
int PS4_SYSV_ABI posix_pthread_rwlock_timedrdlock();
int PS4_SYSV_ABI posix_pthread_rwlock_timedwrlock();
int PS4_SYSV_ABI posix_pthread_rwlock_tryrdlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlock_trywrlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlock_unlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlock_wrlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI posix_pthread_rwlockattr_destroy(OrbisPthreadRwlockattr* attr);
int PS4_SYSV_ABI posix_pthread_rwlockattr_getpshared();
int PS4_SYSV_ABI posix_pthread_rwlockattr_gettype_np();
int PS4_SYSV_ABI posix_pthread_rwlockattr_init(OrbisPthreadRwlockattr* attr);
int PS4_SYSV_ABI posix_pthread_rwlockattr_setpshared();
int PS4_SYSV_ABI posix_pthread_rwlockattr_settype_np();
int PS4_SYSV_ABI scePthreadRwlockattrDestroy(OrbisPthreadRwlockattr* attr);
int PS4_SYSV_ABI scePthreadRwlockattrGetpshared();
int PS4_SYSV_ABI scePthreadRwlockattrGettype();
int PS4_SYSV_ABI scePthreadRwlockattrInit(OrbisPthreadRwlockattr* attr);
int PS4_SYSV_ABI scePthreadRwlockattrSetpshared();
int PS4_SYSV_ABI scePthreadRwlockattrSettype();
int PS4_SYSV_ABI scePthreadRwlockDestroy(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI scePthreadRwlockInit(OrbisPthreadRwlock* rwlock,
                                      const OrbisPthreadRwlockattr* attr, const char* name);
int PS4_SYSV_ABI scePthreadRwlockRdlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI scePthreadRwlockTimedrdlock();
int PS4_SYSV_ABI scePthreadRwlockTimedwrlock();
int PS4_SYSV_ABI scePthreadRwlockTryrdlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI scePthreadRwlockTrywrlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI scePthreadRwlockUnlock(OrbisPthreadRwlock* rwlock);
int PS4_SYSV_ABI scePthreadRwlockWrlock(OrbisPthreadRwlock* rwlock);

void ThreadsRwlockSymbolsRegister(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Kernel