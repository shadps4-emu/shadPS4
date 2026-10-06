// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>
#include <memory>

#include "common/types.h"
#include "core/libraries/net/net_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Net {

OrbisNetId PS4_SYSV_ABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
s32 PS4_SYSV_ABI sceNetAddrConfig6GetInfo();
s32 PS4_SYSV_ABI sceNetAddrConfig6Start();
s32 PS4_SYSV_ABI sceNetAddrConfig6Stop();
s32 PS4_SYSV_ABI sceNetAllocateAllRouteInfo();
s32 PS4_SYSV_ABI sceNetBandwidthControlGetDataTraffic();
s32 PS4_SYSV_ABI sceNetBandwidthControlGetDefaultParam();
s32 PS4_SYSV_ABI sceNetBandwidthControlGetIfParam();
s32 PS4_SYSV_ABI sceNetBandwidthControlGetPolicy();
s32 PS4_SYSV_ABI sceNetBandwidthControlSetDefaultParam();
s32 PS4_SYSV_ABI sceNetBandwidthControlSetIfParam();
s32 PS4_SYSV_ABI sceNetBandwidthControlSetPolicy();
s32 PS4_SYSV_ABI sceNetBind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
s32 PS4_SYSV_ABI sceNetClearDnsCache();
s32 PS4_SYSV_ABI sceNetConfigAddArp();
s32 PS4_SYSV_ABI sceNetConfigAddArpWithInterface();
s32 PS4_SYSV_ABI sceNetConfigAddIfaddr();
s32 PS4_SYSV_ABI sceNetConfigAddMRoute();
s32 PS4_SYSV_ABI sceNetConfigAddRoute();
s32 PS4_SYSV_ABI sceNetConfigAddRoute6();
s32 PS4_SYSV_ABI sceNetConfigAddRouteWithInterface();
s32 PS4_SYSV_ABI sceNetConfigCleanUpAllInterfaces();
s32 PS4_SYSV_ABI sceNetConfigDelArp();
s32 PS4_SYSV_ABI sceNetConfigDelArpWithInterface();
s32 PS4_SYSV_ABI sceNetConfigDelDefaultRoute();
s32 PS4_SYSV_ABI sceNetConfigDelDefaultRoute6();
s32 PS4_SYSV_ABI sceNetConfigDelIfaddr();
s32 PS4_SYSV_ABI sceNetConfigDelIfaddr6();
s32 PS4_SYSV_ABI sceNetConfigDelMRoute();
s32 PS4_SYSV_ABI sceNetConfigDelRoute();
s32 PS4_SYSV_ABI sceNetConfigDelRoute6();
s32 PS4_SYSV_ABI sceNetConfigDownInterface();
s32 PS4_SYSV_ABI sceNetConfigEtherGetLinkMode();
s32 PS4_SYSV_ABI sceNetConfigEtherPostPlugInOutEvent();
s32 PS4_SYSV_ABI sceNetConfigEtherSetLinkMode();
s32 PS4_SYSV_ABI sceNetConfigFlushRoute();
s32 PS4_SYSV_ABI sceNetConfigGetDefaultRoute();
s32 PS4_SYSV_ABI sceNetConfigGetDefaultRoute6();
s32 PS4_SYSV_ABI sceNetConfigGetIfaddr();
s32 PS4_SYSV_ABI sceNetConfigGetIfaddr6();
s32 PS4_SYSV_ABI sceNetConfigRoutingShowRoutingConfig();
s32 PS4_SYSV_ABI sceNetConfigRoutingShowtCtlVar();
s32 PS4_SYSV_ABI sceNetConfigRoutingStart();
s32 PS4_SYSV_ABI sceNetConfigRoutingStop();
s32 PS4_SYSV_ABI sceNetConfigSetDefaultRoute();
s32 PS4_SYSV_ABI sceNetConfigSetDefaultRoute6();
s32 PS4_SYSV_ABI sceNetConfigSetDefaultScope();
s32 PS4_SYSV_ABI sceNetConfigSetIfaddr();
s32 PS4_SYSV_ABI sceNetConfigSetIfaddr6();
s32 PS4_SYSV_ABI sceNetConfigSetIfaddr6WithFlags();
s32 PS4_SYSV_ABI sceNetConfigSetIfFlags();
s32 PS4_SYSV_ABI sceNetConfigSetIfLinkLocalAddr6();
s32 PS4_SYSV_ABI sceNetConfigSetIfmtu();
s32 PS4_SYSV_ABI sceNetConfigUnsetIfFlags();
s32 PS4_SYSV_ABI sceNetConfigUpInterface();
s32 PS4_SYSV_ABI sceNetConfigUpInterfaceWithFlags();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocClearWakeOnWlan();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocCreate();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocGetWakeOnWlanInfo();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocJoin();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocLeave();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuClearWakeOnWlan();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuSetWakeOnWlan();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocScanJoin();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocSetExtInfoElement();
s32 PS4_SYSV_ABI sceNetConfigWlanAdhocSetWakeOnWlan();
s32 PS4_SYSV_ABI sceNetConfigWlanApStart();
s32 PS4_SYSV_ABI sceNetConfigWlanApStop();
s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanQuery();
s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStart();
s32 PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStop();
s32 PS4_SYSV_ABI sceNetConfigWlanDiagGetDeviceInfo();
s32 PS4_SYSV_ABI sceNetConfigWlanDiagSetAntenna();
s32 PS4_SYSV_ABI sceNetConfigWlanDiagSetTxFixedRate();
s32 PS4_SYSV_ABI sceNetConfigWlanGetDeviceConfig();
s32 PS4_SYSV_ABI sceNetConfigWlanInfraGetRssiInfo();
s32 PS4_SYSV_ABI sceNetConfigWlanInfraLeave();
s32 PS4_SYSV_ABI sceNetConfigWlanInfraScanJoin();
s32 PS4_SYSV_ABI sceNetConfigWlanScan();
s32 PS4_SYSV_ABI sceNetConfigWlanSetDeviceConfig();
s32 PS4_SYSV_ABI sceNetConnect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
s32 PS4_SYSV_ABI sceNetControl();
s32 PS4_SYSV_ABI sceNetDhcpdStart();
s32 PS4_SYSV_ABI sceNetDhcpdStop();
s32 PS4_SYSV_ABI sceNetDhcpGetAutoipInfo();
s32 PS4_SYSV_ABI sceNetDhcpGetInfo();
s32 PS4_SYSV_ABI sceNetDhcpGetInfoEx();
s32 PS4_SYSV_ABI sceNetDhcpStart();
s32 PS4_SYSV_ABI sceNetDhcpStop();
s32 PS4_SYSV_ABI sceNetDumpAbort();
s32 PS4_SYSV_ABI sceNetDumpCreate();
s32 PS4_SYSV_ABI sceNetDumpDestroy();
s32 PS4_SYSV_ABI sceNetDumpRead();
s32 PS4_SYSV_ABI sceNetDuplicateIpStart();
s32 PS4_SYSV_ABI sceNetDuplicateIpStop();
s32 PS4_SYSV_ABI sceNetEpollAbort(OrbisNetId eid, s32 flags);
s32 PS4_SYSV_ABI sceNetEpollControl(OrbisNetId eid, s32 op, OrbisNetId id,
                                    OrbisNetEpollEvent* event);
OrbisNetId PS4_SYSV_ABI sceNetEpollCreate(const char* name, s32 flags);
s32 PS4_SYSV_ABI sceNetEpollDestroy(OrbisNetId eid);
s32 PS4_SYSV_ABI sceNetEpollWait(OrbisNetId eid, OrbisNetEpollEvent* events, s32 maxevents,
                                 s32 timeout_us);
s32* PS4_SYSV_ABI sceNetErrnoLoc();
s32 PS4_SYSV_ABI sceNetEtherNtostr(const OrbisNetEtherAddr* n, char* str, u64 len);
s32 PS4_SYSV_ABI sceNetEtherStrton(const char* str, OrbisNetEtherAddr* n);
s32 PS4_SYSV_ABI sceNetEventCallbackCreate();
s32 PS4_SYSV_ABI sceNetEventCallbackDestroy();
s32 PS4_SYSV_ABI sceNetEventCallbackGetError();
s32 PS4_SYSV_ABI sceNetEventCallbackWaitCB();
s32 PS4_SYSV_ABI sceNetFreeAllRouteInfo();
s32 PS4_SYSV_ABI sceNetGetArpInfo();
s32 PS4_SYSV_ABI sceNetGetDns6Info(u8* info, s32 flags);
s32 PS4_SYSV_ABI sceNetGetDnsInfo(u32* info, s32 flags);
s32 PS4_SYSV_ABI sceNetGetIfList();
s32 PS4_SYSV_ABI sceNetGetIfListOnce();
const char* PS4_SYSV_ABI sceNetGetIfName(u32 index);
s32 PS4_SYSV_ABI sceNetGetIfnameNumList();
s32 PS4_SYSV_ABI sceNetGetMacAddress(OrbisNetEtherAddr* addr, s32 flags);
s32 PS4_SYSV_ABI sceNetGetMemoryPoolStats(s32 memid, OrbisNetMemoryPoolStats* stat);
s32 PS4_SYSV_ABI sceNetGetNameToIndex();
s32 PS4_SYSV_ABI sceNetGetpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
s32 PS4_SYSV_ABI sceNetGetRandom(u32* out);
s32 PS4_SYSV_ABI sceNetGetRouteInfo();
s32 PS4_SYSV_ABI sceNetGetSockInfo(OrbisNetId s, OrbisNetSockInfo* info, s32 n, s32 flags);
s32 PS4_SYSV_ABI sceNetGetSockInfo6();
s32 PS4_SYSV_ABI sceNetGetsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
s32 PS4_SYSV_ABI sceNetGetsockopt(OrbisNetId s, s32 level, s32 optname, void* optval, u32* optlen);
s32 PS4_SYSV_ABI sceNetGetStatisticsInfo(OrbisNetStatisticsInfo* info, s32 flags);
s32 PS4_SYSV_ABI sceNetGetStatisticsInfoInternal();
void PS4_SYSV_ABI sceNetGetSystemTime(u64* out);
u32 PS4_SYSV_ABI sceNetHtonl(u32 host32);
u64 PS4_SYSV_ABI sceNetHtonll(u64 host64);
u16 PS4_SYSV_ABI sceNetHtons(u16 host16);
const char* PS4_SYSV_ABI sceNetInetNtop(s32 af, const void* src, char* dst, u32 size);
s32 PS4_SYSV_ABI sceNetInetNtopWithScopeId();
s32 PS4_SYSV_ABI sceNetInetPton(s32 af, const char* src, void* dst);
s32 PS4_SYSV_ABI sceNetInetPtonEx(s32 af, const char* src, void* dst, s32 flags);
s32 PS4_SYSV_ABI sceNetInetPtonWithScopeId();
s32 PS4_SYSV_ABI sceNetInfoDumpStart();
s32 PS4_SYSV_ABI sceNetInfoDumpStop();
s32 PS4_SYSV_ABI sceNetInit();
s32 PS4_SYSV_ABI sceNetInitParam();
s32 PS4_SYSV_ABI sceNetIoctl(OrbisNetId s, u64 cmd, void* data);
s32 PS4_SYSV_ABI sceNetListen(OrbisNetId s, s32 backlog);
void* PS4_SYSV_ABI sceNetMemoryAllocate(s64 size, s32 flags);
void PS4_SYSV_ABI sceNetMemoryFree(void* ptr);
u32 PS4_SYSV_ABI sceNetNtohl(u32 net32);
u64 PS4_SYSV_ABI sceNetNtohll(u64 net64);
u16 PS4_SYSV_ABI sceNetNtohs(u16 net16);
s32 PS4_SYSV_ABI sceNetPoolCreate(const char* name, s32 size, s32 flags);
s32 PS4_SYSV_ABI sceNetPoolDestroy(s32 pool_id);
s32 PS4_SYSV_ABI sceNetPppoeStart();
s32 PS4_SYSV_ABI sceNetPppoeStop();
s32 PS4_SYSV_ABI sceNetRecv(OrbisNetId s, void* buf, u64 len, s32 flags);
s32 PS4_SYSV_ABI sceNetRecvfrom(OrbisNetId s, void* buf, u64 len, s32 flags, OrbisNetSockaddr* addr,
                                u32* paddrlen);
s32 PS4_SYSV_ABI sceNetRecvmsg(OrbisNetId s, OrbisNetMsghdr* msg, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverAbort(OrbisNetId rid, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverConnect();
s32 PS4_SYSV_ABI sceNetResolverConnectAbort();
s32 PS4_SYSV_ABI sceNetResolverConnectCreate();
s32 PS4_SYSV_ABI sceNetResolverConnectDestroy();
OrbisNetId PS4_SYSV_ABI sceNetResolverCreate(const char* name, s32 poolid, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverDestroy(OrbisNetId rid);
s32 PS4_SYSV_ABI sceNetResolverGetError(OrbisNetId rid, s32* status);
s32 PS4_SYSV_ABI sceNetResolverStartAton(OrbisNetId rid, const OrbisNetInAddr* addr, char* hostname,
                                         s32 len, s32 timeout, s32 retry, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverStartAton6();
s32 PS4_SYSV_ABI sceNetResolverStartNtoa(OrbisNetId rid, const char* hostname, OrbisNetInAddr* addr,
                                         s32 timeout, s32 retry, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverStartNtoa6();
s32 PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecords(OrbisNetId rid, const char* hostname,
                                                        OrbisNetResolverInfo* info, s32 timeout,
                                                        s32 retry, s32 flags);
s32 PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecordsEx(OrbisNetId rid, const char* hostname,
                                                          OrbisNetResolverInfo* info, s32 timeout,
                                                          s32 retry, s32 flags);
s32 PS4_SYSV_ABI sceNetSend(OrbisNetId s, const void* buf, u64 len, s32 flags);
s32 PS4_SYSV_ABI sceNetSendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, s32 flags);
s32 PS4_SYSV_ABI sceNetSendto(OrbisNetId s, const void* buf, u64 len, s32 flags,
                              const OrbisNetSockaddr* addr, u32 addrlen);
s32 PS4_SYSV_ABI sceNetSetDns6Info(const u8* info, s32 flags);
s32 PS4_SYSV_ABI sceNetSetDns6InfoToKernel();
s32 PS4_SYSV_ABI sceNetSetDnsInfo(const u32* info, s32 flags);
s32 PS4_SYSV_ABI sceNetSetDnsInfoToKernel();
s32 PS4_SYSV_ABI sceNetSetsockopt(OrbisNetId s, s32 level, s32 optname, const void* optval,
                                  u32 optlen);
s32 PS4_SYSV_ABI sceNetShowIfconfig();
s32 PS4_SYSV_ABI sceNetShowIfconfigForBuffer();
s32 PS4_SYSV_ABI sceNetShowIfconfigWithMemory(s32 memid);
s32 PS4_SYSV_ABI sceNetShowNetstat();
s32 PS4_SYSV_ABI sceNetShowNetstatEx();
s32 PS4_SYSV_ABI sceNetShowNetstatExForBuffer();
s32 PS4_SYSV_ABI sceNetShowNetstatForBuffer();
s32 PS4_SYSV_ABI sceNetShowNetstatWithMemory(s32 memid);
s32 PS4_SYSV_ABI sceNetShowPolicy();
s32 PS4_SYSV_ABI sceNetShowPolicyWithMemory(s32 memid);
s32 PS4_SYSV_ABI sceNetShowRoute();
s32 PS4_SYSV_ABI sceNetShowRoute6();
s32 PS4_SYSV_ABI sceNetShowRoute6ForBuffer();
s32 PS4_SYSV_ABI sceNetShowRoute6WithMemory(s32 memid);
s32 PS4_SYSV_ABI sceNetShowRouteForBuffer();
s32 PS4_SYSV_ABI sceNetShowRouteWithMemory(s32 memid);
s32 PS4_SYSV_ABI sceNetShutdown(OrbisNetId s, s32 how);
OrbisNetId PS4_SYSV_ABI sceNetSocket(const char* name, s32 family, s32 type, s32 protocol);
s32 PS4_SYSV_ABI sceNetSocketAbort(OrbisNetId s, s32 flags);
s32 PS4_SYSV_ABI sceNetSocketClose(OrbisNetId s);
s32 PS4_SYSV_ABI sceNetSyncCreate();
s32 PS4_SYSV_ABI sceNetSyncDestroy();
s32 PS4_SYSV_ABI sceNetSyncGet();
s32 PS4_SYSV_ABI sceNetSyncSignal();
s32 PS4_SYSV_ABI sceNetSyncWait();
s32 PS4_SYSV_ABI sceNetSysctl();
s32 PS4_SYSV_ABI sceNetTerm();
void PS4_SYSV_ABI sceNetThreadExit();
s32 PS4_SYSV_ABI sceNetUsleep(s32 microseconds);
s32 PS4_SYSV_ABI Func_0E707A589F751C68();
s32 PS4_SYSV_ABI sceNetEmulationGet();
s32 PS4_SYSV_ABI sceNetEmulationSet();

void RegisterLib(Core::Loader::SymbolsResolver* sym);

struct SystemHooks {
    std::function<bool()> is_online;
    std::function<bool(std::array<u8, 6>* mac)> mac_address;
    std::function<u16()> p2p_port;
    std::function<void(u16 bound_port)> p2p_started;
    std::function<void(u16 bound_port)> p2p_stopped;
    std::function<u32()> public_addr;
};
void SetSystemHooks(SystemHooks hooks);
SystemHooks GetSystemHooks();
void SetKernelErrnoHook(void (*hook)(int orbis_errno));
void ReleaseResolverPool(OrbisNetId rid);

} // namespace Libraries::Net