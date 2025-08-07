// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "netctl.h"

namespace Core::Loader {
class SymbolsResolver;
}

// Define our own htonll and ntohll because its not available in some systems/platforms
#ifndef HTONLL
#define HTONLL(x) (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32))
#endif
#ifndef NTOHLL
#define NTOHLL(x) (((uint64_t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32_t)((x) >> 32))
#endif

namespace Libraries::Net {

static int ConvertFamilies(int family);

enum OrbisNetFamily : u32 {
    ORBIS_NET_AF_INET = 2,
    ORBIS_NET_AF_INET6 = 28,
};
enum OrbisNetSocketType : u32 {
    ORBIS_NET_SOCK_STREAM = 1,
    ORBIS_NET_SOCK_DGRAM = 2,
    ORBIS_NET_SOCK_RAW = 3,
    ORBIS_NET_SOCK_DGRAM_P2P = 6,
    ORBIS_NET_SOCK_STREAM_P2P = 10
};

enum OrbisNetProtocol : u32 {
    ORBIS_NET_IPPROTO_IP = 0,
    ORBIS_NET_IPPROTO_ICMP = 1,
    ORBIS_NET_IPPROTO_IGMP = 2,
    ORBIS_NET_IPPROTO_TCP = 6,
    ORBIS_NET_IPPROTO_UDP = 17,
    ORBIS_NET_IPPROTO_IPV6 = 41,
    ORBIS_NET_SOL_SOCKET = 0xFFFF
};

enum OrbisNetSocketOption : u32 {
    /* IP */
    ORBIS_NET_IP_HDRINCL = 2,
    ORBIS_NET_IP_TOS = 3,
    ORBIS_NET_IP_TTL = 4,
    ORBIS_NET_IP_MULTICAST_IF = 9,
    ORBIS_NET_IP_MULTICAST_TTL = 10,
    ORBIS_NET_IP_MULTICAST_LOOP = 11,
    ORBIS_NET_IP_ADD_MEMBERSHIP = 12,
    ORBIS_NET_IP_DROP_MEMBERSHIP = 13,
    ORBIS_NET_IP_TTLCHK = 23,
    ORBIS_NET_IP_MAXTTL = 24,
    /* TCP */
    ORBIS_NET_TCP_NODELAY = 1,
    ORBIS_NET_TCP_MAXSEG = 2,
    ORBIS_NET_TCP_MSS_TO_ADVERTISE = 3,
    /* SOCKET */
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
    ORBIS_NET_SO_PRIORITY = 0x1203
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

enum OrbisNetResolverFlag : u32 {
    ORBIS_NET_RESOLVER_ASYNC = 0x1,
    ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS = 0x10000,
};

using OrbisNetId = s32;

struct OrbisNetSockaddr {
    u8 sa_len;
    u8 sa_family;
    char sa_data[14];
};

struct OrbisNetSockaddrIn {
    u8 sin_len;
    u8 sin_family;
    u16 sin_port;
    u32 sin_addr;
    u16 sin_vport;
    char sin_zero[6];
};

using OrbisNetInAddr_t = u32;

struct OrbisNetInAddr {
    OrbisNetInAddr_t inaddr_addr;
};

struct OrbisNetIovec {
    void* iov_base;
    u64 iov_len;
};

struct OrbisNetMsghdr {
    void* msg_name;
    u32 msg_namelen;
    OrbisNetIovec* msg_iov;
    int msg_iovlen;
    void* msg_control;
    u32 msg_controllen;
    int msg_flags;
};

union OrbisNetEpollData {
    void* ptr;
    u32 data_u32;
    int fd;
    u64 data_u64;
};

struct OrbisNetEpollEvent {
    u32 events;
    u32 pad;
    u64 ident;
    OrbisNetEpollData data;
};

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

int PS4_SYSV_ABI in6addr_any();
int PS4_SYSV_ABI in6addr_loopback();
int PS4_SYSV_ABI sce_net_dummy();
int PS4_SYSV_ABI sce_net_in6addr_any();
int PS4_SYSV_ABI sce_net_in6addr_linklocal_allnodes();
int PS4_SYSV_ABI sce_net_in6addr_linklocal_allrouters();
int PS4_SYSV_ABI sce_net_in6addr_loopback();
int PS4_SYSV_ABI sce_net_in6addr_nodelocal_allnodes();
OrbisNetId PS4_SYSV_ABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sceNetAddrConfig6GetInfo();
int PS4_SYSV_ABI sceNetAddrConfig6Start();
int PS4_SYSV_ABI sceNetAddrConfig6Stop();
int PS4_SYSV_ABI sceNetAllocateAllRouteInfo();
int PS4_SYSV_ABI sceNetBandwidthControlGetDataTraffic();
int PS4_SYSV_ABI sceNetBandwidthControlGetDefaultParam();
int PS4_SYSV_ABI sceNetBandwidthControlGetIfParam();
int PS4_SYSV_ABI sceNetBandwidthControlGetPolicy();
int PS4_SYSV_ABI sceNetBandwidthControlSetDefaultParam();
int PS4_SYSV_ABI sceNetBandwidthControlSetIfParam();
int PS4_SYSV_ABI sceNetBandwidthControlSetPolicy();
int PS4_SYSV_ABI sceNetBind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sceNetClearDnsCache();
int PS4_SYSV_ABI sceNetConfigAddArp();
int PS4_SYSV_ABI sceNetConfigAddArpWithInterface();
int PS4_SYSV_ABI sceNetConfigAddIfaddr();
int PS4_SYSV_ABI sceNetConfigAddMRoute();
int PS4_SYSV_ABI sceNetConfigAddRoute();
int PS4_SYSV_ABI sceNetConfigAddRoute6();
int PS4_SYSV_ABI sceNetConfigAddRouteWithInterface();
int PS4_SYSV_ABI sceNetConfigCleanUpAllInterfaces();
int PS4_SYSV_ABI sceNetConfigDelArp();
int PS4_SYSV_ABI sceNetConfigDelArpWithInterface();
int PS4_SYSV_ABI sceNetConfigDelDefaultRoute();
int PS4_SYSV_ABI sceNetConfigDelDefaultRoute6();
int PS4_SYSV_ABI sceNetConfigDelIfaddr();
int PS4_SYSV_ABI sceNetConfigDelIfaddr6();
int PS4_SYSV_ABI sceNetConfigDelMRoute();
int PS4_SYSV_ABI sceNetConfigDelRoute();
int PS4_SYSV_ABI sceNetConfigDelRoute6();
int PS4_SYSV_ABI sceNetConfigDownInterface();
int PS4_SYSV_ABI sceNetConfigEtherGetLinkMode();
int PS4_SYSV_ABI sceNetConfigEtherPostPlugInOutEvent();
int PS4_SYSV_ABI sceNetConfigEtherSetLinkMode();
int PS4_SYSV_ABI sceNetConfigFlushRoute();
int PS4_SYSV_ABI sceNetConfigGetDefaultRoute();
int PS4_SYSV_ABI sceNetConfigGetDefaultRoute6();
int PS4_SYSV_ABI sceNetConfigGetIfaddr();
int PS4_SYSV_ABI sceNetConfigGetIfaddr6();
int PS4_SYSV_ABI sceNetConfigRoutingShowRoutingConfig();
int PS4_SYSV_ABI sceNetConfigRoutingShowtCtlVar();
int PS4_SYSV_ABI sceNetConfigRoutingStart();
int PS4_SYSV_ABI sceNetConfigRoutingStop();
int PS4_SYSV_ABI sceNetConfigSetDefaultRoute();
int PS4_SYSV_ABI sceNetConfigSetDefaultRoute6();
int PS4_SYSV_ABI sceNetConfigSetDefaultScope();
int PS4_SYSV_ABI sceNetConfigSetIfaddr();
int PS4_SYSV_ABI sceNetConfigSetIfaddr6();
int PS4_SYSV_ABI sceNetConfigSetIfaddr6WithFlags();
int PS4_SYSV_ABI sceNetConfigSetIfFlags();
int PS4_SYSV_ABI sceNetConfigSetIfLinkLocalAddr6();
int PS4_SYSV_ABI sceNetConfigSetIfmtu();
int PS4_SYSV_ABI sceNetConfigUnsetIfFlags();
int PS4_SYSV_ABI sceNetConfigUpInterface();
int PS4_SYSV_ABI sceNetConfigUpInterfaceWithFlags();
int PS4_SYSV_ABI sceNetConfigWlanAdhocClearWakeOnWlan();
int PS4_SYSV_ABI sceNetConfigWlanAdhocCreate();
int PS4_SYSV_ABI sceNetConfigWlanAdhocGetWakeOnWlanInfo();
int PS4_SYSV_ABI sceNetConfigWlanAdhocJoin();
int PS4_SYSV_ABI sceNetConfigWlanAdhocLeave();
int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuClearWakeOnWlan();
int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo();
int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuSetWakeOnWlan();
int PS4_SYSV_ABI sceNetConfigWlanAdhocScanJoin();
int PS4_SYSV_ABI sceNetConfigWlanAdhocSetExtInfoElement();
int PS4_SYSV_ABI sceNetConfigWlanAdhocSetWakeOnWlan();
int PS4_SYSV_ABI sceNetConfigWlanApStart();
int PS4_SYSV_ABI sceNetConfigWlanApStop();
int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanQuery();
int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStart();
int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStop();
int PS4_SYSV_ABI sceNetConfigWlanDiagGetDeviceInfo();
int PS4_SYSV_ABI sceNetConfigWlanDiagSetAntenna();
int PS4_SYSV_ABI sceNetConfigWlanDiagSetTxFixedRate();
int PS4_SYSV_ABI sceNetConfigWlanGetDeviceConfig();
int PS4_SYSV_ABI sceNetConfigWlanInfraGetRssiInfo();
int PS4_SYSV_ABI sceNetConfigWlanInfraLeave();
int PS4_SYSV_ABI sceNetConfigWlanInfraScanJoin();
int PS4_SYSV_ABI sceNetConfigWlanScan();
int PS4_SYSV_ABI sceNetConfigWlanSetDeviceConfig();
int PS4_SYSV_ABI sceNetConnect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sceNetControl();
int PS4_SYSV_ABI sceNetDhcpdStart();
int PS4_SYSV_ABI sceNetDhcpdStop();
int PS4_SYSV_ABI sceNetDhcpGetAutoipInfo();
int PS4_SYSV_ABI sceNetDhcpGetInfo();
int PS4_SYSV_ABI sceNetDhcpGetInfoEx();
int PS4_SYSV_ABI sceNetDhcpStart();
int PS4_SYSV_ABI sceNetDhcpStop();
int PS4_SYSV_ABI sceNetDumpAbort();
int PS4_SYSV_ABI sceNetDumpCreate();
int PS4_SYSV_ABI sceNetDumpDestroy();
int PS4_SYSV_ABI sceNetDumpRead();
int PS4_SYSV_ABI sceNetDuplicateIpStart();
int PS4_SYSV_ABI sceNetDuplicateIpStop();
int PS4_SYSV_ABI sceNetEpollAbort();
int PS4_SYSV_ABI sceNetEpollControl();
int PS4_SYSV_ABI sceNetEpollCreate();
int PS4_SYSV_ABI sceNetEpollDestroy();
int PS4_SYSV_ABI sceNetEpollWait();
int* PS4_SYSV_ABI sceNetErrnoLoc();
int PS4_SYSV_ABI sceNetEtherNtostr();
int PS4_SYSV_ABI sceNetEtherStrton();
int PS4_SYSV_ABI sceNetEventCallbackCreate();
int PS4_SYSV_ABI sceNetEventCallbackDestroy();
int PS4_SYSV_ABI sceNetEventCallbackGetError();
int PS4_SYSV_ABI sceNetEventCallbackWaitCB();
int PS4_SYSV_ABI sceNetFreeAllRouteInfo();
int PS4_SYSV_ABI sceNetGetArpInfo();
int PS4_SYSV_ABI sceNetGetDns6Info();
int PS4_SYSV_ABI sceNetGetDnsInfo();
int PS4_SYSV_ABI sceNetGetIfList();
int PS4_SYSV_ABI sceNetGetIfListOnce();
int PS4_SYSV_ABI sceNetGetIfName();
int PS4_SYSV_ABI sceNetGetIfnameNumList();
int PS4_SYSV_ABI sceNetGetMacAddress(Libraries::NetCtl::OrbisNetEtherAddr* addr, int flags);
int PS4_SYSV_ABI sceNetGetMemoryPoolStats();
int PS4_SYSV_ABI sceNetGetNameToIndex();
int PS4_SYSV_ABI sceNetGetpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sceNetGetRandom();
int PS4_SYSV_ABI sceNetGetRouteInfo();
int PS4_SYSV_ABI sceNetGetSockInfo();
int PS4_SYSV_ABI sceNetGetSockInfo6();
int PS4_SYSV_ABI sceNetGetsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sceNetGetsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen);
int PS4_SYSV_ABI sceNetGetStatisticsInfo();
int PS4_SYSV_ABI sceNetGetStatisticsInfoInternal();
int PS4_SYSV_ABI sceNetGetSystemTime();
u32 PS4_SYSV_ABI sceNetHtonl(u32 host32);
u64 PS4_SYSV_ABI sceNetHtonll(u64 host64);
u16 PS4_SYSV_ABI sceNetHtons(u16 host16);
const char* PS4_SYSV_ABI sceNetInetNtop(int af, const void* src, char* dst, u32 size);
int PS4_SYSV_ABI sceNetInetNtopWithScopeId();
int PS4_SYSV_ABI sceNetInetPton(int af, const char* src, void* dst);
int PS4_SYSV_ABI sceNetInetPtonEx(int af, const char* src, void* dst, int flags);
int PS4_SYSV_ABI sceNetInetPtonWithScopeId();
int PS4_SYSV_ABI sceNetInfoDumpStart();
int PS4_SYSV_ABI sceNetInfoDumpStop();
int PS4_SYSV_ABI sceNetInit();
int PS4_SYSV_ABI sceNetInitParam();
int PS4_SYSV_ABI sceNetIoctl();
int PS4_SYSV_ABI sceNetListen(OrbisNetId s, int backlog);
int PS4_SYSV_ABI sceNetMemoryAllocate();
int PS4_SYSV_ABI sceNetMemoryFree();
u32 PS4_SYSV_ABI sceNetNtohl(u32 net32);
int PS4_SYSV_ABI sceNetNtohll();
u16 PS4_SYSV_ABI sceNetNtohs(u16 net16);
int PS4_SYSV_ABI sceNetPoolCreate(const char* name, int size, int flags);
int PS4_SYSV_ABI sceNetPoolDestroy();
int PS4_SYSV_ABI sceNetPppoeStart();
int PS4_SYSV_ABI sceNetPppoeStop();
int PS4_SYSV_ABI sceNetRecv(OrbisNetId s, void* buf, u64 len, int flags);
int PS4_SYSV_ABI sceNetRecvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                                u32* paddrlen);
int PS4_SYSV_ABI sceNetRecvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags);
int PS4_SYSV_ABI sceNetResolverAbort();
int PS4_SYSV_ABI sceNetResolverConnect();
int PS4_SYSV_ABI sceNetResolverConnectAbort();
int PS4_SYSV_ABI sceNetResolverConnectCreate();
int PS4_SYSV_ABI sceNetResolverConnectDestroy();
int PS4_SYSV_ABI sceNetResolverCreate(const char* name, int poolid, int flags);
int PS4_SYSV_ABI sceNetResolverDestroy(OrbisNetId resolverid);
int PS4_SYSV_ABI sceNetResolverGetError();
int PS4_SYSV_ABI sceNetResolverStartAton();
int PS4_SYSV_ABI sceNetResolverStartAton6();
int PS4_SYSV_ABI sceNetResolverStartNtoa(OrbisNetId resolverid, const char* hostname,
                                         OrbisNetInAddr* addr, int timeout, int retry, int flags);
int PS4_SYSV_ABI sceNetResolverStartNtoa6();
int PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecords(OrbisNetId resolverid, const char* hostname,
                                                        OrbisNetResolverInfo* info, int timeout,
                                                        int retry, int flags);
int PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecordsEx();
int PS4_SYSV_ABI sceNetSend(OrbisNetId s, const void* buf, u64 len, int flags);
int PS4_SYSV_ABI sceNetSendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags);
int PS4_SYSV_ABI sceNetSendto(OrbisNetId s, const void* buf, u64 len, int flags,
                              const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sceNetSetDns6Info();
int PS4_SYSV_ABI sceNetSetDns6InfoToKernel();
int PS4_SYSV_ABI sceNetSetDnsInfo();
int PS4_SYSV_ABI sceNetSetDnsInfoToKernel();
int PS4_SYSV_ABI sceNetSetsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                  u32 optlen);
int PS4_SYSV_ABI sceNetShowIfconfig();
int PS4_SYSV_ABI sceNetShowIfconfigForBuffer();
int PS4_SYSV_ABI sceNetShowIfconfigWithMemory();
int PS4_SYSV_ABI sceNetShowNetstat();
int PS4_SYSV_ABI sceNetShowNetstatEx();
int PS4_SYSV_ABI sceNetShowNetstatExForBuffer();
int PS4_SYSV_ABI sceNetShowNetstatForBuffer();
int PS4_SYSV_ABI sceNetShowNetstatWithMemory();
int PS4_SYSV_ABI sceNetShowPolicy();
int PS4_SYSV_ABI sceNetShowPolicyWithMemory();
int PS4_SYSV_ABI sceNetShowRoute();
int PS4_SYSV_ABI sceNetShowRoute6();
int PS4_SYSV_ABI sceNetShowRoute6ForBuffer();
int PS4_SYSV_ABI sceNetShowRoute6WithMemory();
int PS4_SYSV_ABI sceNetShowRouteForBuffer();
int PS4_SYSV_ABI sceNetShowRouteWithMemory();
int PS4_SYSV_ABI sceNetShutdown(OrbisNetId s, int how);
OrbisNetId PS4_SYSV_ABI sceNetSocket(const char* name, int family, int type, int protocol);
int PS4_SYSV_ABI sceNetSocketAbort(OrbisNetId s, int flags);
int PS4_SYSV_ABI sceNetSocketClose(OrbisNetId s);
int PS4_SYSV_ABI sceNetSyncCreate();
int PS4_SYSV_ABI sceNetSyncDestroy();
int PS4_SYSV_ABI sceNetSyncGet();
int PS4_SYSV_ABI sceNetSyncSignal();
int PS4_SYSV_ABI sceNetSyncWait();
int PS4_SYSV_ABI sceNetSysctl();
int PS4_SYSV_ABI sceNetTerm();
int PS4_SYSV_ABI sceNetThreadCreate();
int PS4_SYSV_ABI sceNetThreadExit();
int PS4_SYSV_ABI sceNetThreadJoin();
int PS4_SYSV_ABI sceNetUsleep();
int PS4_SYSV_ABI Func_0E707A589F751C68();
int PS4_SYSV_ABI sceNetEmulationGet();
int PS4_SYSV_ABI sceNetEmulationSet();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Net
