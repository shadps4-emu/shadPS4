// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/process.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SysModule {

using OrbisSysModule = u16;
using OrbisSysModuleInternal = u32;

int PS4_SYSV_ABI sceSysmoduleGetModuleHandleInternal();
s32 PS4_SYSV_ABI sceSysmoduleGetModuleInfoForUnwind(VAddr addr, s32 flags,
                                                    Kernel::OrbisModuleInfoForUnwind* info);
int PS4_SYSV_ABI sceSysmoduleIsCalledFromSysModule();
int PS4_SYSV_ABI sceSysmoduleIsCameraPreloaded();
int PS4_SYSV_ABI sceSysmoduleIsLoaded(OrbisSysModule id);
int PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(OrbisSysModuleInternal id);
int PS4_SYSV_ABI sceSysmoduleLoadModule(OrbisSysModule id);
int PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal();
int PS4_SYSV_ABI sceSysmoduleLoadModuleInternal();
int PS4_SYSV_ABI sceSysmoduleLoadModuleInternalWithArg();
int PS4_SYSV_ABI sceSysmoduleMapLibcForLibkernel();
int PS4_SYSV_ABI sceSysmodulePreloadModuleForLibkernel();
int PS4_SYSV_ABI sceSysmoduleUnloadModule();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleByNameInternal();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternal();
int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternalWithArg();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SysModule
