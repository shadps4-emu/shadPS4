// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"
#include "playgo_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::PlayGo {
constexpr int shadMagic = 0x53484144;

int PS4_SYSV_ABI sceDbgPlayGoRequestNextChunk();
int PS4_SYSV_ABI sceDbgPlayGoSnapshot();
int PS4_SYSV_ABI scePlayGoClose();
int PS4_SYSV_ABI scePlayGoGetChunkId();
int PS4_SYSV_ABI scePlayGoGetEta();
int PS4_SYSV_ABI scePlayGoGetInstallSpeed();
int PS4_SYSV_ABI scePlayGoGetLanguageMask();
int PS4_SYSV_ABI scePlayGoGetLocus();
int PS4_SYSV_ABI scePlayGoGetProgress();
int PS4_SYSV_ABI scePlayGoGetToDoList(OrbisPlayGoHandle handle, OrbisPlayGoToDo* outTodoList,
                                      u32 numberOfEntries, u32* outEntries);
int PS4_SYSV_ABI scePlayGoInitialize(OrbisPlayGoInitParams* param);
int PS4_SYSV_ABI scePlayGoOpen(OrbisPlayGoHandle* outHandle, const void* param);
int PS4_SYSV_ABI scePlayGoPrefetch();
int PS4_SYSV_ABI scePlayGoSetInstallSpeed();
int PS4_SYSV_ABI scePlayGoSetLanguageMask();
int PS4_SYSV_ABI scePlayGoSetToDoList();
int PS4_SYSV_ABI scePlayGoTerminate();

void RegisterlibScePlayGo(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::PlayGo