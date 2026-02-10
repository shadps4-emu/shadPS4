// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/threads.h"
#include "core/libraries/libc_internal/libc_internal_threads.h"
#include "core/libraries/libs.h"

namespace Libraries::LibcInternal {

void getMutexName(char* buf, u64 size, const char* name) {
    if (name != nullptr) {
        std::snprintf(buf, size, "SceLibcI_%s", name);
    } else {
        std::snprintf(buf, size, "SceLibcI");
    }
}

s32 PS4_SYSV_ABI internal__Mtxinit(Libraries::Kernel::PthreadMutexT* mtx, const char* name) {
    char mtx_name[0x20];
    getMutexName(mtx_name, sizeof(mtx_name), name);

    Libraries::Kernel::PthreadMutexAttrT attr{};
    s32 result = Libraries::Kernel::posix_pthread_mutexattr_init(&attr);
    if (result != 0) {
        return 1;
    }

    result = Libraries::Kernel::posix_pthread_mutexattr_settype(
        &attr, Libraries::Kernel::PthreadMutexType::Recursive);
    if (result == 0) {
        s32 mtx_init_result = Libraries::Kernel::scePthreadMutexInit(mtx, &attr, mtx_name);
        result = Libraries::Kernel::posix_pthread_mutexattr_destroy(&attr);
        if (mtx_init_result == 0 && result == 0) {
            return 0;
        }
    } else {
        Libraries::Kernel::posix_pthread_mutexattr_destroy(&attr);
    }

    return 1;
}

s32 PS4_SYSV_ABI internal__Mtxlock(Libraries::Kernel::PthreadMutexT* mtx) {
    s32 result = Libraries::Kernel::posix_pthread_mutex_lock(mtx);
    return result != 0;
}

s32 PS4_SYSV_ABI internal__Mtxunlock(Libraries::Kernel::PthreadMutexT* mtx) {
    s32 result = Libraries::Kernel::posix_pthread_mutex_unlock(mtx);
    return result != 0;
}

s32 PS4_SYSV_ABI internal__Mtxdst(Libraries::Kernel::PthreadMutexT* mtx) {
    s32 result = Libraries::Kernel::posix_pthread_mutex_destroy(mtx);
    return result != 0;
}

void RegisterlibSceLibcInternalThreads(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("z7STeF6abuU", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Mtxinit);
    LIB_FUNCTION("pE4Ot3CffW0", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Mtxlock);
    LIB_FUNCTION("cMwgSSmpE5o", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Mtxunlock);
    LIB_FUNCTION("LaPaA6mYA38", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Mtxdst);
}

} // namespace Libraries::LibcInternal