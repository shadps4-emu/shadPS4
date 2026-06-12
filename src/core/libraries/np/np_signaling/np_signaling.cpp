// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <vector>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/net_util.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_signaling/np_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_helpers.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_stubs.h"

namespace Libraries::Np::NpSignaling {

using Libraries::Net::sceNetNtohs;

s32 PS4_SYSV_ABI sceNpSignalingInitialize(s64 memorySize, s32 threadPriority, s32 cpuAffinityMask,
                                          s64 threadStackSize) {
    if (g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_ALREADY_INITIALIZED;
    }
    InitSignalingMutex();
    SignalingMutexGuard lock;

    u32 app_type_4 = 0;
    const s32 app_type_rc = Helpers::CheckInitializeAppType(&app_type_4);
    if (app_type_rc < 0) {
        return app_type_rc;
    }
    if (app_type_4 != 0) {
        return ORBIS_NP_SIGNALING_ERROR_PROHIBITED_TO_USE;
    }

    if (memorySize == 0) {
        memorySize = 0x20000;
    }
    if (threadPriority == 0) {
        threadPriority = 700;
    }
    if (threadStackSize == 0) {
        threadStackSize = 0x4000;
    }

    LOG_INFO(Lib_NpSignaling,
             "memorySize={} threadPriority={} cpuAffinityMask={} threadStackSize={}", memorySize,
             threadPriority, cpuAffinityMask, threadStackSize);

    const s32 heap_rc = Helpers::InitSignalingHeap(memorySize);
    if (heap_rc < 0) {
        return heap_rc;
    }

    RegisterRuntimeHooks();

    const s32 check_app_type_rc = Helpers::CheckAppType();
    if (check_app_type_rc < 0) {
        Helpers::ShutdownSignalingHeap();
        return check_app_type_rc;
    }

    g_initialized = true;

    const s32 rc = Helpers::StartMainRuntime(threadPriority, cpuAffinityMask, threadStackSize);
    if (rc < 0) {
        g_initialized = false;
        Helpers::ShutdownSignalingHeap();
        return rc;
    }

    const s32 echo_rc = Helpers::StartEchoRuntime(threadPriority, cpuAffinityMask);
    if (echo_rc < 0) {
        g_initialized = false;
        Helpers::ShutdownRuntime();
        Helpers::ShutdownSignalingHeap();
        return echo_rc;
    }

    const s32 callout_rc = NpCommon::sceNpCalloutInitCtx(
        &g_callout_ctx, "SceNpSignalingCallout", static_cast<u64>(threadStackSize), threadPriority,
        static_cast<u64>(cpuAffinityMask));
    if (callout_rc < 0) {
        g_initialized = false;
        Helpers::ShutdownRuntime();
        Helpers::ShutdownSignalingHeap();
        return callout_rc;
    }
    g_callout_ctx_active = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingTerminate() {
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    {
        SignalingMutexGuard lock;
        g_initialized = false;
    }

    Helpers::ShutdownRuntime();

    if (g_callout_ctx_active) {
        NpCommon::sceNpCalloutTermCtx(&g_callout_ctx);
        g_callout_ctx_active = false;
    }

    Helpers::ShutdownSignalingHeap();

    {
        SignalingMutexGuard lock;
        g_contexts.clear();
        g_connections.clear();
        g_npid_to_conn.clear();
        g_peer_netinfo_results.clear();
    }

    DestroySignalingMutex();

    LOG_INFO(Lib_NpSignaling, "cleared all state");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContext(const void* npId, void* callback, void* callbackArg,
                                             OrbisNpSignalingContextId* outContextId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!npId || !outContextId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    OrbisNpOnlineId owner_online_id{};
    if (NormalizeNpId(npId, &owner_npid, &owner_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    s32 ctx_id = 0;
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        ctx_id = AllocateContextIdLocked();
        if (ctx_id < 0) {
            return ORBIS_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE;
        }

        NpSignalingContext& ctx = g_contexts[ctx_id];
        ctx.callback = reinterpret_cast<OrbisNpSignalingHandler>(callback);
        ctx.callback_arg = callbackArg;
        ctx.active = true;
        ctx.owner_npid = owner_npid;
        ctx.owner_online_id = owner_online_id;
        ctx.flags = 0x2;
        ctx.compiled_sdk_version = CaptureCompiledSdkVersion();
        ctx.activate_budget_us = kActivateBudgetMaxUs;
        ctx.activate_last_update_us = NowUs();
        ctx.bound_port = Stubs::ConfiguredPort();
        *outContextId = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctxId={} owner='{}' callback={} arg={} sdk={:#x} p2p_port={}",
                 ctx_id, OnlineIdToString(owner_online_id), fmt::ptr(callback),
                 fmt::ptr(callbackArg), ctx.compiled_sdk_version, ctx.bound_port);
    }

    if (Stubs::TransportIsReady()) {
        SendStunPing(ctx_id);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContextA(s32 userId, void* callback, void* callbackArg,
                                              OrbisNpSignalingContextId* outContextId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (userId == -1 || !outContextId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    const s32 npid_rc = NpManager::sceNpGetNpId(userId, &owner_npid);
    if (npid_rc < 0) {
        return npid_rc;
    }
    // Account id isn't needed without matchmaking wired up; leave it 0 for now.
    const OrbisNpAccountId account_id = 0;

    s32 ctx_id = 0;
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        ctx_id = AllocateContextIdLocked();
        if (ctx_id < 0) {
            return ORBIS_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE;
        }

        NpSignalingContext& ctx = g_contexts[ctx_id];
        ctx.callback = reinterpret_cast<OrbisNpSignalingHandler>(callback);
        ctx.callback_arg = callbackArg;
        ctx.active = true;
        ctx.owner_npid = owner_npid;
        ctx.owner_online_id = owner_npid.handle;
        ctx.flags = 0x4 | 0x2;
        ctx.account_id = account_id;
        ctx.platform_type = 0;
        ctx.compiled_sdk_version = CaptureCompiledSdkVersion();
        ctx.activate_budget_us = kActivateBudgetMaxUs;
        ctx.activate_last_update_us = NowUs();
        ctx.bound_port = Stubs::ConfiguredPort();
        *outContextId = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctxId={} userId={} owner='{}' accountId={:#x}", ctx_id, userId,
                 OnlineIdToString(ctx.owner_online_id), account_id);
    }

    if (Stubs::TransportIsReady()) {
        SendStunPing(ctx_id);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctxId) {
    std::vector<s32> active_conns;
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        LOG_INFO(Lib_NpSignaling, "ctxId={}", ctxId);

        for (const auto& [cid, ci] : g_connections) {
            if (ci.ctx_id == ctxId && ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                active_conns.push_back(cid);
            }
        }
    }

    for (const s32 cid : active_conns) {
        CloseConnectionAndDispatchDead(cid, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_PEER);
    }

    {
        SignalingMutexGuard lock;
        RemoveContextConnectionsLocked(ctxId);
        for (auto it = g_peer_netinfo_results.begin(); it != g_peer_netinfo_results.end();) {
            it = (it->second.ctx_id == ctxId) ? g_peer_netinfo_results.erase(it) : std::next(it);
        }
        g_contexts.erase(ctxId);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctxId,
                                                  const void* peerNpId,
                                                  OrbisNpSignalingConnectionId* outConnId) {
    LOG_INFO(Lib_NpSignaling, "t={} ctxId={} peerNpId={:p} connId={:p}", NowMs(), ctxId, peerNpId,
             fmt::ptr(outConnId));

    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    if (NormalizeNpId(peerNpId, &peer_npid, &peer_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const std::string peer_online_id_str = OnlineIdToString(peer_online_id);

    s32 cid = 0;
    bool reused_established = false;
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        const auto ctx_it = g_contexts.find(ctxId);
        if (ctx_it == g_contexts.end() || !ctx_it->second.active) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        if (std::memcmp(&ctx_it->second.owner_npid, &peer_npid, sizeof(peer_npid)) == 0) {
            return ORBIS_NP_SIGNALING_ERROR_OWN_NP_ID;
        }
        if (!ConsumeActivationBudgetGatedLocked(ctx_it->second)) {
            return ORBIS_NP_SIGNALING_ERROR_EXCEED_RATE_LIMIT;
        }

        const CtxNpIdKey lookup_key = MakeCtxNpIdKey(ctxId, peer_npid);
        const auto existing_it = g_npid_to_conn.find(lookup_key);
        if (existing_it != g_npid_to_conn.end()) {
            const auto conn_it = g_connections.find(existing_it->second);
            if (conn_it != g_connections.end() && conn_it->second.state == ConnState::Inactive) {
                RemoveConnectionLocked(existing_it->second);
            } else if (conn_it != g_connections.end()) {
                cid = existing_it->second;
                ConnectionInfo& ci = conn_it->second;
                ci.locally_activated = true;
                if (ci.state == ConnState::Established) {
                    ClearLingerAndTimeoutLocked(cid);
                    reused_established = true;
                } else {
                    ClearLingerAndTimeoutLocked(cid);
                }
            }
        }
        bool created_new = false;
        if (cid == 0) {
            cid = AllocateConnectionIdLocked();
            if (cid < 0) {
                return ORBIS_NP_SIGNALING_ERROR_OUT_OF_MEMORY;
            }
            ConnectionInfo ci{};
            ci.conn_id = cid;
            ci.ctx_id = ctxId;
            ci.state = ConnState::SendingOffer;
            ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
            ci.is_initiator = true;
            ci.locally_activated = true;
            ci.npid = peer_npid;
            ci.online_id = peer_online_id;
            g_connections[cid] = std::move(ci);
            g_npid_to_conn[lookup_key] = cid;
            created_new = true;
        }
        if (created_new) {
            ArmConnectTimeoutLocked(cid);
            QueueActivationLocked(cid, peer_online_id_str);
        }
        LOG_INFO(Lib_NpSignaling, "ctxId={} peer='{}' connId={} Status: {}", ctxId,
                 peer_online_id_str, cid,
                 created_new          ? "queued, async"
                 : reused_established ? "reused established"
                                      : "reused transient");
    }

    *outConnId = cid;

    if (reused_established) {
        EstablishConnection(cid, false);
        return ORBIS_OK;
    }

    g_dispatch_cv.notify_all();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingActivateConnectionA(
    OrbisNpSignalingContextId ctxId, const OrbisNpSignalingAccountPlatformPair* peerAddr,
    OrbisNpSignalingConnectionId* outConnId) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAddr || peerAddr->accountId == 0 || !outConnId || peerAddr->platformType == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    const auto ctx_it = g_contexts.find(ctxId);
    if (ctx_it == g_contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) != 0 && ctx_it->second.account_id == peerAddr->accountId &&
        ctx_it->second.platform_type == peerAddr->platformType) {
        return ORBIS_NP_SIGNALING_ERROR_OWN_PEER_ADDRESS;
    }

    if (!ConsumeActivationBudgetGatedLocked(ctx_it->second)) {
        return ORBIS_NP_SIGNALING_ERROR_EXCEED_RATE_LIMIT;
    }
    const s32 cid = AllocateConnectionIdLocked();
    if (cid < 0) {
        return ORBIS_NP_SIGNALING_ERROR_OUT_OF_MEMORY;
    }
    ConnectionInfo ci{};
    ci.conn_id = cid;
    ci.ctx_id = ctxId;
    ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    g_connections[cid] = std::move(ci);
    *outConnId = cid;

    LOG_INFO(Lib_NpSignaling, "ctxId={} accountId={:#x} platform={} connId={}", ctxId,
             peerAddr->accountId, peerAddr->platformType, cid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctxId,
                                                    OrbisNpSignalingConnectionId connId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }

        const auto it = g_connections.find(connId);
        if (it == g_connections.end() || it->second.ctx_id != ctxId) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }

        LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} peer='{}' status={}", NowMs(), ctxId,
                 connId, OnlineIdToString(it->second.online_id), it->second.status);
    }
    DeactivateConnectionFaithful(connId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctxId,
                                                   OrbisNpSignalingConnectionId connId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        const auto it = g_connections.find(connId);
        if (it == g_connections.end() || it->second.ctx_id != ctxId) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }
        LOG_INFO(Lib_NpSignaling, "ctxId={} connId={}", ctxId, connId);
    }
    TerminateConnectionFaithful(connId);
    {
        SignalingMutexGuard lock;
        RemoveConnectionLocked(connId);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctxId,
                                                   OrbisNpSignalingConnectionId connId,
                                                   u32* outStatus, u32* outPeerAddr,
                                                   u16* outPeerPort) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outStatus) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = g_connections.find(connId);
    if (it == g_connections.end() || it->second.ctx_id != ctxId) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
        return ORBIS_OK;
    }

    const s32 state = ConnectionStateFromStatus(it->second.status);
    if (state == 10) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
        if (outPeerAddr) {
            *outPeerAddr = it->second.addr;
        }
        if (outPeerPort) {
            *outPeerPort = it->second.port;
        }
    } else if (state != 0) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    } else {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    }

    LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} peer='{}' status={}", NowMs(), ctxId, connId,
             OnlineIdToString(it->second.online_id), *outStatus);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctxId,
                                                 OrbisNpSignalingConnectionId connId, s32 infoCode,
                                                 void* outInfo) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctxId, connId, infoCode, outInfo, nullptr);
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={} infoCode={} rc={:#x}", ctxId, connId, infoCode,
             static_cast<u32>(rc));
    return rc;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctxId,
                                                  OrbisNpSignalingConnectionId connId, s32 infoCode,
                                                  void* outInfo) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctxId, connId, infoCode, nullptr, outInfo);
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={} infoCode={} rc={:#x}", ctxId, connId, infoCode,
             static_cast<u32>(rc));
    return rc;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctxId,
                                                     const void* peerNpId,
                                                     OrbisNpSignalingConnectionId* outConnId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    if (NormalizeNpId(peerNpId, &peer_npid, &peer_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = g_npid_to_conn.find(MakeCtxNpIdKey(ctxId, peer_npid));
    if (it == g_npid_to_conn.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }
    const auto conn_it = g_connections.find(it->second);
    if (conn_it == g_connections.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    *outConnId = it->second;
    LOG_INFO(Lib_NpSignaling, "ctxId={} npid='{}' connId={}", ctxId,
             OnlineIdToString(peer_online_id), it->second);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctxId, u32 peerAddr,
                                           u16 peerPort, OrbisNpSignalingConnectionId* outConnId) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    for (const auto& [id, ci] : g_connections) {
        if (ci.ctx_id == ctxId && ci.addr == peerAddr && ci.port == peerPort &&
            ConnectionStateFromStatus(ci.status) == 10) {
            *outConnId = id;
            LOG_INFO(Lib_NpSignaling, "ctxId={} addr={:#x} port={} connId={}", ctxId, peerAddr,
                     sceNetNtohs(peerPort), id);
            return ORBIS_OK;
        }
    }
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddressA(
    OrbisNpSignalingContextId ctxId, const OrbisNpSignalingAccountPlatformPair* peerAddr,
    OrbisNpSignalingConnectionId* outConnId) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAddr || !outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = g_contexts.find(ctxId);
    if (ctx_it == g_contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpSignaling, "ctxId={} accountId={:#x}", ctxId, peerAddr->accountId);
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 PS4_SYSV_ABI sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                                s32 optionValue) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    const auto it = g_contexts.find(ctxId);
    if (it == g_contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (optionId != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (optionValue == 0) {
        it->second.flags &= ~0x2u;
    } else if (optionValue == 1) {
        it->second.flags |= 0x2u;
    } else {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                                s32* outOptionValue) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outOptionValue) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto it = g_contexts.find(ctxId);
    if (it == g_contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (optionId != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    *outOptionValue = (it->second.flags & 0x2u) != 0 ? 1 : 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctxId,
                                               OrbisNpSignalingNetInfo* info) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (info->size != sizeof(OrbisNpSignalingNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    info->localAddr = ParseIpv4Nbo(netinfo->GetIp());

    NetCtl::OrbisNetCtlNatInfo nat_info{};
    nat_info.size = sizeof(nat_info);
    info->mappedAddr = 0;
    info->natStatus = 0;
    if (NetCtl::sceNetCtlGetNatInfo(&nat_info) >= 0) {
        info->mappedAddr = nat_info.mapped_addr;
        info->natStatus = nat_info.nat_type;
    }
    if (info->mappedAddr == 0) {
        const u32 external = netinfo->GetExternalIp();
        info->mappedAddr = external != 0 ? external : info->localAddr;
    }
    info->_pad_14 = 0;

    LOG_INFO(Lib_NpSignaling, "localAddr={:#x} mappedAddr={:#x} natStatus={}", info->localAddr,
             info->mappedAddr, info->natStatus);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                              OrbisNpSignalingRequestId* outReqId) {
    {
        SignalingMutexGuard lock;
        if (!g_initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outReqId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    if (NormalizeNpId(peerNpId, &peer_npid, nullptr) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    s32 req_id = 0;
    {
        SignalingMutexGuard lock;
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        req_id = StagePeerNetInfoResultLocked(ctxId);
        if (req_id == 0) {
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
        }
    }
    *outReqId = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqId={:#x}", ctxId, *outReqId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctxId,
                                               const void* peerAccountPayload,
                                               OrbisNpSignalingRequestId* outReqId) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAccountPayload || !outReqId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = g_contexts.find(ctxId);
    if (ctx_it == g_contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 req_id = StagePeerNetInfoResultLocked(ctxId);
    if (req_id == 0) {
        return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
    }
    *outReqId = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqId={:#x}", ctxId, *outReqId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctxId, s32 reqOrConnId) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (!DropPeerNetInfoResultLocked(ctxId, reqOrConnId)) {
        return ORBIS_NP_SIGNALING_ERROR_REQ_NOT_FOUND;
    }
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqOrConnId={:#x}", ctxId, reqOrConnId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctxId,
                                                    OrbisNpSignalingConnectionId connId,
                                                    OrbisNpSignalingNetInfo* peerNetInfo) {
    SignalingMutexGuard lock;
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerNetInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (peerNetInfo->size != sizeof(OrbisNpSignalingNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    PeerNetInfoResult result{};
    if (!TakePeerNetInfoResultLocked(ctxId, connId, &result)) {
        return ORBIS_NP_SIGNALING_ERROR_RESULT_NOT_FOUND;
    }
    peerNetInfo->localAddr = result.local_ipv4;
    peerNetInfo->mappedAddr = result.external_ipv4;
    peerNetInfo->natStatus = static_cast<s32>(result.nat_route_kind);
    peerNetInfo->_pad_14 = 0;
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={:#x} ext={:#x}", ctxId, connId,
             result.external_ipv4);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetMemoryInfo(OrbisNpSignalingMemoryInfo* info) {
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    info->currentInUse = 0;
    info->peakInUse = 0;
    info->maxSystemSize = 0x20000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpSignalingGetConnectionStatistics(OrbisNpSignalingConnectionStatistics* stats) {
    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!stats) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    SignalingMutexGuard lock;
    SnapshotConnectionStatisticsLocked(&stats->peakConnectionCount, &stats->activeConnectionCount,
                                       &stats->transientConnectionCount,
                                       &stats->establishedConnectionCount);
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0UvTFeomAUM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnection);
    LIB_FUNCTION("ZPLavCKqAB0", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnectionA);
    LIB_FUNCTION("X1G4kkN2R-8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCancelPeerNetInfo);
    LIB_FUNCTION("5yYjEdd4t8Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContext);
    LIB_FUNCTION("dDLNFdY8dws", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContextA);
    LIB_FUNCTION("6UEembipgrM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeactivateConnection);
    LIB_FUNCTION("hx+LIg-1koI", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeleteContext);
    LIB_FUNCTION("GQ0hqmzj0F4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromNpId);
    LIB_FUNCTION("CkPxQjSm018", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddress);
    LIB_FUNCTION("B7cT9aVby7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddressA);
    LIB_FUNCTION("AN3h0EBSX7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfo);
    LIB_FUNCTION("rcylknsUDwg", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfoA);
    LIB_FUNCTION("C6ZNCDTj00Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatistics);
    LIB_FUNCTION("bD-JizUb3JM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatus);
    LIB_FUNCTION("npU5V56id34", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetContextOption);
    LIB_FUNCTION("U8AQMlOFBc8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetLocalNetInfo);
    LIB_FUNCTION("tOpqyDyMje4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetMemoryInfo);
    LIB_FUNCTION("zFgFHId7vAE", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfo);
    LIB_FUNCTION("Shr7bZq8QHY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoA);
    LIB_FUNCTION("2HajCEGgG4s", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoResult);
    LIB_FUNCTION("3KOuC4RmZZU", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingInitialize);
    LIB_FUNCTION("IHRDvZodPYY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingSetContextOption);
    LIB_FUNCTION("NPhw0UXaNrk", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminate);
    LIB_FUNCTION("b4qaXPzMJxo", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminateConnection);
}

} // namespace Libraries::Np::NpSignaling
