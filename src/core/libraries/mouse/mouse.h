// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Mouse {

int PS4_SYSV_ABI sceMouseClose();
int PS4_SYSV_ABI sceMouseConnectPort();
int PS4_SYSV_ABI sceMouseDebugGetDeviceId();
int PS4_SYSV_ABI sceMouseDeviceOpen();
int PS4_SYSV_ABI sceMouseDisconnectDevice();
int PS4_SYSV_ABI sceMouseDisconnectPort();
int PS4_SYSV_ABI sceMouseGetDeviceInfo();
int PS4_SYSV_ABI sceMouseInit();
int PS4_SYSV_ABI sceMouseMbusInit();
int PS4_SYSV_ABI sceMouseOpen();
int PS4_SYSV_ABI sceMouseRead();
int PS4_SYSV_ABI sceMouseSetHandType();
int PS4_SYSV_ABI sceMouseSetPointerSpeed();
int PS4_SYSV_ABI sceMouseSetProcessPrivilege();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Mouse