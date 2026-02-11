// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Rudp {

enum class State : int {
    IDLE = 0,
    CLOSED = 1,
    SYN_SENT = 2,
    SYN_RCVD = 3,
    ESTABLISHED = 4,
    CLOSE_WAIT = 5,
};

enum class Event : int {
    SEND = 1,
    SOCKET_RELEASED = 2,
    DIAGNOSTIC_SENT = 100,
    DIAGNOSTIC_RCVD = 101,
};

enum class ContextEvent : int {
    CLOSED = 1,
    ESTABLISHED = 2,
    ERROR = 3,
    WRITEABLE = 4,
    READABLE = 5,
    FLUSHED = 6
};

enum class PollOp : int { ADD = 1, MODIFY = 2, REMOVE = 3 };

enum class PollEvent : int { READ = 0x0001, WRITE = 0x0002, FLUSH = 0x0004, ERROR = 0x0008 };

enum class Option : int {
    MAX_PAYLOAD = 1,
    OPTION_SEND_BUFFER_SIZE = 2,
    OPTION_RECEIVE_BUFFER_SIZE = 3,
    OPTION_NO_DELAY = 4,
    OPTION_CRITICAL_DELIVERY = 5,
    OPTION_CRITICAL_ORDER = 6,
    OPTION_NONBLOCK = 7,
    STREAM = 8,
    CONN_TIMEOUT = 9,
    CLOSE_WAIT_TIMEOUT = 10,
    AGGREGATION_TIMEOUT = 11,
    LAST_ERROR = 14,
    READ_TIMEOUT = 15,
    WRITE_TIMEOUT = 16,
    FLUSH_TIMEOUT = 17,
    KEEPALIVE_INTERVAL = 18,
    KEEPALIVE_TIMEOUT = 19,
    AGGREGATION_BUFFER = 20,
};

enum class Message : int {
    DONTWAIT = 0x01,
    LATENCY_CRITICAL = 0x08,
    ALIGN32 = 0x10,
    ALIGN64 = 0x20,
    WITH_TIMESTAMP = 0x40,
};

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

struct OrbisRudpContextStatus {
    State state;
    int parentId;
    u32 children;
    u32 lostPackets;
    u32 sentPackets;
    u32 rcvdPackets;
    u64 sentBytes;
    u64 rcvdBytes;
    u32 retransmissions;
    u32 rtt;
};

struct OrbisRudpPollEvent {
    int ctxId;
    u16 reqEvents;
    u16 rtnEvents;
};

struct OrbisRudpReadInfo {
    u8 size;
    u8 retransmissionCount;
    u16 retransmissionDelay;
    u8 retransmissionDelay2;
    u8 flags;
    u16 sequenceNumber;
    u32 timestamp;
};
typedef s32 OrbisKernelCpumask;

typedef u32 OrbisNetSocklen_t;

typedef void (*OrbisRudpContextEventHandler)(int context_id, ContextEvent event_id, int errorCode,
                                             void* arg);

typedef u64 OrbisRudpUsec;

typedef int (*OrbisRudpEventHandler)(Event event_id, int soc, u8 const* data, size_t dataLen,
                                     Net::OrbisNetSockaddr const* addr, OrbisNetSocklen_t addrLen,
                                     void* arg);

s32 PS4_SYSV_ABI sceRudpAccept();
s32 PS4_SYSV_ABI sceRudpActivate(int context_id, Net::OrbisNetSockaddr* to,
                                 OrbisNetSocklen_t toLen);
s32 PS4_SYSV_ABI sceRudpBind(int context_id, int soc, u16 vport, u8 muxMode);
s32 PS4_SYSV_ABI sceRudpCreateContext(OrbisRudpContextEventHandler handler, void* arg,
                                      int* context_id);
s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread(u32 stackSize, u32 priority);
s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread2(u32 stackSize, u32 priority,
                                                OrbisKernelCpumask affinityMask);
s32 PS4_SYSV_ABI sceRudpEnd();
s32 PS4_SYSV_ABI sceRudpFlush(int context_id);
s32 PS4_SYSV_ABI sceRudpGetContextStatus(int context_id, OrbisRudpContextStatus* status,
                                         size_t statusSize);
s32 PS4_SYSV_ABI sceRudpGetLocalInfo(int context_id, int* soc, Net::OrbisNetSockaddr* addr,
                                     OrbisNetSocklen_t* addrLen, u16* vport, u8* muxMode);
s32 PS4_SYSV_ABI sceRudpGetMaxSegmentSize(u16* mss);
s32 PS4_SYSV_ABI sceRudpGetNumberOfPacketsToRead(int context_id);
s32 PS4_SYSV_ABI sceRudpGetOption(int context_id, Option option, void* optVal, size_t optLen);
s32 PS4_SYSV_ABI sceRudpGetRemoteInfo(int context_id, Net::OrbisNetSockaddr* addr,
                                      OrbisNetSocklen_t* addrLen, u16* vport);
s32 PS4_SYSV_ABI sceRudpGetSizeReadable(int context_id);
s32 PS4_SYSV_ABI sceRudpGetSizeWritable(int context_id);
s32 PS4_SYSV_ABI sceRudpGetStatus(OrbisRudpStatus* status, size_t statusSize);
s32 PS4_SYSV_ABI sceRudpInit(void* memPool, int memPoolSize);
s32 PS4_SYSV_ABI sceRudpInitiate(int context_id, Net::OrbisNetSockaddr* to, OrbisNetSocklen_t toLen,
                                 u16 vport);
s32 PS4_SYSV_ABI sceRudpListen();
s32 PS4_SYSV_ABI sceRudpNetFlush();
s32 PS4_SYSV_ABI sceRudpNetReceived(int soc, u8* data, size_t dataLen, Net::OrbisNetSockaddr* from,
                                    OrbisNetSocklen_t fromLen);
s32 PS4_SYSV_ABI sceRudpPollCancel(PollEvent poll);
s32 PS4_SYSV_ABI sceRudpPollControl(PollEvent poll, PollOp op, int context_id, PollEvent events);
s32 PS4_SYSV_ABI sceRudpPollCreate(size_t size);
s32 PS4_SYSV_ABI sceRudpPollDestroy(PollEvent poll);
s32 PS4_SYSV_ABI sceRudpPollWait(PollEvent poll, OrbisRudpPollEvent* events, size_t eventLen,
                                 OrbisRudpUsec timeout);
s32 PS4_SYSV_ABI sceRudpProcessEvents(OrbisRudpUsec timeout);
s32 PS4_SYSV_ABI sceRudpRead(int context_id, void* data, size_t len, u8 flags,
                             OrbisRudpReadInfo* info);
s32 PS4_SYSV_ABI sceRudpSetEventHandler(OrbisRudpEventHandler handler, void* arg);
s32 PS4_SYSV_ABI sceRudpSetMaxSegmentSize(u16 mss);
s32 PS4_SYSV_ABI sceRudpSetOption(int context_id, Option option, void* optVal, size_t optLen);
s32 PS4_SYSV_ABI sceRudpTerminate(int context_id);
s32 PS4_SYSV_ABI sceRudpWrite(int context_id, void* data, size_t len, Message msg);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Rudp