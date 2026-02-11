// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"
#include "core/libraries/kernel/threads.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal__Mtxinit(Libraries::Kernel::PthreadMutexT* mtx, const char* name);
s32 PS4_SYSV_ABI internal__Mtxlock(Libraries::Kernel::PthreadMutexT* mtx);
s32 PS4_SYSV_ABI internal__Mtxunlock(Libraries::Kernel::PthreadMutexT* mtx);
s32 PS4_SYSV_ABI internal__Mtxdst(Libraries::Kernel::PthreadMutexT* mtx);

void RegisterlibSceLibcInternalThreads(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::LibcInternal