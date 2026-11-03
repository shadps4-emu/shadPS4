// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Voice {

constexpr u32 ORBIS_VOICE_MAX_PORT = 64;

enum OrbisVoicePortType : s32 {
    ORBIS_VOICE_PORTTYPE_IN_DEVICE = 0,
    ORBIS_VOICE_PORTTYPE_IN_PCMAUDIO = 1,
    ORBIS_VOICE_PORTTYPE_IN_VOICE = 2,
    ORBIS_VOICE_PORTTYPE_OUT_PCMAUDIO = 3,
    ORBIS_VOICE_PORTTYPE_OUT_VOICE = 4,
    ORBIS_VOICE_PORTTYPE_OUT_DEVICE = 5,
};

enum OrbisVoicePortAttr : s32 {
    ORBIS_VOICE_ATTR_1000 = 1000,
    ORBIS_VOICE_ATTR_1001 = 1001,
    ORBIS_VOICE_ATTR_1002 = 1002,
    ORBIS_VOICE_ATTR_1003 = 1003,
    ORBIS_VOICE_ATTR_2000 = 2000,
    ORBIS_VOICE_ATTR_2001 = 2001,
    ORBIS_VOICE_ATTR_2002 = 2002,
};

struct OrbisVoicePortInfo {
    s32 port_type;
    s32 state;
    u32* edge;
    u32 byte_count;
    u32 frame_size;
    u16 edge_count;
    u16 reserved;
};

struct OrbisVoicePortParam {
    s32 type;
    s32 reserved0;
    u64 reserved1;
    u64 reserved2;
};
static_assert(sizeof(OrbisVoicePortParam) == 0x18);

struct OrbisVoiceInitParam {
    u64 app_type;
    u64 reserved0;
    u64 reserved1;
    u64 reserved2;
    u64 reserved3;
};
static_assert(sizeof(OrbisVoiceInitParam) == 0x28);

struct OrbisVoiceStartParam {
    void* memory;
    u32 memory_size;
    u32 reserved0;
    u64 reserved1;
    u64 reserved2;
};
static_assert(sizeof(OrbisVoiceStartParam) == 0x20);

struct OrbisVoiceResourceInfo {
    u16 field_0;
    u16 field_2;
    u16 field_4;
    u16 field_6;
};
static_assert(sizeof(OrbisVoiceResourceInfo) == 8);

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort(u32 in_port_id, u32 out_port_id);
s32 PS4_SYSV_ABI sceVoiceCreatePort(s32* port_id, const OrbisVoicePortParam* param);
s32 PS4_SYSV_ABI sceVoiceDeletePort(u32 port_id);
s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort(u32 in_port_id, u32 out_port_id);
s32 PS4_SYSV_ABI sceVoiceEnd();
s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate);
s32 PS4_SYSV_ABI sceVoiceGetMuteFlag(u32 port_id, bool* mute);
s32 PS4_SYSV_ABI sceVoiceGetPortAttr(u32 port_id, s32 attr_id, void* value, u32 size);
s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info);
s32 PS4_SYSV_ABI sceVoiceGetResourceInfo(OrbisVoiceResourceInfo* info);
s32 PS4_SYSV_ABI sceVoiceGetVolume(u32 port_id, float* volume);
s32 PS4_SYSV_ABI sceVoiceInit(const OrbisVoiceInitParam* param, s32 container);
s32 PS4_SYSV_ABI sceVoiceInitHQ(const OrbisVoiceInitParam* param, s32 container);
s32 PS4_SYSV_ABI sceVoicePausePort(u32 port_id);
s32 PS4_SYSV_ABI sceVoicePausePortAll();
s32 PS4_SYSV_ABI sceVoiceReadFromOPort(u32 port_id, void* data, u32* size);
s32 PS4_SYSV_ABI sceVoiceResetPort(u32 port_id);
s32 PS4_SYSV_ABI sceVoiceResumePort(u32 port_id);
s32 PS4_SYSV_ABI sceVoiceResumePortAll();
s32 PS4_SYSV_ABI sceVoiceSetBitRate(u32 port_id, u32 bitrate);
s32 PS4_SYSV_ABI sceVoiceSetMuteFlag(u32 port_id, bool mute);
s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll(bool mute);
s32 PS4_SYSV_ABI sceVoiceSetThreadsParams();
s32 PS4_SYSV_ABI sceVoiceSetVolume(u32 port_id, float volume);
s32 PS4_SYSV_ABI sceVoiceStart(const OrbisVoiceStartParam* param);
s32 PS4_SYSV_ABI sceVoiceStop();
s32 PS4_SYSV_ABI sceVoiceUpdatePort(u32 port_id, const OrbisVoicePortParam* param);
s32 PS4_SYSV_ABI sceVoiceVADAdjustment(float param1, float param2);
s32 PS4_SYSV_ABI sceVoiceVADSetVersion(s32 version);
s32 PS4_SYSV_ABI sceVoiceWriteToIPort(u32 port_id, const void* data, u32* size, s16 unk);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Voice
