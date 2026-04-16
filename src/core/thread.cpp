// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/threads/pthread.h"
#include "thread.h"

#ifdef _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Core {

NativeThread::NativeThread() : native_handle{0} {}

NativeThread::~NativeThread() {}

int NativeThread::Create(ThreadFunc func, void* arg, const ::Libraries::Kernel::PthreadAttr* attr) {
#ifdef _WIN64
    native_handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, nullptr);
    return native_handle ? 0 : -1;
#else
    pthread_t* pthr = reinterpret_cast<pthread_t*>(&native_handle);
    pthread_attr_t pattr;
    pthread_attr_init(&pattr);
    pthread_attr_setstack(&pattr, attr->stackaddr_attr, attr->stacksize_attr);
    return pthread_create(pthr, &pattr, (PthreadFunc)func, arg);
#endif
}

void NativeThread::Exit() {
    if (!native_handle) {
        return;
    }

#ifdef _WIN64
    CloseHandle(native_handle);
    native_handle = nullptr;

    // We call this assuming the thread has finished execution.
    ExitThread(0);
#else
    pthread_exit(nullptr);
#endif
}

void NativeThread::Initialize() {}

} // namespace Core
