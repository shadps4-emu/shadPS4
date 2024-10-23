// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/kernel/threads/threads.h"

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
}

} // namespace Libraries::Kernel
