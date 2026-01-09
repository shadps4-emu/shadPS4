// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Rudp {

s32 PS4_SYSV_ABI sceRudpAccept();
s32 PS4_SYSV_ABI sceRudpActivate();
s32 PS4_SYSV_ABI sceRudpBind();
s32 PS4_SYSV_ABI sceRudpCreateContext();
s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread();
s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread2();
s32 PS4_SYSV_ABI sceRudpEnd();
s32 PS4_SYSV_ABI sceRudpFlush();
s32 PS4_SYSV_ABI sceRudpGetContextStatus();
s32 PS4_SYSV_ABI sceRudpGetLocalInfo();
s32 PS4_SYSV_ABI sceRudpGetMaxSegmentSize();
s32 PS4_SYSV_ABI sceRudpGetNumberOfPacketsToRead();
s32 PS4_SYSV_ABI sceRudpGetOption();
s32 PS4_SYSV_ABI sceRudpGetRemoteInfo();
s32 PS4_SYSV_ABI sceRudpGetSizeReadable();
s32 PS4_SYSV_ABI sceRudpGetSizeWritable();
s32 PS4_SYSV_ABI sceRudpGetStatus();
s32 PS4_SYSV_ABI sceRudpInit();
s32 PS4_SYSV_ABI sceRudpInitiate();
s32 PS4_SYSV_ABI sceRudpListen();
s32 PS4_SYSV_ABI sceRudpNetFlush();
s32 PS4_SYSV_ABI sceRudpNetReceived();
s32 PS4_SYSV_ABI sceRudpPollCancel();
s32 PS4_SYSV_ABI sceRudpPollControl();
s32 PS4_SYSV_ABI sceRudpPollCreate();
s32 PS4_SYSV_ABI sceRudpPollDestroy();
s32 PS4_SYSV_ABI sceRudpPollWait();
s32 PS4_SYSV_ABI sceRudpProcessEvents();
s32 PS4_SYSV_ABI sceRudpRead();
s32 PS4_SYSV_ABI sceRudpSetEventHandler();
s32 PS4_SYSV_ABI sceRudpSetMaxSegmentSize();
s32 PS4_SYSV_ABI sceRudpSetOption();
s32 PS4_SYSV_ABI sceRudpTerminate();
s32 PS4_SYSV_ABI sceRudpWrite();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Rudp