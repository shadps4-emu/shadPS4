// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Voice {

struct OrbisVoicePortInfo {
    s32 port_type;
    s32 state;
    u32* edge;
    u32 byte_count;
    u32 frame_size;
    u16 edge_count;
    u16 reserved;
};

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort();
s32 PS4_SYSV_ABI sceVoiceCreatePort();
s32 PS4_SYSV_ABI sceVoiceDeletePort();
s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort();
s32 PS4_SYSV_ABI sceVoiceEnd();
s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate);
s32 PS4_SYSV_ABI sceVoiceGetMuteFlag();
s32 PS4_SYSV_ABI sceVoiceGetPortAttr();
s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info);
s32 PS4_SYSV_ABI sceVoiceGetResourceInfo();
s32 PS4_SYSV_ABI sceVoiceGetVolume();
s32 PS4_SYSV_ABI sceVoiceInit();
s32 PS4_SYSV_ABI sceVoiceInitHQ();
s32 PS4_SYSV_ABI sceVoicePausePort();
s32 PS4_SYSV_ABI sceVoicePausePortAll();
s32 PS4_SYSV_ABI sceVoiceReadFromOPort();
s32 PS4_SYSV_ABI sceVoiceResetPort();
s32 PS4_SYSV_ABI sceVoiceResumePort();
s32 PS4_SYSV_ABI sceVoiceResumePortAll();
s32 PS4_SYSV_ABI sceVoiceSetBitRate();
s32 PS4_SYSV_ABI sceVoiceSetMuteFlag();
s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll();
s32 PS4_SYSV_ABI sceVoiceSetThreadsParams();
s32 PS4_SYSV_ABI sceVoiceSetVolume();
s32 PS4_SYSV_ABI sceVoiceStart();
s32 PS4_SYSV_ABI sceVoiceStop();
s32 PS4_SYSV_ABI sceVoiceUpdatePort();
s32 PS4_SYSV_ABI sceVoiceVADAdjustment();
s32 PS4_SYSV_ABI sceVoiceVADSetVersion();
s32 PS4_SYSV_ABI sceVoiceWriteToIPort();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Voice