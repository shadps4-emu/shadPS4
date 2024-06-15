// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/thread_management.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

int PS4_SYSV_ABI scePthreadRwlockattrInit(OrbisPthreadRwlockattr* attr);

void SemaphoreSymbolsRegister(Core::Loader::SymbolsResolver* sym);
void RwlockSymbolsRegister(Core::Loader::SymbolsResolver* sym);
void KeySymbolsRegister(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
