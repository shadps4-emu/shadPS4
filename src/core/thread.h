// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core {

class Thread {
public:
    using ThreadFunc = void (*)(void*);
    using PthreadFunc = void* (*)(void*);

    Thread();
    ~Thread();

    int Create(ThreadFunc func, void* arg);
    void Exit();

    uintptr_t GetHandle() {
        return reinterpret_cast<uintptr_t>(native_handle);
    }

private:
#if _WIN64
    void* native_handle;
#else
    uintptr_t native_handle;
#endif
};

} // namespace Core