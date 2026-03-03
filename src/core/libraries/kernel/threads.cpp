// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Libraries::Kernel {

void PS4_SYSV_ABI ClearStack() {
    void* const stackaddr_attr = Libraries::Kernel::g_curthread->attr.stackaddr_attr;
    void* volatile sp;
    asm("mov %%rsp, %0" : "=rm"(sp));
    // leave a safety net of 64 bytes for memset
    const size_t size = ((uintptr_t)sp - (uintptr_t)stackaddr_attr) - 64;
    void* volatile buf = alloca(size);
    memset(buf, 0, size);
    sp = nullptr;
}

void RegisterThreads(Core::Loader::SymbolsResolver* sym) {
    RegisterMutex(sym);
    RegisterCond(sym);
    RegisterRwlock(sym);
    RegisterSemaphore(sym);
    RegisterSpec(sym);
    RegisterThreadAttr(sym);
    RegisterThread(sym);
    RegisterRtld(sym);
    RegisterPthreadClean(sym);
}

} // namespace Libraries::Kernel
