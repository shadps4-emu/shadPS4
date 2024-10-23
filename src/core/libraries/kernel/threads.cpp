// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Libraries::Kernel {

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
