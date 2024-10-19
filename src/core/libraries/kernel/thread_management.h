// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/threads/threads.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

PthreadT LaunchThread(PthreadEntryFunc start_routine, void* arg, const char* name);

void RegisterThreads(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
