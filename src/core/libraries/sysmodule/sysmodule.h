// SPDX-FileCopyrightText: Copyright 2025-2026 shadBloodborne Emulator Project
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

s32 PS4_SYSV_ABI sceSysmoduleGetModuleHandleInternal(OrbisSysModuleInternal id, s32* handle);
s32 PS4_SYSV_ABI sceSysmoduleGetModuleInfoForUnwind(VAddr addr, s32 flags,
                                                    Kernel::OrbisModuleInfoForUnwind* info);
s32 PS4_SYSV_ABI sceSysmoduleIsCalledFromSysModule();
s32 PS4_SYSV_ABI sceSysmoduleIsCameraPreloaded();
s32 PS4_SYSV_ABI sceSysmoduleIsLoaded(OrbisSysModule id);
s32 PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(OrbisSysModuleInternal id);
s32 PS4_SYSV_ABI sceSysmoduleLoadModule(OrbisSysModule id);
s32 PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal();
s32 PS4_SYSV_ABI sceSysmoduleLoadModuleInternal(OrbisSysModuleInternal id);
s32 PS4_SYSV_ABI sceSysmoduleLoadModuleInternalWithArg(OrbisSysModuleInternal id, s32 argc,
                                                       const void* argv, u64 unk, s32* res_out);
s32 PS4_SYSV_ABI sceSysmoduleMapLibcForLibkernel();
s32 PS4_SYSV_ABI sceSysmodulePreloadModuleForLibkernel();
s32 PS4_SYSV_ABI sceSysmoduleUnloadModule(OrbisSysModule id);
s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleByNameInternal();
s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleInternal();
s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleInternalWithArg();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SysModule
