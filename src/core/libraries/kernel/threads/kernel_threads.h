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
int PS4_SYSV_ABI posix_pthread_rwlock_destroy();
int PS4_SYSV_ABI posix_pthread_rwlock_init();
int PS4_SYSV_ABI posix_pthread_rwlock_rdlock();
int PS4_SYSV_ABI posix_pthread_rwlock_reltimedrdlock_np();
int PS4_SYSV_ABI posix_pthread_rwlock_reltimedwrlock_np();
int PS4_SYSV_ABI posix_pthread_rwlock_setname_np();
int PS4_SYSV_ABI posix_pthread_rwlock_timedrdlock();
int PS4_SYSV_ABI posix_pthread_rwlock_timedwrlock();
int PS4_SYSV_ABI posix_pthread_rwlock_tryrdlock();
int PS4_SYSV_ABI posix_pthread_rwlock_trywrlock();
int PS4_SYSV_ABI posix_pthread_rwlock_unlock();
int PS4_SYSV_ABI posix_pthread_rwlock_wrlock();
int PS4_SYSV_ABI posix_pthread_rwlockattr_destroy();
int PS4_SYSV_ABI posix_pthread_rwlockattr_getpshared();
int PS4_SYSV_ABI posix_pthread_rwlockattr_gettype_np();
int PS4_SYSV_ABI posix_pthread_rwlockattr_init();
int PS4_SYSV_ABI posix_pthread_rwlockattr_setpshared();
int PS4_SYSV_ABI posix_pthread_rwlockattr_settype_np();
int PS4_SYSV_ABI scePthreadRwlockattrDestroy();
int PS4_SYSV_ABI scePthreadRwlockattrGetpshared();
int PS4_SYSV_ABI scePthreadRwlockattrGettype();
int PS4_SYSV_ABI scePthreadRwlockattrInit();
int PS4_SYSV_ABI scePthreadRwlockattrSetpshared();
int PS4_SYSV_ABI scePthreadRwlockattrSettype();
int PS4_SYSV_ABI scePthreadRwlockDestroy();
int PS4_SYSV_ABI scePthreadRwlockInit();
int PS4_SYSV_ABI scePthreadRwlockRdlock();
int PS4_SYSV_ABI scePthreadRwlockTimedrdlock();
int PS4_SYSV_ABI scePthreadRwlockTimedwrlock();
int PS4_SYSV_ABI scePthreadRwlockTryrdlock();
int PS4_SYSV_ABI scePthreadRwlockTrywrlock();
int PS4_SYSV_ABI scePthreadRwlockUnlock();
int PS4_SYSV_ABI scePthreadRwlockWrlock();

void ThreadsRwlockSymbolsRegister(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Kernel