// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <cstring>
#include <mutex>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "core/libraries/rudp/rudp.h"
#include "core/libraries/rudp/rudp_error.h"

namespace Libraries::Rudp {

static OrbisRudpStatus g_rudpStatusInternal = {0};

void* g_RudpContext = nullptr;

struct RudpInternalState {
    std::atomic<s32> current_contexts{0};
    std::atomic<s32> allocs{0};
    std::atomic<s32> frees{0};
    std::atomic<s32> rcvdQualityLevel4Packets{0};
    std::atomic<s32> sentQualityLevel4Packets{0};
};

static RudpInternalState g_state;

std::recursive_mutex g_RudpMutex;
bool g_isRudpInitialized = false;

s32 PS4_SYSV_ABI sceRudpAccept() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpActivate(int context_id, Net::OrbisNetSockaddr* to,
                                 OrbisNetSocklen_t toLen) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpBind(int context_id, int soc, u16 virtualPort, u8 muxMode) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpCreateContext(OrbisRudpContextEventHandler handler, void* arg,
                                      int* context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread(u32 stackSize, u32 priority) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnableInternalIOThread2(u32 stackSize, u32 priority,
                                                OrbisKernelCpumask affinityMask) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpEnd() {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpFlush(int context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetContextStatus(int context_id, OrbisRudpContextStatus* status,
                                         size_t statusSize) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetLocalInfo(int context_id, int* soc, Net::OrbisNetSockaddr* addr,
                                     u32* addrlen, u16* virtualPort, u8* muxMode) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetMaxSegmentSize(u16* mss) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetNumberOfPacketsToRead(int context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetOption(int context_id, Option option, void* optVal, size_t optLen) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetRemoteInfo(int context_id, Net::OrbisNetSockaddr* addr, u32* addrlen,
                                      u16* virtualPort) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetSizeReadable(int context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpGetSizeWritable(int context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

u32 RudpGetContextCount(uintptr_t globalContextListPtr) {
    if (globalContextListPtr == 0) {
        return 0;
    }

    u32* countPtr = reinterpret_cast<u32*>(globalContextListPtr + 8);
    return *countPtr;
}

void RudpFillStatusCounters(s32* sentQualityLevel4Packets, s32* rcvdQualityLevel4Packets, s32* allocs,
                   s32* frees) {
    if (sentQualityLevel4Packets)
        *sentQualityLevel4Packets = g_state.sentQualityLevel4Packets.load();
    if (rcvdQualityLevel4Packets)
        *rcvdQualityLevel4Packets = g_state.rcvdQualityLevel4Packets.load();
    if (allocs)
        *allocs = g_state.allocs.load();
    if (frees)
        *frees = g_state.frees.load();
}

s32 PS4_SYSV_ABI sceRudpGetStatus(OrbisRudpStatus* status, size_t statusSize) {
    std::lock_guard lock(g_RudpMutex);

    if (!g_isRudpInitialized) {
        return ORBIS_RUDP_ERROR_NOT_INITIALIZED;
    }

    int result = ORBIS_RUDP_ERROR_INVALID_ARGUMENT;
    if ((status != (OrbisRudpStatus *)0x0) && (statusSize - 1 < 0xf8)) {
        std::memcpy(status, &g_rudpStatusInternal, statusSize);

        status->currentContexts = static_cast<s32>(RudpGetContextCount(reinterpret_cast<uintptr_t>(g_RudpContext)));
        RudpFillStatusCounters(&status->sentQualityLevel4Packets, &status->rcvdQualityLevel4Packets,
                      &status->allocs, &status->frees);

        return ORBIS_OK;
    }
    return result;
}

s32 PS4_SYSV_ABI sceRudpInit(void* memPool, int memPoolSize) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpInitiate(int context_id, Net::OrbisNetSockaddr* to, OrbisNetSocklen_t toLen,
                                 u16 virtualPort) {
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

s32 PS4_SYSV_ABI sceRudpNetReceived(int soc, u8* data, size_t dataLen, Net::OrbisNetSockaddr* from,
                                    OrbisNetSocklen_t fromLen) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollCancel(PollEvent poll) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollControl(PollEvent poll, PollOp op, int context_id, PollEvent events) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollCreate(size_t size) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollDestroy(PollEvent poll) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpPollWait(PollEvent poll, OrbisRudpPollEvent* events, size_t eventLen,
                                 OrbisRudpUsec timeout) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpProcessEvents(OrbisRudpUsec timeout) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpRead(int context_id, void* data, size_t len, u8 flags,
                             OrbisRudpReadInfo* info) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetEventHandler(OrbisRudpEventHandler handler, void* arg) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetMaxSegmentSize(u16 mss) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpSetOption(int context_id, Option option, void* optVal, size_t optLen) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpTerminate(int context_id) {
    LOG_ERROR(Lib_Rudp, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRudpWrite(int context_id, void* data, size_t len, Message msg) {
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