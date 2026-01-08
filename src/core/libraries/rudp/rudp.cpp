// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/rudp/rudp.h"

namespace Libraries::Rudp {

s32 PS4_SYSV_ABI sceRudpAccept() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpActivate() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpBind() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpCreateContext() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread2() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnd() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpFlush() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetContextStatus() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetLocalInfo() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetMaxSegmentSize() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetNumberOfPacketsToRead() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetOption() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetRemoteInfo() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetSizeReadable() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetSizeWritable() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetStatus() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpInit() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpInitiate() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpListen() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpNetFlush() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpNetReceived() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollCancel() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollControl() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollCreate() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollDestroy() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollWait() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpProcessEvents() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpRead() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetEventHandler() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetMaxSegmentSize() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetOption() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpTerminate() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpWrite() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uQiK7fjU6y8", "libSceRudp", 1, "libSceRudp", sceRudpAccept);
    LIB_FUNCTION("J-6d0WTjzMc", "libSceRudp", 1, "libSceRudp", sceRudpActivate);
    LIB_FUNCTION("l4SLBpKUDK4", "libSceRudp", 1, "libSceRudp", sceRudpBind);
    LIB_FUNCTION("CAbbX6BuQZ0", "libSceRudp", 1, "libSceRudp", sceRudpCreateContext);
    LIB_FUNCTION("6PBNpsgyaxw", "libSceRudp", 1, "libSceRudp", sceRudpEnableInternalIOThread);
    LIB_FUNCTION("fJ51weR1WAI", "libSceRudp", 1, "libSceRudp", sceRudpEnableInternalIOThread2);
    LIB_FUNCTION("3hBvwqEwqj8", "libSceRudp", 1, "libSceRudp", sceRudpEnd);
    LIB_FUNCTION("Ms0cLK8sTtE", "libSceRudp", 1, "libSceRudp", sceRudpFlush);
    LIB_FUNCTION("wIJsiqY+BMk", "libSceRudp", 1, "libSceRudp", sceRudpGetContextStatus);
    LIB_FUNCTION("2G7-vVz9SIg", "libSceRudp", 1, "libSceRudp", sceRudpGetLocalInfo);
    LIB_FUNCTION("vfrL8gPlm2Y", "libSceRudp", 1, "libSceRudp", sceRudpGetMaxSegmentSize);
    LIB_FUNCTION("Px0miD2LuW0", "libSceRudp", 1, "libSceRudp", sceRudpGetNumberOfPacketsToRead);
    LIB_FUNCTION("mCQIhSmCP6o", "libSceRudp", 1, "libSceRudp", sceRudpGetOption);
    LIB_FUNCTION("Qignjmfgha0", "libSceRudp", 1, "libSceRudp", sceRudpGetRemoteInfo);
    LIB_FUNCTION("sAZqO2+5Qqo", "libSceRudp", 1, "libSceRudp", sceRudpGetSizeReadable);
    LIB_FUNCTION("fRc1ahQppR4", "libSceRudp", 1, "libSceRudp", sceRudpGetSizeWritable);
    LIB_FUNCTION("i3STzxuwPx0", "libSceRudp", 1, "libSceRudp", sceRudpGetStatus);
    LIB_FUNCTION("amuBfI-AQc4", "libSceRudp", 1, "libSceRudp", sceRudpInit);
    LIB_FUNCTION("szEVu+edXV4", "libSceRudp", 1, "libSceRudp", sceRudpInitiate);
    LIB_FUNCTION("tYVWcWDnctE", "libSceRudp", 1, "libSceRudp", sceRudpListen);
    LIB_FUNCTION("+BJ9svDmjYs", "libSceRudp", 1, "libSceRudp", sceRudpNetFlush);
    LIB_FUNCTION("vPzJldDSxXc", "libSceRudp", 1, "libSceRudp", sceRudpNetReceived);
    LIB_FUNCTION("yzeXuww-UWg", "libSceRudp", 1, "libSceRudp", sceRudpPollCancel);
    LIB_FUNCTION("haMpc7TFx0A", "libSceRudp", 1, "libSceRudp", sceRudpPollControl);
    LIB_FUNCTION("MVbmLASjn5M", "libSceRudp", 1, "libSceRudp", sceRudpPollCreate);
    LIB_FUNCTION("LjwbHpEeW0A", "libSceRudp", 1, "libSceRudp", sceRudpPollDestroy);
    LIB_FUNCTION("M6ggviwXpLs", "libSceRudp", 1, "libSceRudp", sceRudpPollWait);
    LIB_FUNCTION("9U9m1YH0ScQ", "libSceRudp", 1, "libSceRudp", sceRudpProcessEvents);
    LIB_FUNCTION("rZqWV3eXgOA", "libSceRudp", 1, "libSceRudp", sceRudpRead);
    LIB_FUNCTION("SUEVes8gvmw", "libSceRudp", 1, "libSceRudp", sceRudpSetEventHandler);
    LIB_FUNCTION("beAsSTVWVPQ", "libSceRudp", 1, "libSceRudp", sceRudpSetMaxSegmentSize);
    LIB_FUNCTION("0yzYdZf0IwE", "libSceRudp", 1, "libSceRudp", sceRudpSetOption);
    LIB_FUNCTION("OMYRTU0uc4w", "libSceRudp", 1, "libSceRudp", sceRudpTerminate);
    LIB_FUNCTION("KaPL3fbTLCA", "libSceRudp", 1, "libSceRudp", sceRudpWrite);
};

} // namespace Libraries::Rudp