// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static constexpr u32 PthreadKeysMax = 256;
static constexpr u32 PthreadDestructorIterations = 4;

static std::array<PthreadKey, PthreadKeysMax> ThreadKeytable{};
static std::mutex KeytableLock;

int PS4_SYSV_ABI posix_pthread_key_create(PthreadKeyT* key, PthreadKeyDestructor destructor) {
    std::scoped_lock lk{KeytableLock};
    const auto it = std::ranges::find(ThreadKeytable, 0, &PthreadKey::allocated);
    if (it != ThreadKeytable.end()) {
        it->allocated = 1;
        it->destructor = destructor;
        it->seqno++;
        *key = std::distance(ThreadKeytable.begin(), it);
        return 0;
    }
    return POSIX_EAGAIN;
}

int PS4_SYSV_ABI posix_pthread_key_delete(PthreadKeyT key) {
    if (key >= PthreadKeysMax) {
        return POSIX_EINVAL;
    }

    std::scoped_lock lk{KeytableLock};
    if (!ThreadKeytable[key].allocated) {
        return POSIX_EINVAL;
    }

    ThreadKeytable[key].allocated = 0;
    return 0;
}

void _thread_cleanupspecific() {
    Pthread* curthread = g_curthread;
    PthreadKeyDestructor destructor;
    const void* data = NULL;

    if (curthread->specific == nullptr) {
        return;
    }

    std::unique_lock lk{KeytableLock};
    for (int i = 0; (i < PthreadDestructorIterations) && (curthread->specific_data_count > 0);
         i++) {
        for (int key = 0; (key < PthreadKeysMax) && (curthread->specific_data_count > 0); key++) {
            destructor = nullptr;

            if (ThreadKeytable[key].allocated && (curthread->specific[key].data != nullptr)) {
                if (curthread->specific[key].seqno == ThreadKeytable[key].seqno) {
                    data = curthread->specific[key].data;
                    destructor = ThreadKeytable[key].destructor;
                }
                curthread->specific[key].data = nullptr;
                curthread->specific_data_count--;
            } else if (curthread->specific[key].data != NULL) {
                /*
                 * This can happen if the key is deleted via
                 * pthread_key_delete without first setting the value
                 * to NULL in all threads.  POSIX says that the
                 * destructor is not invoked in this case.
                 */
                curthread->specific[key].data = nullptr;
                curthread->specific_data_count--;
            }

            /*
             * If there is a destructor, call it
             * with the key table entry unlocked:
             */
            if (destructor != nullptr) {
                /*
                 * Don't hold the lock while calling the
                 * destructor:
                 */
                lk.unlock();
                Core::ExecuteGuest(destructor, data);
                lk.lock();
            }
        }
    }
    delete[] curthread->specific;
    curthread->specific = nullptr;
    if (curthread->specific_data_count > 0) {
        LOG_WARNING(Lib_Kernel, "Thread has exited with leftover thread-specific data");
    }
}

int PS4_SYSV_ABI posix_pthread_setspecific(PthreadKeyT key, const void* value) {
    int ret = 0;
    Pthread* pthread = g_curthread;

    if (!pthread->specific) {
        pthread->specific = new PthreadSpecificElem[PthreadKeysMax]{};
        if (!pthread->specific) {
            return POSIX_ENOMEM;
        }
    }
    if (key >= PthreadKeysMax) {
        return POSIX_EINVAL;
    }
    if (!ThreadKeytable[key].allocated) {
        return POSIX_EINVAL;
    }

    if (pthread->specific[key].data == nullptr) {
        if (value != nullptr) {
            pthread->specific_data_count++;
        }
    } else if (value == nullptr) {
        pthread->specific_data_count--;
    }
    pthread->specific[key].data = value;
    pthread->specific[key].seqno = ThreadKeytable[key].seqno;
    return 0;
}

const void* PS4_SYSV_ABI posix_pthread_getspecific(PthreadKeyT key) {
    Pthread* pthread = g_curthread;

    if (!pthread->specific || key >= PthreadKeysMax) {
        return nullptr;
    }

    if (ThreadKeytable[key].allocated &&
        (pthread->specific[key].seqno == ThreadKeytable[key].seqno)) {
        return pthread->specific[key].data;
    }

    return nullptr;
}

void RegisterSpec(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("mqULNdimTn0", "libScePosix", 1, "libkernel", posix_pthread_key_create);
    LIB_FUNCTION("6BpEZuDT7YI", "libScePosix", 1, "libkernel", posix_pthread_key_delete);
    LIB_FUNCTION("0-KXaS70xy4", "libScePosix", 1, "libkernel", posix_pthread_getspecific);
    LIB_FUNCTION("WrOLvHU0yQM", "libScePosix", 1, "libkernel", posix_pthread_setspecific);

    // Posix-Kernel
    LIB_FUNCTION("mqULNdimTn0", "libkernel", 1, "libkernel", posix_pthread_key_create);
    LIB_FUNCTION("0-KXaS70xy4", "libkernel", 1, "libkernel", posix_pthread_getspecific);
    LIB_FUNCTION("WrOLvHU0yQM", "libkernel", 1, "libkernel", posix_pthread_setspecific);

    // Orbis
    LIB_FUNCTION("geDaqgH9lTg", "libkernel", 1, "libkernel", ORBIS(posix_pthread_key_create));
    LIB_FUNCTION("PrdHuuDekhY", "libkernel", 1, "libkernel", ORBIS(posix_pthread_key_delete));
    LIB_FUNCTION("eoht7mQOCmo", "libkernel", 1, "libkernel", posix_pthread_getspecific);
    LIB_FUNCTION("+BzXYkqYeLE", "libkernel", 1, "libkernel", ORBIS(posix_pthread_setspecific));
}

} // namespace Libraries::Kernel
