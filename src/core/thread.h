// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Kernel {
struct PthreadAttr;
} // namespace Libraries::Kernel

namespace Core {

using ThreadFunc = void (*)(void*);
using PthreadFunc = void* (*)(void*);

class NativeThread {
public:
    NativeThread();
    ~NativeThread();

    int Create(ThreadFunc func, void* arg);
    void Exit();

    void Initialize();

    uintptr_t GetHandle() {
        return reinterpret_cast<uintptr_t>(native_handle);
    }

    u64 GetTid() {
        return tid;
    }

private:
#ifdef _WIN64
    void* native_handle;
#else
    uintptr_t native_handle;
    void* sig_stack_ptr = nullptr;
#endif
    void* init_stack_ptr = nullptr;
    u64 tid;
};

} // namespace Core