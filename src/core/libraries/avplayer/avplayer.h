// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AvPlayer {

int PS4_SYSV_ABI sceAvPlayerAddSource();
int PS4_SYSV_ABI sceAvPlayerAddSourceEx();
int PS4_SYSV_ABI sceAvPlayerChangeStream();
int PS4_SYSV_ABI sceAvPlayerClose();
int PS4_SYSV_ABI sceAvPlayerCurrentTime();
int PS4_SYSV_ABI sceAvPlayerDisableStream();
int PS4_SYSV_ABI sceAvPlayerEnableStream();
int PS4_SYSV_ABI sceAvPlayerGetAudioData();
int PS4_SYSV_ABI sceAvPlayerGetStreamInfo();
int PS4_SYSV_ABI sceAvPlayerGetVideoData();
int PS4_SYSV_ABI sceAvPlayerGetVideoDataEx();
int PS4_SYSV_ABI sceAvPlayerInit();
int PS4_SYSV_ABI sceAvPlayerInitEx();
int PS4_SYSV_ABI sceAvPlayerIsActive();
int PS4_SYSV_ABI sceAvPlayerJumpToTime();
int PS4_SYSV_ABI sceAvPlayerPause();
int PS4_SYSV_ABI sceAvPlayerPostInit();
int PS4_SYSV_ABI sceAvPlayerPrintf();
int PS4_SYSV_ABI sceAvPlayerResume();
int PS4_SYSV_ABI sceAvPlayerSetAvSyncMode();
int PS4_SYSV_ABI sceAvPlayerSetLogCallback();
int PS4_SYSV_ABI sceAvPlayerSetLooping();
int PS4_SYSV_ABI sceAvPlayerSetTrickSpeed();
int PS4_SYSV_ABI sceAvPlayerStart();
int PS4_SYSV_ABI sceAvPlayerStop();
int PS4_SYSV_ABI sceAvPlayerStreamCount();
int PS4_SYSV_ABI sceAvPlayerVprintf();

void RegisterlibSceAvPlayer(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AvPlayer