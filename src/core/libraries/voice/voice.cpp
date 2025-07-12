// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/voice/voice.h"

namespace Libraries::Voice {

s32 PS4_SYSV_ABI sceVoiceConnectIPortToOPort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceCreatePort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDeletePort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceDisconnectIPortFromOPort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceEnd() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetBitRate(u32 port_id, u32* bitrate) {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    *bitrate = 48000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetMuteFlag() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortAttr() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetPortInfo(u32 port_id, OrbisVoicePortInfo* info) {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    info->port_type = 0;
    info->state = 3;
    info->byte_count = 0;
    info->frame_size = 1;
    info->edge_count = 0;
    info->reserved = 0;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetResourceInfo() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceGetVolume() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceInit() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceInitHQ() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoicePausePort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoicePausePortAll() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceReadFromOPort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResetPort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceResumePortAll() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetBitRate() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlag() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetMuteFlagAll() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetThreadsParams() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceSetVolume() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStart() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceStop() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceUpdatePort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADAdjustment() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceVADSetVersion() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVoiceWriteToIPort() {
    LOG_ERROR(Lib_Voice, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oV9GAdJ23Gw", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceConnectIPortToOPort);
    LIB_FUNCTION("nXpje5yNpaE", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceCreatePort);
    LIB_FUNCTION("b7kJI+nx2hg", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceDeletePort);
    LIB_FUNCTION("ajVj3QG2um4", "libSceVoice", 1, "libSceVoice", 0, 0,
                 sceVoiceDisconnectIPortFromOPort);
    LIB_FUNCTION("Oo0S5PH7FIQ", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceEnd);
    LIB_FUNCTION("cJLufzou6bc", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetBitRate);
    LIB_FUNCTION("Pc4z1QjForU", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetMuteFlag);
    LIB_FUNCTION("elcxZTEfHZM", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetPortAttr);
    LIB_FUNCTION("CrLqDwWLoXM", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetPortInfo);
    LIB_FUNCTION("Z6QV6j7igvE", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetResourceInfo);
    LIB_FUNCTION("jjkCjneOYSs", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceGetVolume);
    LIB_FUNCTION("9TrhuGzberQ", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceInit);
    LIB_FUNCTION("IPHvnM5+g04", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceInitHQ);
    LIB_FUNCTION("x0slGBQW+wY", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoicePausePort);
    LIB_FUNCTION("Dinob0yMRl8", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoicePausePortAll);
    LIB_FUNCTION("cQ6DGsQEjV4", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceReadFromOPort);
    LIB_FUNCTION("udAxvCePkUs", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceResetPort);
    LIB_FUNCTION("gAgN+HkiEzY", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceResumePort);
    LIB_FUNCTION("jbkJFmOZ9U0", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceResumePortAll);
    LIB_FUNCTION("TexwmOHQsDg", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceSetBitRate);
    LIB_FUNCTION("gwUynkEgNFY", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceSetMuteFlag);
    LIB_FUNCTION("oUha0S-Ij9Q", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceSetMuteFlagAll);
    LIB_FUNCTION("clyKUyi3RYU", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceSetThreadsParams);
    LIB_FUNCTION("QBFoAIjJoXQ", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceSetVolume);
    LIB_FUNCTION("54phPH2LZls", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceStart);
    LIB_FUNCTION("Ao2YNSA7-Qo", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceStop);
    LIB_FUNCTION("jSZNP7xJrcw", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceUpdatePort);
    LIB_FUNCTION("hg9T73LlRiU", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceVADAdjustment);
    LIB_FUNCTION("wFeAxEeEi-8", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceVADSetVersion);
    LIB_FUNCTION("YeJl6yDlhW0", "libSceVoice", 1, "libSceVoice", 0, 0, sceVoiceWriteToIPort);
};

} // namespace Libraries::Voice