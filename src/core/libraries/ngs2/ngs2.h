// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ngs2 {

int PS4_SYSV_ABI sceNgs2CalcWaveformBlock();
int PS4_SYSV_ABI sceNgs2CustomRackGetModuleInfo();
int PS4_SYSV_ABI sceNgs2FftInit();
int PS4_SYSV_ABI sceNgs2FftProcess();
int PS4_SYSV_ABI sceNgs2FftQuerySize();
int PS4_SYSV_ABI sceNgs2GeomApply();
int PS4_SYSV_ABI sceNgs2GeomCalcListener();
int PS4_SYSV_ABI sceNgs2GeomResetListenerParam();
int PS4_SYSV_ABI sceNgs2GeomResetSourceParam();
int PS4_SYSV_ABI sceNgs2GetWaveformFrameInfo();
int PS4_SYSV_ABI sceNgs2JobSchedulerResetOption();
int PS4_SYSV_ABI sceNgs2ModuleArrayEnumItems();
int PS4_SYSV_ABI sceNgs2ModuleEnumConfigs();
int PS4_SYSV_ABI sceNgs2ModuleQueueEnumItems();
int PS4_SYSV_ABI sceNgs2PanGetVolumeMatrix();
int PS4_SYSV_ABI sceNgs2PanInit();
int PS4_SYSV_ABI sceNgs2ParseWaveformData();
int PS4_SYSV_ABI sceNgs2ParseWaveformFile();
int PS4_SYSV_ABI sceNgs2ParseWaveformUser();
int PS4_SYSV_ABI sceNgs2RackCreate();
int PS4_SYSV_ABI sceNgs2RackCreateWithAllocator();
int PS4_SYSV_ABI sceNgs2RackDestroy();
int PS4_SYSV_ABI sceNgs2RackGetInfo();
int PS4_SYSV_ABI sceNgs2RackGetUserData();
int PS4_SYSV_ABI sceNgs2RackGetVoiceHandle();
int PS4_SYSV_ABI sceNgs2RackLock();
int PS4_SYSV_ABI sceNgs2RackQueryBufferSize();
int PS4_SYSV_ABI sceNgs2RackQueryInfo();
int PS4_SYSV_ABI sceNgs2RackRunCommands();
int PS4_SYSV_ABI sceNgs2RackSetUserData();
int PS4_SYSV_ABI sceNgs2RackUnlock();
int PS4_SYSV_ABI sceNgs2ReportRegisterHandler();
int PS4_SYSV_ABI sceNgs2ReportUnregisterHandler();
int PS4_SYSV_ABI sceNgs2SystemCreate();
int PS4_SYSV_ABI sceNgs2SystemCreateWithAllocator();
int PS4_SYSV_ABI sceNgs2SystemDestroy();
int PS4_SYSV_ABI sceNgs2SystemEnumHandles();
int PS4_SYSV_ABI sceNgs2SystemEnumRackHandles();
int PS4_SYSV_ABI sceNgs2SystemGetInfo();
int PS4_SYSV_ABI sceNgs2SystemGetUserData();
int PS4_SYSV_ABI sceNgs2SystemLock();
int PS4_SYSV_ABI sceNgs2SystemQueryBufferSize();
int PS4_SYSV_ABI sceNgs2SystemQueryInfo();
int PS4_SYSV_ABI sceNgs2SystemRender();
int PS4_SYSV_ABI sceNgs2SystemResetOption();
int PS4_SYSV_ABI sceNgs2SystemRunCommands();
int PS4_SYSV_ABI sceNgs2SystemSetGrainSamples();
int PS4_SYSV_ABI sceNgs2SystemSetLoudThreshold();
int PS4_SYSV_ABI sceNgs2SystemSetSampleRate();
int PS4_SYSV_ABI sceNgs2SystemSetUserData();
int PS4_SYSV_ABI sceNgs2SystemUnlock();
int PS4_SYSV_ABI sceNgs2StreamCreate();
int PS4_SYSV_ABI sceNgs2StreamCreateWithAllocator();
int PS4_SYSV_ABI sceNgs2StreamDestroy();
int PS4_SYSV_ABI sceNgs2StreamQueryBufferSize();
int PS4_SYSV_ABI sceNgs2StreamQueryInfo();
int PS4_SYSV_ABI sceNgs2StreamResetOption();
int PS4_SYSV_ABI sceNgs2StreamRunCommands();
int PS4_SYSV_ABI sceNgs2VoiceControl();
int PS4_SYSV_ABI sceNgs2VoiceGetMatrixInfo();
int PS4_SYSV_ABI sceNgs2VoiceGetOwner();
int PS4_SYSV_ABI sceNgs2VoiceGetPortInfo();
int PS4_SYSV_ABI sceNgs2VoiceGetState();
int PS4_SYSV_ABI sceNgs2VoiceGetStateFlags();
int PS4_SYSV_ABI sceNgs2VoiceQueryInfo();
int PS4_SYSV_ABI sceNgs2VoiceRunCommands();

void RegisterlibSceNgs2(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ngs2