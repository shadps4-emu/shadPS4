// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Net {

using OrbisNetId = s32;

enum OrbisNetFamily : u32 {
    ORBIS_NET_AF_UNIX = 1,
    ORBIS_NET_AF_INET = 2,
    ORBIS_NET_AF_INET6 = 28,
};

enum OrbisNetSocketType : u32 {
    ORBIS_NET_SOCK_STREAM = 1,
    ORBIS_NET_SOCK_DGRAM = 2,
    ORBIS_NET_SOCK_RAW = 3,
    ORBIS_NET_SOCK_DGRAM_P2P = 6,
    ORBIS_NET_SOCK_STREAM_P2P = 10,
};

enum OrbisNetProtocol : u32 {
    ORBIS_NET_IPPROTO_IP = 0,
    ORBIS_NET_IPPROTO_ICMP = 1,
    ORBIS_NET_IPPROTO_IGMP = 2,
    ORBIS_NET_IPPROTO_TCP = 6,
    ORBIS_NET_IPPROTO_UDP = 17,
    ORBIS_NET_IPPROTO_IPV6 = 41,
    ORBIS_NET_SOL_SOCKET = 0xFFFF,
};

enum OrbisNetSocketIpOption : u32 {
    ORBIS_NET_IP_HDRINCL = 2,
    ORBIS_NET_IP_TOS = 3,
    ORBIS_NET_IP_TTL = 4,
    ORBIS_NET_IP_MULTICAST_IF = 9,
    ORBIS_NET_IP_MULTICAST_TTL = 10,
    ORBIS_NET_IP_MULTICAST_LOOP = 11,
    ORBIS_NET_IP_ADD_MEMBERSHIP = 12,
    ORBIS_NET_IP_DROP_MEMBERSHIP = 13,
    ORBIS_NET_IP_TTLCHK = 30,
    ORBIS_NET_IP_MAXTTL = 31,
    ORBIS_NET_IP_DONTFRAG = 67,
};

enum OrbisNetSocketTcpOption : u32 {
    ORBIS_NET_TCP_NODELAY = 1,
    ORBIS_NET_TCP_MAXSEG = 2,
    ORBIS_NET_TCP_MSS_TO_ADVERTISE = 3,
    ORBIS_NET_TCP_KEEPLISTEN = 128,
};

enum OrbisNetSocketUdpOption : u32 {
    ORBIS_NET_UDP_SND_ON_SUSPEND = 128,
};

// SO_POLICY / SO_PRIORITY ranges
inline constexpr s32 ORBIS_NET_SOCK_POLICY_NUM_MIN = 0;
inline constexpr s32 ORBIS_NET_SOCK_POLICY_NUM_MAX = 15;
inline constexpr s32 ORBIS_NET_SOCK_POLICY_NA = -1; // not applicable
inline constexpr s32 ORBIS_NET_SOCK_PRIORITY_NUM_USER_MIN = 8;
inline constexpr s32 ORBIS_NET_SOCK_PRIORITY_NUM_USER_MAX = 23;

// sceNetSocketAbort flags
inline constexpr u32 ORBIS_NET_SOCKET_ABORT_FLAG_RCV_PRESERVATION = 0x1;
inline constexpr u32 ORBIS_NET_SOCKET_ABORT_FLAG_SND_PRESERVATION = 0x2;
inline constexpr u32 ORBIS_NET_SOCKET_ABORT_FLAG_SND_PRESERVATION_AGAIN = 0x4;

inline constexpr u16 ORBIS_NET_ADHOC_PORT = 3658;

enum OrbisNetSocketSoOption : u32 {
    ORBIS_NET_SO_REUSEADDR = 0x00000004,
    ORBIS_NET_SO_KEEPALIVE = 0x00000008,
    ORBIS_NET_SO_BROADCAST = 0x00000020,
    ORBIS_NET_SO_LINGER = 0x00000080,
    ORBIS_NET_SO_REUSEPORT = 0x00000200,
    ORBIS_NET_SO_ONESBCAST = 0x00010000,
    ORBIS_NET_SO_USECRYPTO = 0x00020000,
    ORBIS_NET_SO_USESIGNATURE = 0x00040000,
    ORBIS_NET_SO_SNDBUF = 0x1001,
    ORBIS_NET_SO_RCVBUF = 0x1002,
    ORBIS_NET_SO_ERROR = 0x1007,
    ORBIS_NET_SO_TYPE = 0x1008,
    ORBIS_NET_SO_SNDTIMEO = 0x1105,
    ORBIS_NET_SO_RCVTIMEO = 0x1106,
    ORBIS_NET_SO_ERROR_EX = 0x1107,
    ORBIS_NET_SO_ACCEPTTIMEO = 0x1108,
    ORBIS_NET_SO_CONNECTTIMEO = 0x1109,
    ORBIS_NET_SO_NBIO = 0x1200,
    ORBIS_NET_SO_POLICY = 0x1201,
    ORBIS_NET_SO_NAME = 0x1202,
    ORBIS_NET_SO_PRIORITY = 0x1203,
};

enum OrbisNetFlags : u32 {
    ORBIS_NET_MSG_PEEK = 0x00000002,
    ORBIS_NET_MSG_WAITALL = 0x00000040,
    ORBIS_NET_MSG_DONTWAIT = 0x00000080,
    ORBIS_NET_MSG_USECRYPTO = 0x00100000,
    ORBIS_NET_MSG_USESIGNATURE = 0x00200000,
    ORBIS_NET_MSG_PEEKLEN = (0x00400000 | ORBIS_NET_MSG_PEEK),
};

enum OrbisNetShutdown : s32 {
    ORBIS_NET_SHUT_RD = 0,
    ORBIS_NET_SHUT_WR = 1,
    ORBIS_NET_SHUT_RDWR = 2,
};

enum OrbisNetEpollFlag : u32 {
    ORBIS_NET_EPOLL_CTL_ADD = 1,
    ORBIS_NET_EPOLL_CTL_MOD = 2,
    ORBIS_NET_EPOLL_CTL_DEL = 3,
};

enum OrbisNetEpollEvents : u32 {
    ORBIS_NET_EPOLLIN = 0x1,
    ORBIS_NET_EPOLLOUT = 0x2,
    ORBIS_NET_EPOLLERR = 0x8,
    ORBIS_NET_EPOLLHUP = 0x10,
    ORBIS_NET_EPOLLDESCID = 0x10000,
};

struct OrbisNetSockaddr {
    u8 sa_len;
    u8 sa_family;
    char sa_data[14];
};

struct OrbisNetSockaddrIn {
    u8 sin_len;
    u8 sin_family;
    u16 sin_port;  // network byte order
    u32 sin_addr;  // network byte order
    u16 sin_vport; // P2P virtual port, network byte order
    char sin_zero[6];
};

struct OrbisNetSockaddrIn6 {
    u8 sin6_len;
    u8 sin6_family;
    u16 sin6_port; // network byte order
    u32 sin6_flowinfo;
    u8 sin6_addr[16];
    u32 sin6_scope_id;
};

struct OrbisNetLinger {
    s32 l_onoff;
    s32 l_linger;
};

union OrbisNetEpollData {
    void* ptr;
    u32 data_u32;
    s32 fd;
    u64 data_u64;
};

struct OrbisNetEpollEvent {
    u32 events;
    u32 pad;
    u64 ident;
    OrbisNetEpollData data;
};

struct OrbisNetInAddr {
    u32 inaddr_addr; // network byte order
};
// Host byte order
inline constexpr u32 ORBIS_NET_INADDR_ANY = 0x00000000;
inline constexpr u32 ORBIS_NET_INADDR_LOOPBACK = 0x7f000001;
inline constexpr u32 ORBIS_NET_INADDR_BROADCAST = 0xffffffff;
inline constexpr u32 ORBIS_NET_INADDR_UNSPEC_GROUP = 0xe0000000;
inline constexpr u32 ORBIS_NET_INADDR_AUTOIP = 0xa9fe0000;
inline constexpr u32 ORBIS_NET_IN_CLASSD_NET = 0xf0000000;
inline constexpr u32 ORBIS_NET_IN_AUTOIP_NET = 0xffff0000;
inline constexpr int ORBIS_NET_INET_ADDRSTRLEN = 16;
inline constexpr int ORBIS_NET_ETHER_ADDRSTRLEN = 18;

// Option value for ORBIS_NET_UDP_SND_ON_SUSPEND.
struct OrbisNetUdpSndOnSuspend {
    s32 onoff;
    struct OrbisNetSockaddr* addr;
    u32 addrlen;
    void* data;
    u32 datalen;
};
inline constexpr u32 ORBIS_NET_UDP_SND_ON_SUSPEND_DATALEN_MAX = 512;

struct OrbisNetMemoryPoolStats {
    u64 pool_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
    s32 reserved;
};

struct OrbisNetStatisticsInfo {
    s32 kernel_mem_free_size;
    s32 kernel_mem_free_min;
    s32 packet_count;
    s32 packet_qos_count;
    s32 libnet_mem_free_size;
    s32 libnet_mem_free_min;
};

struct OrbisNetSockaddrUn {
    u8 sun_len;
    u8 sun_family;
    char sun_path[104];
};

struct OrbisNetIovec {
    void* iov_base;
    u64 iov_len;
};

struct OrbisNetMsghdr {
    void* msg_name;
    u32 msg_namelen;
    OrbisNetIovec* msg_iov;
    s32 msg_iovlen;
    void* msg_control;
    u32 msg_controllen;
    s32 msg_flags;
};

enum OrbisNetResolverFlag : u32 {
    ORBIS_NET_RESOLVER_ASYNC = 0x1,
    ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS = 0x10000,
};
inline constexpr u32 ORBIS_NET_RESOLVER_ABORT_FLAG_NTOA_PRESERVATION = 0x1;
inline constexpr u32 ORBIS_NET_RESOLVER_ABORT_FLAG_ATON_PRESERVATION = 0x2;
inline constexpr int ORBIS_NET_RESOLVER_HOSTNAME_LEN_MAX = 255;

union OrbisNetAddrUnion {
    OrbisNetInAddr addr;
    u8 addr6[16];
};

struct OrbisNetResolverAddr {
    OrbisNetAddrUnion u;
    u32 af;
    u32 pad[3];
};

struct OrbisNetResolverInfo {
    OrbisNetResolverAddr addrs[10];
    u32 records;
    u32 recordsv4;
    u32 pad[14];
};

inline constexpr int ORBIS_NET_DEBUG_NAME_LEN_MAX = 31;

enum OrbisNetSockInfoState : s32 {
    ORBIS_NET_SOCKINFO_STATE_UNKNOWN = 0,
    ORBIS_NET_SOCKINFO_STATE_CLOSED = 1,
    ORBIS_NET_SOCKINFO_STATE_OPENED = 2,
    ORBIS_NET_SOCKINFO_STATE_LISTEN = 3,
    ORBIS_NET_SOCKINFO_STATE_SYN_SENT = 4,
    ORBIS_NET_SOCKINFO_STATE_SYN_RECEIVED = 5,
    ORBIS_NET_SOCKINFO_STATE_ESTABLISHED = 6,
    ORBIS_NET_SOCKINFO_STATE_FIN_WAIT_1 = 7,
    ORBIS_NET_SOCKINFO_STATE_FIN_WAIT_2 = 8,
    ORBIS_NET_SOCKINFO_STATE_CLOSE_WAIT = 9,
    ORBIS_NET_SOCKINFO_STATE_CLOSING = 10,
    ORBIS_NET_SOCKINFO_STATE_LAST_ACK = 11,
    ORBIS_NET_SOCKINFO_STATE_TIME_WAIT = 12,
};

enum OrbisNetSockInfoFlags : s32 {
    ORBIS_NET_SOCKINFO_F_SELF = 0x00000001,
    ORBIS_NET_SOCKINFO_F_KERNEL = 0x00000002,
    ORBIS_NET_SOCKINFO_F_OTHERS = 0x00000004,
    ORBIS_NET_SOCKINFO_F_RECV_WAIT = 0x00010000,
    ORBIS_NET_SOCKINFO_F_SEND_WAIT = 0x00020000,
    ORBIS_NET_SOCKINFO_F_RECV_EWAIT = 0x00040000,
    ORBIS_NET_SOCKINFO_F_SEND_EWAIT = 0x00080000,
    ORBIS_NET_SOCKINFO_F_NONBLOCK = 0x00200000,
    ORBIS_NET_SOCKINFO_F_ALL = 0x006F0007,
};

struct OrbisNetSockInfo {
    char name[ORBIS_NET_DEBUG_NAME_LEN_MAX + 1];
    s32 pid;
    OrbisNetId s;
    s8 socket_type;
    s8 policy;
    s8 priority;
    s8 reserved8;
    s32 recv_queue_length;
    s32 send_queue_length;
    OrbisNetInAddr local_adr;
    OrbisNetInAddr remote_adr;
    u16 local_port; // network byte order
    u16 remote_port;
    u16 local_vport;
    u16 remote_vport;
    s32 state;
    s32 flags;
    s32 tx_bps;
    s32 rx_bps;
    s32 max_tx_bps;
    s32 max_rx_bps;
    s32 tx_vbps;
    s32 rx_vbps;
    s32 recv_buffer_size;
    s32 send_buffer_size;
    s32 reserved6[8];
    s32 tx_drops;
    s32 rx_drops;
    s32 tx_wait;
    s32 reserved[2];
};

struct OrbisNetEtherAddr {
    u8 data[6];
};

static_assert(sizeof(OrbisNetSockaddr) == 16);
static_assert(sizeof(OrbisNetSockaddrIn) == 16);
static_assert(sizeof(OrbisNetSockaddrIn6) == 28);
static_assert(sizeof(OrbisNetEpollEvent) == 24);
static_assert(sizeof(OrbisNetSockaddrUn) == 106);
static_assert(sizeof(OrbisNetMsghdr) == 48);
static_assert(sizeof(OrbisNetResolverInfo) == 384);
static_assert(sizeof(OrbisNetSockInfo) == 160);
static_assert(sizeof(OrbisNetUdpSndOnSuspend) == 40);
static_assert(sizeof(OrbisNetMemoryPoolStats) == 32);
static_assert(sizeof(OrbisNetStatisticsInfo) == 24);

} // namespace Libraries::Net
