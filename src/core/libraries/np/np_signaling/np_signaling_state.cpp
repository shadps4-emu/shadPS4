// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstring>
#include <mutex>
#include <random>
#include <string_view>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/net_util.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_stubs.h"

namespace Libraries::Np::NpSignaling {

using Libraries::Net::sceNetNtohs;

std::unordered_map<s32, NpSignalingContext> g_contexts;
std::unordered_map<s32, ConnectionInfo> g_connections;
std::unordered_map<CtxNpIdKey, s32, CtxNpIdKeyHash> g_npid_to_conn;
std::unordered_map<s32, PeerNetInfoResult> g_peer_netinfo_results;
std::vector<PendingActivation> g_pending_activations;
u32 g_peer_netinfo_next_id = 1;
u32 g_peak_connection_count = 0;
u32 g_last_assigned_context_id = 0;
u32 g_connection_id_seed = 0;
bool g_initialized = false;
Libraries::Kernel::PthreadMutexT g_mutex_storage{};

void InitSignalingMutex() {
    NpCommon::sceNpLwMutexInit(&g_mutex_storage, "SceNpSignalingLock", 1);
}
void DestroySignalingMutex() {
    NpCommon::sceNpLwMutexDestroy(&g_mutex_storage);
}

SignalingMutexGuard::SignalingMutexGuard() {
    NpCommon::sceNpLwMutexLock(&g_mutex_storage);
}
SignalingMutexGuard::~SignalingMutexGuard() {
    NpCommon::sceNpLwMutexUnlock(&g_mutex_storage);
}

std::multimap<std::chrono::steady_clock::time_point, QueuedDispatch> g_dispatch_queue;
std::mutex g_dispatch_mutex;
std::condition_variable g_dispatch_cv;
bool g_dispatch_stop = false;

NpCommon::OrbisNpCalloutContext g_callout_ctx{};
bool g_callout_ctx_active = false;

constexpr s64 kHandshakeConnectTimeoutMs = 30'000;
constexpr s64 kHandshakeRetransmitMs = 500;
constexpr s64 kKeepaliveIntervalMs = 45'000;
constexpr s64 kEstablishedLivenessTimeoutMs = 60'000;

static void PS4_SYSV_ABI TimeoutCalloutHandler(u64 arg);
static void PS4_SYSV_ABI StepCalloutHandler(u64 arg);

static void ArmTimeoutCalloutLocked(ConnectionInfo& ci, s64 delay_us) {
    if (!g_callout_ctx_active) {
        return;
    }
    if (ci.timeout_callout_armed) {
        u32 removed = 0;
        NpCommon::sceNpCalloutStopOnCtx(&g_callout_ctx, &ci.timeout_callout, &removed);
        ci.timeout_callout_armed = false;
    }
    NpCommon::sceNpCalloutStartOnCtx64(&g_callout_ctx, &ci.timeout_callout, delay_us,
                                       reinterpret_cast<u64>(&TimeoutCalloutHandler),
                                       static_cast<u64>(ci.conn_id));
    ci.timeout_callout_armed = true;
}

static void CancelTimeoutCalloutLocked(ConnectionInfo& ci) {
    if (ci.timeout_callout_armed) {
        u32 removed = 0;
        NpCommon::sceNpCalloutStopOnCtx(&g_callout_ctx, &ci.timeout_callout, &removed);
        ci.timeout_callout_armed = false;
    }
}

static void ArmStepCalloutLocked(ConnectionInfo& ci, s64 delay_us) {
    if (!g_callout_ctx_active) {
        return;
    }
    if (ci.step_callout_armed) {
        u32 removed = 0;
        NpCommon::sceNpCalloutStopOnCtx(&g_callout_ctx, &ci.step_callout, &removed);
        ci.step_callout_armed = false;
    }
    NpCommon::sceNpCalloutStartOnCtx64(&g_callout_ctx, &ci.step_callout, delay_us,
                                       reinterpret_cast<u64>(&StepCalloutHandler),
                                       static_cast<u64>(ci.conn_id));
    ci.step_callout_armed = true;
}

static void CancelStepCalloutLocked(ConnectionInfo& ci) {
    if (ci.step_callout_armed) {
        u32 removed = 0;
        NpCommon::sceNpCalloutStopOnCtx(&g_callout_ctx, &ci.step_callout, &removed);
        ci.step_callout_armed = false;
    }
}

static void CancelAllCalloutsLocked(ConnectionInfo& ci) {
    CancelTimeoutCalloutLocked(ci);
    CancelStepCalloutLocked(ci);
}

void ArmConnectTimeoutLocked(OrbisNpSignalingConnectionId conn_id) {
    const auto it = g_connections.find(conn_id);
    if (it == g_connections.end()) {
        return;
    }
    ArmTimeoutCalloutLocked(it->second, kHandshakeConnectTimeoutMs * 1000);
}

void ClearLingerAndTimeoutLocked(OrbisNpSignalingConnectionId conn_id) {
    const auto it = g_connections.find(conn_id);
    if (it == g_connections.end()) {
        return;
    }
    it->second.deactivate_lingering = false;
    CancelTimeoutCalloutLocked(it->second);
}

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
    const auto it = g_contexts.find(ctx_id);
    return it != g_contexts.end() && it->second.active;
}

s32 AllocateContextIdLocked() {
    u32 candidate = g_last_assigned_context_id + 1;
    if (g_last_assigned_context_id == 8) {
        candidate = 1;
    }
    for (int tried = 0; tried < 8; ++tried) {
        if (g_contexts.find(static_cast<s32>(candidate)) == g_contexts.end()) {
            g_last_assigned_context_id = candidate;
            return static_cast<s32>(candidate);
        }
        candidate = (candidate == 8) ? 1u : candidate + 1u;
    }
    return -1;
}

s32 AllocateConnectionIdLocked() {
    if (g_connection_id_seed == 0) {
        std::random_device rd;
        const u32 rnd = static_cast<u32>(rd());
        g_connection_id_seed = (rnd / 0x4001u) & 0x7fffu;
        if (g_connection_id_seed == 0) {
            g_connection_id_seed = 1;
        }
    }
    for (int tried = 0; tried <= 0xffff; ++tried) {
        const u32 candidate = g_connection_id_seed;
        g_connection_id_seed = g_connection_id_seed + 1;
        if (g_connection_id_seed == 0 || g_connection_id_seed > 0x7fff) {
            g_connection_id_seed = 1;
        }
        if (candidate != 0 &&
            g_connections.find(static_cast<s32>(candidate)) == g_connections.end()) {
            return static_cast<s32>(candidate);
        }
    }
    return -1;
}

void RemoveConnectionLocked(s32 conn_id) {
    const auto it = g_connections.find(conn_id);
    if (it == g_connections.end()) {
        return;
    }
    CancelAllCalloutsLocked(it->second);
    const auto map_key = MakeCtxNpIdKey(it->second.ctx_id, it->second.npid);
    auto key_it = g_npid_to_conn.find(map_key);
    if (key_it != g_npid_to_conn.end() && key_it->second == conn_id) {
        g_npid_to_conn.erase(key_it);
    }
    g_connections.erase(it);
}

void RemoveContextConnectionsLocked(s32 ctx_id) {
    std::vector<s32> to_remove;
    for (const auto& [cid, ci] : g_connections) {
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

s32 AverageProbeRtt(const ConnectionInfo& ci) {
    s64 sum = 0;
    s32 count = 0;
    for (const s32 sample : ci.probe_rtt_samples) {
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

s32 ProbeLossPercent(const ConnectionInfo& ci) {
    s32 zero_samples = 0;
    s32 present_samples = 0;
    for (const s32 sample : ci.probe_rtt_samples) {
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

    const auto it = g_connections.find(conn_id);
    if (it == g_connections.end() || it->second.ctx_id != ctx_id) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    const ConnectionInfo& ci = it->second;
    const s32 state = ConnectionStateFromStatus(ci.status);
    const bool established = state == 10;

    s32 result = ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;

    switch (info_code) {
    case ORBIS_NP_SIGNALING_CONN_INFO_RTT: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        const s32 avg = AverageProbeRtt(ci);
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
            *reinterpret_cast<u32*>(out_a) = ci.echo_derived_value;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = ci.echo_derived_value;
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
            *reinterpret_cast<u32*>(out_a) = ci.addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_a) + 4) = ci.port;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = ci.addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_b) + 4) = ci.port;
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_MAPPED_ADDR: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        if (out_a) {
            *reinterpret_cast<u32*>(out_a) = ci.local_addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_a) + 4) = ci.local_port;
        }
        if (out_b) {
            *reinterpret_cast<u32*>(out_b) = ci.local_addr;
            *reinterpret_cast<u16*>(reinterpret_cast<u8*>(out_b) + 4) = ci.local_port;
        }
        result = ORBIS_OK;
        break;
    }
    case ORBIS_NP_SIGNALING_CONN_INFO_PACKET_LOSS: {
        if (!established) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_IN_PROGRESS;
        }
        const s32 loss = ProbeLossPercent(ci);
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
            const auto ctx_it = g_contexts.find(ctx_id);
            if (ctx_it != g_contexts.end() &&
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
    for (const auto& [cid, ci] : g_connections) {
        const s32 state = ConnectionStateFromStatus(ci.status);
        if (state == 10) {
            ++established;
        } else if (state != 0) {
            ++transient;
        }
    }
    const u32 active = transient + established;
    if (active > g_peak_connection_count) {
        g_peak_connection_count = active;
    }
    if (out_peak) {
        *out_peak = g_peak_connection_count;
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
        kPeerNetInfoRequestIdPrefix | (g_peer_netinfo_next_id++);

    PeerNetInfoResult result{};
    result.ctx_id = ctx_id;
    result.conn_id = static_cast<OrbisNpSignalingConnectionId>(req_id);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    result.external_ipv4 = netinfo->GetExternalIp();
    result.nat_route_kind = static_cast<u32>(netinfo->GetNatType());
    g_peer_netinfo_results[result.conn_id] = result;
    return req_id;
}

bool TakePeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id,
                                 PeerNetInfoResult* out) {
    const auto it = g_peer_netinfo_results.find(req_or_conn_id);
    if (it == g_peer_netinfo_results.end() || it->second.ctx_id != ctx_id) {
        return false;
    }
    if (out) {
        *out = it->second;
    }
    g_peer_netinfo_results.erase(it);
    return true;
}

bool DropPeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id) {
    return TakePeerNetInfoResultLocked(ctx_id, req_or_conn_id, nullptr);
}

void RecordRttSampleLocked(OrbisNpSignalingConnectionId conn_id, s32 rtt_us) {
    const auto it = g_connections.find(conn_id);
    if (it == g_connections.end()) {
        return;
    }
    ConnectionInfo& ci = it->second;
    const u32 idx = ci.probe_sample_write_index % ConnectionInfo::kProbeSampleCount;
    ci.probe_rtt_samples[idx] = rtt_us;
    ci.probe_sample_write_index = idx + 1;
}

void SendEchoPings() {
    constexpr s64 kEchoIntervalUs = 1'000'000;

    struct PingTarget {
        s32 conn_id;
        u32 addr;
        u16 port;
        s64 now_us;
    };
    std::vector<PingTarget> targets;
    {
        SignalingMutexGuard lock;
        const s64 now = NowUs();
        for (auto& [cid, ci] : g_connections) {
            if (ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE || ci.addr == 0 ||
                ci.port == 0) {
                continue;
            }
            if (ci.last_echo_ping_us != 0 && now - ci.last_echo_ping_us < kEchoIntervalUs) {
                continue;
            }
            ci.last_echo_ping_us = now;
            targets.push_back({cid, ci.addr, ci.port, now});
        }
    }

    for (const PingTarget& t : targets) {
        SignalingEchoPing ping{};
        ping.conn_id = static_cast<u32>(t.conn_id);
        ping.send_ts_us = static_cast<u64>(t.now_us);
        Stubs::SignalingSendTo(&ping, sizeof(ping), t.addr, t.port);
    }
}

static s32 StatusFromState(ConnState state) {
    if (state == ConnState::Established) {
        return ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
    }
    if (state == ConnState::Inactive) {
        return ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    }
    return ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
}

static void SetConnStateLocked(ConnectionInfo& ci, ConnState new_state) {
    ci.state = new_state;
    ci.status = StatusFromState(new_state);
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
        std::lock_guard<std::mutex> dlock(g_dispatch_mutex);
        if (g_dispatch_stop) {
            return;
        }
        g_dispatch_queue.emplace(dispatch.fire_at, std::move(dispatch));
    }
    g_dispatch_cv.notify_all();
}

void DispatchConnectionEvent(s32 conn_id, s32 event_type, s32 event_data) {
    SignalingMutexGuard lock;
    const auto conn_it = g_connections.find(conn_id);
    if (conn_it == g_connections.end()) {
        return;
    }
    const s32 owner_ctx = conn_it->second.ctx_id;
    const auto ctx_it = g_contexts.find(owner_ctx);
    if (ctx_it != g_contexts.end() && ctx_it->second.active) {
        StageBasicCallbackLocked(ctx_it->second, owner_ctx, conn_id, event_type, event_data);
    }
}

void DispatchPeerActivatedEvent(s32 conn_id) {
    SignalingMutexGuard lock;
    const auto conn_it = g_connections.find(conn_id);
    if (conn_it == g_connections.end()) {
        return;
    }
    const s32 owner_ctx = conn_it->second.ctx_id;
    const auto owner_it = g_contexts.find(owner_ctx);
    const OrbisNpId local_npid =
        owner_it != g_contexts.end() ? owner_it->second.owner_npid : OrbisNpId{};
    for (auto& [cid, ctx] : g_contexts) {
        if (cid == owner_ctx || !ctx.active) {
            continue;
        }
        if (std::memcmp(&ctx.owner_npid, &local_npid, sizeof(local_npid)) == 0) {
            StageBasicCallbackLocked(ctx, cid, conn_id, ORBIS_NP_SIGNALING_EVENT_PEER_ACTIVATED, 0);
        }
    }
}

void CloseConnectionAndDispatchDead(s32 conn_id, s32 error_code) {
    bool fire = false;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        if (ci.state == ConnState::Inactive || ci.dead_fired) {
            return;
        }
        SetConnStateLocked(ci, ConnState::Inactive);
        ci.dead_fired = true;
        CancelAllCalloutsLocked(ci);
        fire = true;
    }
    if (fire) {
        LOG_DEBUG(Lib_NpSignaling, "Connection {} -> DEAD (errorCode={:#x})", conn_id,
                  static_cast<u32>(error_code));
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_DEAD, error_code);
        SignalingMutexGuard lock;
        RemoveConnectionLocked(conn_id);
    }
}

static SignalingControl MakeControlLocked(const ConnectionInfo& ci, ControlKind kind);

void EstablishConnection(s32 conn_id, bool peer_activated_hint) {
    bool first_establish = false;
    bool fire_established = false;
    bool fire_peer_activated = false;
    bool fire_mutual = false;
    SignalingControl established_pkt{};
    bool send_established = false;
    u32 peer_addr = 0;
    u16 peer_port = 0;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        if (peer_activated_hint) {
            ci.peer_activated = true;
        }

        if (!ci.established_fired) {
            ci.state = ConnState::Established;
            ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
            ci.established_fired = true;
            CancelTimeoutCalloutLocked(ci);
            ArmStepCalloutLocked(ci, kKeepaliveIntervalMs * 1000);
            ci.last_peer_rx_us = NowUs();
            first_establish = true;

            if (ci.addr != 0 && ci.port != 0) {
                established_pkt = MakeControlLocked(ci, ControlKind::Established);
                send_established = true;
                peer_addr = ci.addr;
                peer_port = ci.port;
            }

            if (!ci.locally_activated && !ci.peer_activated_fired) {
                ci.peer_activated_fired = true;
                fire_peer_activated = true;
            }
        }

        if (ci.locally_activated && ci.state == ConnState::Established &&
            !ci.established_event_fired) {
            ci.established_event_fired = true;
            fire_established = true;
        }

        if (ci.locally_activated && ci.state == ConnState::Established && ci.peer_established &&
            !ci.mutual_fired) {
            ci.mutual_fired = true;
            fire_mutual = true;
        }
    }

    if (send_established) {
        Stubs::ControlSendTo(&established_pkt, sizeof(established_pkt), peer_addr, peer_port);
    }
    if (first_establish) {
        LOG_DEBUG(Lib_NpSignaling, "Connection {} -> ESTABLISHED ({})", conn_id,
                  fire_established ? "owner" : "answerer/non-owner");
    }
    if (fire_peer_activated) {
        DispatchPeerActivatedEvent(conn_id);
    }
    if (fire_established) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_ESTABLISHED, 0);
    }
    if (fire_mutual) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_MUTUAL_ACTIVATED, 0);
    }
}

void DeactivateConnectionFaithful(s32 conn_id) {
    bool transient_close = false;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        if (ci.state == ConnState::Established && !ci.dead_fired && ci.deactivate_lingering) {
            return;
        }
        if (ci.state == ConnState::Established && !ci.dead_fired) {
            SendControlCloseLocked(ci, ControlReason::Deactivate);
            ci.locally_activated = false;
            ci.deactivate_lingering = true;
            CancelStepCalloutLocked(ci);
            ArmTimeoutCalloutLocked(ci, 60'000'000);
            LOG_DEBUG(Lib_NpSignaling,
                      "Connection {} deactivated (established) -> 60s keepalive linger before DEAD",
                      conn_id);
        } else if (!ci.dead_fired) {
            transient_close = true;
        }
    }
    if (transient_close) {
        CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
    }
}

void TerminateConnectionFaithful(s32 conn_id) {
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it != g_connections.end()) {
            SendControlCloseLocked(it->second, ControlReason::Terminate);
        }
    }
    CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
}

namespace {

SignalingHandshake MakeHandshakeLocked(const ConnectionInfo& ci, HandshakeKind kind) {
    SignalingHandshake pkt{};
    pkt.kind = static_cast<u8>(kind);
    pkt.from_conn_id = static_cast<u32>(ci.conn_id);
    pkt.to_conn_id = 0;
    const auto ctx_it = g_contexts.find(ci.ctx_id);
    if (ctx_it != g_contexts.end()) {
        std::memcpy(pkt.online_id_from, ctx_it->second.owner_online_id.data,
                    ORBIS_NP_ONLINEID_MAX_LENGTH);
    }
    std::memcpy(pkt.online_id_to, ci.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    pkt.mapped_addr = netinfo->GetExternalIp();
    pkt.mapped_port = 0;
    return pkt;
}

void SendHandshakeLocked(ConnectionInfo& ci, const SignalingHandshake& pkt) {
    if (ci.addr == 0 || ci.port == 0) {
        return;
    }
    ci.last_handshake_send_us = NowUs();
    const int sent = Stubs::SignalingSendTo(&pkt, sizeof(pkt), ci.addr, ci.port);
    LOG_DEBUG(Lib_NpSignaling, "Handshake[{}] SEND kind={} -> {:#x}:{} ({} bytes, rc={})",
              ci.conn_id, pkt.kind, ci.addr, sceNetNtohs(ci.port), sizeof(pkt), sent);
}

} // namespace

void StartHandshakeInitiator(OrbisNpSignalingConnectionId conn_id) {
    SignalingHandshake offer{};
    bool send = false;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        ci.is_initiator = true;
        SetConnStateLocked(ci, ConnState::SendingOffer);
        if (!ci.timeout_callout_armed) {
            ArmTimeoutCalloutLocked(ci, kHandshakeConnectTimeoutMs * 1000);
        }
        ArmStepCalloutLocked(ci, kHandshakeRetransmitMs * 1000);
        offer = MakeHandshakeLocked(ci, HandshakeKind::Offer);
        SendHandshakeLocked(ci, offer);
        send = true;
    }
    if (send) {
        LOG_DEBUG(Lib_NpSignaling, "Handshake[{}]: -> SENDING_OFFER", conn_id);
    }
}

void QueueActivationLocked(OrbisNpSignalingConnectionId conn_id, std::string_view peer_online_id,
                           bool start_handshake) {
    g_pending_activations.push_back({conn_id, std::string(peer_online_id), start_handshake});
}

void ProcessPendingActivations() {
    std::vector<PendingActivation> work;
    {
        SignalingMutexGuard lock;
        if (g_pending_activations.empty()) {
            return;
        }
        work.swap(g_pending_activations);
    }

    for (const PendingActivation& act : work) {
        bool already_established = false;
        {
            SignalingMutexGuard lock;
            const auto it = g_connections.find(act.conn_id);
            if (it == g_connections.end() || it->second.state == ConnState::Inactive) {
                continue;
            }
        }

        u32 peer_addr = 0;
        u16 peer_port = 0;
        const bool resolved = Stubs::ResolvePeer(act.peer_online_id, &peer_addr, &peer_port) &&
                              peer_addr != 0 && peer_port != 0;

        if (!resolved) {
            LOG_WARNING(Lib_NpSignaling, "peer '{}' endpoint unresolved; connection {} 30s timeout",
                        act.peer_online_id, act.conn_id);
            continue;
        }

        {
            SignalingMutexGuard lock;
            const auto it = g_connections.find(act.conn_id);
            if (it == g_connections.end() || it->second.state == ConnState::Inactive) {
                continue;
            }
            it->second.addr = peer_addr;
            it->second.port = peer_port;
            SendActivationRequestLocked(it->second);
            already_established = it->second.state == ConnState::Established;
            LOG_INFO(Lib_NpSignaling, "connection {} sent local activation to '{}' at {:#x}:{}{}",
                     act.conn_id, act.peer_online_id, peer_addr, sceNetNtohs(peer_port),
                     act.start_handshake ? "" : " (existing connection)");
        }
        if (act.start_handshake) {
            StartHandshakeInitiator(act.conn_id);
        } else if (already_established) {
            EstablishConnection(act.conn_id, false);
        }
    }
}

void HandleHandshakePacket(u32 from_addr, u16 from_port, const SignalingHandshake& pkt) {
    char from_id_buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
    std::memcpy(from_id_buf, pkt.online_id_from, ORBIS_NP_ONLINEID_MAX_LENGTH);
    const std::string from_id(from_id_buf);
    const HandshakeKind kind = static_cast<HandshakeKind>(pkt.kind);

    s32 establish_conn = 0;
    SignalingHandshake reply{};
    bool send_reply = false;
    s32 reply_conn = 0;

    {
        SignalingMutexGuard lock;

        s32 conn_id = 0;
        for (auto& [cid, ci] : g_connections) {
            if (ci.state == ConnState::Inactive || ci.dead_fired) {
                continue;
            }
            if (OnlineIdEqualsString(ci.online_id, from_id)) {
                conn_id = cid;
                break;
            }
        }

        if (conn_id != 0) {
            g_connections[conn_id].last_peer_rx_us = NowUs();
        }

        if (kind == HandshakeKind::Offer) {
            if (conn_id == 0) {
                return;
            }
            ConnectionInfo& ci = g_connections[conn_id];
            ci.addr = from_addr;
            ci.port = from_port;
            ci.peer_activated = true;
            if (pkt.mapped_addr != 0) {
                ci.addr = pkt.mapped_addr;
            }
            if (ci.state != ConnState::Established) {
                SetConnStateLocked(ci, ConnState::SendingAccept);
                if (!ci.timeout_callout_armed) {
                    ArmTimeoutCalloutLocked(ci, kHandshakeConnectTimeoutMs * 1000);
                }
                ArmStepCalloutLocked(ci, kHandshakeRetransmitMs * 1000);
                reply = MakeHandshakeLocked(ci, HandshakeKind::Accept);
                send_reply = true;
                reply_conn = conn_id;
            }
        } else if (conn_id == 0) {
            return;
        } else if (kind == HandshakeKind::Accept) {
            ConnectionInfo& ci = g_connections[conn_id];
            ci.peer_activated = true;
            if (pkt.mapped_addr != 0) {
                ci.addr = pkt.mapped_addr;
            } else {
                ci.addr = from_addr;
                ci.port = from_port;
            }
            if (ci.state == ConnState::SendingOffer || ci.state == ConnState::WaitAccept ||
                ci.state == ConnState::SendingAccept || ci.state == ConnState::WaitOffer) {
                SetConnStateLocked(ci, ConnState::ConnCheck);
                ArmStepCalloutLocked(ci, kHandshakeRetransmitMs * 1000);
                reply = MakeHandshakeLocked(ci, HandshakeKind::Check);
                reply.nonce = static_cast<u64>(NowUs());
                send_reply = true;
                reply_conn = conn_id;
            }
        } else if (kind == HandshakeKind::Check) {
            ConnectionInfo& ci = g_connections[conn_id];
            reply = MakeHandshakeLocked(ci, HandshakeKind::CheckAck);
            reply.nonce = pkt.nonce;
            send_reply = true;
            reply_conn = conn_id;
            if (ci.state == ConnState::SendingAccept || ci.state == ConnState::WaitOffer) {
                SetConnStateLocked(ci, ConnState::ConnCheck);
            }
        } else if (kind == HandshakeKind::CheckAck) {
            ConnectionInfo& ci = g_connections[conn_id];
            const s64 rtt_us = NowUs() - static_cast<s64>(pkt.nonce);
            if (rtt_us >= 0) {
                RecordRttSampleLocked(conn_id, static_cast<s32>(rtt_us));
            }
            if (ci.state != ConnState::Established) {
                establish_conn = conn_id;
            }
        }
    }

    if (send_reply) {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(reply_conn);
        if (it != g_connections.end()) {
            SendHandshakeLocked(it->second, reply);
        }
    }

    if (establish_conn != 0) {
        EstablishConnection(establish_conn, true);
    }
}

static void PS4_SYSV_ABI TimeoutCalloutHandler(u64 arg) {
    const s32 conn_id = static_cast<s32>(arg);
    bool fire_dead = false;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        ci.timeout_callout_armed = false;
        if (ci.state != ConnState::Established) {
            fire_dead = true;
        } else if (ci.deactivate_lingering) {
            fire_dead = true;
        }
    }
    if (fire_dead) {
        CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TIMEOUT);
    }
}

static void PS4_SYSV_ABI StepCalloutHandler(u64 arg) {
    const s32 conn_id = static_cast<s32>(arg);
    bool liveness_dead = false;
    {
        SignalingMutexGuard lock;
        const auto it = g_connections.find(conn_id);
        if (it == g_connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        ci.step_callout_armed = false;
        const s64 now_us = NowUs();

        if (ci.state == ConnState::Established) {
            if (!ci.deactivate_lingering && ci.addr != 0 && ci.port != 0) {
                if (ci.last_peer_rx_us != 0 &&
                    now_us - ci.last_peer_rx_us >= kEstablishedLivenessTimeoutMs * 1000) {
                    liveness_dead = true;
                } else {
                    SignalingHandshake pkt = MakeHandshakeLocked(ci, HandshakeKind::Check);
                    pkt.nonce = static_cast<u64>(now_us);
                    SendHandshakeLocked(ci, pkt);
                    ArmStepCalloutLocked(ci, kKeepaliveIntervalMs * 1000);
                }
            }
        } else if (ci.state != ConnState::Inactive) {
            if (ci.addr != 0 && ci.port != 0) {
                HandshakeKind kind = HandshakeKind::Offer;
                switch (ci.state) {
                case ConnState::SendingOffer:
                case ConnState::WaitAccept:
                    kind = HandshakeKind::Offer;
                    break;
                case ConnState::SendingAccept:
                case ConnState::WaitOffer:
                    kind = HandshakeKind::Accept;
                    break;
                case ConnState::ConnCheck:
                    kind = HandshakeKind::Check;
                    break;
                default:
                    break;
                }
                SignalingHandshake pkt = MakeHandshakeLocked(ci, kind);
                if (kind == HandshakeKind::Check) {
                    pkt.nonce = static_cast<u64>(now_us);
                }
                SendHandshakeLocked(ci, pkt);
            }
            ArmStepCalloutLocked(ci, kHandshakeRetransmitMs * 1000);
        }
    }
    if (liveness_dead) {
        CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TIMEOUT);
    }
}

static SignalingControl MakeControlLocked(const ConnectionInfo& ci, ControlKind kind) {
    SignalingControl pkt{};
    pkt.kind = static_cast<u8>(kind);
    pkt.from_conn_id = static_cast<u32>(ci.conn_id);
    const auto ctx_it = g_contexts.find(ci.ctx_id);
    if (ctx_it != g_contexts.end()) {
        std::memcpy(pkt.online_id_from, ctx_it->second.owner_online_id.data,
                    ORBIS_NP_ONLINEID_MAX_LENGTH);
    }
    std::memcpy(pkt.online_id_to, ci.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    pkt.mapped_addr = netinfo->GetExternalIp();
    pkt.mapped_port = 0;
    return pkt;
}

void SendActivationRequestLocked(const ConnectionInfo& ci) {
    if (ci.addr == 0 || ci.port == 0) {
        return;
    }
    SignalingControl pkt = MakeControlLocked(ci, ControlKind::ActivationRequest);
    Stubs::ControlSendTo(&pkt, sizeof(pkt), ci.addr, ci.port);
}

void SendControlCloseLocked(const ConnectionInfo& ci, ControlReason reason) {
    if (ci.addr == 0 || ci.port == 0) {
        LOG_WARNING(Lib_NpSignaling,
                    "Control Close SEND skipped: connId={} reason={} unresolved endpoint",
                    ci.conn_id, static_cast<u16>(reason));
        return;
    }
    SignalingControl pkt = MakeControlLocked(ci, ControlKind::Close);
    pkt.reason = static_cast<u16>(reason);
    const int rc = Stubs::ControlSendTo(&pkt, sizeof(pkt), ci.addr, ci.port);
    LOG_INFO(Lib_NpSignaling, "Control Close SEND: connId={} reason={} dst={:#x}:{} rc={}",
             ci.conn_id, static_cast<u16>(reason), ci.addr, sceNetNtohs(ci.port), rc);
}

void HandleControlPacket(u32 from_addr, u16 from_port, const SignalingControl& pkt) {
    char from_id_buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
    std::memcpy(from_id_buf, pkt.online_id_from, ORBIS_NP_ONLINEID_MAX_LENGTH);
    const std::string from_id(from_id_buf);
    const ControlKind kind = static_cast<ControlKind>(pkt.kind);

    s32 start_handshake_conn = 0;
    s32 establish_conn = 0;
    s32 dead_conn = 0;
    s32 peer_deactivated_conn = 0;
    SignalingControl ack{};
    bool send_ack = false;
    u32 ack_addr = 0;
    u16 ack_port = 0;

    {
        SignalingMutexGuard lock;

        s32 conn_id = 0;
        for (auto& [cid, ci] : g_connections) {
            if (ci.state == ConnState::Inactive || ci.dead_fired) {
                continue;
            }
            if (OnlineIdEqualsString(ci.online_id, from_id)) {
                conn_id = cid;
                break;
            }
        }

        if (kind == ControlKind::ActivationRequest) {
            if (conn_id == 0) {
                s32 ctx_id = 0;
                char to_id_buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
                std::memcpy(to_id_buf, pkt.online_id_to, ORBIS_NP_ONLINEID_MAX_LENGTH);
                const std::string to_id(to_id_buf);
                for (auto& [cid, ctx] : g_contexts) {
                    if (ctx.active && OnlineIdEqualsString(ctx.owner_online_id, to_id)) {
                        ctx_id = cid;
                        break;
                    }
                }
                if (ctx_id == 0) {
                    return;
                }
                conn_id = AllocateConnectionIdLocked();
                if (conn_id < 0) {
                    return;
                }
                ConnectionInfo ci{};
                ci.conn_id = conn_id;
                ci.ctx_id = ctx_id;
                ci.locally_activated = false;
                SetNpOnlineId(ci.online_id,
                              std::string_view(reinterpret_cast<const char*>(pkt.online_id_from),
                                               ORBIS_NP_ONLINEID_MAX_LENGTH));
                ci.npid.handle = ci.online_id;
                g_connections[conn_id] = std::move(ci);
                g_npid_to_conn[MakeCtxNpIdKey(ctx_id, g_connections[conn_id].npid)] = conn_id;
                LOG_INFO(Lib_NpSignaling,
                         "Control: ActivationRequest from '{}' -> created answerer connection {}",
                         from_id, conn_id);
            }
            ConnectionInfo& ci = g_connections[conn_id];
            ci.peer_activated = true;
            ci.addr = pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr;
            ci.port = from_port;
            if (ci.state == ConnState::Inactive) {
                if (!ci.timeout_callout_armed) {
                    ArmTimeoutCalloutLocked(ci, kHandshakeConnectTimeoutMs * 1000);
                }
                start_handshake_conn = conn_id;
            }
            ack = MakeControlLocked(ci, ControlKind::ActivationAck);
            send_ack = true;
            ack_addr = ci.addr;
            ack_port = ci.port;
        } else if (conn_id == 0) {
            LOG_INFO(Lib_NpSignaling,
                     "Control RECV ignored: kind={} from='{}' src={:#x}:{} no matching connection",
                     pkt.kind, from_id, from_addr, sceNetNtohs(from_port));
            return;
        } else if (kind == ControlKind::ActivationAck) {
            ConnectionInfo& ci = g_connections[conn_id];
            ci.peer_activated = true;
            if (pkt.mapped_addr != 0) {
                ci.addr = pkt.mapped_addr;
            }
        } else if (kind == ControlKind::Established) {
            ConnectionInfo& ci = g_connections[conn_id];
            ci.peer_established = true;
            if (ci.state == ConnState::Established) {
                establish_conn = conn_id;
            }
        } else if (kind == ControlKind::Close) {
            ConnectionInfo& ci = g_connections[conn_id];
            const ControlReason reason = static_cast<ControlReason>(pkt.reason);
            LOG_INFO(Lib_NpSignaling,
                     "Control Close RECV: connId={} from='{}' reason={} src={:#x}:{} state={}",
                     conn_id, from_id, pkt.reason, from_addr, sceNetNtohs(from_port),
                     static_cast<s32>(ci.state));
            if (reason == ControlReason::Deactivate && ci.state == ConnState::Established) {
                peer_deactivated_conn = conn_id;
            } else {
                dead_conn = conn_id;
            }
        }
    }

    if (send_ack && ack_addr != 0 && ack_port != 0) {
        Stubs::ControlSendTo(&ack, sizeof(ack), ack_addr, ack_port);
    }
    if (start_handshake_conn != 0) {
        StartHandshakeInitiator(start_handshake_conn);
    }
    if (establish_conn != 0) {
        EstablishConnection(establish_conn, false);
    }
    if (peer_deactivated_conn != 0) {
        DispatchConnectionEvent(peer_deactivated_conn, ORBIS_NP_SIGNALING_EVENT_PEER_DEACTIVATED,
                                0);
    }
    if (dead_conn != 0) {
        CloseConnectionAndDispatchDead(dead_conn, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_PEER);
    }
}

void SendStunPing(s32 ctx_id) {
    OrbisNpOnlineId online_id{};

    {
        SignalingMutexGuard lock;
        const auto it = g_contexts.find(ctx_id);
        if (it == g_contexts.end() || !it->second.active) {
            return;
        }
        online_id = it->second.owner_online_id;
    }

    if (!Stubs::EnsureTransport()) {
        return;
    }

    const u32 server_addr = Stubs::MmServerAddr();
    const u16 server_udp = Stubs::MmServerUdpPort();

    if (server_addr == 0 || server_udp == 0) {
        LOG_WARNING(Lib_NpSignaling, "ctxId={} skipped (server_addr={:#x} udp_port={})", ctx_id,
                    server_addr, sceNetNtohs(server_udp));
        return;
    }

    StunPing ping{};
    ping.cmd = 0x01;
    std::memcpy(ping.online_id, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    ping.local_ip = Stubs::AdvertisedAddr();

    LOG_DEBUG(Lib_NpSignaling, "ctxId={} online_id='{}' server={:#x}:{} local_ip={:#x}", ctx_id,
              OnlineIdToString(online_id), server_addr, sceNetNtohs(server_udp), ping.local_ip);

    Stubs::SignalingSendTo(&ping, sizeof(ping), server_addr, server_udp);
}

s32 GetActiveConnectionIdForPeer(std::string_view online_id) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : g_connections) {
        if (OnlineIdEqualsString(ci.online_id, online_id) &&
            ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE) {
            return conn_id;
        }
    }
    return 0;
}

s32 GetConnectionStatusForPeer(std::string_view online_id, s32* out_conn_id) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : g_connections) {
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

void ClearActiveDeadlines() {}
void SetRoomLeft() {}
void ClearConnectionDeadlineForPeer(std::string_view) {}

bool GetPeerAddress(std::string_view online_id, u32* out_addr, u16* out_port) {
    SignalingMutexGuard lock;
    for (const auto& [conn_id, ci] : g_connections) {
        if (OnlineIdEqualsString(ci.online_id, online_id)) {
            if (out_addr)
                *out_addr = ci.addr;
            if (out_port)
                *out_port = ci.port;
            return true;
        }
    }
    return false;
}

} // namespace Libraries::Np::NpSignaling
