// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <string_view>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_util.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"

namespace Libraries::Np::NpSignaling {

using Libraries::Net::sceNetNtohs;

namespace {

std::string HexBytes(const void* data, size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    const auto* bytes = static_cast<const u8*>(data);
    std::string result;
    result.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(kHex[bytes[i] >> 4]);
        result.push_back(kHex[bytes[i] & 0xf]);
    }
    return result;
}

std::string PrintableOnlineId(const OrbisNpOnlineId& online_id) {
    std::string result;
    result.reserve(ORBIS_NP_ONLINEID_MAX_LENGTH);
    for (const char value : online_id.data) {
        const auto byte = static_cast<unsigned char>(value);
        result.push_back(std::isprint(byte) ? value : '.');
    }
    return result;
}

const char* OnlineIdValidationFailure(const OrbisNpOnlineId& online_id) {
    if (online_id.term != 0) {
        return "term_nonzero";
    }

    const size_t len = strnlen(online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH + 1);
    if (len < 3) {
        return "length_below_minimum";
    }
    if (len > ORBIS_NP_ONLINEID_MAX_LENGTH) {
        return "length_above_maximum";
    }
    if (!std::isalnum(static_cast<unsigned char>(online_id.data[0]))) {
        return "first_character_invalid";
    }
    for (size_t i = 1; i < len; ++i) {
        const auto value = static_cast<unsigned char>(online_id.data[i]);
        if (!std::isalnum(value) && value != '_' && value != '-') {
            return "character_invalid";
        }
    }
    return "validator_rejected";
}

} // namespace

SignalingMutexGuard::SignalingMutexGuard()
    : lock(NpHandler::GetInstance().GetSignalingState().mutex) {}

long long NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

s64 NowUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::string OnlineIdToString(const OrbisNpOnlineId& online_id_in) {
    std::string online_id(online_id_in.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    const auto nul = online_id.find('\0');
    if (nul != std::string::npos) {
        online_id.resize(nul);
    }
    return online_id;
}

std::string OnlineIdFromNpId(const OrbisNpId& np_id) {
    return OnlineIdToString(np_id.handle);
}

OrbisNpId NpIdFromOnlineId(const OrbisNpOnlineId& online_id) {
    OrbisNpId npid{};
    npid.handle = online_id;
    return npid;
}

bool OnlineIdEqualsString(const OrbisNpOnlineId& online_id, std::string_view value) {
    return OnlineIdToString(online_id) == value;
}

bool IsValidNpId(const OrbisNpId& np_id) {
    return NpCommon::sceNpIntIsValidOnlineId(&np_id.handle) != 0;
}

s32 NormalizeNpId(const void* np_id, OrbisNpId* out_npid, OrbisNpOnlineId* out_online_id) {
    if (!np_id || !out_npid) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    std::memcpy(out_npid, np_id, sizeof(*out_npid));
    if (!IsValidNpId(*out_npid)) {
        const OrbisNpOnlineId& handle = out_npid->handle;
        LOG_INFO(Lib_NpSignaling,
                 "rejected: reason={} handle_ascii='{}' handle_hex={} term={:#04x} "
                 "dummy_hex={} opt_hex={} reserved_hex={} raw_hex={}",
                 OnlineIdValidationFailure(handle), PrintableOnlineId(handle),
                 HexBytes(handle.data, sizeof(handle.data)), static_cast<u8>(handle.term),
                 HexBytes(handle.dummy, sizeof(handle.dummy)),
                 HexBytes(out_npid->opt, sizeof(out_npid->opt)),
                 HexBytes(out_npid->reserved, sizeof(out_npid->reserved)),
                 HexBytes(out_npid, sizeof(*out_npid)));
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (out_online_id) {
        *out_online_id = out_npid->handle;
    }
    return ORBIS_OK;
}

CtxNpIdKey MakeCtxNpIdKey(s32 ctx_id, const OrbisNpId& npid) {
    CtxNpIdKey key{};
    key.ctx_id = ctx_id;
    key.npid = npid;
    return key;
}

bool IsContextValidLocked(s32 ctx_id) {
    const auto it = NpHandler::GetInstance().GetSignalingState().contexts.find(ctx_id);
    return it != NpHandler::GetInstance().GetSignalingState().contexts.end() && it->second.active;
}

s32 AllocateContextIdLocked() {
    u32 candidate = NpHandler::GetInstance().GetSignalingState().last_assigned_context_id + 1;
    if (NpHandler::GetInstance().GetSignalingState().last_assigned_context_id == 8) {
        candidate = 1;
    }
    for (int tried = 0; tried < 8; ++tried) {
        if (NpHandler::GetInstance().GetSignalingState().contexts.find(static_cast<s32>(
                candidate)) == NpHandler::GetInstance().GetSignalingState().contexts.end()) {
            NpHandler::GetInstance().GetSignalingState().last_assigned_context_id = candidate;
            return static_cast<s32>(candidate);
        }
        candidate = (candidate == 8) ? 1u : candidate + 1u;
    }
    return -1;
}

s32 AllocateConnectionIdLocked() {
    if (NpHandler::GetInstance().GetSignalingState().connection_id_seed == 0) {
        std::random_device rd;
        const u32 rnd = static_cast<u32>(rd());
        NpHandler::GetInstance().GetSignalingState().connection_id_seed = (rnd / 0x4001u) & 0x7fffu;
        if (NpHandler::GetInstance().GetSignalingState().connection_id_seed == 0) {
            NpHandler::GetInstance().GetSignalingState().connection_id_seed = 1;
        }
    }
    for (int tried = 0; tried <= 0xffff; ++tried) {
        const u32 candidate = NpHandler::GetInstance().GetSignalingState().connection_id_seed;
        NpHandler::GetInstance().GetSignalingState().connection_id_seed =
            NpHandler::GetInstance().GetSignalingState().connection_id_seed + 1;
        if (NpHandler::GetInstance().GetSignalingState().connection_id_seed == 0 ||
            NpHandler::GetInstance().GetSignalingState().connection_id_seed > 0x7fff) {
            NpHandler::GetInstance().GetSignalingState().connection_id_seed = 1;
        }
        if (candidate != 0 && NpHandler::GetInstance().GetSignalingState().connections.find(
                                  static_cast<s32>(candidate)) ==
                                  NpHandler::GetInstance().GetSignalingState().connections.end()) {
            return static_cast<s32>(candidate);
        }
    }
    return -1;
}

void RemoveConnectionLocked(s32 conn_id) {
    const auto it = NpHandler::GetInstance().GetSignalingState().connections.find(conn_id);
    if (it == NpHandler::GetInstance().GetSignalingState().connections.end()) {
        return;
    }
    const auto map_key = MakeCtxNpIdKey(it->second.ctx_id, it->second.npid);
    auto key_it = NpHandler::GetInstance().GetSignalingState().npid_to_conn.find(map_key);
    if (key_it != NpHandler::GetInstance().GetSignalingState().npid_to_conn.end() &&
        key_it->second == conn_id) {
        NpHandler::GetInstance().GetSignalingState().npid_to_conn.erase(key_it);
    }
    NpHandler::GetInstance().GetSignalingState().connections.erase(it);
}

void RemoveContextConnectionsLocked(s32 ctx_id) {
    std::vector<s32> to_remove;
    for (const auto& [cid, ci] : NpHandler::GetInstance().GetSignalingState().connections) {
        if (ci.ctx_id == ctx_id) {
            to_remove.push_back(cid);
        }
    }
    for (s32 cid : to_remove) {
        RemoveConnectionLocked(cid);
    }
}

const char* SignalingEventName(u32 event_type) {
    switch (event_type) {
    case ORBIS_NP_SIGNALING_EVENT_DEAD:
        return "DEAD";
    case ORBIS_NP_SIGNALING_EVENT_ESTABLISHED:
        return "ESTABLISHED";
    case ORBIS_NP_SIGNALING_EVENT_NETINFO_ERROR:
        return "NETINFO_ERROR";
    case ORBIS_NP_SIGNALING_EVENT_NETINFO_RESULT:
        return "NETINFO_RESULT";
    case ORBIS_NP_SIGNALING_EVENT_PEER_ACTIVATED:
        return "PEER_ACTIVATED";
    case ORBIS_NP_SIGNALING_EVENT_PEER_DEACTIVATED:
        return "PEER_DEACTIVATED";
    case ORBIS_NP_SIGNALING_EVENT_MUTUAL_ACTIVATED:
        return "MUTUAL_ACTIVATED";
    default:
        return "UNKNOWN";
    }
}

bool ConsumeActivationBudgetLocked(NpSignalingContext& ctx) {
    const s64 now = NowUs();

    s64 budget = ctx.activate_budget_us;
    if (ctx.activate_last_update_us == 0) {
        budget = kActivateBudgetMaxUs;
    } else {
        const s64 elapsed = std::max<s64>(0, now - ctx.activate_last_update_us);
        budget = std::min<s64>(kActivateBudgetMaxUs, budget + elapsed);
    }

    ctx.activate_last_update_us = now;
    ctx.activate_budget_us = budget;

    if (budget < kActivateCooldownUs) {
        return false;
    }

    ctx.activate_budget_us = budget - kActivateCooldownUs;
    return true;
}

bool ConsumeActivationBudgetGatedLocked(NpSignalingContext& ctx) {
    if (ctx.compiled_sdk_version <= 0x16fffff) {
        return true;
    }
    return ConsumeActivationBudgetLocked(ctx);
}

s32 ConnectionStateFromStatus(s32 status) {
    switch (status) {
    case ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE:
        return 10;
    case ORBIS_NP_SIGNALING_CONN_STATUS_PENDING:
        return 1;
    default:
        return 0;
    }
}

u32 CaptureCompiledSdkVersion() {
    s32 ver = 0;
    if (Libraries::Kernel::sceKernelGetCompiledSdkVersion(&ver) < 0) {
        return 0;
    }
    return static_cast<u32>(ver);
}

u32 ParseIpv4Nbo(const std::string& dotted) {
    u32 octets[4] = {0, 0, 0, 0};
    int idx = 0;
    u32 cur = 0;
    bool any_digit = false;
    for (const char c : dotted) {
        if (c >= '0' && c <= '9') {
            cur = cur * 10 + static_cast<u32>(c - '0');
            if (cur > 255) {
                return 0;
            }
            any_digit = true;
        } else if (c == '.') {
            if (!any_digit || idx >= 3) {
                return 0;
            }
            octets[idx++] = cur;
            cur = 0;
            any_digit = false;
        } else {
            return 0;
        }
    }
    if (!any_digit || idx != 3) {
        return 0;
    }
    octets[3] = cur;
    return octets[0] | (octets[1] << 8) | (octets[2] << 16) | (octets[3] << 24);
}

namespace {

s32 AverageProbeRtt(const PeerTransport& transport) {
    s64 sum = 0;
    s32 count = 0;
    for (const s32 sample : transport.probe_rtt_samples) {
        if (sample > 0) {
            sum += sample;
            ++count;
        }
    }
    if (count == 0) {
        return 0;
    }
    return static_cast<s32>(sum / count);
}

s32 ProbeLossPercent(const PeerTransport& transport) {
    s32 zero_samples = 0;
    s32 present_samples = 0;
    for (const s32 sample : transport.probe_rtt_samples) {
        if (sample != -1) {
            ++present_samples;
            if (sample == 0) {
                ++zero_samples;
            }
        }
    }
    if (present_samples == 0) {
        return 0;
    }
    return static_cast<s16>((zero_samples * 100) / present_samples);
}

} // namespace

s32 GetConnectionInfoInternal(s32 ctx_id, s32 conn_id, s32 info_code, void* out_a, void* out_b) {
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = NpHandler::GetInstance().GetSignalingState().connections.find(conn_id);
    if (it == NpHandler::GetInstance().GetSignalingState().connections.end() ||
        it->second.ctx_id != ctx_id) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    const ConnectionInfo& ci = it->second;
    const auto transport_it =
        NpHandler::GetInstance().GetSignalingState().peer_transports.find(ci.transport_id);
    const PeerTransport* transport =
        transport_it != NpHandler::GetInstance().GetSignalingState().peer_transports.end()
            ? &transport_it->second
            : nullptr;
    const s32 state = ConnectionStateFromStatus(ci.status);
    const bool established = state == 10;
    if (established && !transport) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    s32 result = ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;

    switch (info_code) {
    case ORBIS_NP_SIGNALING_CONN_INFO_RTT: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        const s32 avg = AverageProbeRtt(*transport);
        if (out_a) {
            *reinterpret_cast<s32*>(out_a) = avg;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = static_cast<u32>(avg);
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_BANDWIDTH: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        if (out_a) {
            *reinterpret_cast<u32*>(out_a) = 0;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = 0;
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_PEER_NP_ID: {
        if (out_a) {
            std::memcpy(out_a, &ci.npid, sizeof(ci.npid));
            result = ORBIS_OK;
        } else {
            result = ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
        }
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDR: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        if (out_a) {
            *reinterpret_cast<u32*>(out_a) = transport->addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_a) + 4) = transport->port;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = transport->addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_b) + 4) = transport->port;
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_MAPPED_ADDR: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        if (out_a) {
            *reinterpret_cast<u32*>(out_a) = transport->local_addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_a) + 4) = transport->local_port;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = transport->local_addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_b) + 4) = transport->local_port;
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_PACKET_LOSS: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        const s32 loss = ProbeLossPercent(*transport);
        if (out_a) {
            *reinterpret_cast<s32*>(out_a) = loss;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = static_cast<u32>(loss);
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDRESS_A: {
        if (out_a) {
            result = ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
            break;
        }
        if (out_b) {
            const auto ctx_it = NpHandler::GetInstance().GetSignalingState().contexts.find(ctx_id);
            if (ctx_it != NpHandler::GetInstance().GetSignalingState().contexts.end() &&
                (ctx_it->second.account_id != 0 || ctx_it->second.platform_type != 0)) {
                auto* pair = reinterpret_cast<OrbisNpSignalingAccountPlatformPair*>(out_b);
                pair->accountId = ctx_it->second.account_id;
                pair->platformType = ctx_it->second.platform_type;
                pair->_pad_0c = 0;
                result = ORBIS_OK;
            } else {
                result = ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
            }
        }
        break;
    }
    default:
        result = ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
        break;
    }

    return result;
}

void SnapshotConnectionStatisticsLocked(u32* out_peak, u32* out_active, u32* out_transient,
                                        u32* out_established) {
    u32 transient = 0;
    u32 established = 0;
    for (const auto& [cid, ci] : NpHandler::GetInstance().GetSignalingState().connections) {
        const s32 state = ConnectionStateFromStatus(ci.status);
        if (state == 10) {
            ++established;
        } else if (state != 0) {
            ++transient;
        }
    }
    const u32 active = transient + established;
    if (active > NpHandler::GetInstance().GetSignalingState().peak_connection_count) {
        NpHandler::GetInstance().GetSignalingState().peak_connection_count = active;
    }
    if (out_peak) {
        *out_peak = NpHandler::GetInstance().GetSignalingState().peak_connection_count;
    }
    if (out_active) {
        *out_active = active;
    }
    if (out_transient) {
        *out_transient = transient;
    }
    if (out_established) {
        *out_established = established;
    }
}

OrbisNpSignalingRequestId StagePeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id) {
    const OrbisNpSignalingRequestId req_id =
        kPeerNetInfoRequestIdPrefix |
        (NpHandler::GetInstance().GetSignalingState().peer_netinfo_next_id++);

    PeerNetInfoResult result{};
    result.ctx_id = ctx_id;
    result.conn_id = static_cast<OrbisNpSignalingConnectionId>(req_id);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    result.external_ipv4 = netinfo->GetExternalIp();
    result.nat_route_kind = static_cast<u32>(netinfo->GetNatType());
    NpHandler::GetInstance().GetSignalingState().peer_netinfo_results[result.conn_id] = result;
    return req_id;
}

bool TakePeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id,
                                 PeerNetInfoResult* out) {
    const auto it =
        NpHandler::GetInstance().GetSignalingState().peer_netinfo_results.find(req_or_conn_id);
    if (it == NpHandler::GetInstance().GetSignalingState().peer_netinfo_results.end() ||
        it->second.ctx_id != ctx_id) {
        return false;
    }
    if (out) {
        *out = it->second;
    }
    NpHandler::GetInstance().GetSignalingState().peer_netinfo_results.erase(it);
    return true;
}

bool DropPeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id) {
    return TakePeerNetInfoResultLocked(ctx_id, req_or_conn_id, nullptr);
}

static void StageBasicCallbackLocked(const NpSignalingContext& ctx, s32 ctx_id, s32 conn_id,
                                     s32 event_type, s32 event_data) {
    if (!ctx.callback) {
        return;
    }
    QueuedDispatch dispatch;
    dispatch.fire_at = std::chrono::steady_clock::now();
    dispatch.delay_ms = 0;
    dispatch.ctx_id = ctx_id;
    dispatch.conn_id = conn_id;
    dispatch.event_type = static_cast<u32>(event_type);
    dispatch.error_code = static_cast<u32>(event_data);
    dispatch.callback = ctx.callback;
    dispatch.callback_arg = ctx.callback_arg;
    {
        std::lock_guard<std::mutex> dlock(
            NpHandler::GetInstance().GetSignalingState().dispatch_mutex);
        if (NpHandler::GetInstance().GetSignalingState().dispatch_stop) {
            return;
        }
        NpHandler::GetInstance().GetSignalingState().dispatch_queue.emplace(dispatch.fire_at,
                                                                            std::move(dispatch));
    }
    NpHandler::GetInstance().GetSignalingState().dispatch_cv.notify_all();
}

void DispatchConnectionEvent(s32 conn_id, s32 event_type, s32 event_data) {
    SignalingMutexGuard lock;
    const auto conn_it = NpHandler::GetInstance().GetSignalingState().connections.find(conn_id);
    if (conn_it == NpHandler::GetInstance().GetSignalingState().connections.end()) {
        return;
    }
    const s32 owner_ctx = conn_it->second.ctx_id;
    const auto ctx_it = NpHandler::GetInstance().GetSignalingState().contexts.find(owner_ctx);
    if (ctx_it != NpHandler::GetInstance().GetSignalingState().contexts.end() &&
        ctx_it->second.active) {
        StageBasicCallbackLocked(ctx_it->second, owner_ctx, conn_id, event_type, event_data);
    }
}

void DispatchPeerActivatedEvent(s32 conn_id) {
    SignalingMutexGuard lock;
    const auto conn_it = NpHandler::GetInstance().GetSignalingState().connections.find(conn_id);
    if (conn_it == NpHandler::GetInstance().GetSignalingState().connections.end()) {
        return;
    }
    const s32 owner_ctx = conn_it->second.ctx_id;
    const auto owner_it = NpHandler::GetInstance().GetSignalingState().contexts.find(owner_ctx);
    const OrbisNpId local_npid =
        owner_it != NpHandler::GetInstance().GetSignalingState().contexts.end()
            ? owner_it->second.owner_npid
            : OrbisNpId{};
    for (auto& [cid, ctx] : NpHandler::GetInstance().GetSignalingState().contexts) {
        if (cid == owner_ctx || !ctx.active) {
            continue;
        }
        if (std::memcmp(&ctx.owner_npid, &local_npid, sizeof(local_npid)) == 0) {
            StageBasicCallbackLocked(ctx, cid, conn_id, ORBIS_NP_SIGNALING_EVENT_PEER_ACTIVATED, 0);
        }
    }
}

s32 GetActiveConnectionIdForPeer(std::string_view online_id) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : NpHandler::GetInstance().GetSignalingState().connections) {
        if (OnlineIdEqualsString(ci.online_id, online_id) &&
            ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE) {
            return conn_id;
        }
    }
    return 0;
}

s32 GetConnectionStatusForPeer(std::string_view online_id, s32* out_conn_id) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : NpHandler::GetInstance().GetSignalingState().connections) {
        if (OnlineIdEqualsString(ci.online_id, online_id)) {
            if (out_conn_id) {
                *out_conn_id = conn_id;
            }
            return ci.status;
        }
    }
    if (out_conn_id) {
        *out_conn_id = 0;
    }
    return ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
}

bool GetPeerAddress(std::string_view online_id, u32* out_addr, u16* out_port) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : NpHandler::GetInstance().GetSignalingState().connections) {
        if (OnlineIdEqualsString(ci.online_id, online_id)) {
            const auto transport_it =
                NpHandler::GetInstance().GetSignalingState().peer_transports.find(ci.transport_id);
            if (transport_it ==
                NpHandler::GetInstance().GetSignalingState().peer_transports.end()) {
                return false;
            }
            if (out_addr)
                *out_addr = transport_it->second.addr;
            if (out_port)
                *out_port = transport_it->second.port;
            return true;
        }
    }
    return false;
}

} // namespace Libraries::Np::NpSignaling
