// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Mouse {

struct OrbisMouseOpenParam {
    u8 behaviorFlag;
    u8 reserve[7];
};

struct OrbisMouseData {
    u64 timestamp{};
    bool connected{};
    u32 buttons{};
    s32 xAxis{};
    s32 yAxis{};
    s32 wheel{};
    s32 tilt{};
    std::array<u8, 8> reserve{};
};

enum OrbisMouseButtonDataOffset {
    ORBIS_MOUSE_BUTTON_PRIMARY = 0x00000001,
    ORBIS_MOUSE_BUTTON_SECONDARY = 0x00000002,
    ORBIS_MOUSE_BUTTON_OPTIONAL = 0x00000004,
    ORBIS_MOUSE_BUTTON_OPTIONAL2 = 0x00000008,
    ORBIS_MOUSE_BUTTON_OPTIONAL3 = 0x00000010,
};

int PS4_SYSV_ABI sceMouseClose(s32 handle);
int PS4_SYSV_ABI sceMouseConnectPort();
int PS4_SYSV_ABI sceMouseDebugGetDeviceId();
int PS4_SYSV_ABI sceMouseDeviceOpen();
int PS4_SYSV_ABI sceMouseDisconnectDevice();
int PS4_SYSV_ABI sceMouseDisconnectPort();
int PS4_SYSV_ABI sceMouseGetDeviceInfo();
int PS4_SYSV_ABI sceMouseInit();
int PS4_SYSV_ABI sceMouseMbusInit();
int PS4_SYSV_ABI sceMouseOpen(s32 userId, s32 type, s32 index, OrbisMouseOpenParam* pParam);
int PS4_SYSV_ABI sceMouseRead(s32 handle, OrbisMouseData* pData, s32 num);
int PS4_SYSV_ABI sceMouseSetHandType();
int PS4_SYSV_ABI sceMouseSetPointerSpeed();
int PS4_SYSV_ABI sceMouseSetProcessPrivilege();

void RegisterlibSceMouse(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Mouse