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

s32 PS4_SYSV_ABI sceDbgPlayGoRequestNextChunk();
s32 PS4_SYSV_ABI sceDbgPlayGoSnapshot();
s32 PS4_SYSV_ABI scePlayGoClose();
s32 PS4_SYSV_ABI scePlayGoGetChunkId();
s32 PS4_SYSV_ABI scePlayGoGetEta();
s32 PS4_SYSV_ABI scePlayGoGetInstallSpeed();
s32 PS4_SYSV_ABI scePlayGoGetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask* outLanguageMask);
s32 PS4_SYSV_ABI scePlayGoGetLocus(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                   uint32_t numberOfEntries, OrbisPlayGoLocus* outLoci);
s32 PS4_SYSV_ABI scePlayGoGetProgress(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                      uint32_t numberOfEntries, OrbisPlayGoProgress* outProgress);
s32 PS4_SYSV_ABI scePlayGoGetToDoList(OrbisPlayGoHandle handle, OrbisPlayGoToDo* outTodoList,
                                      u32 numberOfEntries, u32* outEntries);
s32 PS4_SYSV_ABI scePlayGoInitialize(OrbisPlayGoInitParams* param);
s32 PS4_SYSV_ABI scePlayGoOpen(OrbisPlayGoHandle* outHandle, const void* param);
s32 PS4_SYSV_ABI scePlayGoPrefetch();
s32 PS4_SYSV_ABI scePlayGoSetInstallSpeed();
s32 PS4_SYSV_ABI scePlayGoSetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask languageMask);
s32 PS4_SYSV_ABI scePlayGoSetToDoList(OrbisPlayGoHandle handle, const OrbisPlayGoToDo* todoList,
                                      uint32_t numberOfEntries);
s32 PS4_SYSV_ABI scePlayGoTerminate();

void RegisterlibScePlayGo(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::PlayGo