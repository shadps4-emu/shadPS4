// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <sys/types.h>
#include <cstddef>

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Rudp {
struct OrbisRudpStatus {
    s64 sentUdpBytes;
    s64 rcvdUdpBytes;
    s32 sentUdpPackets;
    s32 rcvdUdpPackets;

    s64 sentUserBytes;
    s32 sentUserPackets;
    s32 sentLatencyCriticalPackets;

    s64 rcvdUserBytes;
    s32 rcvdUserPackets;
    s32 rcvdLatencyCriticalPackets;

    s32 sentSynPackets;
    s32 rcvdSynPackets;
    s32 sentUsrPackets;
    s32 rcvdUsrPackets;
    s32 sentPrbPackets;
    s32 rcvdPrbPackets;
    s32 sentRstPackets;
    s32 rcvdRstPackets;

    s32 lostPackets;
    s32 retransmittedPackets;
    s32 reorderedPackets;

    s32 currentContexts;

    s64 sentQualityLevel1Bytes;
    s64 rcvdQualityLevel1Bytes;
    s32 sentQualityLevel1Packets;
    s32 rcvdQualityLevel1Packets;

    s64 sentQualityLevel2Bytes;
    s64 rcvdQualityLevel2Bytes;
    s32 sentQualityLevel2Packets;
    s32 rcvdQualityLevel2Packets;

    s64 sentQualityLevel3Bytes;
    s64 rcvdQualityLevel3Bytes;
    s32 sentQualityLevel3Packets;
    s32 rcvdQualityLevel3Packets;

    s64 sentQualityLevel4Bytes;
    s64 rcvdQualityLevel4Bytes;
    s32 sentQualityLevel4Packets;
    s32 rcvdQualityLevel4Packets;

    s32 allocs;
    s32 frees;
    s32 memCurrent;
    s32 memPeak;

    s32 establishedConnections;
    s32 failedConnections;

    s32 failedConnectionsReset;
    s32 failedConnectionsRefused;
    s32 failedConnectionsTimeout;
    s32 failedConnectionsVersionMismatch;
    s32 failedConnectionsTransportTypeMismatch;
    s32 failedConnectionsQualityLevelMismatch;
};

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
s32 PS4_SYSV_ABI sceRudpGetStatus(OrbisRudpStatus* status, size_t statusSize);
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