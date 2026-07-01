// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_matching2/np_matching2_types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpMatching2 {

static bool g_initialized = false;

int PS4_SYSV_ABI sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                             OrbisNpMatching2ContextId* ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, npId = {}, serviceLabel = {}, size = {}",
             param->npId->handle.data, param->serviceLabel, param->size);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x28 || !ctxId) {
        LOG_ERROR(Lib_NpMatching2, "param null, invalid size, or ctxId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    return ContextManager::Instance().CreateContext(param->npId, param->serviceLabel, ctxId);
}

int PS4_SYSV_ABI sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                              OrbisNpMatching2ContextId* ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, userId = {}, serviceLabel = {}, size = {}", param->userId,
             param->serviceLabel, param->size);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x10 || !ctxId) {
        LOG_ERROR(Lib_NpMatching2, "param null, invalid size, or ctxId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    const Libraries::Np::OrbisNpId np_id =
        Libraries::Np::NpHandler::GetInstance().GetNpId(param->userId);

    return ContextManager::Instance().CreateContext(&np_id, param->serviceLabel, ctxId);
}

static void StoreRequestCallback(ContextObject* ctx,
                                 const OrbisNpMatching2RequestOptParam* requestOpt) {
    if (requestOpt && requestOpt->callback) {
        ctx->default_request_callback = requestOpt->callback;
        ctx->default_request_callback_arg = requestOpt->arg;
    }
}

static u32 GetRoomPingUs(const ContextObject& ctx, OrbisNpMatching2RoomId roomId) {
    const auto room_it = ctx.room_cache.find(roomId);
    if (room_it == ctx.room_cache.end()) {
        return 0;
    }

    u64 total_ping = 0;
    u32 ping_count = 0;
    for (const auto& [member_id, member] : room_it->second.members) {
        if (member_id == ctx.my_member_id) {
            continue;
        }
        const auto peer_it = ctx.peers.find(member_id);
        if (peer_it == ctx.peers.end() || peer_it->second.ping_us == 0) {
            continue;
        }
        total_ping += peer_it->second.ping_us;
        ++ping_count;
    }

    return ping_count == 0 ? 0 : static_cast<u32>(total_ping / ping_count);
}

static void* BuildSignalingGetPingInfoPayload(ContextObject& ctx, OrbisNpMatching2RoomId roomId) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();

    const auto room_it = ctx.room_cache.find(roomId);
    p.ping_info_response = std::make_unique<OrbisNpMatching2SignalingGetPingInfoResponse>();
    auto& out = *p.ping_info_response;
    out = {};
    if (room_it != ctx.room_cache.end()) {
        out.serverId = room_it->second.server_id;
        out.worldId = room_it->second.world_id;
        out.roomId = room_it->second.room_id;
    } else {
        out.serverId = ctx.server_id;
        out.worldId = ctx.world_id;
        out.roomId = roomId;
    }
    out.rtt = GetRoomPingUs(ctx, roomId);

    p.request_data = p.ping_info_response.get();
    return p.request_data;
}

static Libraries::Np::OrbisNpId MakePeerNpId(const PeerInfo* peer, const MemberCache* member) {
    Libraries::Np::OrbisNpId npid{};
    if (member) {
        npid = member->np_id;
    } else if (peer) {
        npid.handle = peer->online_id;
    }
    return npid;
}

static u64 StableAccountIdFromNpId(const Libraries::Np::OrbisNpId& npid) {
    u64 hash = 1469598103934665603ULL;
    for (const char c : npid.handle.data) {
        if (c == '\0') {
            break;
        }
        hash ^= static_cast<u8>(c);
        hash *= 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

static Libraries::Np::OrbisNpPeerAddressA MakePeerAddressA(const PeerInfo* peer,
                                                           const MemberCache* member) {
    const Libraries::Np::OrbisNpId npid = MakePeerNpId(peer, member);
    Libraries::Np::OrbisNpPeerAddressA addr{};
    addr.accountId = StableAccountIdFromNpId(npid);
    addr.platform = Libraries::Np::OrbisNpPlatformType::PS4;
    return addr;
}

static s32 FillMatching2ConnectionInfo(const ContextObject& ctx, OrbisNpMatching2RoomId roomId,
                                       OrbisNpMatching2RoomMemberId memberId, u32 infoType,
                                       void* connInfo, bool a_variant) {
    if (!connInfo) {
        LOG_ERROR(Lib_NpMatching2, "connInfo null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    const auto room_it = ctx.room_cache.find(roomId);
    if (room_it == ctx.room_cache.end()) {
        LOG_INFO(Lib_NpMatching2, "room={} not cached for connection info", roomId);
        if (a_variant) {
            *static_cast<OrbisNpMatching2SignalingConnectionInfoA*>(connInfo) = {};
        } else {
            *static_cast<OrbisNpMatching2SignalingConnectionInfo*>(connInfo) = {};
        }
        return ORBIS_OK;
    }

    const auto member_it = room_it->second.members.find(memberId);
    const MemberCache* member =
        member_it != room_it->second.members.end() ? &member_it->second : nullptr;
    const auto peer_it = ctx.peers.find(memberId);
    const PeerInfo* peer = peer_it != ctx.peers.end() ? &peer_it->second : nullptr;

    if (a_variant) {
        auto* out = static_cast<OrbisNpMatching2SignalingConnectionInfoA*>(connInfo);
        *out = {};
        switch (infoType) {
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT:
            out->rtt = peer ? peer->ping_us : 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH:
            out->bandwidth = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            if (peer) {
                out->address.addr = peer->addr;
                out->address.port = peer->port;
            } else if (member) {
                out->address.addr = member->addr;
                out->address.port = member->port;
            }
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS:
            out->packetLoss = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDRESS_A:
            out->peerAddrA = MakePeerAddressA(peer, member);
            break;
        default:
            LOG_WARNING(Lib_NpMatching2, "unsupported connection info A type={}", infoType);
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_INVALID_ARGUMENT;
        }
    } else {
        auto* out = static_cast<OrbisNpMatching2SignalingConnectionInfo*>(connInfo);
        *out = {};
        switch (infoType) {
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT:
            out->rtt = peer ? peer->ping_us : 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH:
            out->bandwidth = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NP_ID:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NPID:
            out->npId = MakePeerNpId(peer, member);
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            if (peer) {
                out->address.addr = peer->addr;
                out->address.port = peer->port;
            } else if (member) {
                out->address.addr = member->addr;
                out->address.port = member->port;
            }
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS:
            out->packetLoss = 0;
            break;
        default:
            LOG_WARNING(Lib_NpMatching2, "unsupported connection info type={}", infoType);
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_INVALID_ARGUMENT;
        }
    }

    LOG_INFO(Lib_NpMatching2, "connection info{}: ctx={} room={} member={} type={} rtt={}",
             a_variant ? "A" : "", ctx.ctx_id, roomId, memberId, infoType,
             peer ? peer->ping_us : 0);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2CreateJoinRoom(OrbisNpMatching2ContextId ctxId,
                                              OrbisNpMatching2CreateJoinRoomRequest* request,
                                              OrbisNpMatching2RequestOptParam* requestOpt,
                                              OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmCreateJoinRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2JoinRoom(OrbisNpMatching2ContextId ctxId,
                                        OrbisNpMatching2JoinRoomRequest* request,
                                        OrbisNpMatching2RequestOptParam* requestOpt,
                                        OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmJoinRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2CreateJoinRoomRequestA* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    LOG_WARNING(Lib_NpMatching2, "not implemented");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                                       void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, userdata = {}", userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    ContextManager::Instance().ApplyContextCallback(callback, userdata);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterLobbyEventCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2LobbyEventCallback callback, void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->lobby_event_callback = callback;
    ctx->lobby_event_callback_arg = userdata;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2RoomEventCallback callback,
                                                         void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->room_event_callback = callback;
    ctx->room_event_callback_arg = userdata;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2SignalingCallback callback,
                                                         void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->signaling_callback = callback;
    ctx->signaling_callback_arg = userdata;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, timeout = {}", ctxId, timeout);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!EmulatorSettings.IsConnectedToNetwork() || !EmulatorSettings.IsShadNetEnabled()) {
        // error confirmed with a real console disconnected from the internet
        constexpr int ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT = 0x804101e2;
        LOG_ERROR(Lib_NpMatching2, "not connected to network");
        return ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT;
    }

    const s32 rc = ContextManager::Instance().Start(ctxId);
    if (rc != ORBIS_OK) {
        return rc;
    }

    MmContextStart(ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextStop(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    const s32 rc = ContextManager::Instance().Stop(ctxId);
    if (rc != ORBIS_OK) {
        return rc;
    }

    MmContextStop(ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param) {
    LOG_INFO(Lib_NpMatching2, "called");

    if (g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "already initialized");
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    if (!param || (param->size != 0x30 && param->size != 0x28)) {
        LOG_ERROR(Lib_NpMatching2, "param null or invalid size");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpMatching2, "poolSize={:#x} stackSize={:#x} priority={} size={:#x}",
             param->poolSize, param->stackSize, param->priority, param->size);
    if (param->size == 0x30) {
        LOG_INFO(Lib_NpMatching2, "sslPoolSize={:#x}", param->sslPoolSize);
    }

    InitEventDispatcher();

    g_initialized = true;
    g_state.initialized.store(true);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2Terminate() {
    LOG_INFO(Lib_NpMatching2, "called");

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    g_initialized = false;
    g_state.initialized.store(false);

    TermEventDispatcher();
    ContextManager::Instance().Reset();

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetDefaultRequestOptParam(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RequestOptParam* requestOpt) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!requestOpt) {
        LOG_ERROR(Lib_NpMatching2, "requestOpt null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->default_request_callback = requestOpt->callback;
    ctx->default_request_callback_arg = requestOpt->arg;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetServerId(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2ServerId* serverId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!serverId) {
        LOG_ERROR(Lib_NpMatching2, "serverId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    *serverId = 1;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctxId,
                                                OrbisNpMatching2GetWorldInfoListRequest* request,
                                                OrbisNpMatching2RequestOptParam* requestOpt,
                                                OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, request.serverId = {}, requestOpt = {}", ctxId,
             request ? request->serverId : 0xFFFF, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetWorldInfoList(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2LeaveRoom(OrbisNpMatching2ContextId ctxId,
                                         OrbisNpMatching2LeaveRoomRequest* request,
                                         OrbisNpMatching2RequestOptParam* requestOpt,
                                         OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmLeaveRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctxId,
                                          OrbisNpMatching2SearchRoomRequest* request,
                                          OrbisNpMatching2RequestOptParam* requestOpt,
                                          OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSearchRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetUserInfo(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2SetUserInfoRequest* request,
                                           OrbisNpMatching2RequestOptParam* requestOpt,
                                           OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    *requestId = AllocRequestId();
    LOG_WARNING(Lib_NpMatching2, "not implemented");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SendRoomMessage(OrbisNpMatching2ContextId ctxId, void* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    *requestId = AllocRequestId();
    LOG_WARNING(Lib_NpMatching2, "not implemented");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetRoomDataExternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SetRoomDataExternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetRoomDataExternal(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetRoomDataInternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SetRoomDataInternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetRoomDataInternal(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionStatus(OrbisNpMatching2ContextId ctxId,
                                                            OrbisNpMatching2RoomId roomId,
                                                            OrbisNpMatching2RoomMemberId memberId,
                                                            s32* connStatus, void* peerAddr,
                                                            u16* peerPort) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    constexpr s32 kInactive = 0;
    constexpr s32 kPending = 1;
    constexpr s32 kActive = 2;
    const auto room_it = ctx->room_cache.find(roomId);
    const bool in_room = room_it != ctx->room_cache.end() &&
                         room_it->second.members.find(memberId) != room_it->second.members.end();

    if (!in_room) {
        if (connStatus) {
            *connStatus = kInactive;
        }
        LOG_INFO(Lib_NpMatching2, "member={} is not in room={}, status=inactive", memberId, roomId);
        return ORBIS_OK;
    }

    auto peer_it = ctx->peers.find(memberId);
    if (peer_it == ctx->peers.end()) {
        PeerInfo pi{};
        pi.member_id = memberId;
        pi.status = kPending;
        const auto member_it = room_it->second.members.find(memberId);
        if (member_it != room_it->second.members.end()) {
            pi.addr = member_it->second.addr;
            pi.port = member_it->second.port;
            std::strncpy(pi.online_id.data, member_it->second.np_id.handle.data,
                         sizeof(pi.online_id.data) - 1);
        }
        peer_it = ctx->peers.emplace(memberId, pi).first;
    }

    const s32 status = peer_it->second.status == kActive ? kActive : kPending;
    if (connStatus) {
        *connStatus = status;
    }
    if (peerAddr) {
        *reinterpret_cast<u32*>(peerAddr) = peer_it->second.addr;
    }
    if (peerPort) {
        *peerPort = peer_it->second.port;
    }

    LOG_INFO(Lib_NpMatching2, "member={} is in room={}, status={} addr={:#x}:{}", memberId, roomId,
             status, peer_it->second.addr, Libraries::Net::sceNetNtohs(peer_it->second.port));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2AbortContextStart(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2DestroyContext(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!ContextManager::Instance().Destroy(ctxId)) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterLobbyMessageCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2LobbyMessageCallback callback,
    void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->lobby_message_callback = callback;
    ctx->lobby_message_callback_arg = userdata;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterRoomMessageCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomMessageCallback callback, void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->room_message_callback = callback;
    ctx->room_message_callback_arg = userdata;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetMemoryInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetSslMemoryInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomMemberIdListLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomPasswordLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetSignalingOptParamLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetLobbyInfoList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetLobbyMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetLobbyMemberDataInternalList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomDataExternalList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomDataInternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2GetRoomDataInternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;

    void* request_data = BuildGetRoomDataInternalPayload(*ctx, request->roomId);

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = ctxId;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = reqId;
    ev.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_INTERNAL;
    ev.error_code = 0;
    ev.request_cb = ctx->default_request_callback;
    ev.request_cb_arg = ctx->default_request_callback_arg;
    ev.request_data = request_data;
    ScheduleEvent(std::move(ev));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomMemberDataExternalList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetRoomMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetUserInfoListA() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetUserInfoList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GrantRoomOwner(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                              void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2JoinLobby(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                         void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2JoinRoomA(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                         void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2KickoutRoomMember(OrbisNpMatching2ContextId ctxId,
                                                 OrbisNpMatching2KickoutRoomMemberRequest* request,
                                                 OrbisNpMatching2RequestOptParam* requestOpt,
                                                 OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmKickoutRoomMember(ctxId, reqId, *request);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2LeaveLobby() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SendLobbyChatMessage() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SendRoomChatMessage(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                                   void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetLobbyMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetRoomMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetSignalingOptParam() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetPeerNetInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetPingInfo(OrbisNpMatching2ContextId ctxId,
                                                    const void* reqParam, const void* optParam,
                                                    s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!reqParam || !reqId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    const auto* request = static_cast<const OrbisNpMatching2SignalingGetPingInfoRequest*>(reqParam);
    StoreRequestCallback(ctx, static_cast<const OrbisNpMatching2RequestOptParam*>(optParam));
    const OrbisNpMatching2RequestId request_id = AllocRequestId();
    *reqId = request_id;

    const bool room_found = ctx->room_cache.find(request->roomId) != ctx->room_cache.end();
    void* request_data =
        room_found ? BuildSignalingGetPingInfoPayload(*ctx, request->roomId) : nullptr;

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = ctxId;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = request_id;
    ev.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO;
    ev.error_code = room_found ? ORBIS_OK : ORBIS_NP_MATCHING2_ERROR_ROOM_NOT_FOUND;
    ev.request_cb = ctx->default_request_callback;
    ev.request_cb_arg = ctx->default_request_callback_arg;
    ev.request_data = request_data;
    ScheduleEvent(std::move(ev));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingCancelPeerNetInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionInfoA(OrbisNpMatching2ContextId ctxId,
                                                           OrbisNpMatching2RoomId roomId,
                                                           OrbisNpMatching2RoomMemberId memberId,
                                                           u32 infoType, void* connInfo) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);
    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    return FillMatching2ConnectionInfo(*ctx, roomId, memberId, infoType, connInfo, true);
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionInfo(OrbisNpMatching2ContextId ctxId,
                                                          OrbisNpMatching2RoomId roomId,
                                                          OrbisNpMatching2RoomMemberId memberId,
                                                          u32 infoType, void* connInfo) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);
    if (!g_initialized) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    ContextObject* ctx = ContextManager::Instance().Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    return FillMatching2ConnectionInfo(*ctx, roomId, memberId, infoType, connInfo, false);
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetLocalNetInfo(OrbisNpMatching2ContextId ctxId,
                                                        void* info) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetPeerNetInfoResult() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("10t3e5+JPnU", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2Initialize);
    LIB_FUNCTION("Mqp3lJ+sjy4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2Terminate);
    LIB_FUNCTION("YfmpW719rMo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContext);
    LIB_FUNCTION("ajvzc8e2upo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContextA);
    LIB_FUNCTION("V6KSpKv9XJE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateJoinRoomA);
    LIB_FUNCTION("zCWZmXXN600", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateJoinRoom);
    LIB_FUNCTION("CSIMDsVjs-g", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2JoinRoom);
    LIB_FUNCTION("fQQfP87I7hs", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterContextCallback);
    LIB_FUNCTION("4Nj7u5B5yCA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterLobbyEventCallback);
    LIB_FUNCTION("p+2EnxmaAMM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterRoomEventCallback);
    LIB_FUNCTION("0UMeWRGnZKA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterSignalingCallback);
    LIB_FUNCTION("7vjNQ6Z1op0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStart);
    LIB_FUNCTION("-f6M4caNe8k", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStop);
    LIB_FUNCTION("LhCPctIICxQ", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetServerId);
    LIB_FUNCTION("rJNPJqDCpiI", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetWorldInfoList);
    LIB_FUNCTION("BD6kfx442Do", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2LeaveRoom);
    LIB_FUNCTION("+8e7wXLmjds", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetDefaultRequestOptParam);
    LIB_FUNCTION("VqZX7POg2Mk", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SearchRoom);
    LIB_FUNCTION("Iw2h0Jrrb5U", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SendRoomMessage);
    LIB_FUNCTION("meEjIdbjAA0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetUserInfo);
    LIB_FUNCTION("q7GK98-nYSE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetRoomDataExternal);
    LIB_FUNCTION("S9D8JSYIrjE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetRoomDataInternal);
    LIB_FUNCTION("tHD5FPFXtu4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetConnectionStatus);
    LIB_FUNCTION("pFzhpCMlJXQ", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2AbortContextStart);
    LIB_FUNCTION("Nz-ZE7ur32I", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2DestroyContext);
    LIB_FUNCTION("DnPUsBAe8oI", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterLobbyMessageCallback);
    LIB_FUNCTION("uBESzz4CQws", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterRoomMessageCallback);
    LIB_FUNCTION("gpSAvdheZ0Q", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetMemoryInfo);
    LIB_FUNCTION("8btynvj0KNA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetSslMemoryInfo);
    LIB_FUNCTION("KC+GnHzrK2o", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomMemberIdListLocal);
    LIB_FUNCTION("vbtWT3lZBOM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomPasswordLocal);
    LIB_FUNCTION("cgQhq3E0eGo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetSignalingOptParamLocal);
    LIB_FUNCTION("wyvlEgZ-55w", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetLobbyInfoList);
    LIB_FUNCTION("1JtbJ0kxm3E", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetLobbyMemberDataInternal);
    LIB_FUNCTION("1Z4Xxumgm+Y", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetLobbyMemberDataInternalList);
    LIB_FUNCTION("26vWrPAWJfM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomDataExternalList);
    LIB_FUNCTION("Jraxifmoet4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomDataInternal);
    LIB_FUNCTION("dMQ+xGvTdqM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomMemberDataExternalList);
    LIB_FUNCTION("5lhvOqheFBA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetRoomMemberDataInternal);
    LIB_FUNCTION("GyI2f9yDUXM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetUserInfoListA);
    LIB_FUNCTION("qeF-q5KDtAc", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetUserInfoList);
    LIB_FUNCTION("NCP3bLGPt+o", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GrantRoomOwner);
    LIB_FUNCTION("n5JmImxTiZU", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2JoinLobby);
    LIB_FUNCTION("gQ6cUriNpgs", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2JoinRoomA);
    LIB_FUNCTION("AUVfU6byg3c", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2KickoutRoomMember);
    LIB_FUNCTION("BBbJ92uUdCg", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2LeaveLobby);
    LIB_FUNCTION("K+KtxhPsMZ4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SendLobbyChatMessage);
    LIB_FUNCTION("opDpl74pi2E", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SendRoomChatMessage);
    LIB_FUNCTION("ir2CzSs9K-g", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetLobbyMemberDataInternal);
    LIB_FUNCTION("HoqTrkS9c5Q", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetRoomMemberDataInternal);
    LIB_FUNCTION("ES3UMUWWj9U", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetSignalingOptParam);
    LIB_FUNCTION("8CqniKDzjvg", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetPeerNetInfo);
    LIB_FUNCTION("wUmwXZHaX1w", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetPingInfo);
    LIB_FUNCTION("GNSN5849fjU", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingCancelPeerNetInfo);
    LIB_FUNCTION("nNeC3F8-g+4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetConnectionInfoA);
    LIB_FUNCTION("twVupeaYYrk", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetConnectionInfo);
    LIB_FUNCTION("380EWm2DrVg", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetLocalNetInfo);
    LIB_FUNCTION("CTy4PBhpWDw", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetPeerNetInfoResult);
};

} // namespace Libraries::Np::NpMatching2
