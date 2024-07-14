// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/assert.h"
#include "common/types.h"
#include "core/tls.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace Core {

#ifdef _WIN32

static DWORD slot = 0;

static void AllocTcbKey() {
    slot = TlsAlloc();
}

void SetTcbBase(void* image_address) {
    const BOOL result = TlsSetValue(GetTcbKey(), image_address);
    ASSERT(result != 0);
}

Tcb* GetTcbBase() {
    return reinterpret_cast<Tcb*>(TlsGetValue(GetTcbKey()));
}

#elif defined(__APPLE__)

static pthread_key_t slot = 0;

static void AllocTcbKey() {
    ASSERT(pthread_key_create(&slot, nullptr) == 0);
}

void SetTcbBase(void* image_address) {
    ASSERT(pthread_setspecific(GetTcbKey(), image_address) == 0);
}

Tcb* GetTcbBase() {
    return reinterpret_cast<Tcb*>(pthread_getspecific(GetTcbKey()));
}

#else

// Placeholder for code compatibility.
static constexpr u32 slot = 0;

static void AllocTcbKey() {}

void SetTcbBase(void* image_address) {
    asm volatile("wrgsbase %0" ::"r"(image_address) : "memory");
}

Tcb* GetTcbBase() {
    Tcb* tcb;
    asm volatile("rdgsbase %0" : "=r"(tcb)::"memory");
    return tcb;
}

#endif

static std::once_flag slot_alloc_flag;

u32 GetTcbKey() {
    std::call_once(slot_alloc_flag, &AllocTcbKey);
    return slot;
}

} // namespace Core
