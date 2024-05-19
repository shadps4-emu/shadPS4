// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {
int PS4_SYSV_ABI sceKernelCreateEventFlag();
int PS4_SYSV_ABI sceKernelDeleteEventFlag();
int PS4_SYSV_ABI sceKernelOpenEventFlag();
int PS4_SYSV_ABI sceKernelCloseEventFlag();
int PS4_SYSV_ABI sceKernelClearEventFlag();
int PS4_SYSV_ABI sceKernelCancelEventFlag();
int PS4_SYSV_ABI sceKernelSetEventFlag();
int PS4_SYSV_ABI sceKernelPollEventFlag();
int PS4_SYSV_ABI sceKernelWaitEventFlag();

void RegisterKernelEventFlag(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel