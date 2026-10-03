// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Voice {

// Retail hardware rejects any port id above this (confirmed via disassembly).
constexpr u32 ORBIS_VOICE_MAX_PORT = 64;

struct OrbisVoicePortInfo {
    s32 port_type;
    s32 state;
    u32* edge;
    u32 byte_count;
    u32 frame_size;
    u16 edge_count;
    u16 reserved;
};

// Size confirmed via disassembly; field layout beyond `type` is best-effort.
struct OrbisVoicePortParam {
    s32 type;
    s32 reserved0;
    u64 reserved1;
    u64 reserved2;
};
static_assert(sizeof(OrbisVoicePortParam) == 0x18);

// Size confirmed via disassembly; field layout beyond `app_type` is best-effort.
struct OrbisVoiceInitParam {
    u64 app_type;
    u64 reserved0;
    u64 reserved1;
    u64 reserved2;
    u64 reserved3;
};
static_assert(sizeof(OrbisVoiceInitParam) == 0x28);

// Size confirmed via disassembly; `container` must be non-zero.
struct OrbisVoiceStartParam {
    u64 container;
    u64 reserved0;
    u64 reserved1;
    u64 reserved2;
};
static_assert(sizeof(OrbisVoiceStartParam) == 0x20);

// Required `size` per id for sceVoiceGetPortAttr, confirmed via disassembly. Meaning of each id
// is unknown, so they're named by id.
enum OrbisVoicePortAttr : s32 {
    ORBIS_VOICE_ATTR_1000 = 1000, // size = 1
    ORBIS_VOICE_ATTR_1001 = 1001, // size = 1
    ORBIS_VOICE_ATTR_1002 = 1002, // size = 4
    ORBIS_VOICE_ATTR_1003 = 1003, // size = 2
    ORBIS_VOICE_ATTR_2000 = 2000, // size = 1
    ORBIS_VOICE_ATTR_2001 = 2001, // size = 4
    ORBIS_VOICE_ATTR_2002 = 2002, // size = 1
};

// Layout (4 u16 fields, in this order) confirmed via disassembly; field meaning is not.
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
s32 PS4_SYSV_ABI sceVoiceSetThreadsParams(void* params);
s32 PS4_SYSV_ABI sceVoiceSetVolume(u32 port_id, float volume);
s32 PS4_SYSV_ABI sceVoiceStart(const OrbisVoiceStartParam* param);
s32 PS4_SYSV_ABI sceVoiceStop();
s32 PS4_SYSV_ABI sceVoiceUpdatePort(u32 port_id, const OrbisVoicePortParam* param);
s32 PS4_SYSV_ABI sceVoiceVADAdjustment(float param1, float param2);
s32 PS4_SYSV_ABI sceVoiceVADSetVersion(s32 version);
// `unk`: confirmed present in the real signature; its meaning is not.
s32 PS4_SYSV_ABI sceVoiceWriteToIPort(u32 port_id, const void* data, u32* size, s16 unk);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Voice
