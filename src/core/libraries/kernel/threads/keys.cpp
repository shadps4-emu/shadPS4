// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI scePthreadKeyCreate(OrbisPthreadKey* key, PthreadKeyDestructor destructor) {
    if (key == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    pthread_key_t thread_key;
    int result = pthread_key_create(&thread_key, nullptr);
    *key = static_cast<OrbisPthreadKey>(thread_key);

    if (destructor) {
        auto thread = scePthreadSelf();
        thread->key_destructors.emplace_back(*key, destructor);
    }

    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadKeyCreate: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

void* PS4_SYSV_ABI scePthreadGetspecific(OrbisPthreadKey key) {
    return pthread_getspecific(key);
}

int PS4_SYSV_ABI scePthreadSetspecific(OrbisPthreadKey key, /* const*/ void* value) {
    int result = pthread_setspecific(key, value);
    if (result != 0) {
        LOG_ERROR(Kernel_Pthread, "scePthreadSetspecific: error = {}", result);
        result += ORBIS_KERNEL_ERROR_UNKNOWN;
    }
    return result;
}

void KeySymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("geDaqgH9lTg", "libkernel", 1, "libkernel", 1, 1, scePthreadKeyCreate);
    LIB_FUNCTION("eoht7mQOCmo", "libkernel", 1, "libkernel", 1, 1, scePthreadGetspecific);
    LIB_FUNCTION("+BzXYkqYeLE", "libkernel", 1, "libkernel", 1, 1, scePthreadSetspecific);
}

} // namespace Libraries::Kernel
