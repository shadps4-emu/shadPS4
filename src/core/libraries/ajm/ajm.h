// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ajm {

int PS4_SYSV_ABI sceAjmBatchCancel();
int PS4_SYSV_ABI sceAjmBatchErrorDump();
int PS4_SYSV_ABI sceAjmBatchJobControlBufferRa();
int PS4_SYSV_ABI sceAjmBatchJobInlineBuffer();
int PS4_SYSV_ABI sceAjmBatchJobRunBufferRa();
int PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa();
int PS4_SYSV_ABI sceAjmBatchStartBuffer();
int PS4_SYSV_ABI sceAjmBatchWait();
int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData();
int PS4_SYSV_ABI sceAjmDecMp3ParseFrame();
int PS4_SYSV_ABI sceAjmFinalize();
int PS4_SYSV_ABI sceAjmInitialize();
int PS4_SYSV_ABI sceAjmInstanceCodecType();
int PS4_SYSV_ABI sceAjmInstanceCreate();
int PS4_SYSV_ABI sceAjmInstanceDestroy();
int PS4_SYSV_ABI sceAjmInstanceExtend();
int PS4_SYSV_ABI sceAjmInstanceSwitch();
int PS4_SYSV_ABI sceAjmMemoryRegister();
int PS4_SYSV_ABI sceAjmMemoryUnregister();
int PS4_SYSV_ABI sceAjmModuleRegister();
int PS4_SYSV_ABI sceAjmModuleUnregister();
int PS4_SYSV_ABI sceAjmStrError();

void RegisterlibSceAjm(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ajm