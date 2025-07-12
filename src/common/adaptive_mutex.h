// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef __linux__
#include <pthread.h>
#endif

namespace Common {

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
class AdaptiveMutex {
public:
    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }
    [[nodiscard]] bool try_lock() {
        return pthread_mutex_trylock(&mutex) == 0;
    }

private:
    pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
};
#endif // PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP

} // namespace Common
