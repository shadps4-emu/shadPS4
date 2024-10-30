// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {

class Thread {
public:
    using ThreadFunc = void (*)(void*);
    using PthreadFunc = void* (*)(void*);

    Thread();
    ~Thread();

    int Create(ThreadFunc func, void* arg);
    void Exit();

    void* GetHandle() {
        return native_handle;
    }

private:
    void* native_handle;
};

} // namespace Core