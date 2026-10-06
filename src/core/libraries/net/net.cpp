// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <mutex>
#include <optional>
#include <random>
#include <source_location>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_p2p.h"
#include "core/libraries/net/net_p2p_ioctl.h"
#include "core/libraries/net/net_resolver.h"
#include "core/libraries/net/net_translate.h"
#include "core/net/guest_net.h"
#include "core/net/p2p_codec.h"
#include "core/net/p2p_tcp.h"

namespace Libraries::Net {

namespace {

using Host::Error;

thread_local s32 g_net_errno = 0;

constexpr s32 FirstPoolId = 3000;
constexpr size_t MaxPools = 31;
constexpr s32 MinPoolSize = 4 * 1024;
struct Pool {
    bool in_use = false;
    s32 size = 0;
    s32 users = 0; // ENOTEMPTY on destroy
};
std::mutex g_pools_mutex;
std::array<Pool, MaxPools> g_pools{};
std::unordered_map<OrbisNetId, s32> g_resolver_pools; // resolver id -> memory id

// Caller holds g_pools_mutex.
Pool* FindPool(s32 memid) {
    const auto index = static_cast<size_t>(memid - FirstPoolId);
    if (memid < FirstPoolId || index >= g_pools.size() || !g_pools[index].in_use) {
        return nullptr;
    }
    return &g_pools[index];
}

s32 CheckPoolId(s32 memid, const std::source_location where = std::source_location::current()) {
    if (memid == 0) {
        return ORBIS_NET_EINVAL;
    }
    std::scoped_lock lock{g_pools_mutex};
    if (FindPool(memid) == nullptr) {
        LOG_ERROR(Lib_Net, "{}: unknown memory pool id {}", where.function_name(), memid);
        return ORBIS_NET_EBADF;
    }
    return 0;
}

bool IsEpoll(OrbisNetId id) {
    return Core::Net::GetObjectKind(id) == Core::Net::NetObjectKind::Epoll;
}

std::atomic<void (*)(int)> g_kernel_errno_hook{nullptr};

s32 SetErrno(int orbis_errno, const std::source_location where = std::source_location::current()) {
    g_net_errno = orbis_errno;
    if (const auto hook = g_kernel_errno_hook.load(std::memory_order_relaxed)) {
        hook(orbis_errno);
    }
    LogFailure(orbis_errno, where);
    return ORBIS_NET_ERROR_BASE | orbis_errno;
}

s32 Return(const Core::Net::NetResult& result,
           const std::source_location where = std::source_location::current()) {
    if (result.error == Error::Ok) {
        return static_cast<s32>(result.value);
    }
    return SetErrno(ToOrbisErrno(result.error), where);
}

OrbisNetSockaddrIn AsP2PSockaddr(const OrbisNetSockaddr& addr) {
    OrbisNetSockaddrIn in;
    std::memcpy(&in, &addr, sizeof(in)); // caller checked length
    return in;
}

int CheckP2PBindAddress(const OrbisNetSockaddr& addr, P2PKind kind) {
    const OrbisNetSockaddrIn in = AsP2PSockaddr(addr);
    if ((ntohl(in.sin_addr) & 0xf0000000u) == 0xe0000000u) {
        return ORBIS_NET_EADDRNOTAVAIL;
    }
    if (kind == P2PKind::Datagram) {
        return in.sin_port == 0 ? ORBIS_NET_EADDRNOTAVAIL : 0;
    }
    // TODO: the PS4 only accepts 3658. The configured port is accepted too, for tests and setups
    // that change it.
    const u16 udp_port = ntohs(in.sin_vport);
    const bool ok = udp_port == 0 || udp_port == DefaultP2PUdpPort ||
                    udp_port == GetP2PBoundPort() || udp_port == GetP2PConfiguredPort();
    return ok ? 0 : ORBIS_NET_EINVAL;
}

bool IsCompleteP2PAddress(const OrbisNetSockaddr& addr, P2PKind kind) {
    const OrbisNetSockaddrIn in = AsP2PSockaddr(addr);
    return in.sin_addr != 0 && in.sin_port != 0 && (kind == P2PKind::Stream || in.sin_vport != 0);
}

bool IsP2PBound(OrbisNetId s) {
    u16 udp_port = 0;
    u16 vport = 0;
    return Core::Net::P2PSocketGetName(s, &udp_port, &vport).error == Error::Ok && vport != 0;
}

// Socket options

enum KindBits : u8 {
    KindTcp = 1u << 0,
    KindUdp = 1u << 1,
    KindRaw = 1u << 2,
    KindUdpP2P = 1u << 3,
    KindTcpP2P = 1u << 4,
};
constexpr u8 AllKinds = KindTcp | KindUdp | KindRaw | KindUdpP2P | KindTcpP2P;
constexpr u8 Streams = KindTcp | KindTcpP2P;
constexpr u8 Datagrams = KindUdp | KindUdpP2P;

u8 KindBit(const Core::Net::SocketInfo& info) {
    if (info.p2p) {
        return info.type == SOCK_STREAM ? KindTcpP2P : KindUdpP2P;
    }
    switch (info.type) {
    case SOCK_STREAM:
        return KindTcp;
    case SOCK_DGRAM:
        return KindUdp;
    default:
        return KindRaw;
    }
}

constexpr s32 BsdSndTimeo = 0x1005;
constexpr s32 BsdRcvTimeo = 0x1006;
constexpr s32 IpDontFrag = ORBIS_NET_IP_DONTFRAG;
constexpr s32 MaxSocketBuffer = 512 * 1024;

struct OptionRule {
    s32 level;
    s32 name;
    bool get;
    bool set;
    u8 valid_for;
    bool others_ignored; // ignored on other kinds
    bool stored;         // no host equivalent
    bool byte_or_int;    // u8 or int
};

// clang-format off
constexpr OptionRule OptionRules[] = {
    // level                  name                            get    set    valid_for             ignored stored byte
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_ACCEPTTIMEO,       true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_BROADCAST,         true,  true,  Datagrams,           true,  false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_CONNECTTIMEO,      true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_ERROR,             true,  false, AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_ERROR_EX,          true,  false, AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_KEEPALIVE,         true,  true,  Streams,             false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_LINGER,            true,  true,  Streams,             false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_NAME,              false, true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_NBIO,              true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_ONESBCAST,         true,  true,  AllKinds,            false, true,  false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_POLICY,            true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_PRIORITY,          true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_RCVBUF,            true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_RCVTIMEO,          true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  BsdRcvTimeo,                   true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_REUSEADDR,         true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_REUSEPORT,         true,  true,  AllKinds,            false, true,  false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_SNDBUF,            true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_SNDTIMEO,          true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  BsdSndTimeo,                   true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_TYPE,              true,  false, AllKinds,            false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_USECRYPTO,         true,  true,  KindUdpP2P | KindTcpP2P, false, false, false},
    {ORBIS_NET_SOL_SOCKET,  ORBIS_NET_SO_USESIGNATURE,      true,  true,  KindUdpP2P | KindTcpP2P, false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_ADD_MEMBERSHIP,    false, true,  KindUdp | KindRaw,  false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_DROP_MEMBERSHIP,   false, true,  KindUdp | KindRaw,  false, false, false},
    {ORBIS_NET_IPPROTO_IP,  IpDontFrag,                    true,  true,  Datagrams | KindRaw, false, true, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_HDRINCL,           true,  true,  KindRaw,             false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_MAXTTL,            true,  false, AllKinds & ~KindRaw, false, true, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_MULTICAST_IF,      true,  true,  KindUdp,             false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_MULTICAST_LOOP,    true,  true,  KindUdp | KindRaw,  false, false, true},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_MULTICAST_TTL,     true,  true,  KindUdp | KindRaw,  false, false, true},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_TOS,               true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_TTL,               true,  true,  AllKinds,            false, false, false},
    {ORBIS_NET_IPPROTO_IP,  ORBIS_NET_IP_TTLCHK,            true,  true,  AllKinds & ~KindRaw, false, true, false},
    {ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_MAXSEG,           true,  true,  Streams,             false, false, false},
    {ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_MSS_TO_ADVERTISE, true,  true,  Streams,             false, true,  false},
    {ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_NODELAY,          true,  true,  Streams,             false, false, false},
    {ORBIS_NET_IPPROTO_TCP, ORBIS_NET_TCP_KEEPLISTEN,       true,  true,  KindTcp,             false, true,  false},
    {ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND,   true,  true,  KindUdp,             false, true,  false},
};
// clang-format on

const OptionRule* FindOptionRule(s32 level, s32 name) {
    for (const auto& rule : OptionRules) {
        if (rule.level == level && rule.name == name) {
            return &rule;
        }
    }
    return nullptr;
}

s32 StoredOption(OrbisNetId s, s32 level, s32 name, const Core::Net::SocketInfo& info) {
    Core::Net::SocketAttributes attributes;
    Core::Net::SocketGetAttributes(s, &attributes);
    if (const auto it = attributes.stored_options.find({level, name});
        it != attributes.stored_options.end()) {
        return it->second;
    }
    if (level == ORBIS_NET_IPPROTO_TCP && name == ORBIS_NET_TCP_NODELAY) {
        return info.p2p ? 1 : 0; // on for STREAM_P2P
    }
    if (level == ORBIS_NET_IPPROTO_TCP && name == ORBIS_NET_TCP_MAXSEG && info.p2p) {
        return Core::Net::P2P::TcpConfig{}.mss;
    }
    if (level == ORBIS_NET_IPPROTO_IP && name == ORBIS_NET_IP_TTL) {
        return 64; // TODO: FreeBSD default
    }
    if (level == ORBIS_NET_IPPROTO_IP &&
        (name == ORBIS_NET_IP_MULTICAST_TTL || name == ORBIS_NET_IP_MULTICAST_LOOP)) {
        return 1;
    }
    return 0;
}

// FreeBSD timeval on the PS4: two 8-byte fields.
struct OrbisTimeval {
    s64 tv_sec;
    s64 tv_usec;
};

std::optional<std::chrono::microseconds> ReadTimeval(const void* optval, u32 optlen) {
    if (optval == nullptr || optlen < sizeof(OrbisTimeval)) {
        return std::nullopt;
    }
    OrbisTimeval tv;
    std::memcpy(&tv, optval, sizeof(tv));
    if (tv.tv_sec < 0 || tv.tv_usec < 0 || tv.tv_usec > 999'999) {
        return std::nullopt;
    }
    return std::chrono::seconds{tv.tv_sec} + std::chrono::microseconds{tv.tv_usec};
}

s32 WriteTimeval(s64 microseconds, void* optval, u32* optlen) {
    if (*optlen < sizeof(OrbisTimeval)) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    const OrbisTimeval tv{microseconds / 1'000'000, microseconds % 1'000'000};
    std::memcpy(optval, &tv, sizeof(tv));
    *optlen = sizeof(tv);
    return ORBIS_OK;
}

// Each host OS spells IP_DONTFRAG differently. Returns 0 or the guest errno.
int SetHostDontFragment(OrbisNetId s, bool enable) {
#if defined(_WIN32)
    const DWORD value = enable ? 1 : 0;
    const auto r =
        Core::Net::SocketSetHostOption(s, IPPROTO_IP, IP_DONTFRAGMENT, &value, sizeof(value));
#elif defined(IP_MTU_DISCOVER)
    const int value = enable ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;
    const auto r =
        Core::Net::SocketSetHostOption(s, IPPROTO_IP, IP_MTU_DISCOVER, &value, sizeof(value));
#elif defined(IP_DONTFRAG)
    const int value = enable ? 1 : 0;
    const auto r =
        Core::Net::SocketSetHostOption(s, IPPROTO_IP, IP_DONTFRAG, &value, sizeof(value));
#else
    (void)s;
    (void)enable;
    const Core::Net::NetResult r = Core::Net::NetResult::Ok();
#endif
    return r.error == Error::Ok ? 0 : ToOrbisErrno(r.error);
}

// P2P SO_LINGER: on/off is stored under the option, seconds under this key.
constexpr s32 LingerSecondsKey = -ORBIS_NET_SO_LINGER;

bool IsBound(OrbisNetId s) {
    sockaddr_storage local{};
    socklen_t len = sizeof(local);
    if (Core::Net::SocketGetName(s, reinterpret_cast<sockaddr*>(&local), &len).error != Error::Ok) {
        return false; // Windows: unbound sockets have no name
    }
    if (local.ss_family == AF_INET) {
        return reinterpret_cast<const sockaddr_in*>(&local)->sin_port != 0;
    }
    if (local.ss_family == AF_INET6) {
        return reinterpret_cast<const sockaddr_in6*>(&local)->sin6_port != 0;
    }
    return true;
}

s32 SetMembership(OrbisNetId s, bool add, const HostOption& option, const void* optval,
                  u32 optlen) {
    std::array<u8, 8> request{};
    if (optval == nullptr || optlen < request.size()) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::memcpy(request.data(), optval, request.size());
    bool was_pending = false;
    if (!add) {
        Core::Net::SocketUpdateAttributes(s, [&](Core::Net::SocketAttributes& a) {
            was_pending = std::erase(a.pending_memberships, request) > 0;
        });
        if (was_pending) {
            return ORBIS_OK;
        }
    }
    const auto r =
        Core::Net::SocketSetHostOption(s, option.level, option.name, optval, request.size());
    if (add && r.error == Error::Inval && !IsBound(s)) {
        return Return(Core::Net::SocketUpdateAttributes(
            s, [&](Core::Net::SocketAttributes& a) { a.pending_memberships.push_back(request); }));
    }
    return Return(r);
}

void ApplyPendingMemberships(OrbisNetId s) {
    std::vector<std::array<u8, 8>> pending;
    Core::Net::SocketUpdateAttributes(
        s, [&](Core::Net::SocketAttributes& a) { pending.swap(a.pending_memberships); });
    for (const auto& request : pending) {
        const auto r = Core::Net::SocketSetHostOption(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                                      request.data(), request.size());
        if (r.error != Error::Ok) {
            LOG_WARNING(Lib_Net, "multicast join after bind failed: {}", static_cast<int>(r.error));
        }
    }
}

constexpr s32 MaxRawSend = 8192;
constexpr u64 MaxPeekLen = 512 * 1024;

s32 SetSendOnSuspend(OrbisNetId s, const Core::Net::SocketInfo& info, const void* optval,
                     u32 optlen) {
    if (optval == nullptr || optlen < sizeof(OrbisNetUdpSndOnSuspend)) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    OrbisNetUdpSndOnSuspend value;
    std::memcpy(&value, optval, sizeof(value));
    if (value.onoff != 0) {
        if (value.addr == nullptr || value.data == nullptr || value.datalen == 0 ||
            value.datalen > ORBIS_NET_UDP_SND_ON_SUSPEND_DATALEN_MAX) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        if (info.connected) {
            return SetErrno(ORBIS_NET_EPROCUNAVAIL); // not allowed after connect
        }
    }
    return Return(Core::Net::SocketUpdateAttributes(s, [&](Core::Net::SocketAttributes& a) {
        a.stored_options[{ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND}] =
            value.onoff != 0 ? 1 : 0;
    }));
}

constexpr int ReservedPortErrno = ORBIS_NET_EACCES;

P2PKind KindOf(const Core::Net::SocketInfo& info) {
    return info.type == SOCK_STREAM ? P2PKind::Stream : P2PKind::Datagram;
}

std::mutex g_hooks_mutex;
SystemHooks g_hooks;

s32 ResolverReturn(s32 code) {
    if (code < 0) {
        g_net_errno = static_cast<s32>(static_cast<u32>(code) & 0xff);
    }
    return code;
}

int GatherSize(const OrbisNetMsghdr& msg, std::vector<u8>* buffer) {
    if (msg.msg_iovlen > 1024) {
        return ORBIS_NET_EMSGSIZE; // more than 1024 iovecs
    }
    if (msg.msg_iovlen < 0 || (msg.msg_iovlen > 0 && msg.msg_iov == nullptr)) {
        return ORBIS_NET_EINVAL;
    }
    size_t total = 0;
    for (s32 i = 0; i < msg.msg_iovlen; ++i) {
        if (msg.msg_iov[i].iov_base == nullptr && msg.msg_iov[i].iov_len != 0) {
            return ORBIS_NET_EFAULT;
        }
        total += msg.msg_iov[i].iov_len;
        if (total > static_cast<size_t>(INT32_MAX)) {
            return ORBIS_NET_EINVAL;
        }
    }
    buffer->resize(total);
    return 0;
}

bool FillSockInfo(OrbisNetId id, OrbisNetSockInfo* out) {
    Core::Net::SocketInfo info{};
    if (Core::Net::SocketGetInfo(id, &info).error != Error::Ok) {
        return false;
    }
    Core::Net::SocketAttributes attributes;
    Core::Net::SocketGetAttributes(id, &attributes);
    OrbisNetSockInfo record{};
    std::memcpy(record.name, attributes.name.data(),
                std::min<size_t>(sizeof(record.name) - 1, attributes.name.size()));
    record.s = id;
    record.socket_type = static_cast<s8>(ToOrbisSocketType(info.type, info.p2p));
    record.policy = static_cast<s8>(attributes.policy);
    record.priority = static_cast<s8>(attributes.priority);
    record.flags =
        ORBIS_NET_SOCKINFO_F_SELF | (info.nonblocking ? ORBIS_NET_SOCKINFO_F_NONBLOCK : 0);
    record.send_buffer_size =
        attributes.snd_buf != 0 ? attributes.snd_buf : DefaultSendBuffer(info.type);
    record.recv_buffer_size =
        attributes.rcv_buf != 0 ? attributes.rcv_buf : DefaultReceiveBuffer(info.type);
    // TODO: TCP state, queues and bandwidth aren't tracked. Reports OPENED, or ESTABLISHED
    // once there is a peer.
    record.state = ORBIS_NET_SOCKINFO_STATE_OPENED;
    if (info.p2p) {
        u16 udp_port = 0;
        u16 vport = 0;
        if (Core::Net::P2PSocketGetName(id, &udp_port, &vport).error == Error::Ok) {
            record.local_port = htons(udp_port);
            record.local_vport = htons(vport);
        }
        P2P::Endpoint peer;
        u16 peer_vport = 0;
        if (Core::Net::P2PSocketGetPeerName(id, &peer, &peer_vport).error == Error::Ok) {
            OrbisNetSockaddrIn in{};
            u32 len = sizeof(in);
            ToOrbisP2PAddress(peer, peer_vport, KindOf(info),
                              reinterpret_cast<OrbisNetSockaddr*>(&in), &len);
            record.remote_adr.inaddr_addr = in.sin_addr;
            record.remote_port = in.sin_port;
            record.remote_vport = in.sin_vport;
            record.state = ORBIS_NET_SOCKINFO_STATE_ESTABLISHED;
        }
    } else {
        sockaddr_storage host{};
        socklen_t len = sizeof(host);
        if (Core::Net::SocketGetName(id, reinterpret_cast<sockaddr*>(&host), &len).error ==
                Error::Ok &&
            host.ss_family == AF_INET) {
            const auto* in = reinterpret_cast<const sockaddr_in*>(&host);
            record.local_adr.inaddr_addr = in->sin_addr.s_addr;
            record.local_port = in->sin_port;
        }
        len = sizeof(host);
        if (Core::Net::SocketGetPeerName(id, reinterpret_cast<sockaddr*>(&host), &len).error ==
                Error::Ok &&
            host.ss_family == AF_INET) {
            const auto* in = reinterpret_cast<const sockaddr_in*>(&host);
            record.remote_adr.inaddr_addr = in->sin_addr.s_addr;
            record.remote_port = in->sin_port;
            record.state = ORBIS_NET_SOCKINFO_STATE_ESTABLISHED;
        }
    }
    std::memcpy(out, &record, sizeof(record));
    return true;
}

template <typename T>
constexpr T ToBigEndian(T value) {
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    } else {
        return value;
    }
}

} // namespace

namespace {
using In6Addr = std::array<u8, 16>;
constexpr In6Addr In6Any{};
constexpr In6Addr In6Loopback{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
constexpr In6Addr In6NodeLocalAllNodes{0xff, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
constexpr In6Addr In6LinkLocalAllNodes{0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
constexpr In6Addr In6LinkLocalAllRouters{0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// Guests may hold non-const pointers, so give each export its own writable copy.
In6Addr g_in6addr_any = In6Any;
In6Addr g_in6addr_loopback = In6Loopback;
In6Addr g_sce_net_in6addr_any = In6Any;
In6Addr g_sce_net_in6addr_loopback = In6Loopback;
In6Addr g_sce_net_in6addr_nodelocal_allnodes = In6NodeLocalAllNodes;
In6Addr g_sce_net_in6addr_linklocal_allnodes = In6LinkLocalAllNodes;
In6Addr g_sce_net_in6addr_linklocal_allrouters = In6LinkLocalAllRouters;
u32 g_sce_net_dummy = 0;
} // namespace

static OrbisNetId sceNetAcceptImpl(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (addr != nullptr && paddrlen == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        P2P::Endpoint peer;
        u16 peer_vport = 0;
        const auto r = Core::Net::P2PSocketAccept(s, &peer, &peer_vport);
        if (r.error == Error::Ok && addr != nullptr) {
            ToOrbisP2PAddress(peer, peer_vport, P2PKind::Stream, addr, paddrlen);
        }
        return Return(r);
    }
    sockaddr_storage host{};
    socklen_t host_len = sizeof(host);
    const auto r = Core::Net::SocketAccept(s, reinterpret_cast<sockaddr*>(&host), &host_len);
    if (r.error == Error::Ok && addr != nullptr) {
        ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), host_len, addr, paddrlen);
    }
    return Return(r);
}

OrbisNetId PS4_SYSV_ABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    const OrbisNetId r = sceNetAcceptImpl(s, addr, paddrlen);
    if (r >= 0) {
        LOG_DEBUG(Lib_Net, "s = {} -> new socket {}{}", s, r,
                  addr != nullptr && paddrlen != nullptr
                      ? " from " + FormatSockaddr(addr, *paddrlen)
                      : std::string{});
    }
    return r;
}

s32 PS4_SYSV_ABI sceNetAddrConfig6GetInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetAddrConfig6Start() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetAddrConfig6Stop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetAllocateAllRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlGetDataTraffic() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlGetDefaultParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlGetIfParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlGetPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlSetDefaultParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlSetIfParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBandwidthControlSetPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetBind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    LOG_INFO(Lib_Net, "s = {}, addr = {}", s, FormatSockaddr(addr, addrlen));
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        const P2PKind kind = KindOf(info);
        P2P::Endpoint udp;
        u16 port = 0;
        if (const int e = ToP2PAddress(addr, addrlen, kind, &udp, &port); e != 0) {
            return SetErrno(e);
        }
        // sa_len is not checked, games leave it 0 (seen binding DGRAM_P2P during boot).
        if (const int e = CheckP2PBindAddress(*addr, kind); e != 0) {
            return SetErrno(e);
        }
        // P2P sockets share the transport, so no UDP port is bound here.
        const bool reserved =
            kind == P2PKind::Stream ? IsReservedPort(port) : IsReservedP2PVport(port);
        if (port != 0 && reserved) {
            return SetErrno(ReservedPortErrno);
        }
        const auto r = Core::Net::P2PSocketBind(s, port);
        if (r.error == Error::Ok && kind == P2PKind::Stream && port != 0) {
            // Listener port. Connecting from it is EPROTO.
            Core::Net::SocketUpdateAttributes(
                s, [](Core::Net::SocketAttributes& a) { a.p2p_tcp_port_bound = true; });
        }
        return Return(r);
    }
    sockaddr_storage host{};
    socklen_t host_len = 0;
    if (const int e = ToHostSockaddr(addr, addrlen, &host, &host_len); e != 0) {
        return SetErrno(e);
    }
    u16 port = 0;
    if (host.ss_family == AF_INET) {
        port = ntohs(reinterpret_cast<const sockaddr_in*>(&host)->sin_port);
    } else if (host.ss_family == AF_INET6) {
        port = ntohs(reinterpret_cast<const sockaddr_in6*>(&host)->sin6_port);
    }
    if (port != 0 && IsReservedPort(port)) {
        return SetErrno(ReservedPortErrno);
    }
    const auto r = Core::Net::SocketBind(s, reinterpret_cast<sockaddr*>(&host), host_len);
    if (r.error == Error::Ok) {
        ApplyPendingMemberships(s);
    }
    return Return(r);
}

s32 PS4_SYSV_ABI sceNetClearDnsCache() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddArp() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddArpWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddMRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigAddRouteWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigCleanUpAllInterfaces() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelArp() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelArpWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelMRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDelRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigDownInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigEtherGetLinkMode() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigEtherPostPlugInOutEvent() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigEtherSetLinkMode() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigFlushRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigGetDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigGetDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigGetIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigGetIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigRoutingShowRoutingConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigRoutingShowtCtlVar() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigRoutingStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigRoutingStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetDefaultScope() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfaddr6WithFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfLinkLocalAddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigSetIfmtu() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigUnsetIfFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigUpInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigUpInterfaceWithFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocClearWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocGetWakeOnWlanInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocLeave() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuClearWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuSetWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocScanJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocSetExtInfoElement() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanAdhocSetWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanApStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanApStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanQuery() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanDiagGetDeviceInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanDiagSetAntenna() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanDiagSetTxFixedRate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanGetDeviceConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanInfraGetRssiInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanInfraLeave() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanInfraScanJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanScan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConfigWlanSetDeviceConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetConnect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    LOG_DEBUG(Lib_Net, "s = {}, addr = {}", s, FormatSockaddr(addr, addrlen));
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        const P2PKind kind = KindOf(info);
        const bool bound = IsP2PBound(s);
        if (!bound && kind == P2PKind::Datagram) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        P2P::Endpoint peer;
        u16 port = 0;
        if (const int e = ToP2PAddress(addr, addrlen, kind, &peer, &port); e != 0) {
            return SetErrno(e);
        }
        if (!IsCompleteP2PAddress(*addr, kind)) {
            return SetErrno(ORBIS_NET_EADDRNOTAVAIL);
        }
        if (info.listening) {
            return SetErrno(ORBIS_NET_EOPNOTSUPP);
        }
        if (kind == P2PKind::Stream) {
            Core::Net::SocketAttributes attributes;
            Core::Net::SocketGetAttributes(s, &attributes);
            if (attributes.p2p_tcp_port_bound) {
                return SetErrno(ORBIS_NET_EPROTO);
            }
            if (!bound) {
                if (const auto r = Core::Net::P2PSocketBind(s, 0); r.error != Error::Ok) {
                    return Return(r);
                }
                LOG_DEBUG(Lib_Net, "s = {}: connect on an unbound P2P stream, bound implicitly", s);
            }
        }
        if (kind == P2PKind::Datagram) {
            P2P::Endpoint current;
            u16 current_vport = 0;
            if (Core::Net::P2PSocketGetPeerName(s, &current, &current_vport).error == Error::Ok) {
                return SetErrno(ORBIS_NET_EISCONN);
            }
        }
        return Return(Core::Net::P2PSocketConnect(s, peer, port));
    }
    sockaddr_storage host{};
    socklen_t host_len = 0;
    if (const int e = ToHostSockaddr(addr, addrlen, &host, &host_len); e != 0) {
        return SetErrno(e);
    }
    if (info.type == SOCK_DGRAM &&
        StoredOption(s, ORBIS_NET_IPPROTO_UDP, ORBIS_NET_UDP_SND_ON_SUSPEND, info) != 0) {
        return SetErrno(ORBIS_NET_EPROCUNAVAIL);
    }
    if (info.type == SOCK_STREAM && host.ss_family == AF_INET &&
        reinterpret_cast<const sockaddr_in*>(&host)->sin_addr.s_addr ==
            ORBIS_NET_INADDR_BROADCAST) {
        return SetErrno(ORBIS_NET_EACCES);
    }
    return Return(Core::Net::SocketConnect(s, reinterpret_cast<sockaddr*>(&host), host_len));
}

s32 PS4_SYSV_ABI sceNetControl() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpdStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpdStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpGetAutoipInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpGetInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpGetInfoEx() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDhcpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDumpAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDumpCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDumpDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDumpRead() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDuplicateIpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetDuplicateIpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEpollAbort(OrbisNetId eid, s32 flags) {
    LOG_DEBUG(Lib_Net, "eid = {}, flags = {:#x}", eid, flags);
    if (!IsEpoll(eid)) {
        return SetErrno(ORBIS_NET_EBADF);
    }
    if ((static_cast<u32>(flags) & ~Core::Net::kEpollAbortPreserve) != 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    return Return(Core::Net::EpollAbort(eid, static_cast<u32>(flags)));
}

s32 PS4_SYSV_ABI sceNetEpollControl(OrbisNetId eid, s32 op, OrbisNetId id,
                                    OrbisNetEpollEvent* event) {
    LOG_DEBUG(Lib_Net, "eid = {}, op = {}, id = {}, events = {}", eid,
              op == ORBIS_NET_EPOLL_CTL_ADD   ? "ADD"
              : op == ORBIS_NET_EPOLL_CTL_MOD ? "MOD"
              : op == ORBIS_NET_EPOLL_CTL_DEL ? "DEL"
                                              : "invalid",
              id, event != nullptr ? EpollEventsName(event->events) : "none");
    Core::Net::EpollOp guest_op;
    switch (op) {
    case ORBIS_NET_EPOLL_CTL_ADD:
        guest_op = Core::Net::EpollOp::Add;
        break;
    case ORBIS_NET_EPOLL_CTL_MOD:
        guest_op = Core::Net::EpollOp::Modify;
        break;
    case ORBIS_NET_EPOLL_CTL_DEL:
        guest_op = Core::Net::EpollOp::Delete;
        break;
    default:
        return SetErrno(ORBIS_NET_EINVAL);
    }
    if ((guest_op == Core::Net::EpollOp::Delete) != (event == nullptr)) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    // Epolls can't be added (EPERM). Unknown ids are EBADF.
    const auto kind = Core::Net::GetObjectKind(id);
    if (!IsEpoll(eid) || !kind) {
        return SetErrno(ORBIS_NET_EBADF);
    }
    if (*kind == Core::Net::NetObjectKind::Epoll) {
        return SetErrno(ORBIS_NET_EPERM);
    }
    if (event != nullptr && event->events == 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    const u32 events =
        event != nullptr
            ? ToHostEpollEvents(event->events & (ORBIS_NET_EPOLLIN | ORBIS_NET_EPOLLOUT))
            : 0;
    const u64 data = event != nullptr ? event->data.data_u64 : 0;
    auto r = Core::Net::EpollControl(eid, guest_op, id, events, 0, data);
    if (r.error == Error::NoEnt && guest_op == Core::Net::EpollOp::Modify) {
        r = Core::Net::EpollControl(eid, Core::Net::EpollOp::Add, id, events, 0, data);
    }
    if (r.error == Error::NoEnt) {
        // DEL of an unregistered id: EBADF, not ENOENT.
        return SetErrno(ORBIS_NET_EBADF);
    }
    return Return(r);
}

OrbisNetId PS4_SYSV_ABI sceNetEpollCreate(const char* name, s32 flags) {
    LOG_DEBUG(Lib_Net, "name = {}, flags = {:#x}", name != nullptr ? name : "", flags);
    if (flags != 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    if (name != nullptr &&
        strnlen(name, ORBIS_NET_DEBUG_NAME_LEN_MAX + 1) > ORBIS_NET_DEBUG_NAME_LEN_MAX) {
        return SetErrno(ORBIS_NET_ENAMETOOLONG);
    }
    return Return(Core::Net::EpollCreate());
}

s32 PS4_SYSV_ABI sceNetEpollDestroy(OrbisNetId eid) {
    LOG_DEBUG(Lib_Net, "eid = {}", eid);
    const auto kind = Core::Net::GetObjectKind(eid);
    if (kind && *kind != Core::Net::NetObjectKind::Epoll) {
        return SetErrno(ORBIS_NET_EPERM);
    }
    return Return(Core::Net::EpollDestroy(eid));
}

static s32 sceNetEpollWaitImpl(OrbisNetId eid, OrbisNetEpollEvent* events, s32 maxevents,
                               s32 timeout_us) {
    if (!IsEpoll(eid)) {
        return SetErrno(ORBIS_NET_EBADF);
    }
    if (events == nullptr || maxevents <= 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::vector<Core::Net::GuestEpollEvent> ready(std::min(maxevents, 256));
    const auto r = Core::Net::EpollWait(eid, ready, timeout_us < 0 ? -1 : timeout_us);
    if (r.error != Error::Ok) {
        return Return(r);
    }
    for (s64 i = 0; i < r.value; ++i) {
        events[i] = {};
        events[i].events = ToOrbisEpollEvents(ready[i].events);
        events[i].ident = static_cast<u64>(ready[i].ident);
        events[i].data.data_u64 = ready[i].data;
    }
    return static_cast<s32>(r.value);
}

s32 PS4_SYSV_ABI sceNetEpollWait(OrbisNetId eid, OrbisNetEpollEvent* events, s32 maxevents,
                                 s32 timeout_us) {
    const s32 r = sceNetEpollWaitImpl(eid, events, maxevents, timeout_us);
    if (r > 0) {
        LOG_TRACE(Lib_Net, "eid = {}, timeout = {} us -> {} (first: id {} {})", eid, timeout_us, r,
                  events[0].ident, EpollEventsName(events[0].events));
    } else {
        LOG_TRACE(Lib_Net, "eid = {}, timeout = {} us -> {}", eid, timeout_us,
                  r == 0 ? "timed out" : ErrorCodeName(r));
    }
    return r;
}

s32* PS4_SYSV_ABI sceNetErrnoLoc() {
    return &g_net_errno;
}

s32 PS4_SYSV_ABI sceNetEtherNtostr(const OrbisNetEtherAddr* n, char* str, u64 len) {
    if (n == nullptr || str == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    if (len < static_cast<u64>(ORBIS_NET_ETHER_ADDRSTRLEN)) {
        return SetErrno(ORBIS_NET_ENOSPC);
    }
    std::snprintf(str, static_cast<size_t>(len), "%02x:%02x:%02x:%02x:%02x:%02x", n->data[0],
                  n->data[1], n->data[2], n->data[3], n->data[4], n->data[5]);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEtherStrton(const char* str, OrbisNetEtherAddr* n) {
    if (str == nullptr || n == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    OrbisNetEtherAddr out{};
    const char* p = str;
    for (int i = 0; i < 6; ++i) {
        int value = 0;
        int digits = 0;
        for (; digits < 2 && std::isxdigit(static_cast<unsigned char>(*p)); ++digits, ++p) {
            const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
            value = value * 16 + (c <= '9' ? c - '0' : c - 'a' + 10);
        }
        if (digits == 0 || (i < 5 && *p != ':' && *p != '-') || (i == 5 && *p != '\0')) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        out.data[i] = static_cast<u8>(value);
        if (i < 5) {
            ++p;
        }
    }
    *n = out;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEventCallbackCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEventCallbackDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEventCallbackGetError() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEventCallbackWaitCB() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetFreeAllRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetArpInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

namespace {
std::mutex g_dns_mutex;
std::array<u32, 2> g_dns_override{};
std::array<u8, 32> g_dns6_override{};

bool IsZero(std::span<const u8> bytes) {
    return std::all_of(bytes.begin(), bytes.end(), [](u8 b) { return b == 0; });
}
} // namespace

// Returns the number of servers, 0 to 2.
s32 PS4_SYSV_ABI sceNetGetDnsInfo(u32* info, s32 flags) {
    if (info == nullptr || flags != 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::array<u32, 2> servers{};
    {
        std::scoped_lock lock{g_dns_mutex};
        servers = g_dns_override;
    }
    if (servers[0] == 0) {
        // What sceNetCtlGetInfo reports, while online.
        const auto hooks = GetSystemHooks();
        if (!hooks.is_online || hooks.is_online()) {
            const u32 cloudflare = ToBigEndian(0x01010101u);
            servers = {cloudflare, cloudflare};
        }
    }
    info[0] = servers[0];
    info[1] = servers[1];
    return servers[0] == 0 ? 0 : servers[1] == 0 ? 1 : 2;
}

s32 PS4_SYSV_ABI sceNetGetDns6Info(u8* info, s32 flags) {
    if (info == nullptr || flags != 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::array<u8, 32> servers{};
    {
        std::scoped_lock lock{g_dns_mutex};
        servers = g_dns6_override; // no IPv6 servers otherwise
    }
    std::memcpy(info, servers.data(), servers.size());
    const std::span<const u8> view{servers};
    return IsZero(view.first(16)) ? 0 : IsZero(view.last(16)) ? 1 : 2;
}

// A null info clears the override.
s32 PS4_SYSV_ABI sceNetSetDnsInfo(const u32* info, s32 flags) {
    if (flags != 0 || (info != nullptr && info[0] == 0)) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::scoped_lock lock{g_dns_mutex};
    g_dns_override = info != nullptr ? std::array<u32, 2>{info[0], info[1]} : std::array<u32, 2>{};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSetDns6Info(const u8* info, s32 flags) {
    if (flags != 0 || (info != nullptr && IsZero({info, 16}))) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::scoped_lock lock{g_dns_mutex};
    g_dns6_override = {};
    if (info != nullptr) {
        std::memcpy(g_dns6_override.data(), info, g_dns6_override.size());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetIfList() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetIfListOnce() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

const char* PS4_SYSV_ABI sceNetGetIfName(u32 index) {
    static constexpr std::array<const char*, 10> Names = {
        "lo0", "eth0", "eth1", "dbg0", "wlan0", "wlan1", "gbe0", "bt0", "phone0", "pppoe0"};
    if (index >= Names.size()) {
        g_net_errno = ORBIS_NET_EINVAL;
        return "";
    }
    return Names[index];
}

s32 PS4_SYSV_ABI sceNetGetIfnameNumList() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetMacAddress(OrbisNetEtherAddr* addr, s32 flags) {
    LOG_DEBUG(Lib_Net, "flags = {:#x}", flags);
    if (addr == nullptr) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::array<u8, 6> mac{};
    const auto hooks = GetSystemHooks();
    if (hooks.mac_address && !hooks.mac_address(&mac)) {
        LOG_WARNING(Lib_Net, "host MAC address not available");
    }
    std::memcpy(addr->data, mac.data(), mac.size());
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetMemoryPoolStats(s32 memid, OrbisNetMemoryPoolStats* stat) {
    std::scoped_lock lock{g_pools_mutex};
    const Pool* pool = FindPool(memid);
    if (pool == nullptr) {
        return SetErrno(ORBIS_NET_EBADF);
    }
    if (stat == nullptr) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    *stat = {};
    stat->pool_size = static_cast<u64>(pool->size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetNameToIndex() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (addr == nullptr || paddrlen == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        P2P::Endpoint peer;
        u16 vport = 0;
        const auto r = Core::Net::P2PSocketGetPeerName(s, &peer, &vport);
        if (r.error == Error::Ok) {
            ToOrbisP2PAddress(peer, vport, KindOf(info), addr, paddrlen);
        }
        return Return(r);
    }
    sockaddr_storage host{};
    socklen_t host_len = sizeof(host);
    const auto r = Core::Net::SocketGetPeerName(s, reinterpret_cast<sockaddr*>(&host), &host_len);
    if (r.error == Error::Ok) {
        ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), host_len, addr, paddrlen);
    }
    return Return(r);
}

s32 PS4_SYSV_ABI sceNetGetRandom(u32* out) {
    if (out == nullptr) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    static std::mutex mutex;
    static std::mt19937 rng{std::random_device{}()};
    std::scoped_lock lock{mutex};
    *out = static_cast<u32>(rng());
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetSockInfo(OrbisNetId s, OrbisNetSockInfo* info, s32 n, s32 flags) {
    LOG_DEBUG(Lib_Net, "s = {}, n = {}, flags = {:#x}", s, n, flags);
    if (info == nullptr) {
        // No buffer and s < 0: return the socket count.
        return s < 0 ? static_cast<s32>(Core::Net::ListSockets().size())
                     : SetErrno(ORBIS_NET_EINVAL);
    }
    if (n <= 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    if (s >= 0) {
        if (!FillSockInfo(s, info)) {
            return SetErrno(ORBIS_NET_EBADF);
        }
        return 1;
    }
    s32 written = 0;
    for (const s32 id : Core::Net::ListSockets()) {
        if (written == n) {
            break;
        }
        written += FillSockInfo(id, &info[written]) ? 1 : 0; // closed meanwhile
    }
    return written;
}

s32 PS4_SYSV_ABI sceNetGetSockInfo6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (addr == nullptr || paddrlen == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        u16 udp_port = 0;
        u16 vport = 0;
        const auto r = Core::Net::P2PSocketGetName(s, &udp_port, &vport);
        if (r.error == Error::Ok) {
            // TODO: reports INADDR_ANY. maybe need to  report the advertised (NAT) address.
            ToOrbisP2PAddress(P2P::Endpoint::IPv4("0.0.0.0", udp_port), vport, KindOf(info), addr,
                              paddrlen);
        }
        return Return(r);
    }
    sockaddr_storage host{};
    socklen_t host_len = sizeof(host);
    const auto r = Core::Net::SocketGetName(s, reinterpret_cast<sockaddr*>(&host), &host_len);
    if (r.error == Error::Ok) {
        ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), host_len, addr, paddrlen);
    }
    return Return(r);
}

static s32 sceNetGetsockoptImpl(OrbisNetId s, s32 level, s32 optname, void* optval, u32* optlen) {
    if (optval == nullptr || optlen == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    const auto write_int = [&](s64 value) -> s32 {
        if (*optlen < sizeof(s32)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        const s32 v = static_cast<s32>(std::clamp<s64>(value, INT32_MIN, INT32_MAX));
        std::memcpy(optval, &v, sizeof(v));
        *optlen = sizeof(v);
        return ORBIS_OK;
    };

    const OptionRule* rule = FindOptionRule(level, optname);
    if (rule == nullptr || !rule->get) {
        return SetErrno(ORBIS_NET_ENOPROTOOPT);
    }
    if (!(rule->valid_for & KindBit(info))) {
        if (!rule->others_ignored) {
            return SetErrno(ORBIS_NET_EPROCUNAVAIL);
        }
        return write_int(StoredOption(s, level, optname, info));
    }
    Core::Net::SocketAttributes attributes;
    Core::Net::SocketGetAttributes(s, &attributes);

    if (level == ORBIS_NET_SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_NBIO:
            return write_int(info.nonblocking ? 1 : 0);
        case ORBIS_NET_SO_RCVTIMEO:
            return write_int(info.rcv_timeout_us);
        case ORBIS_NET_SO_SNDTIMEO:
            return write_int(info.snd_timeout_us);
        case ORBIS_NET_SO_CONNECTTIMEO:
            return write_int(info.connect_timeout_us);
        case ORBIS_NET_SO_ACCEPTTIMEO:
            return write_int(info.accept_timeout_us);
        case BsdRcvTimeo:
        case BsdSndTimeo:
            return WriteTimeval(optname == BsdRcvTimeo ? info.rcv_timeout_us : info.snd_timeout_us,
                                optval, optlen);
        case ORBIS_NET_SO_REUSEADDR:
            return write_int(info.reuse_addr ? 1 : 0);
        case ORBIS_NET_SO_TYPE:
            return write_int(ToOrbisSocketType(info.type, info.p2p));
        case ORBIS_NET_SO_ERROR:
            return write_int(info.pending_error == Error::Ok ? 0
                                                             : ToOrbisErrno(info.pending_error));
        case ORBIS_NET_SO_ERROR_EX:
            // Full SCE error code. Reading either option clears the error.
            return write_int(info.pending_error == Error::Ok
                                 ? 0
                                 : ORBIS_NET_ERROR_BASE | ToOrbisErrno(info.pending_error));
        case ORBIS_NET_SO_SNDBUF:
            // PS4 value. Linux reports double what was set.
            return write_int(attributes.snd_buf != 0 ? attributes.snd_buf
                                                     : DefaultSendBuffer(info.type));
        case ORBIS_NET_SO_RCVBUF:
            return write_int(attributes.rcv_buf != 0 ? attributes.rcv_buf
                                                     : DefaultReceiveBuffer(info.type));
        case ORBIS_NET_SO_POLICY:
            return write_int(attributes.policy);
        case ORBIS_NET_SO_PRIORITY:
            return write_int(attributes.priority);
        case ORBIS_NET_SO_USECRYPTO:
        case ORBIS_NET_SO_USESIGNATURE: {
            bool crypto = false;
            bool signature = false;
            Core::Net::P2PSocketGetProtection(s, &crypto, &signature);
            return write_int((optname == ORBIS_NET_SO_USECRYPTO ? crypto : signature) ? 1 : 0);
        }
        default:
            break;
        }
    }

    if (level == ORBIS_NET_IPPROTO_UDP && optname == ORBIS_NET_UDP_SND_ON_SUSPEND) {
        // Only onoff is stored.
        if (*optlen < sizeof(OrbisNetUdpSndOnSuspend)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        OrbisNetUdpSndOnSuspend value{};
        value.onoff = StoredOption(s, level, optname, info);
        std::memcpy(optval, &value, sizeof(value));
        *optlen = sizeof(value);
        return ORBIS_OK;
    }
    const auto option = ToHostOption(level, optname);
    if (level == ORBIS_NET_SOL_SOCKET && optname == ORBIS_NET_SO_LINGER && info.p2p) {
        if (*optlen < sizeof(OrbisNetLinger)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        const OrbisNetLinger guest{StoredOption(s, level, optname, info),
                                   StoredOption(s, level, LingerSecondsKey, info)};
        std::memcpy(optval, &guest, sizeof(guest));
        *optlen = sizeof(guest);
        return ORBIS_OK;
    }
#ifdef _WIN32
    if (level == ORBIS_NET_IPPROTO_TCP && optname == ORBIS_NET_TCP_MAXSEG && !info.p2p &&
        attributes.stored_options.contains({level, optname})) {
        return write_int(attributes.stored_options.at({level, optname}));
    }
#endif
    if (info.p2p || !option || rule->stored) {
        const s32 value = StoredOption(s, level, optname, info);
        if (rule->byte_or_int && *optlen == 1) {
            *static_cast<u8*>(optval) = static_cast<u8>(value);
            return ORBIS_OK;
        }
        return write_int(value);
    }
    switch (option->value) {
    case OptionValue::Int: {
        int value = 0;
        socklen_t len = sizeof(value);
        const auto r = Core::Net::SocketGetHostOption(s, option->level, option->name, &value, &len);
        if (r.error != Error::Ok) {
            return Return(r);
        }
        if (rule->byte_or_int && *optlen == 1) {
            *static_cast<u8*>(optval) = static_cast<u8>(value);
            return ORBIS_OK;
        }
        return write_int(value);
    }
    case OptionValue::Linger: {
        if (*optlen < sizeof(OrbisNetLinger)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        linger host{};
        socklen_t len = sizeof(host);
        const auto r = Core::Net::SocketGetHostOption(s, option->level, option->name, &host, &len);
        if (r.error != Error::Ok) {
            return Return(r);
        }
        const OrbisNetLinger guest{host.l_onoff, host.l_linger};
        std::memcpy(optval, &guest, sizeof(guest));
        *optlen = sizeof(guest);
        return ORBIS_OK;
    }
    case OptionValue::Raw: {
        socklen_t len = static_cast<socklen_t>(*optlen);
        const auto r = Core::Net::SocketGetHostOption(s, option->level, option->name, optval, &len);
        if (r.error == Error::Ok) {
            *optlen = static_cast<u32>(len);
        }
        return Return(r);
    }
    }
    return SetErrno(ORBIS_NET_ENOPROTOOPT);
}

s32 PS4_SYSV_ABI sceNetGetsockopt(OrbisNetId s, s32 level, s32 optname, void* optval, u32* optlen) {
    const s32 r = sceNetGetsockoptImpl(s, level, optname, optval, optlen);
    if (r == ORBIS_OK && optval != nullptr && optlen != nullptr && *optlen >= sizeof(s32)) {
        s32 value;
        std::memcpy(&value, optval, sizeof(value));
        LOG_DEBUG(Lib_Net, "s = {}, {} -> {}", s, OptionName(level, optname), value);
    } else {
        LOG_DEBUG(Lib_Net, "s = {}, {} -> {}", s, OptionName(level, optname),
                  r == ORBIS_OK ? "ok" : ErrorCodeName(r));
    }
    return r;
}

s32 PS4_SYSV_ABI sceNetGetStatisticsInfo(OrbisNetStatisticsInfo* info, s32 flags) {
    LOG_DEBUG(Lib_Net, "flags = {:#x}", flags);
    if (info == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    // TODO: no host equivalent. Report plenty free and nothing queued so games don't throttle.
    constexpr s32 PlentyFree = 16 * 1024 * 1024;
    *info = {PlentyFree, PlentyFree, 0, 0, PlentyFree, PlentyFree};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetGetStatisticsInfoInternal() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

// Process time in microseconds. No return value, a null pointer is ignored.
void PS4_SYSV_ABI sceNetGetSystemTime(u64* out) {
    if (out != nullptr) {
        *out = Kernel::sceKernelGetProcessTime();
    }
}

u32 PS4_SYSV_ABI sceNetHtonl(u32 host32) {
    return ToBigEndian(host32);
}

u64 PS4_SYSV_ABI sceNetHtonll(u64 host64) {
    return ToBigEndian(host64);
}

u16 PS4_SYSV_ABI sceNetHtons(u16 host16) {
    return ToBigEndian(host16);
}

const char* PS4_SYSV_ABI sceNetInetNtop(s32 af, const void* src, char* dst, u32 size) {
    if (src == nullptr || dst == nullptr) {
        g_net_errno = ORBIS_NET_EFAULT;
        return nullptr;
    }
    const auto family = ToHostFamily(af);
    if (!family || (*family != AF_INET && *family != AF_INET6)) {
        g_net_errno = ORBIS_NET_EAFNOSUPPORT;
        return nullptr;
    }
    // TODO: uses host inet_ntop. glibc matches FreeBSD, Windows may format some IPv6
    // addresses differently.
    if (inet_ntop(*family, const_cast<void*>(src), dst, size) == nullptr) {
        g_net_errno = ORBIS_NET_ENOSPC;
        return nullptr;
    }
    return dst;
}

s32 PS4_SYSV_ABI sceNetInetNtopWithScopeId() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetInetPton(s32 af, const char* src, void* dst) {
    if (src == nullptr || dst == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    const auto family = ToHostFamily(af);
    if (!family || (*family != AF_INET && *family != AF_INET6)) {
        return SetErrno(ORBIS_NET_EAFNOSUPPORT);
    }
    return inet_pton(*family, src, dst); // 1 on success, 0 for malformed text
}

s32 PS4_SYSV_ABI sceNetInetPtonEx(s32 af, const char* src, void* dst, s32 flags) {
    // TODO: flags unknown (IPv6 scope?). IPv4 needs none.
    if (flags != 0) {
        LOG_WARNING(Lib_Net, "ignoring flags {:#x}", flags);
    }
    return sceNetInetPton(af, src, dst);
}

s32 PS4_SYSV_ABI sceNetInetPtonWithScopeId() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetInfoDumpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetInfoDumpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetInit() {
    LOG_INFO(Lib_Net, "called");
    static const bool host_ready = Host::Initialize();
    if (!host_ready) {
        LOG_ERROR(Lib_Net, "host networking could not be initialized");
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetInitParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetIoctl(OrbisNetId s, u64 cmd, void* data) {
    LOG_DEBUG(Lib_Net, "s = {}, cmd = {:#x}", s, cmd);
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (!info.p2p) {
        constexpr u32 Fionbio = 0x8004667e;
        constexpr u32 Fionread = 0x4004667f;
        switch (static_cast<u32>(cmd)) {
        case Fionbio:
            if (data == nullptr) {
                return SetErrno(ORBIS_NET_EFAULT);
            }
            return Return(Core::Net::SocketSetNonBlocking(s, *static_cast<const s32*>(data) != 0));
        case Fionread: {
            if (data == nullptr) {
                return SetErrno(ORBIS_NET_EFAULT);
            }
            const auto r = Core::Net::SocketBytesReadable(s);
            if (r.error != Error::Ok) {
                return Return(r);
            }
            *static_cast<s32*>(data) = static_cast<s32>(r.value);
            return ORBIS_OK;
        }
        default:
            LOG_WARNING(Lib_Net, "unsupported ioctl {:#x} on a non-P2P socket", cmd);
            return SetErrno(ORBIS_NET_EINVAL);
        }
    }
    const u16 np_port = sceNetHtons(GetP2PBoundPort());
    if (const int e = P2PKeyIoctl(*GetP2PKeyring(), np_port, cmd, data); e != 0) {
        LOG_WARNING(Lib_Net, "P2P ioctl {:#x} failed: {}", cmd, e);
        return SetErrno(e);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetListen(OrbisNetId s, s32 backlog) {
    LOG_INFO(Lib_Net, "s = {}, backlog = {}", s, backlog);
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p && !IsP2PBound(s)) {
        return SetErrno(ORBIS_NET_EINVAL); // P2P listeners must be bound
    }
    return Return(Core::Net::SocketListen(s, backlog));
}

void* PS4_SYSV_ABI sceNetMemoryAllocate(s64 size, s32 flags) {
    if (size <= 0) {
        return nullptr;
    }
    constexpr s32 ZeroFill = 2;
    return (flags & ZeroFill) != 0 ? std::calloc(1, static_cast<size_t>(size))
                                   : std::malloc(static_cast<size_t>(size));
}

void PS4_SYSV_ABI sceNetMemoryFree(void* ptr) {
    std::free(ptr);
}

u32 PS4_SYSV_ABI sceNetNtohl(u32 net32) {
    return ToBigEndian(net32);
}

u64 PS4_SYSV_ABI sceNetNtohll(u64 net64) {
    return ToBigEndian(net64);
}

u16 PS4_SYSV_ABI sceNetNtohs(u16 net16) {
    return ToBigEndian(net16);
}

s32 PS4_SYSV_ABI sceNetPoolCreate(const char* name, s32 size, s32 flags) {
    LOG_INFO(Lib_Net, "name = {}, size = {}, flags = {:#x}", name != nullptr ? name : "", size,
             flags);
    if (name != nullptr &&
        strnlen(name, ORBIS_NET_DEBUG_NAME_LEN_MAX + 1) > ORBIS_NET_DEBUG_NAME_LEN_MAX) {
        return ORBIS_NET_ERROR_ENAMETOOLONG;
    }
    if (flags != 0 || size < MinPoolSize) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    std::scoped_lock lock{g_pools_mutex};
    for (size_t i = 0; i < g_pools.size(); ++i) {
        if (!g_pools[i].in_use) {
            g_pools[i] = {true, size, 0};
            return FirstPoolId + static_cast<s32>(i);
        }
    }
    return ORBIS_NET_ERROR_ENFILE;
}

s32 PS4_SYSV_ABI sceNetPoolDestroy(s32 pool_id) {
    LOG_INFO(Lib_Net, "pool_id = {}", pool_id);
    std::scoped_lock lock{g_pools_mutex};
    Pool* pool = FindPool(pool_id);
    if (pool == nullptr) {
        return ORBIS_NET_ERROR_EBADF;
    }
    if (pool->users > 0) {
        return ORBIS_NET_ERROR_ENOTEMPTY;
    }
    *pool = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetPppoeStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetPppoeStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetRecv(OrbisNetId s, void* buf, u64 len, s32 flags) {
    return sceNetRecvfrom(s, buf, len, flags, nullptr, nullptr);
}

static s32 sceNetRecvfromImpl(OrbisNetId s, void* buf, u64 len, s32 flags, OrbisNetSockaddr* addr,
                              u32* paddrlen) {
    const auto msg = ToHostMsgFlags(flags);
    if (msg.peeklen && buf == nullptr) {
        // MSG_PEEKLEN with no buffer: peek into our own and return the size waiting.
        std::vector<u8> scratch(static_cast<size_t>(std::min<u64>(len, MaxPeekLen)));
        return sceNetRecvfrom(s, scratch.data(), scratch.size(),
                              (flags & ~ORBIS_NET_MSG_PEEKLEN) | ORBIS_NET_MSG_PEEK, addr,
                              paddrlen);
    }
    if (buf == nullptr && len != 0) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    if (paddrlen == nullptr) {
        addr = nullptr; // FreeBSD: no length, no address, not an error
    }
    if (msg.unsupported != 0) {
        LOG_WARNING(Lib_Net, "ignoring flags {:#x}", msg.unsupported);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.p2p) {
        P2P::Endpoint from;
        u16 from_vport = 0;
        const auto r = Core::Net::P2PSocketRecvFrom(s, buf, static_cast<size_t>(len), msg.peek,
                                                    msg.dontwait, addr ? &from : nullptr,
                                                    addr ? &from_vport : nullptr, msg.waitall);
        if (r.error == Error::Ok && addr != nullptr) {
            ToOrbisP2PAddress(from, from_vport, KindOf(info), addr, paddrlen);
        }
        return Return(r);
    }
    sockaddr_storage host{};
    socklen_t host_len = sizeof(host);
    const auto r = Core::Net::SocketRecvFrom(
        s, buf, static_cast<size_t>(len), msg.host, msg.dontwait,
        addr ? reinterpret_cast<sockaddr*>(&host) : nullptr, addr ? &host_len : nullptr);
    if (r.error == Error::Ok && addr != nullptr) {
        ToOrbisSockaddr(reinterpret_cast<sockaddr*>(&host), host_len, addr, paddrlen);
    }
    return Return(r);
}

s32 PS4_SYSV_ABI sceNetRecvfrom(OrbisNetId s, void* buf, u64 len, s32 flags, OrbisNetSockaddr* addr,
                                u32* paddrlen) {
    const s32 r = sceNetRecvfromImpl(s, buf, len, flags, addr, paddrlen);
    if (r > 0) {
        CountTraffic(s, false, r);
        if (addr != nullptr && paddrlen != nullptr) {
            NoteP2PPeer(s, false, addr, *paddrlen);
        }
    }
    LOG_TRACE(Lib_Net, "s = {}, len = {}, flags = {:#x} -> {}{}", s, len, flags,
              r >= 0 ? std::to_string(r) : ErrorCodeName(r),
              r >= 0 && addr != nullptr && paddrlen != nullptr
                  ? " from " + FormatSockaddr(addr, *paddrlen)
                  : std::string{});
    return r;
}

static s32 sceNetRecvmsgImpl(OrbisNetId s, OrbisNetMsghdr* msg, s32 flags) {
    if (msg == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    if ((flags & ORBIS_NET_MSG_PEEKLEN) == ORBIS_NET_MSG_PEEKLEN) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    std::vector<u8> buffer;
    if (const int e = GatherSize(*msg, &buffer); e != 0) {
        return SetErrno(e);
    }
    u32 namelen = msg->msg_name != nullptr ? msg->msg_namelen : 0;
    const s32 r = sceNetRecvfrom(s, buffer.data(), buffer.size(), flags,
                                 static_cast<OrbisNetSockaddr*>(msg->msg_name),
                                 msg->msg_name != nullptr ? &namelen : nullptr);
    if (r < 0) {
        return r;
    }
    size_t offset = 0;
    for (s32 i = 0; i < msg->msg_iovlen && offset < static_cast<size_t>(r); ++i) {
        const size_t chunk =
            std::min<size_t>(msg->msg_iov[i].iov_len, static_cast<size_t>(r) - offset);
        std::memcpy(msg->msg_iov[i].iov_base, buffer.data() + offset, chunk);
        offset += chunk;
    }
    msg->msg_namelen = namelen;
    return r;
}

s32 PS4_SYSV_ABI sceNetRecvmsg(OrbisNetId s, OrbisNetMsghdr* msg, s32 flags) {
    const s32 r = sceNetRecvmsgImpl(s, msg, flags);
    if (r > 0) {
        CountTraffic(s, false, r);
    }
    LOG_TRACE(Lib_Net, "s = {}, flags = {:#x} -> {}", s, flags,
              r >= 0 ? std::to_string(r) : ErrorCodeName(r));
    return r;
}

s32 PS4_SYSV_ABI sceNetResolverAbort(OrbisNetId rid, s32 flags) {
    LOG_DEBUG(Lib_Net, "rid = {}, flags = {:#x}", rid, flags);
    const int e = AbortResolver(rid, static_cast<u32>(flags));
    return e == 0 ? ORBIS_OK : SetErrno(e);
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverConnect() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverConnectAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverConnectCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverConnectDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

void ReleaseResolverPool(OrbisNetId rid) {
    std::scoped_lock lock{g_pools_mutex};
    if (const auto it = g_resolver_pools.find(rid); it != g_resolver_pools.end()) {
        if (Pool* pool = FindPool(it->second)) {
            --pool->users;
        }
        g_resolver_pools.erase(it);
    }
}

OrbisNetId PS4_SYSV_ABI sceNetResolverCreate(const char* name, s32 poolid, s32 flags) {
    LOG_INFO(Lib_Net, "name = {}, poolid = {}, flags = {:#x}", name != nullptr ? name : "", poolid,
             flags);
    if (flags != 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    if (name != nullptr &&
        strnlen(name, ORBIS_NET_DEBUG_NAME_LEN_MAX + 1) > ORBIS_NET_DEBUG_NAME_LEN_MAX) {
        return SetErrno(ORBIS_NET_ENAMETOOLONG);
    }
    std::scoped_lock lock{g_pools_mutex};
    Pool* pool = FindPool(poolid);
    if (pool == nullptr) {
        LOG_ERROR(Lib_Net, "unknown memory pool id {}", poolid);
        return SetErrno(ORBIS_NET_EBADF);
    }
    const s32 id = CreateResolver(name != nullptr ? name : "");
    if (id < 0) {
        return SetErrno(-id);
    }
    // The resolver holds its pool until destroyed.
    ++pool->users;
    g_resolver_pools[id] = poolid;
    return id;
}

s32 PS4_SYSV_ABI sceNetResolverDestroy(OrbisNetId rid) {
    LOG_DEBUG(Lib_Net, "rid = {}", rid);
    const int e = DestroyResolver(rid);
    if (e != 0) {
        return SetErrno(e);
    }
    ReleaseResolverPool(rid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetResolverGetError(OrbisNetId rid, s32* status) {
    if (status == nullptr) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    const int e = ResolverGetError(rid, status);
    LOG_DEBUG(Lib_Net, "rid = {}, status = {:#x}", rid, static_cast<u32>(e == 0 ? *status : 0));
    return e == 0 ? ORBIS_OK : SetErrno(e);
}

s32 PS4_SYSV_ABI sceNetResolverStartAton(OrbisNetId rid, const OrbisNetInAddr* addr, char* hostname,
                                         s32 len, s32 timeout, s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "rid = {}, timeout = {}, retry = {}, flags = {:#x}", rid, timeout, retry,
             flags);
    return ResolverReturn(
        ResolverStartAton(rid, addr, hostname, len, (flags & ORBIS_NET_RESOLVER_ASYNC) != 0));
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverStartAton6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

s32 PS4_SYSV_ABI sceNetResolverStartNtoa(OrbisNetId rid, const char* hostname, OrbisNetInAddr* addr,
                                         s32 timeout, s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "rid = {}, hostname = {}, timeout = {}, retry = {}, flags = {:#x}", rid,
             hostname != nullptr ? hostname : "", timeout, retry, flags);
    return ResolverReturn(
        ResolverStartNtoa(rid, hostname, {addr, nullptr}, (flags & ORBIS_NET_RESOLVER_ASYNC) != 0,
                          (flags & ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS) != 0));
}

// Not implemented. Fail rather than report success with nothing written.
s32 PS4_SYSV_ABI sceNetResolverStartNtoa6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENOSUPPORT);
}

s32 PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecords(OrbisNetId rid, const char* hostname,
                                                        OrbisNetResolverInfo* info, s32 timeout,
                                                        s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "rid = {}, hostname = {}, timeout = {}, retry = {}, flags = {:#x}", rid,
             hostname != nullptr ? hostname : "", timeout, retry, flags);
    return ResolverReturn(
        ResolverStartNtoa(rid, hostname, {nullptr, info}, (flags & ORBIS_NET_RESOLVER_ASYNC) != 0,
                          (flags & ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS) != 0));
}

// Flags 0x1000000 ask for AAAA records and 0x2000000 for A and AAAA. Only IPv4 is looked up,
// so AAAA-only has no records and A+AAAA returns the A records.
s32 PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecordsEx(OrbisNetId rid, const char* hostname,
                                                          OrbisNetResolverInfo* info, s32 timeout,
                                                          s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "rid = {}, hostname = {}, timeout = {}, retry = {}, flags = {:#x}", rid,
             hostname != nullptr ? hostname : "", timeout, retry, flags);
    constexpr u32 Aaaa = 0x1000000;
    constexpr u32 AAndAaaa = 0x2000000;
    if ((static_cast<u32>(flags) & 0xfccefffeu) != 0) {
        return ResolverReturn(ORBIS_NET_ERROR_EINVAL);
    }
    if ((flags & Aaaa) != 0 && (flags & AAndAaaa) == 0) {
        LOG_WARNING(Lib_Net, "AAAA lookup of {} not supported",
                    hostname != nullptr ? hostname : "");
        return ResolverReturn(ORBIS_NET_ERROR_RESOLVER_ENORECORD);
    }
    return ResolverReturn(
        ResolverStartNtoa(rid, hostname, {nullptr, info}, (flags & ORBIS_NET_RESOLVER_ASYNC) != 0,
                          (flags & ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS) != 0));
}

s32 PS4_SYSV_ABI sceNetSend(OrbisNetId s, const void* buf, u64 len, s32 flags) {
    return sceNetSendto(s, buf, len, flags, nullptr, 0);
}

static s32 sceNetSendmsgImpl(OrbisNetId s, const OrbisNetMsghdr* msg, s32 flags) {
    if (msg == nullptr) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    if (msg->msg_control != nullptr && msg->msg_controllen != 0) {
        LOG_WARNING(Lib_Net, "ancillary data ignored ({} bytes)", msg->msg_controllen);
    }
    // Gather into one buffer so a datagram goes out whole.
    std::vector<u8> buffer;
    if (const int e = GatherSize(*msg, &buffer); e != 0) {
        return SetErrno(e);
    }
    size_t offset = 0;
    for (s32 i = 0; i < msg->msg_iovlen; ++i) {
        std::memcpy(buffer.data() + offset, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
        offset += msg->msg_iov[i].iov_len;
    }
    return sceNetSendto(s, buffer.data(), buffer.size(), flags,
                        static_cast<const OrbisNetSockaddr*>(msg->msg_name), msg->msg_namelen);
}

s32 PS4_SYSV_ABI sceNetSendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, s32 flags) {
    const s32 r = sceNetSendmsgImpl(s, msg, flags);
    if (r > 0) {
        CountTraffic(s, true, r);
    }
    LOG_TRACE(Lib_Net, "s = {}, flags = {:#x} -> {}", s, flags,
              r >= 0 ? std::to_string(r) : ErrorCodeName(r));
    return r;
}

static s32 sceNetSendtoImpl(OrbisNetId s, const void* buf, u64 len, s32 flags,
                            const OrbisNetSockaddr* addr, u32 addrlen) {
    if (buf == nullptr && len != 0) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    const auto msg = ToHostMsgFlags(flags);
    if (msg.unsupported != 0) {
        LOG_WARNING(Lib_Net, "ignoring flags {:#x}", msg.unsupported);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }
    if (info.type != SOCK_STREAM) {
        Core::Net::SocketAttributes attributes;
        Core::Net::SocketGetAttributes(s, &attributes);
        const s32 limit = info.type == SOCK_RAW     ? MaxRawSend
                          : attributes.snd_buf != 0 ? attributes.snd_buf
                                                    : DefaultSendBuffer(info.type);
        if (len > static_cast<u64>(limit)) {
            return SetErrno(ORBIS_NET_EMSGSIZE);
        }
    }
    if (addr != nullptr && info.connected) {
        if (info.type != SOCK_STREAM) {
            return SetErrno(ORBIS_NET_EISCONN);
        }
        addr = nullptr;
        addrlen = 0;
    }
    if (info.p2p) {
        P2P::Endpoint to;
        u16 to_vport = 0;
        if (addr != nullptr) {
            if (const int e = ToP2PAddress(addr, addrlen, KindOf(info), &to, &to_vport); e != 0) {
                return SetErrno(e);
            }
            if (info.type == SOCK_DGRAM && !IsCompleteP2PAddress(*addr, P2PKind::Datagram)) {
                return SetErrno(ORBIS_NET_EADDRNOTAVAIL);
            }
        }
        if ((msg.crypto || msg.signature) && info.type == SOCK_STREAM) {
            LOG_WARNING(Lib_Net,
                        "per-message protection ignored on a stream (fixed per connection)");
        }
        return Return(Core::Net::P2PSocketSendTo(s, buf, static_cast<size_t>(len), msg.dontwait,
                                                 addr ? &to : nullptr, to_vport,
                                                 {msg.crypto, msg.signature}));
    }
    sockaddr_storage host{};
    socklen_t host_len = 0;
    if (addr != nullptr) {
        if (const int e = ToHostSockaddr(addr, addrlen, &host, &host_len); e != 0) {
            return SetErrno(e);
        }
    }
    return Return(Core::Net::SocketSendTo(s, buf, static_cast<size_t>(len), msg.host, msg.dontwait,
                                          addr ? reinterpret_cast<sockaddr*>(&host) : nullptr,
                                          host_len));
}

s32 PS4_SYSV_ABI sceNetSendto(OrbisNetId s, const void* buf, u64 len, s32 flags,
                              const OrbisNetSockaddr* addr, u32 addrlen) {
    const s32 r = sceNetSendtoImpl(s, buf, len, flags, addr, addrlen);
    if (r > 0) {
        CountTraffic(s, true, r);
        NoteP2PPeer(s, true, addr, addrlen);
    }
    LOG_TRACE(Lib_Net, "s = {}, len = {}, flags = {:#x}, to = {} -> {}", s, len, flags,
              addr != nullptr ? FormatSockaddr(addr, addrlen) : "connected peer",
              r >= 0 ? std::to_string(r) : ErrorCodeName(r));
    return r;
}

s32 PS4_SYSV_ABI sceNetSetDns6InfoToKernel() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSetDnsInfoToKernel() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSetsockopt(OrbisNetId s, s32 level, s32 optname, const void* optval,
                                  u32 optlen) {
    if (optval != nullptr && optlen >= sizeof(s32)) {
        s32 logged;
        std::memcpy(&logged, optval, sizeof(logged));
        LOG_DEBUG(Lib_Net, "s = {}, {} = {} (optlen {})", s, OptionName(level, optname), logged,
                  optlen);
    } else {
        LOG_DEBUG(Lib_Net, "s = {}, {} (optlen {})", s, OptionName(level, optname), optlen);
    }
    if (optval == nullptr && optlen != 0) {
        return SetErrno(ORBIS_NET_EFAULT);
    }
    Core::Net::SocketInfo info{};
    if (const auto r = Core::Net::SocketGetInfo(s, &info); r.error != Error::Ok) {
        return Return(r);
    }

    const OptionRule* rule = FindOptionRule(level, optname);
    if (rule == nullptr || !rule->set) {
        return SetErrno(ORBIS_NET_ENOPROTOOPT);
    }

    // Multicast TTL/loop also accept a single byte.
    s32 value = 0;
    bool has_int = false;
    if (optval != nullptr && optlen >= sizeof(s32)) {
        std::memcpy(&value, optval, sizeof(value));
        has_int = true;
    } else if (optval != nullptr && optlen == 1 && rule->byte_or_int) {
        value = *static_cast<const u8*>(optval);
        has_int = true;
    }
    const auto store = [&](s32 v) {
        return Return(Core::Net::SocketUpdateAttributes(
            s, [&](Core::Net::SocketAttributes& a) { a.stored_options[{level, optname}] = v; }));
    };

    if (!(rule->valid_for & KindBit(info))) {
        if (!rule->others_ignored) {
            return SetErrno(ORBIS_NET_EPROCUNAVAIL);
        }
        return has_int ? store(value) : ORBIS_OK;
    }

    if (level == ORBIS_NET_SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_NBIO:
            return has_int ? Return(Core::Net::SocketSetNonBlocking(s, value != 0))
                           : SetErrno(ORBIS_NET_EINVAL);
        case ORBIS_NET_SO_RCVTIMEO:
        case ORBIS_NET_SO_SNDTIMEO:
        case ORBIS_NET_SO_CONNECTTIMEO:
        case ORBIS_NET_SO_ACCEPTTIMEO: {
            if (!has_int) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            // Microseconds, <= 0 means no timeout.
            const std::chrono::microseconds timeout{std::max(value, 0)};
            switch (optname) {
            case ORBIS_NET_SO_RCVTIMEO:
                return Return(Core::Net::SocketSetRecvTimeout(s, timeout));
            case ORBIS_NET_SO_SNDTIMEO:
                return Return(Core::Net::SocketSetSendTimeout(s, timeout));
            case ORBIS_NET_SO_CONNECTTIMEO:
                return Return(Core::Net::SocketSetConnectTimeout(s, timeout));
            default:
                return Return(Core::Net::SocketSetAcceptTimeout(s, timeout));
            }
        }
        case BsdRcvTimeo:
        case BsdSndTimeo: {
            const auto timeout = ReadTimeval(optval, optlen);
            if (!timeout) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            return Return(optname == BsdRcvTimeo ? Core::Net::SocketSetRecvTimeout(s, *timeout)
                                                 : Core::Net::SocketSetSendTimeout(s, *timeout));
        }
        case ORBIS_NET_SO_REUSEADDR:
            return has_int ? Return(Core::Net::SocketSetReuseAddr(s, value != 0))
                           : SetErrno(ORBIS_NET_EINVAL);
        case ORBIS_NET_SO_REUSEPORT:
            if (!has_int) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            if (!info.p2p) {
                Core::Net::SocketSetReusePort(s, value != 0);
            }
            return store(value);
        case ORBIS_NET_SO_USECRYPTO:
        case ORBIS_NET_SO_USESIGNATURE: {
            if (!has_int) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            bool crypto = false;
            bool signature = false;
            Core::Net::P2PSocketGetProtection(s, &crypto, &signature);
            (optname == ORBIS_NET_SO_USECRYPTO ? crypto : signature) = value != 0;
            return Return(Core::Net::P2PSocketSetProtection(s, crypto, signature));
        }
        case ORBIS_NET_SO_SNDBUF:
        case ORBIS_NET_SO_RCVBUF: {
            if (!has_int || value <= 0) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            value = std::min(value, MaxSocketBuffer);
            Core::Net::SocketUpdateAttributes(s, [&](Core::Net::SocketAttributes& a) {
                (optname == ORBIS_NET_SO_SNDBUF ? a.snd_buf : a.rcv_buf) = value;
            });
            if (info.p2p || KindBit(info) == KindRaw) {
                return ORBIS_OK; // meaningless for RAW
            }
            break;
        }
        case ORBIS_NET_SO_POLICY:
            if (!has_int ||
                (value != ORBIS_NET_SOCK_POLICY_NA && (value < ORBIS_NET_SOCK_POLICY_NUM_MIN ||
                                                       value > ORBIS_NET_SOCK_POLICY_NUM_MAX))) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            return Return(Core::Net::SocketUpdateAttributes(
                s, [&](Core::Net::SocketAttributes& a) { a.policy = value; }));
        case ORBIS_NET_SO_PRIORITY:
            if (!has_int || value < ORBIS_NET_SOCK_PRIORITY_NUM_USER_MIN ||
                value > ORBIS_NET_SOCK_PRIORITY_NUM_USER_MAX) {
                return SetErrno(ORBIS_NET_EINVAL);
            }
            return Return(Core::Net::SocketUpdateAttributes(
                s, [&](Core::Net::SocketAttributes& a) { a.priority = value; }));
        case ORBIS_NET_SO_NAME: {
            const auto* text = static_cast<const char*>(optval);
            const size_t length = text ? strnlen(text, optlen) : 0;
            if (length > static_cast<size_t>(ORBIS_NET_DEBUG_NAME_LEN_MAX)) {
                return SetErrno(ORBIS_NET_ENAMETOOLONG);
            }
            return Return(Core::Net::SocketUpdateAttributes(s, [&](Core::Net::SocketAttributes& a) {
                a.name = {};
                std::memcpy(a.name.data(), text, length);
            }));
        }
        case ORBIS_NET_SO_ONESBCAST:
            // TODO: 255.255.255.255 isn't converted to the interface broadcast address. Hosts
            // deliver it on the LAN anyway.
            return has_int ? store(value) : SetErrno(ORBIS_NET_EINVAL);
        default:
            break;
        }
    }
    if (level == ORBIS_NET_IPPROTO_UDP && optname == ORBIS_NET_UDP_SND_ON_SUSPEND) {
        return SetSendOnSuspend(s, info, optval, optlen);
    }
    if (level == ORBIS_NET_IPPROTO_TCP && optname == ORBIS_NET_TCP_MSS_TO_ADVERTISE) {
        // 0 means the interface MSS.
        if (!has_int || (value != 0 && value < 216)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
    }

    const auto option = ToHostOption(level, optname);
    if (level == ORBIS_NET_SOL_SOCKET && optname == ORBIS_NET_SO_LINGER && info.p2p) {
        // Stored whole so getsockopt can return it.
        // TODO: not applied on P2P TCP close.
        if (optlen < sizeof(OrbisNetLinger)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        OrbisNetLinger guest;
        std::memcpy(&guest, optval, sizeof(guest));
        return Return(Core::Net::SocketUpdateAttributes(s, [&](Core::Net::SocketAttributes& a) {
            a.stored_options[{level, optname}] = guest.l_onoff;
            a.stored_options[{level, LingerSecondsKey}] = guest.l_linger;
        }));
    }
#ifdef _WIN32
    if (level == ORBIS_NET_IPPROTO_TCP && optname == ORBIS_NET_TCP_MAXSEG && !info.p2p) {
        // Windows can read TCP_MAXSEG but not set it, so just store it.
        if (!has_int) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        return store(value);
    }
#endif
    if (info.p2p || !option || rule->stored) {
        if (!has_int) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        if (!info.p2p && level == ORBIS_NET_IPPROTO_IP && optname == IpDontFrag) {
            if (const int e = SetHostDontFragment(s, value != 0); e != 0) {
                return SetErrno(e);
            }
        }
        return store(value);
    }
    switch (option->value) {
    case OptionValue::Int: {
        if (!has_int) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        const int host_value = value;
        return Return(Core::Net::SocketSetHostOption(s, option->level, option->name, &host_value,
                                                     sizeof(host_value)));
    }
    case OptionValue::Linger: {
        if (optlen < sizeof(OrbisNetLinger)) {
            return SetErrno(ORBIS_NET_EINVAL);
        }
        OrbisNetLinger guest;
        std::memcpy(&guest, optval, sizeof(guest));
        linger host{};
        host.l_onoff = static_cast<decltype(host.l_onoff)>(guest.l_onoff);
        host.l_linger = static_cast<decltype(host.l_linger)>(guest.l_linger);
        return Return(
            Core::Net::SocketSetHostOption(s, option->level, option->name, &host, sizeof(host)));
    }
    case OptionValue::Raw:
        if (level == ORBIS_NET_IPPROTO_IP &&
            (optname == ORBIS_NET_IP_ADD_MEMBERSHIP || optname == ORBIS_NET_IP_DROP_MEMBERSHIP)) {
            return SetMembership(s, optname == ORBIS_NET_IP_ADD_MEMBERSHIP, *option, optval,
                                 optlen);
        }
        return Return(Core::Net::SocketSetHostOption(s, option->level, option->name, optval,
                                                     static_cast<socklen_t>(optlen)));
    }
    return SetErrno(ORBIS_NET_ENOPROTOOPT);
}

s32 PS4_SYSV_ABI sceNetShowIfconfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowIfconfigForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowIfconfigWithMemory(s32 memid) {
    if (const s32 e = CheckPoolId(memid); e != 0) {
        return SetErrno(e);
    }
    LOG_ERROR(Lib_Net, "(STUBBED) called, memid = {}", memid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowNetstat() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowNetstatEx() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowNetstatExForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowNetstatForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowNetstatWithMemory(s32 memid) {
    if (const s32 e = CheckPoolId(memid); e != 0) {
        return SetErrno(e);
    }
    LOG_ERROR(Lib_Net, "(STUBBED) called, memid = {}", memid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowPolicyWithMemory(s32 memid) {
    if (const s32 e = CheckPoolId(memid); e != 0) {
        return SetErrno(e);
    }
    LOG_ERROR(Lib_Net, "(STUBBED) called, memid = {}", memid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRoute6ForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRoute6WithMemory(s32 memid) {
    if (const s32 e = CheckPoolId(memid); e != 0) {
        return SetErrno(e);
    }
    LOG_ERROR(Lib_Net, "(STUBBED) called, memid = {}", memid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRouteForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShowRouteWithMemory(s32 memid) {
    if (const s32 e = CheckPoolId(memid); e != 0) {
        return SetErrno(e);
    }
    LOG_ERROR(Lib_Net, "(STUBBED) called, memid = {}", memid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetShutdown(OrbisNetId s, s32 how) {
    LOG_DEBUG(Lib_Net, "s = {}, how = {}", s, how);
    if (how < ORBIS_NET_SHUT_RD || how > ORBIS_NET_SHUT_RDWR) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    return Return(Core::Net::SocketShutdown(s, how));
}

static OrbisNetId sceNetSocketImpl(const char* name, s32 family, s32 type, s32 protocol) {
    int error = 0;
    const auto kind = ToHostSocketKind(family, type, protocol, &error);
    if (!kind) {
        return SetErrno(error);
    }
    // Names over 31 chars are ENAMETOOLONG, not truncated.
    if (name != nullptr &&
        strnlen(name, ORBIS_NET_DEBUG_NAME_LEN_MAX + 1) > ORBIS_NET_DEBUG_NAME_LEN_MAX) {
        return SetErrno(ORBIS_NET_ENAMETOOLONG);
    }
    if (kind->p2p && !EnsureP2PTransport()) {
        return SetErrno(ORBIS_NET_EADDRINUSE); // P2P port taken
    }
    const auto r = kind->p2p ? Core::Net::P2PSocketCreate(kind->family, kind->type == SOCK_STREAM)
                             : Core::Net::SocketCreate(kind->family, kind->type, kind->protocol);
    if (r.error == Error::Ok && name != nullptr) {
        Core::Net::SocketUpdateAttributes(static_cast<s32>(r.value),
                                          [&](Core::Net::SocketAttributes& a) {
                                              std::strncpy(a.name.data(), name, a.name.size() - 1);
                                          });
    }
    return Return(r);
}

OrbisNetId PS4_SYSV_ABI sceNetSocket(const char* name, s32 family, s32 type, s32 protocol) {
    const OrbisNetId r = sceNetSocketImpl(name, family, type, protocol);
    LOG_INFO(Lib_Net, "'{}' family {} {} protocol {} -> {}", name != nullptr ? name : "", family,
             SocketTypeName(type), protocol, r >= 0 ? std::to_string(r) : ErrorCodeName(r));
    return r;
}

s32 PS4_SYSV_ABI sceNetSocketAbort(OrbisNetId s, s32 flags) {
    LOG_DEBUG(Lib_Net, "s = {}, flags = {:#x}", s, flags);
    return Return(Core::Net::SocketAbort(s, static_cast<u32>(flags)));
}

s32 PS4_SYSV_ABI sceNetSocketClose(OrbisNetId s) {
    const auto r = Core::Net::SocketClose(s);
    if (r.error == Error::Ok) {
        LogSocketClosed(s);
    }
    return Return(r);
}

s32 PS4_SYSV_ABI sceNetSyncCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSyncDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSyncGet() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSyncSignal() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSyncWait() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetSysctl() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetTerm() {
    LOG_INFO(Lib_Net, "called");
    return ORBIS_OK;
}

namespace {
struct OrbisNetThreadParam {
    void(PS4_SYSV_ABI* entry)(void* arg);
    void* arg;
    u64 reserved;
    u32 flags;
};

void* PS4_SYSV_ABI NetThreadEntry(void* p) {
    const auto* param = static_cast<const OrbisNetThreadParam*>(p);
    param->entry(param->arg);
    if ((param->flags & 1) != 0) {
        Kernel::posix_pthread_exit(nullptr);
    }
    return nullptr;
}

s32 ThreadReturn(s32 code) {
    if (code < 0) {
        g_net_errno = (static_cast<u32>(code) & 0xff00) == 0x100
                          ? static_cast<s32>(static_cast<u32>(code) & 0xff)
                          : 0xcd;
    }
    return code;
}
} // namespace

s32 PS4_SYSV_ABI sceNetThreadCreate(Kernel::PthreadT* thread, void* param, const char* name) {
    if (param == nullptr) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    Kernel::PthreadT created = nullptr;
    const int e =
        Kernel::posix_pthread_create_name_np(&created, nullptr, NetThreadEntry, param, name);
    if (e != 0) {
        LOG_ERROR(Lib_Net, "thread '{}' not created: {}", name != nullptr ? name : "", e);
        return ThreadReturn(ORBIS_KERNEL_ERROR_UNKNOWN + e);
    }
    if (thread != nullptr) {
        *thread = created;
    }
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceNetThreadExit() {
    Kernel::posix_pthread_exit(nullptr);
}

s32 PS4_SYSV_ABI sceNetThreadJoin(Kernel::PthreadT thread) {
    const int e = Kernel::posix_pthread_join(thread, nullptr);
    return e == 0 ? ORBIS_OK : ThreadReturn(ORBIS_KERNEL_ERROR_UNKNOWN + e);
}

s32 PS4_SYSV_ABI sceNetUsleep(s32 microseconds) {
    if (microseconds < 0) {
        return SetErrno(ORBIS_NET_EINVAL);
    }
    return Kernel::sceKernelUsleep(static_cast<u32>(microseconds));
}

s32 PS4_SYSV_ABI Func_0E707A589F751C68() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNetEmulationGet() {
    LOG_WARNING(Lib_Net, "network emulation is not available on retail units");
    return SetErrno(ORBIS_NET_EOPNOTSUPP);
}

s32 PS4_SYSV_ABI sceNetEmulationSet() {
    LOG_WARNING(Lib_Net, "network emulation is not available on retail units");
    return SetErrno(ORBIS_NET_EOPNOTSUPP);
}

void SetKernelErrnoHook(void (*hook)(int orbis_errno)) {
    g_kernel_errno_hook.store(hook);
}

SystemHooks GetSystemHooks() {
    std::scoped_lock lock{g_hooks_mutex};
    return g_hooks;
}

void SetSystemHooks(SystemHooks hooks) {
    SetResolverOnlineCheck(hooks.is_online);
    std::scoped_lock lock{g_hooks_mutex};
    g_hooks = std::move(hooks);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_OBJ("ZRAJo-A-ukc", "libSceNet", 1, "libSceNet", &g_in6addr_any);
    LIB_OBJ("XCuA-GqjA-k", "libSceNet", 1, "libSceNet", &g_in6addr_loopback);
    LIB_OBJ("VZgoeBxPXUQ", "libSceNet", 1, "libSceNet", &g_sce_net_dummy);
    LIB_OBJ("GAtITrgxKDE", "libSceNet", 1, "libSceNet", &g_sce_net_in6addr_any);
    LIB_OBJ("84MgU4MMTLQ", "libSceNet", 1, "libSceNet", &g_sce_net_in6addr_linklocal_allnodes);
    LIB_OBJ("2uSWyOKYc1M", "libSceNet", 1, "libSceNet", &g_sce_net_in6addr_linklocal_allrouters);
    LIB_OBJ("P3AeWBvPrkg", "libSceNet", 1, "libSceNet", &g_sce_net_in6addr_loopback);
    LIB_OBJ("PgNI+j4zxzM", "libSceNet", 1, "libSceNet", &g_sce_net_in6addr_nodelocal_allnodes);
    LIB_FUNCTION("PIWqhn9oSxc", "libSceNet", 1, "libSceNet", sceNetAccept);
    LIB_FUNCTION("BTUvkWzrP68", "libSceNet", 1, "libSceNet", sceNetAddrConfig6GetInfo);
    LIB_FUNCTION("3qG7UJy2Fq8", "libSceNet", 1, "libSceNet", sceNetAddrConfig6Start);
    LIB_FUNCTION("P+0ePpDfUAQ", "libSceNet", 1, "libSceNet", sceNetAddrConfig6Stop);
    LIB_FUNCTION("PcdLABhYga4", "libSceNet", 1, "libSceNet", sceNetAllocateAllRouteInfo);
    LIB_FUNCTION("xHq87H78dho", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetDataTraffic);
    LIB_FUNCTION("c8IRpl4L74I", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetDefaultParam);
    LIB_FUNCTION("b9Ft65tqvLk", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetIfParam);
    LIB_FUNCTION("PDkapOwggRw", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetPolicy);
    LIB_FUNCTION("P4zZXE7bpsA", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetDefaultParam);
    LIB_FUNCTION("g4DKkzV2qC4", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetIfParam);
    LIB_FUNCTION("7Z1hhsEmkQU", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetPolicy);
    LIB_FUNCTION("bErx49PgxyY", "libSceNet", 1, "libSceNet", sceNetBind);
    LIB_FUNCTION("eyLyLJrdEOU", "libSceNet", 1, "libSceNet", sceNetClearDnsCache);
    LIB_FUNCTION("Ea2NaVMQNO8", "libSceNet", 1, "libSceNet", sceNetConfigAddArp);
    LIB_FUNCTION("0g0qIuPN3ZQ", "libSceNet", 1, "libSceNet", sceNetConfigAddArpWithInterface);
    LIB_FUNCTION("ge7g15Sqhks", "libSceNet", 1, "libSceNet", sceNetConfigAddIfaddr);
    LIB_FUNCTION("FDHr4Iz7dQU", "libSceNet", 1, "libSceNet", sceNetConfigAddMRoute);
    LIB_FUNCTION("Cyjl1yzi1qY", "libSceNet", 1, "libSceNet", sceNetConfigAddRoute);
    LIB_FUNCTION("Bu+L5r1lKRg", "libSceNet", 1, "libSceNet", sceNetConfigAddRoute6);
    LIB_FUNCTION("wIGold7Lro0", "libSceNet", 1, "libSceNet", sceNetConfigAddRouteWithInterface);
    LIB_FUNCTION("MzA1YrRE6rA", "libSceNet", 1, "libSceNet", sceNetConfigCleanUpAllInterfaces);
    LIB_FUNCTION("HJt+4x-CnY0", "libSceNet", 1, "libSceNet", sceNetConfigDelArp);
    LIB_FUNCTION("xTcttXJ3Utg", "libSceNet", 1, "libSceNet", sceNetConfigDelArpWithInterface);
    LIB_FUNCTION("RuVwHEW6dM4", "libSceNet", 1, "libSceNet", sceNetConfigDelDefaultRoute);
    LIB_FUNCTION("UMlVCy7RX1s", "libSceNet", 1, "libSceNet", sceNetConfigDelDefaultRoute6);
    LIB_FUNCTION("0239JNsI6PE", "libSceNet", 1, "libSceNet", sceNetConfigDelIfaddr);
    LIB_FUNCTION("hvCXMwd45oc", "libSceNet", 1, "libSceNet", sceNetConfigDelIfaddr6);
    LIB_FUNCTION("5Yl1uuh5i-A", "libSceNet", 1, "libSceNet", sceNetConfigDelMRoute);
    LIB_FUNCTION("QO7+2E3cD-U", "libSceNet", 1, "libSceNet", sceNetConfigDelRoute);
    LIB_FUNCTION("4wDGvfhmkmk", "libSceNet", 1, "libSceNet", sceNetConfigDelRoute6);
    LIB_FUNCTION("3WzWV86AJ3w", "libSceNet", 1, "libSceNet", sceNetConfigDownInterface);
    LIB_FUNCTION("mOUkgTaSkJU", "libSceNet", 1, "libSceNet", sceNetConfigEtherGetLinkMode);
    LIB_FUNCTION("pF3Vy1iZ5bs", "libSceNet", 1, "libSceNet", sceNetConfigEtherPostPlugInOutEvent);
    LIB_FUNCTION("QltDK6wWqF0", "libSceNet", 1, "libSceNet", sceNetConfigEtherSetLinkMode);
    LIB_FUNCTION("18KNgSvYx+Y", "libSceNet", 1, "libSceNet", sceNetConfigFlushRoute);
    LIB_FUNCTION("lFJb+BlPK1c", "libSceNet", 1, "libSceNet", sceNetConfigGetDefaultRoute);
    LIB_FUNCTION("mCLdiNIKtW0", "libSceNet", 1, "libSceNet", sceNetConfigGetDefaultRoute6);
    LIB_FUNCTION("ejwa0hWWhDs", "libSceNet", 1, "libSceNet", sceNetConfigGetIfaddr);
    LIB_FUNCTION("FU6NK4RHQVE", "libSceNet", 1, "libSceNet", sceNetConfigGetIfaddr6);
    LIB_FUNCTION("vbZLomImmEE", "libSceNet", 1, "libSceNet", sceNetConfigRoutingShowRoutingConfig);
    LIB_FUNCTION("a6sS6iSE0IA", "libSceNet", 1, "libSceNet", sceNetConfigRoutingShowtCtlVar);
    LIB_FUNCTION("eszLdtIMfQE", "libSceNet", 1, "libSceNet", sceNetConfigRoutingStart);
    LIB_FUNCTION("toi8xxcSfJ0", "libSceNet", 1, "libSceNet", sceNetConfigRoutingStop);
    LIB_FUNCTION("EAl7xvi7nXg", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultRoute);
    LIB_FUNCTION("4zLOHbt3UFk", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultRoute6);
    LIB_FUNCTION("yaVAdLDxUj0", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultScope);
    LIB_FUNCTION("8Kh+1eidI3c", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr);
    LIB_FUNCTION("QJbV3vfBQ8Q", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr6);
    LIB_FUNCTION("POrSEl8zySw", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr6WithFlags);
    LIB_FUNCTION("0sesmAYH3Lk", "libSceNet", 1, "libSceNet", sceNetConfigSetIfFlags);
    LIB_FUNCTION("uNTluLfYgS8", "libSceNet", 1, "libSceNet", sceNetConfigSetIfLinkLocalAddr6);
    LIB_FUNCTION("s31rYkpIMMQ", "libSceNet", 1, "libSceNet", sceNetConfigSetIfmtu);
    LIB_FUNCTION("tvdzQkm+UaY", "libSceNet", 1, "libSceNet", sceNetConfigUnsetIfFlags);
    LIB_FUNCTION("oGEBX0eXGFs", "libSceNet", 1, "libSceNet", sceNetConfigUpInterface);
    LIB_FUNCTION("6HNbayHPL7c", "libSceNet", 1, "libSceNet", sceNetConfigUpInterfaceWithFlags);
    LIB_FUNCTION("6A6EweB3Dto", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocClearWakeOnWlan);
    LIB_FUNCTION("ZLdJyQJUMkM", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocCreate);
    LIB_FUNCTION("Yr3UeApLWTY", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocGetWakeOnWlanInfo);
    LIB_FUNCTION("Xma8yHmV+TQ", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocJoin);
    LIB_FUNCTION("K4o48GTNbSc", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocLeave);
    LIB_FUNCTION("ZvKgNrrLCCQ", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuClearWakeOnWlan);
    LIB_FUNCTION("1j4DZ5dXbeQ", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo);
    LIB_FUNCTION("C-+JPjaEhdA", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuSetWakeOnWlan);
    LIB_FUNCTION("7xYdUWg1WdY", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocScanJoin);
    LIB_FUNCTION("Q7ee2Uav5f8", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocSetExtInfoElement);
    LIB_FUNCTION("xaOTiuxIQNY", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocSetWakeOnWlan);
    LIB_FUNCTION("QlRJWya+dtE", "libSceNet", 1, "libSceNet", sceNetConfigWlanApStart);
    LIB_FUNCTION("6uYcvVjH7Ms", "libSceNet", 1, "libSceNet", sceNetConfigWlanApStop);
    LIB_FUNCTION("MDbg-oAj8Aw", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanQuery);
    LIB_FUNCTION("cMA8f6jI6s0", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanStart);
    LIB_FUNCTION("3T5aIe-7L84", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanStop);
    LIB_FUNCTION("+3KMyS93TOs", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagGetDeviceInfo);
    LIB_FUNCTION("9oiOWQ5FMws", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagSetAntenna);
    LIB_FUNCTION("fHr45B97n0U", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagSetTxFixedRate);
    LIB_FUNCTION("PNDDxnqqtk4", "libSceNet", 1, "libSceNet", sceNetConfigWlanGetDeviceConfig);
    LIB_FUNCTION("Pkx0lwWVzmQ", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraGetRssiInfo);
    LIB_FUNCTION("IkBCxG+o4Nk", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraLeave);
    LIB_FUNCTION("273-I-zD8+8", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraScanJoin);
    LIB_FUNCTION("-Mi5hNiWC4c", "libSceNet", 1, "libSceNet", sceNetConfigWlanScan);
    LIB_FUNCTION("U1q6DrPbY6k", "libSceNet", 1, "libSceNet", sceNetConfigWlanSetDeviceConfig);
    LIB_FUNCTION("OXXX4mUk3uk", "libSceNet", 1, "libSceNet", sceNetConnect);
    LIB_FUNCTION("lDTIbqNs0ps", "libSceNet", 1, "libSceNet", sceNetControl);
    LIB_FUNCTION("Q6T-zIblNqk", "libSceNet", 1, "libSceNet", sceNetDhcpdStart);
    LIB_FUNCTION("xwWm8jzrpeM", "libSceNet", 1, "libSceNet", sceNetDhcpdStop);
    LIB_FUNCTION("KhQxhlEslo0", "libSceNet", 1, "libSceNet", sceNetDhcpGetAutoipInfo);
    LIB_FUNCTION("ix4LWXd12F0", "libSceNet", 1, "libSceNet", sceNetDhcpGetInfo);
    LIB_FUNCTION("DrZuCQDnm3w", "libSceNet", 1, "libSceNet", sceNetDhcpGetInfoEx);
    LIB_FUNCTION("Wzv6dngR-DQ", "libSceNet", 1, "libSceNet", sceNetDhcpStart);
    LIB_FUNCTION("6AN7OlSMWk0", "libSceNet", 1, "libSceNet", sceNetDhcpStop);
    LIB_FUNCTION("+ezgWao0wo8", "libSceNet", 1, "libSceNet", sceNetDumpAbort);
    LIB_FUNCTION("bghgkeLKq1Q", "libSceNet", 1, "libSceNet", sceNetDumpCreate);
    LIB_FUNCTION("xZ54Il-u1vs", "libSceNet", 1, "libSceNet", sceNetDumpDestroy);
    LIB_FUNCTION("YWTpt45PxbI", "libSceNet", 1, "libSceNet", sceNetDumpRead);
    LIB_FUNCTION("TwjkDIPdZ1Q", "libSceNet", 1, "libSceNet", sceNetDuplicateIpStart);
    LIB_FUNCTION("QCbvCx9HL30", "libSceNet", 1, "libSceNet", sceNetDuplicateIpStop);
    LIB_FUNCTION("w21YgGGNtBk", "libSceNet", 1, "libSceNet", sceNetEpollAbort);
    LIB_FUNCTION("ZVw46bsasAk", "libSceNet", 1, "libSceNet", sceNetEpollControl);
    LIB_FUNCTION("SF47kB2MNTo", "libSceNet", 1, "libSceNet", sceNetEpollCreate);
    LIB_FUNCTION("Inp1lfL+Jdw", "libSceNet", 1, "libSceNet", sceNetEpollDestroy);
    LIB_FUNCTION("drjIbDbA7UQ", "libSceNet", 1, "libSceNet", sceNetEpollWait);
    LIB_FUNCTION("HQOwnfMGipQ", "libSceNet", 1, "libSceNet", sceNetErrnoLoc);
    LIB_FUNCTION("v6M4txecCuo", "libSceNet", 1, "libSceNet", sceNetEtherNtostr);
    LIB_FUNCTION("b-bFZvNV59I", "libSceNet", 1, "libSceNet", sceNetEtherStrton);
    LIB_FUNCTION("cWGGXoeZUzA", "libSceNet", 1, "libSceNet", sceNetEventCallbackCreate);
    LIB_FUNCTION("jzP0MoZpYnI", "libSceNet", 1, "libSceNet", sceNetEventCallbackDestroy);
    LIB_FUNCTION("tB3BB8AsrjU", "libSceNet", 1, "libSceNet", sceNetEventCallbackGetError);
    LIB_FUNCTION("5isaotjMWlA", "libSceNet", 1, "libSceNet", sceNetEventCallbackWaitCB);
    LIB_FUNCTION("2ee14ktE1lw", "libSceNet", 1, "libSceNet", sceNetFreeAllRouteInfo);
    LIB_FUNCTION("q8j9OSdnN1Y", "libSceNet", 1, "libSceNet", sceNetGetArpInfo);
    LIB_FUNCTION("wmoIm94hqik", "libSceNet", 1, "libSceNet", sceNetGetDns6Info);
    LIB_FUNCTION("nCL0NyZsd5A", "libSceNet", 1, "libSceNet", sceNetGetDnsInfo);
    LIB_FUNCTION("HoV-GJyx7YY", "libSceNet", 1, "libSceNet", sceNetGetIfList);
    LIB_FUNCTION("ahiOMqoYYMc", "libSceNet", 1, "libSceNet", sceNetGetIfListOnce);
    LIB_FUNCTION("0MT2l3uIX7c", "libSceNet", 1, "libSceNet", sceNetGetIfName);
    LIB_FUNCTION("5lrSEHdqyos", "libSceNet", 1, "libSceNet", sceNetGetIfnameNumList);
    LIB_FUNCTION("6Oc0bLsIYe0", "libSceNet", 1, "libSceNet", sceNetGetMacAddress);
    LIB_FUNCTION("rMyh97BU5pY", "libSceNet", 1, "libSceNet", sceNetGetMemoryPoolStats);
    LIB_FUNCTION("+S-2-jlpaBo", "libSceNet", 1, "libSceNet", sceNetGetNameToIndex);
    LIB_FUNCTION("TCkRD0DWNLg", "libSceNet", 1, "libSceNet", sceNetGetpeername);
    LIB_FUNCTION("G3O2j9f5z00", "libSceNet", 1, "libSceNet", sceNetGetRandom);
    LIB_FUNCTION("6Nx1hIQL9h8", "libSceNet", 1, "libSceNet", sceNetGetRouteInfo);
    LIB_FUNCTION("hLuXdjHnhiI", "libSceNet", 1, "libSceNet", sceNetGetSockInfo);
    LIB_FUNCTION("Cidi9Y65mP8", "libSceNet", 1, "libSceNet", sceNetGetSockInfo6);
    LIB_FUNCTION("hoOAofhhRvE", "libSceNet", 1, "libSceNet", sceNetGetsockname);
    LIB_FUNCTION("xphrZusl78E", "libSceNet", 1, "libSceNet", sceNetGetsockopt);
    LIB_FUNCTION("GA5ZDaLtUBE", "libSceNet", 1, "libSceNet", sceNetGetStatisticsInfo);
    LIB_FUNCTION("9mIcUExH34w", "libSceNet", 1, "libSceNet", sceNetGetStatisticsInfoInternal);
    LIB_FUNCTION("p2vxsE2U3RQ", "libSceNet", 1, "libSceNet", sceNetGetSystemTime);
    LIB_FUNCTION("9T2pDF2Ryqg", "libSceNet", 1, "libSceNet", sceNetHtonl);
    LIB_FUNCTION("3CHi1K1wsCQ", "libSceNet", 1, "libSceNet", sceNetHtonll);
    LIB_FUNCTION("iWQWrwiSt8A", "libSceNet", 1, "libSceNet", sceNetHtons);
    LIB_FUNCTION("9vA2aW+CHuA", "libSceNet", 1, "libSceNet", sceNetInetNtop);
    LIB_FUNCTION("Eh+Vqkrrc00", "libSceNet", 1, "libSceNet", sceNetInetNtopWithScopeId);
    LIB_FUNCTION("8Kcp5d-q1Uo", "libSceNet", 1, "libSceNet", sceNetInetPton);
    LIB_FUNCTION("Xn2TA2QhxHc", "libSceNet", 1, "libSceNet", sceNetInetPtonEx);
    LIB_FUNCTION("b+LixqREH6A", "libSceNet", 1, "libSceNet", sceNetInetPtonWithScopeId);
    LIB_FUNCTION("cYW1ISGlOmo", "libSceNet", 1, "libSceNet", sceNetInfoDumpStart);
    LIB_FUNCTION("XfV-XBCuhDo", "libSceNet", 1, "libSceNet", sceNetInfoDumpStop);
    LIB_FUNCTION("Nlev7Lg8k3A", "libSceNet", 1, "libSceNet", sceNetInit);
    LIB_FUNCTION("6MojQ8uFHEI", "libSceNet", 1, "libSceNet", sceNetInitParam);
    LIB_FUNCTION("ghqRRVQxqKo", "libSceNet", 1, "libSceNet", sceNetIoctl);
    LIB_FUNCTION("kOj1HiAGE54", "libSceNet", 1, "libSceNet", sceNetListen);
    LIB_FUNCTION("HKIa-WH0AZ4", "libSceNet", 1, "libSceNet", sceNetMemoryAllocate);
    LIB_FUNCTION("221fvqVs+sQ", "libSceNet", 1, "libSceNet", sceNetMemoryFree);
    LIB_FUNCTION("pQGpHYopAIY", "libSceNet", 1, "libSceNet", sceNetNtohl);
    LIB_FUNCTION("tOrRi-v3AOM", "libSceNet", 1, "libSceNet", sceNetNtohll);
    LIB_FUNCTION("Rbvt+5Y2iEw", "libSceNet", 1, "libSceNet", sceNetNtohs);
    LIB_FUNCTION("dgJBaeJnGpo", "libSceNet", 1, "libSceNet", sceNetPoolCreate);
    LIB_FUNCTION("K7RlrTkI-mw", "libSceNet", 1, "libSceNet", sceNetPoolDestroy);
    LIB_FUNCTION("QGOqGPnk5a4", "libSceNet", 1, "libSceNet", sceNetPppoeStart);
    LIB_FUNCTION("FIV95WE1EuE", "libSceNet", 1, "libSceNet", sceNetPppoeStop);
    LIB_FUNCTION("9wO9XrMsNhc", "libSceNet", 1, "libSceNet", sceNetRecv);
    LIB_FUNCTION("304ooNZxWDY", "libSceNet", 1, "libSceNet", sceNetRecvfrom);
    LIB_FUNCTION("wvuUDv0jrMI", "libSceNet", 1, "libSceNet", sceNetRecvmsg);
    LIB_FUNCTION("AzqoBha7js4", "libSceNet", 1, "libSceNet", sceNetResolverAbort);
    LIB_FUNCTION("JQk8ck8vnPY", "libSceNet", 1, "libSceNet", sceNetResolverConnect);
    LIB_FUNCTION("bonnMiDoOZg", "libSceNet", 1, "libSceNet", sceNetResolverConnectAbort);
    LIB_FUNCTION("V5q6gvEJpw4", "libSceNet", 1, "libSceNet", sceNetResolverConnectCreate);
    LIB_FUNCTION("QFPjG6rqeZg", "libSceNet", 1, "libSceNet", sceNetResolverConnectDestroy);
    LIB_FUNCTION("C4UgDHHPvdw", "libSceNet", 1, "libSceNet", sceNetResolverCreate);
    LIB_FUNCTION("kJlYH5uMAWI", "libSceNet", 1, "libSceNet", sceNetResolverDestroy);
    LIB_FUNCTION("J5i3hiLJMPk", "libSceNet", 1, "libSceNet", sceNetResolverGetError);
    LIB_FUNCTION("Apb4YDxKsRI", "libSceNet", 1, "libSceNet", sceNetResolverStartAton);
    LIB_FUNCTION("zvzWA5IZMsg", "libSceNet", 1, "libSceNet", sceNetResolverStartAton6);
    LIB_FUNCTION("Nd91WaWmG2w", "libSceNet", 1, "libSceNet", sceNetResolverStartNtoa);
    LIB_FUNCTION("zl35YNs9jnI", "libSceNet", 1, "libSceNet", sceNetResolverStartNtoa6);
    LIB_FUNCTION("RCCY01Xd+58", "libSceNet", 1, "libSceNet",
                 sceNetResolverStartNtoaMultipleRecords);
    LIB_FUNCTION("sT4nBQKUPqM", "libSceNet", 1, "libSceNet",
                 sceNetResolverStartNtoaMultipleRecordsEx);
    LIB_FUNCTION("beRjXBn-z+o", "libSceNet", 1, "libSceNet", sceNetSend);
    LIB_FUNCTION("2eKbgcboJso", "libSceNet", 1, "libSceNet", sceNetSendmsg);
    LIB_FUNCTION("gvD1greCu0A", "libSceNet", 1, "libSceNet", sceNetSendto);
    LIB_FUNCTION("15Ywg-ZsSl0", "libSceNet", 1, "libSceNet", sceNetSetDns6Info);
    LIB_FUNCTION("E3oH1qsdqCA", "libSceNet", 1, "libSceNet", sceNetSetDns6InfoToKernel);
    LIB_FUNCTION("B-M6KjO8-+w", "libSceNet", 1, "libSceNet", sceNetSetDnsInfo);
    LIB_FUNCTION("8s+T0bJeyLQ", "libSceNet", 1, "libSceNet", sceNetSetDnsInfoToKernel);
    LIB_FUNCTION("2mKX2Spso7I", "libSceNet", 1, "libSceNet", sceNetSetsockopt);
    LIB_FUNCTION("k1V1djYpk7k", "libSceNet", 1, "libSceNet", sceNetShowIfconfig);
    LIB_FUNCTION("j6pkkO2zJtg", "libSceNet", 1, "libSceNet", sceNetShowIfconfigForBuffer);
    LIB_FUNCTION("E8dTcvQw3hg", "libSceNet", 1, "libSceNet", sceNetShowIfconfigWithMemory);
    LIB_FUNCTION("WxislcDAW5I", "libSceNet", 1, "libSceNet", sceNetShowNetstat);
    LIB_FUNCTION("rX30iWQqqzg", "libSceNet", 1, "libSceNet", sceNetShowNetstatEx);
    LIB_FUNCTION("vjwKTGa21f0", "libSceNet", 1, "libSceNet", sceNetShowNetstatExForBuffer);
    LIB_FUNCTION("mqoB+LN0pW8", "libSceNet", 1, "libSceNet", sceNetShowNetstatForBuffer);
    LIB_FUNCTION("H5WHYRfDkR0", "libSceNet", 1, "libSceNet", sceNetShowNetstatWithMemory);
    LIB_FUNCTION("tk0p0JmiBkM", "libSceNet", 1, "libSceNet", sceNetShowPolicy);
    LIB_FUNCTION("dbrSNEuZfXI", "libSceNet", 1, "libSceNet", sceNetShowPolicyWithMemory);
    LIB_FUNCTION("cEMX1VcPpQ8", "libSceNet", 1, "libSceNet", sceNetShowRoute);
    LIB_FUNCTION("fCa7-ihdRdc", "libSceNet", 1, "libSceNet", sceNetShowRoute6);
    LIB_FUNCTION("nTJqXsbSS1I", "libSceNet", 1, "libSceNet", sceNetShowRoute6ForBuffer);
    LIB_FUNCTION("TCZyE2YI1uM", "libSceNet", 1, "libSceNet", sceNetShowRoute6WithMemory);
    LIB_FUNCTION("n-IAZb7QB1Y", "libSceNet", 1, "libSceNet", sceNetShowRouteForBuffer);
    LIB_FUNCTION("0-XSSp1kEFM", "libSceNet", 1, "libSceNet", sceNetShowRouteWithMemory);
    LIB_FUNCTION("TSM6whtekok", "libSceNet", 1, "libSceNet", sceNetShutdown);
    LIB_FUNCTION("Q4qBuN-c0ZM", "libSceNet", 1, "libSceNet", sceNetSocket);
    LIB_FUNCTION("zJGf8xjFnQE", "libSceNet", 1, "libSceNet", sceNetSocketAbort);
    LIB_FUNCTION("45ggEzakPJQ", "libSceNet", 1, "libSceNet", sceNetSocketClose);
    LIB_FUNCTION("6AJE2jKg-c0", "libSceNet", 1, "libSceNet", sceNetSyncCreate);
    LIB_FUNCTION("atGfzCaXMak", "libSceNet", 1, "libSceNet", sceNetSyncDestroy);
    LIB_FUNCTION("sAleh-BoxLA", "libSceNet", 1, "libSceNet", sceNetSyncGet);
    LIB_FUNCTION("Z-8Jda650Vk", "libSceNet", 1, "libSceNet", sceNetSyncSignal);
    LIB_FUNCTION("NP5gxDeYhIM", "libSceNet", 1, "libSceNet", sceNetSyncWait);
    LIB_FUNCTION("3zRdT3O2Kxo", "libSceNet", 1, "libSceNet", sceNetSysctl);
    LIB_FUNCTION("cTGkc6-TBlI", "libSceNet", 1, "libSceNet", sceNetTerm);
    LIB_FUNCTION("j-Op3ibRJaQ", "libSceNet", 1, "libSceNet", sceNetThreadCreate);
    LIB_FUNCTION("KirVfZbqniw", "libSceNet", 1, "libSceNet", sceNetThreadExit);
    LIB_FUNCTION("pRbEzaV30qI", "libSceNet", 1, "libSceNet", sceNetThreadJoin);
    LIB_FUNCTION("bjrzRLFali0", "libSceNet", 1, "libSceNet", sceNetUsleep);
    LIB_FUNCTION("DnB6WJ91HGg", "libSceNet", 1, "libSceNet", Func_0E707A589F751C68);
    LIB_FUNCTION("JK1oZe4UysY", "libSceNetDebug", 1, "libSceNet", sceNetEmulationGet);
    LIB_FUNCTION("pfn3Fha1ydc", "libSceNetDebug", 1, "libSceNet", sceNetEmulationSet);
};

} // namespace Libraries::Net
