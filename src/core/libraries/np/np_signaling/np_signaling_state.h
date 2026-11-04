// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_signaling/np_signaling.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpSignaling {

constexpr s64 kActivateCooldownUs = 6'000'000;
constexpr s64 kActivateBudgetMaxUs = 600'000'000;
constexpr u32 kSigRetryMs = 500;
constexpr u32 kSigPingMs = 5'000;

#pragma pack(push, 1)
struct StunPing {
    u8 cmd = 0x01;
    u8 online_id[16]{};
    u32 local_ip = 0;
};
static_assert(sizeof(StunPing) == 21, "StunPing must be exactly 21 bytes");
#pragma pack(pop)

#pragma pack(push, 1)
struct StunEcho {
    u32 ext_ip = 0;
    u16 ext_port = 0;
};
static_assert(sizeof(StunEcho) == 6, "StunEcho must be exactly 6 bytes");
#pragma pack(pop)

inline constexpr u8 kSignalingMagic[4] = {'S', 'H', 'A', 'D'};

enum class SignalingPacketType : u8 {
    EchoPing = 0x05,
    EchoPong = 0x06,
    Handshake = 0x07,
    Control = 0x10,
};

#pragma pack(push, 1)
struct SignalingEchoPing {
    u8 magic[4] = {kSignalingMagic[0], kSignalingMagic[1], kSignalingMagic[2], kSignalingMagic[3]};
    u8 type = static_cast<u8>(SignalingPacketType::EchoPing);
    u32 conn_id = 0;
    u64 send_ts_us = 0;
};
static_assert(sizeof(SignalingEchoPing) == 17);
struct SignalingEchoPong {
    u8 magic[4] = {kSignalingMagic[0], kSignalingMagic[1], kSignalingMagic[2], kSignalingMagic[3]};
    u8 type = static_cast<u8>(SignalingPacketType::EchoPong);
    u32 conn_id = 0;
    u64 orig_ts_us = 0;
};
static_assert(sizeof(SignalingEchoPong) == 17);
#pragma pack(pop)

#pragma pack(push, 1)
struct SignalingHandshake {
    u8 magic[4] = {kSignalingMagic[0], kSignalingMagic[1], kSignalingMagic[2], kSignalingMagic[3]};
    u8 type = static_cast<u8>(SignalingPacketType::Handshake);
    u8 kind = 0;
    u8 online_id_from[16]{};
    u8 online_id_to[16]{};
    u32 from_conn_id = 0;
    u32 to_conn_id = 0;
    u32 mapped_addr = 0;
    u16 mapped_port = 0;
    u16 reserved = 0;
    u64 nonce = 0;
};
static_assert(sizeof(SignalingHandshake) == 0x3e);
#pragma pack(pop)

enum class HandshakeKind : u8 {
    Offer = 1,
    Accept = 2,
    Check = 3,
    CheckAck = 4,
};

#pragma pack(push, 1)
struct SignalingControl {
    u8 magic[4] = {kSignalingMagic[0], kSignalingMagic[1], kSignalingMagic[2], kSignalingMagic[3]};
    u8 type = static_cast<u8>(SignalingPacketType::Control);
    u8 kind = 0;
    u8 online_id_from[16]{};
    u8 online_id_to[16]{};
    u32 from_conn_id = 0;
    u32 mapped_addr = 0;
    u16 mapped_port = 0;
    u16 reason = 0;
};
static_assert(sizeof(SignalingControl) == 0x32);
#pragma pack(pop)

enum class ControlKind : u8 {
    ActivationRequest = 1,
    ActivationAck = 2,
    Close = 3,
    Established = 4,
};

enum class ControlReason : u16 {
    Terminate = 0,
    Deactivate = 1,
};

enum class ConnState : s32 {
    Inactive = 0,
    SendingOffer = 5,
    WaitOffer = 6,
    ConnCheck = 7,
    SendingAccept = 8,
    WaitAccept = 9,
    Established = 10,
};

struct PeerTransport {
    s32 transport_id = 0;

    u32 addr = 0;
    u16 port = 0;

    u32 local_addr = 0;
    u16 local_port = 0;

    OrbisNpId npid{};
    OrbisNpOnlineId online_id{};
    OrbisNpOnlineId local_online_id{};

    s32 status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    ConnState state = ConnState::Inactive;

    bool peer_activated = false;
    bool peer_established = false;
    bool transport_established_fired = false;
    bool is_initiator = false;

    static constexpr u32 kProbeSampleCount = 6;
    s32 probe_rtt_samples[kProbeSampleCount] = {-1, -1, -1, -1, -1, -1};
    u32 probe_sample_write_index = 0;

    s64 last_echo_ping_us = 0;
    s64 last_peer_rx_us = 0;
    s64 last_handshake_send_us = 0;
};

struct ConnectionInfo {
    OrbisNpSignalingConnectionId conn_id = 0;
    OrbisNpSignalingContextId ctx_id = 0;
    s32 transport_id = 0;
    s32 status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;

    OrbisNpId npid{};
    OrbisNpOnlineId online_id{};
    bool sig1_established_event_fired = false;
    bool locally_activated = false;
    bool peer_activated = false;
    bool peer_activated_fired = false;
    bool mutual_fired = false;
    bool dead_fired = false;
};

struct NpSignalingContext {
    OrbisNpSignalingHandler callback{nullptr};
    void* callback_arg{nullptr};
    bool active{false};

    u32 flags{0};

    u32 compiled_sdk_version{0};

    u64 account_id{0};
    u32 platform_type{0};

    OrbisNpId owner_npid{};
    OrbisNpOnlineId owner_online_id{};

    u16 bound_port{0};

    std::atomic<u32> ext_addr{0};
    std::atomic<u16> ext_port{0};
    std::mutex stun_mutex{};
    std::condition_variable stun_cv{};

    s64 activate_budget_us{0};
    s64 activate_last_update_us{0};
};

struct CtxNpIdKey {
    OrbisNpSignalingContextId ctx_id = 0;
    OrbisNpId npid{};
};

struct CtxNpIdKeyHash {
    size_t operator()(const CtxNpIdKey& key) const noexcept {
        size_t hash = static_cast<size_t>(key.ctx_id);
        const auto* bytes = reinterpret_cast<const u8*>(&key.npid);
        for (size_t i = 0; i < sizeof(key.npid); ++i) {
            hash ^= static_cast<size_t>(bytes[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

inline bool operator==(const CtxNpIdKey& lhs, const CtxNpIdKey& rhs) {
    return lhs.ctx_id == rhs.ctx_id && std::memcmp(&lhs.npid, &rhs.npid, sizeof(lhs.npid)) == 0;
}

struct QueuedDispatch {
    std::chrono::steady_clock::time_point fire_at{};
    u32 delay_ms = 0;
    OrbisNpSignalingContextId ctx_id = 0;
    OrbisNpSignalingConnectionId conn_id = 0;
    u32 event_type = 0;
    u32 error_code = 0;
    OrbisNpSignalingHandler callback = nullptr;
    void* callback_arg = nullptr;
};

constexpr u32 kPeerNetInfoRequestIdPrefix = 0x21000000;

struct PeerNetInfoResult {
    OrbisNpSignalingContextId ctx_id = 0;
    OrbisNpSignalingConnectionId conn_id = 0;
    u32 local_ipv4 = 0;
    u32 external_ipv4 = 0;
    u32 nat_route_kind = 0;
};

struct PendingActivation {
    OrbisNpSignalingConnectionId conn_id = 0;
    std::string peer_online_id;
    bool start_handshake = true;
};

struct NpSignalingState {
    std::unordered_map<s32, NpSignalingContext> contexts;
    std::unordered_map<s32, ConnectionInfo> connections;
    std::unordered_map<s32, PeerTransport> peer_transports;
    std::unordered_map<CtxNpIdKey, s32, CtxNpIdKeyHash> npid_to_conn;
    std::unordered_map<s32, PeerNetInfoResult> peer_netinfo_results;
    std::vector<PendingActivation> pending_activations;

    u32 peer_netinfo_next_id = 1;
    u32 peak_connection_count = 0;
    u32 last_assigned_context_id = 0;
    u32 connection_id_seed = 0;
    u32 transport_id_seed = 0;
    bool initialized = false;

    std::recursive_mutex mutex;

    std::multimap<std::chrono::steady_clock::time_point, QueuedDispatch> dispatch_queue;
    std::mutex dispatch_mutex;
    std::condition_variable dispatch_cv;
    Libraries::Kernel::PthreadT dispatch_thread = nullptr;
    bool dispatch_stop = false;

    std::atomic<bool> runtime_stop{false};
    std::mutex runtime_thread_mutex;
    std::thread runtime_thread;
    std::mutex activation_mutex;
    std::condition_variable activation_cv;
    std::thread activation_thread;
    u32 context_refs = 0;

    void ResetSig1Data() {
        contexts.clear();
        connections.clear();
        npid_to_conn.clear();
        peer_netinfo_results.clear();
        pending_activations.clear();
        peer_netinfo_next_id = 1;
        peak_connection_count = 0;
        last_assigned_context_id = 0;
        connection_id_seed = 0;
    }
};

struct SignalingMutexGuard {
    SignalingMutexGuard();
    ~SignalingMutexGuard() = default;
    SignalingMutexGuard(const SignalingMutexGuard&) = delete;
    SignalingMutexGuard& operator=(const SignalingMutexGuard&) = delete;

private:
    std::unique_lock<std::recursive_mutex> lock;
};

long long NowMs();
s64 NowUs();
std::string OnlineIdToString(const OrbisNpOnlineId& online_id_in);
std::string OnlineIdFromNpId(const OrbisNpId& np_id);
OrbisNpId NpIdFromOnlineId(const OrbisNpOnlineId& online_id);
bool OnlineIdEqualsString(const OrbisNpOnlineId& online_id, std::string_view value);
bool IsValidNpId(const OrbisNpId& np_id);
s32 NormalizeNpId(const void* np_id, OrbisNpId* out_npid, OrbisNpOnlineId* out_online_id = nullptr);
CtxNpIdKey MakeCtxNpIdKey(OrbisNpSignalingContextId ctx_id, const OrbisNpId& npid);

bool IsContextValidLocked(OrbisNpSignalingContextId ctx_id);
OrbisNpSignalingContextId AllocateContextIdLocked();
OrbisNpSignalingConnectionId AllocateConnectionIdLocked();
void RemoveConnectionLocked(OrbisNpSignalingConnectionId conn_id);
void RemoveContextConnectionsLocked(OrbisNpSignalingContextId ctx_id);

s32 ConnectionStateFromStatus(s32 status);

u32 CaptureCompiledSdkVersion();

u32 ParseIpv4Nbo(const std::string& dotted);

s32 GetConnectionInfoInternal(OrbisNpSignalingContextId ctx_id,
                              OrbisNpSignalingConnectionId conn_id, s32 info_code, void* out_a,
                              void* out_b);

bool ConsumeActivationBudgetGatedLocked(NpSignalingContext& ctx);

void SnapshotConnectionStatisticsLocked(u32* out_peak, u32* out_active, u32* out_transient,
                                        u32* out_established);

OrbisNpSignalingRequestId StagePeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id);
bool TakePeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id,
                                 PeerNetInfoResult* out);
bool DropPeerNetInfoResultLocked(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id);

void DispatchConnectionEvent(OrbisNpSignalingConnectionId conn_id, s32 event_type, s32 event_data);

void DispatchPeerActivatedEvent(OrbisNpSignalingConnectionId conn_id);

const char* SignalingEventName(u32 event_type);
bool ConsumeActivationBudgetLocked(NpSignalingContext& ctx);

} // namespace Libraries::Np::NpSignaling
