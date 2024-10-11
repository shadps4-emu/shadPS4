// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/singleton.h>
#include <core/linker.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "np_manager.h"

namespace Libraries::NpManager {

int PS4_SYSV_ABI Func_EF4378573542A508() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcCreateMemoryFromKernel() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcCreateMemoryFromPool() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcDestroyMemory() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcFreeImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcGetNpMemAllocator() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcMallocImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpIpcReallocImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerCreateMemoryFromKernel() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerCreateMemoryFromPool() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerDestroyMemory() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerFreeImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerGetNpMemAllocator() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerMallocImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpManagerReallocImpl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineId13GetNpOnlineIdERKNS0_4UserEP13SceNpOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineId13GetNpOnlineIdERKNS0_4UserEPS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineId16SetNpOnlineIdStrEPKc() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineId5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC1ERK13SceNpOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC1ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC2ERK13SceNpOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC2ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdC2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10NpOnlineIdD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTrans13GetResultCodeEPNS0_6HandleE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTrans21SetRequestAccessTokenEPNS0_6HandleE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTrans24BuildAuthorizationHeaderERKNS0_13NpAccessTokenEPcm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTransC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTransD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTransD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np11NpHttpTransD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClient4InitEii() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClientC1EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClientC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClientD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClientD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpHttpClientD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleToken5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenC1ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenC1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenC2ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenC2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12NpTitleTokenD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessToken14GetAccessTokenEPNS0_6HandleERKNS0_4UserEPS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessToken5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC1ERK16SceNpAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC1ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC2ERK16SceNpAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC2ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenC2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpAccessTokenD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client10EndRequestEii() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client11InitServiceEi() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client11TermServiceEi() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client11WaitRequestEiij() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client12AbortRequestEii() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client12BeginRequestEii() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client13CreateRequestEimPKvPi() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client13DeleteRequestEii() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client13GetIpmiClientEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client13PollEventFlagEijmjPm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc14service_client14PollEventQueueEiPvm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpId5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC1ERK7SceNpId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC1ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC2ERK7SceNpId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC2ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdC2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4NpIdD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4User5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4User7GetUserEiPS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC1Ei() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC1ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC2Ei() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC2ERKS1_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserC2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4UserD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpTicket5ClearEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpTicketD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpTicketD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpTicketD2Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERK13SceNpOnlineIdRKNS0_10NpOnlineIdE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_10NpOnlineIdERK13SceNpOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_10NpOnlineIdES3_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_4UserERKi() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_4UserES3_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERK13SceNpOnlineIdRKNS0_10NpOnlineIdE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_10NpOnlineIdERK13SceNpOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_10NpOnlineIdES3_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_4UserERKi() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_4UserES3_() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10NpOnlineId7IsEmptyEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np12NpTitleToken6GetStrEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np13NpAccessToken7IsEmptyEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User10IsLoggedInEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User12GetAccountIdEPm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User12HasAccountIdEPb() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User25GetAccountIdFromRegistoryEPm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User7IsEmptyEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User7IsGuestEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4User9GetUserIdEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8NpTicket13GetTicketDataEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8NpTicket13GetTicketSizeEv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np11NpHttpTransD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np11NpHttpTransD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np11NpHttpTransD0Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np11NpHttpTransD1Ev() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAbortRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmAbort() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientAbortRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientClearNpTitleToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientClearNpTitleTokenA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientCreateRequest2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientCreateResourceContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientCreateResourceContext2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientDeleteRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientDeleteResourceContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientDeleteResourceContext2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetAppId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetCacheControlMaxAge() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetGameNpTitleInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetGameNpTitleToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetGameTitleBanInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpComInfo2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpComInfo2A() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpComInfo2WithHmac() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpComInfo3() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpComInfo4() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpTitleId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpTitleToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpTitleToken2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetNpTitleTokenA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetRelatedGameNpTitleIds() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetRelatedGameNpTitleIdsA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetRelatedGameNpTitleIdsResult() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceBaseUrl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceBaseUrlA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceBaseUrlWithNpTitleId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceBaseUrlWithNpTitleIdA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceIdInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientGetServiceIdInfoA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientInitialize() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientSetNpTitleId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmClientTerminate() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmCreateConnection() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmCreateRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmDeleteConnection() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmDeleteRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGenerateNpTitleToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGenerateNpTitleToken2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGetNpCommInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGetNpCommInfo2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGetRelatedGameNpTitleIds() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGetServiceBaseUrl() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmGetServiceIdInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmInitialize() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAsmTerminate() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCheckCallback() {
    LOG_TRACE(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCheckNpAvailability() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCheckNpAvailabilityA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCheckNpReachability() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCheckPlus() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCreateAsyncRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCreateRequest() {
    LOG_ERROR(Lib_NpManager, "(DUMMY) called");
    static int id = 0;
    return ++id;
}

int PS4_SYSV_ABI sceNpDeleteRequest(int reqId) {
    LOG_ERROR(Lib_NpManager, "(DUMMY) called reqId = {}", reqId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountAge() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountCountry() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountCountryA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountDateOfBirth() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountDateOfBirthA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountIdA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountLanguage() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountLanguage2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetAccountLanguageA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetGamePresenceStatus() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetGamePresenceStatusA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetNpId(OrbisUserServiceUserId userId, OrbisNpId* npId) {
    LOG_INFO(Lib_NpManager, "userId {}", userId);
    std::string name = Config::getUserName();
    // Fill the unused stuffs to 0
    memset(npId, 0, sizeof(*npId));
    strcpy(npId->handle.data, name.c_str());
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetNpReachabilityState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetParentalControlInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetParentalControlInfoA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetState(s32 userId, OrbisNpState* state) {
    *state = ORBIS_NP_STATE_SIGNED_OUT;
    LOG_DEBUG(Lib_NpManager, "Signed out");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetUserIdByAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetUserIdByOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHasSignedUp() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperAbortRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperAccountIdToNpId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperAccountIdToOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperCreateRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperDeleteRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperNpIdToAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIdMapperOnlineIdToAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageAbortHandle() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageCheckCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageCreateHandle() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageDeleteHandle() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageGetMemoryPoolStatistics() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageInitialize() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessagePrepare() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessagePrepareA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageSendData() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageSendDataA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInGameMessageTerminate() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntCheckPlus() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntGetAppType() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntGetGamePresenceStatus() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntGetNpTitleId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntGetNpTitleIdSecret() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntRegisterGamePresenceCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIsPlusMember() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAbortRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAddActiveSigninStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAddOnlineIdChangeCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAddPlusMemberTypeCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAddSigninStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAuthGetAuthorizationCode() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntAuthGetIdToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntBindOfflineAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCheckGameNpAvailability() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCheckNpAvailability() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCheckNpAvailabilityByPid() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCheckNpState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCheckNpStateA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntClearGameAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntClearOnlineIdChangeFlag() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntClearTicket() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntClearUsedFlag() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntClearVshAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCreateLoginContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCreateLoginRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntCreateRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntDeleteLoginContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntDeleteRequest() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountCountry() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountCountryA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountCountrySdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountDateOfBirthA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountDateOfBirthSdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountIdSdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountLanguage() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountLanguageA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountNpEnv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAccountType() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetActiveSigninState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAuthorizationCodeA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAuthorizationCodeWithPsnoUri() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetAuthServerErrorFlag() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetClientCredentialAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetCommunicationRestrictionStatus() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetGameAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetIssuerId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetLastAccountLanguage() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetMAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetNpEnv() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetNpId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetNpIdByOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetNpIdSdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetOfflineAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetOnlineIdByAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetOnlineIdChangeFlag() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetOnlineIdInternal() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetOnlineIdSdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetParentalControlFlag() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetParentalControlInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetParentalControlInfoA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetPlusMemberType() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetPlusMemberTypeNB() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetServerError() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetSigninState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetTicket() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetTicketA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetTitleTokenWithCheck() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByMAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByNpId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByOfflineAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserIdByOnlineIdSdk() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserList() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetUserNum() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetVshAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetVshAccessTokenWithCheck() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntGetVshClientId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntIsSubAccount() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntIsTemporarySignout() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntIsUnregisteredClientError() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginAddJsonInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginAuthenticate() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginBind() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGet2svInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetAccessTokenViaImplicitFlow() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetAuthenticateResponse() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetAuthorizationCode() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetDeviceCodeInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetEmail() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetOnlineId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginGetUserId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginParseJsonUserInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginResetSsoToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginRevalidatePassword() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginSetAccountInfo() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginSetSsoToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginSignin() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginValidateCredential() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginValidateKratosAuthCode() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntLoginVerifyDeviceCode() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntPfAuth() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntRemoveActiveSigninStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntRemoveOnlineIdChangeCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntRemovePlusMemberTypeCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntRemoveSigninStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntRevalidatePassword() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntSetPlusMemberTypeNB() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntSetTimeout() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntSignout() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntSubmitUserCode() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntTemporarySignout() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntUnbindOfflineAccountId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntUpdateVshAccessToken() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerIntWebLoginRequired() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerPrxStartVsh() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpManagerPrxStopVsh() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpNotifyPlusFeature() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPollAsync() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2CreateUserContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2DeleteUserContext() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2Init() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2IsInit() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2OptionalCheckCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2RegisterDataType() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2RegisterExtendedDataFilter() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2RegisterNotificationExCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2SendPushStatisticsDataSystemTelemetry() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2SetGlobalMutex() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2SetNpCommunicationId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2Term() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2TriggerEmptyUserEvent() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2UnregisterDataType() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2UnregisterExtendedDataFilter() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2UnregisterNotificationExCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2UnsetNpCommunicationId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPush2WaitCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushCheckCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushInit() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntBeginInactive() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntEndInactive() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntGetBindUserState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntGetConnectionState() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntRegisterNotificationPacketCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushIntUnregisterNotificationPacketCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushRegisterExtendedDataFilter() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushRegisterNotificationExCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushSetNpCommunicationId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushStartNotification() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushStartNotificationA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushStopNotification() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushStopNotificationA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushTerm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushUnregisterExtendedDataFilter() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushUnregisterNotificationCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPushUnsetNpCommunicationId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterGamePresenceCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterGamePresenceCallbackA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterNpReachabilityStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterPlusEventCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterStateCallbackA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServiceClientInit() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServiceClientTerm() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetAdditionalScope() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetContentRestriction() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetGamePresenceOnline() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetGamePresenceOnlineA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetNpTitleId() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetNpTitleIdVsh() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetTimeout() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUnregisterGamePresenceCallbackA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUnregisterNpReachabilityStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUnregisterPlusEventCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUnregisterStateCallback() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUnregisterStateCallbackA() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpWaitAsync() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_05003628D66BD87D() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0C388A4F21C98AF9() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0CECC7A08A3E50AF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0D17030A1DA18EEB() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0F0F320B6AD8A53D() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_113C477090F9A174() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_12D367D5C727F008() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1640120BD475931E() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1AFE1C07C95E65A5() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1D983C7E0C28AC72() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_208943695A3B58FE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_258A3D10C99A43BB() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_263E325794B412AC() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2B6A4BF35C5E240D() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2B707FFE05ACB009() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2CE5AB230EBAF8B4() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3237EE3C3AFC187B() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_33D4DFB2A1603BFF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3821D79C1ED86F33() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3F431997C7105BBF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4192797C2D2D3FC3() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_41C7E3D88BBB7F75() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_438F60858A883FCF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4C4A062E5660FABD() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4C808F7A4EFA36A7() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4E1CED7E62F68F46() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5161A48C6A61C4BF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_542603999CA0AEE9() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_54690B41C1128799() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_55A76C7C29521FAD() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_562B234AAE25F80C() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_58D1975026DD864A() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5A60395F8C3FE128() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5B382777E9B5F294() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5DB301F9CD649671() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6441D55869D8D6F2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_69068E18854284DE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6F59C3B00B03E05A() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_724CCE7F78A1356B() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_750F1B053C243308() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_760F079BB91DE258() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_78657523221556EF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8089888BD363EDA6() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_810CA029B6F7C3A1() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8253B94686A8D3FD() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8665138A709E1654() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_882F48FAE6097C0C() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_89DBE4B3303FF888() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8BD3E57620BDDC38() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8F0A74013AD633EC() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8FA6264BF3F6CC00() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9292E87C2C0971E4() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_92CA292318CA03A8() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9348596C2B17F662() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9507E9B321A5E0D7() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_98CA95E231980731() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9E66CC4BBF2C1990() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9E6CEF7064891F84() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A7BC2C792E9522C5() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ABBA0F809548CB02() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B14A27A4CEDE020F() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B323EE1C23AB97F3() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B429819DAEF40BAC() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B54B9571BEAD82C5() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B5ACB5CF4A4114A6() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BA41BE0F44157EE4() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BAA1DEC848D99690() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BB8CCCD6C9480EB2() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BEC25DAAE8B8B81F() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BFEE936391AB0C70() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C0DD2DBE2EA66F7A() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C1F858BF5B86C2A1() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C240618E6FC39206() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C338A34450310E79() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C91EE3603D966909() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB67035ED668CF6B() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D5A5A28B7351A9BE() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DA8426059F1D5A2D() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DA8E15DD00AF9DF8() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DB86987643BB5DD7() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DEC53D7165C137DF() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DFDEEE26F2EB96B3() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E2056A6F01642866() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E240E9B8597EE56E() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E32CE33B706F05F7() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E4F67EFC91C84F87() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E6F041A2660F83EB() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E979BA413BD84D38() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EDDDF2D305DB7866() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F08EC7725B42E2F9() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F3595D8EFFF26EC0() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F3DF5271142F155D() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F89997168DC987A8() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F91B5B25CC9B30D9() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FC335B7102A585B3() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FCEAC354CA8B206E() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FF966E4351E564D6() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

struct NpStateCallbackForNpToolkit {
    OrbisNpStateCallbackForNpToolkit func;
    void* userdata;
};

NpStateCallbackForNpToolkit NpStateCbForNp;

int PS4_SYSV_ABI sceNpCheckCallbackForLib() {
    // LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    const auto* linker = Common::Singleton<Core::Linker>::Instance();
    linker->ExecuteGuest(NpStateCbForNp.func, 1, ORBIS_NP_STATE_SIGNED_OUT,
                         NpStateCbForNp.userdata);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForNpToolkit callback,
                                                      void* userdata) {
    static int id = 0;
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    NpStateCbForNp.func = callback;
    NpStateCbForNp.userdata = userdata;
    return id;
}

int PS4_SYSV_ABI sceNpUnregisterStateCallbackForToolkit() {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceNpManager(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("70N4VzVCpQg", "libSceNpManagerForSys", 1, "libSceNpManager", 1, 1,
                 Func_EF4378573542A508);
    LIB_FUNCTION("pHLjntY0psg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpIpcCreateMemoryFromKernel);
    LIB_FUNCTION("UdhQmx64-uM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpIpcCreateMemoryFromPool);
    LIB_FUNCTION("hyuye+88uPo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpIpcDestroyMemory);
    LIB_FUNCTION("VY8Xji9cAFA", "libSceNpManager", 1, "libSceNpManager", 1, 1, _sceNpIpcFreeImpl);
    LIB_FUNCTION("V38nfJwXYhg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpIpcGetNpMemAllocator);
    LIB_FUNCTION("VBZtcFn7+aY", "libSceNpManager", 1, "libSceNpManager", 1, 1, _sceNpIpcMallocImpl);
    LIB_FUNCTION("TyACAxDH3Uw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpIpcReallocImpl);
    LIB_FUNCTION("fHGhS3uP52k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerCreateMemoryFromKernel);
    LIB_FUNCTION("v8+25H9WIX4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerCreateMemoryFromPool);
    LIB_FUNCTION("4uhgVNAqiag", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerDestroyMemory);
    LIB_FUNCTION("8JX-S2ADen4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerFreeImpl);
    LIB_FUNCTION("ukEeOizCkIU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerGetNpMemAllocator);
    LIB_FUNCTION("p0TfCdPEcsk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerMallocImpl);
    LIB_FUNCTION("PIYEFT1iG0Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _sceNpManagerReallocImpl);
    LIB_FUNCTION("o1azI8TGjbc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineId13GetNpOnlineIdERKNS0_4UserEP13SceNpOnlineId);
    LIB_FUNCTION("1GRQfw+bhcE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineId13GetNpOnlineIdERKNS0_4UserEPS1_);
    LIB_FUNCTION("Icc9+aRUQfQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineId16SetNpOnlineIdStrEPKc);
    LIB_FUNCTION("-QlrD62pWME", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineId5ClearEv);
    LIB_FUNCTION("oGASj6Qjq7M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC1ERK13SceNpOnlineId);
    LIB_FUNCTION("dgSWiLGbjuY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC1ERKS1_);
    LIB_FUNCTION("YYfLHMi0+2M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC1Ev);
    LIB_FUNCTION("mt2Be6qsnsw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC2ERK13SceNpOnlineId);
    LIB_FUNCTION("gPux+0B5N9I", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC2ERKS1_);
    LIB_FUNCTION("gBeifc27nO4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdC2Ev);
    LIB_FUNCTION("kUsK6ZtqofM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdD0Ev);
    LIB_FUNCTION("UyUHeYA21sg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdD1Ev);
    LIB_FUNCTION("YcMKsqoMBtg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np10NpOnlineIdD2Ev);
    LIB_FUNCTION("7I2lZS0DRjA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTrans13GetResultCodeEPNS0_6HandleE);
    LIB_FUNCTION("WoaqjY1ccEo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTrans21SetRequestAccessTokenEPNS0_6HandleE);
    LIB_FUNCTION("mCqfLfIWWuo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTrans24BuildAuthorizationHeaderERKNS0_13NpAccessTokenEPcm);
    LIB_FUNCTION("JDYbbgccPDE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTransC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("Yd7V7lM4bSA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTransD0Ev);
    LIB_FUNCTION("7OiI1ObT1QU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTransD1Ev);
    LIB_FUNCTION("D5qJmwMlccI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np11NpHttpTransD2Ev);
    LIB_FUNCTION("CvGog64+vCk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClient4InitEii);
    LIB_FUNCTION("QvqOkNK5ThU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClientC1EP16SceNpAllocatorEx);
    LIB_FUNCTION("t+T8UG8jats", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClientC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("FjbLZy95ts4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClientD0Ev);
    LIB_FUNCTION("kR3ed2pAvV8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClientD1Ev);
    LIB_FUNCTION("9Uew6b9Pp8U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpHttpClientD2Ev);
    LIB_FUNCTION("zAvqfrR2f7c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleToken5ClearEv);
    LIB_FUNCTION("xQM94RIreRc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenC1ERKS1_);
    LIB_FUNCTION("j6oWzyuDal4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenC1Ev);
    LIB_FUNCTION("oDMQ96CSgKE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenC2ERKS1_);
    LIB_FUNCTION("+3JWTVP4NUc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenC2Ev);
    LIB_FUNCTION("SyxdUakD7HU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenD0Ev);
    LIB_FUNCTION("+fA-tpNvZNg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenD1Ev);
    LIB_FUNCTION("43B0VnF0P7E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np12NpTitleTokenD2Ev);
    LIB_FUNCTION("jjpTY0fRA44", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessToken14GetAccessTokenEPNS0_6HandleERKNS0_4UserEPS1_);
    LIB_FUNCTION("Y5eglu1FrsE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessToken5ClearEv);
    LIB_FUNCTION("FMoSUe3Uac4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC1ERK16SceNpAccessToken);
    LIB_FUNCTION("MwAYknEWfAU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC1ERKS1_);
    LIB_FUNCTION("h8d3tfMiyhc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC1Ev);
    LIB_FUNCTION("h0EenX2eWyA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC2ERK16SceNpAccessToken);
    LIB_FUNCTION("CNJoUbqVaFc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC2ERKS1_);
    LIB_FUNCTION("2lzWy2xmnhY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenC2Ev);
    LIB_FUNCTION("SFZYbH7eOnk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenD0Ev);
    LIB_FUNCTION("0SfP+1+7aB4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenD1Ev);
    LIB_FUNCTION("u9tBiSNnvn8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np13NpAccessTokenD2Ev);
    LIB_FUNCTION("D3ucpMtsmEw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client10EndRequestEii);
    LIB_FUNCTION("tpXVNSFwJRs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client11InitServiceEi);
    LIB_FUNCTION("UvDQq9+QMuI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client11TermServiceEi);
    LIB_FUNCTION("huJ-2GzzNXs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client11WaitRequestEiij);
    LIB_FUNCTION("EPEUMPT+9XI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client12AbortRequestEii);
    LIB_FUNCTION("HhtuEAftVvk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client12BeginRequestEii);
    LIB_FUNCTION("t5cZhzOEeDM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client13CreateRequestEimPKvPi);
    LIB_FUNCTION("aFpR2VzmSqA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client13DeleteRequestEii);
    LIB_FUNCTION("hKTdrR1+dN0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client13GetIpmiClientEv);
    LIB_FUNCTION("ZDocIq+2jbI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client13PollEventFlagEijmjPm);
    LIB_FUNCTION("fs2BaxmsAZg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np3ipc14service_client14PollEventQueueEiPvm);
    LIB_FUNCTION("HSh8IJaDD7o", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4NpId5ClearEv);
    LIB_FUNCTION("6WTgpKqUxRo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4NpIdC1ERK7SceNpId);
    LIB_FUNCTION("SuCCD+AZwCc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4NpIdC1ERKS1_);
    LIB_FUNCTION("YU-PxwZq21U", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4NpIdC1Ev);
    LIB_FUNCTION("ZHZ6QZ8xHLE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4NpIdC2ERK7SceNpId);
    LIB_FUNCTION("qBlMzJbMa7c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4NpIdC2ERKS1_);
    LIB_FUNCTION("OIdCMA7vGHA", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4NpIdC2Ev);
    LIB_FUNCTION("lUXyFGSFXKo", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4NpIdD0Ev);
    LIB_FUNCTION("WcfJXQ2NFP4", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4NpIdD1Ev);
    LIB_FUNCTION("ya+s8zGxVQQ", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4NpIdD2Ev);
    LIB_FUNCTION("GtMgx4YcBuo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4User5ClearEv);
    LIB_FUNCTION("bwwspVgS4hQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4User7GetUserEiPS1_);
    LIB_FUNCTION("Z4wnPrd9jIE", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserC1Ei);
    LIB_FUNCTION("rgtbpTzx0RA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4UserC1ERKS1_);
    LIB_FUNCTION("S7Afe0llsL8", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserC1Ev);
    LIB_FUNCTION("i2KGykoRA-4", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserC2Ei);
    LIB_FUNCTION("YvL0D8Vg6VM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np4UserC2ERKS1_);
    LIB_FUNCTION("F-AkFa9cABI", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserC2Ev);
    LIB_FUNCTION("HhKQodH164k", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserD0Ev);
    LIB_FUNCTION("gQFyT9aIsOk", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserD1Ev);
    LIB_FUNCTION("itBuc3IIDaY", "libSceNpManager", 1, "libSceNpManager", 1, 1, _ZN3sce2np4UserD2Ev);
    LIB_FUNCTION("BI3Wo2RpVmA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np8NpTicket5ClearEv);
    LIB_FUNCTION("KjXpVcQXaYc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np8NpTicketD0Ev);
    LIB_FUNCTION("AIMCjPPVWZM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np8NpTicketD1Ev);
    LIB_FUNCTION("JL4zz6ehIWE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2np8NpTicketD2Ev);
    LIB_FUNCTION("-WGPScpDMWA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npeqERK13SceNpOnlineIdRKNS0_10NpOnlineIdE);
    LIB_FUNCTION("m3jEtGAP9jE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npeqERKNS0_10NpOnlineIdERK13SceNpOnlineId);
    LIB_FUNCTION("KGitZXuSY7U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npeqERKNS0_10NpOnlineIdES3_);
    LIB_FUNCTION("0qjYM9bp5vs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npeqERKNS0_4UserERKi);
    LIB_FUNCTION("-BgzebSMaVY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npeqERKNS0_4UserES3_);
    LIB_FUNCTION("-lWtMfBycrg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npneERK13SceNpOnlineIdRKNS0_10NpOnlineIdE);
    LIB_FUNCTION("d-nucrQRJZg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npneERKNS0_10NpOnlineIdERK13SceNpOnlineId);
    LIB_FUNCTION("pt8E9JYqZm4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npneERKNS0_10NpOnlineIdES3_);
    LIB_FUNCTION("XlGEzCqlHpI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npneERKNS0_4UserERKi);
    LIB_FUNCTION("ta8lASAxro4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZN3sce2npneERKNS0_4UserES3_);
    LIB_FUNCTION("IIBBieYYH6M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np10NpOnlineId7IsEmptyEv);
    LIB_FUNCTION("lDCWWROsrEg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np12NpTitleToken6GetStrEv);
    LIB_FUNCTION("2lvOARTF5x0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np13NpAccessToken7IsEmptyEv);
    LIB_FUNCTION("noJm52uLN00", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User10IsLoggedInEv);
    LIB_FUNCTION("f2K8i7KU20k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User12GetAccountIdEPm);
    LIB_FUNCTION("2hiV0v27kcY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User12HasAccountIdEPb);
    LIB_FUNCTION("GZ2YtnlAzH4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User25GetAccountIdFromRegistoryEPm);
    LIB_FUNCTION("IyT41iG8Ync", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User7IsEmptyEv);
    LIB_FUNCTION("JwpT2LYSxrg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User7IsGuestEv);
    LIB_FUNCTION("td8GJFROaEA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np4User9GetUserIdEv);
    LIB_FUNCTION("ox2Ie98lPAQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np8NpTicket13GetTicketDataEv);
    LIB_FUNCTION("fs1TCWwTYCA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZNK3sce2np8NpTicket13GetTicketSizeEv);
    LIB_FUNCTION("i80IWKzGrCE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZThn16_N3sce2np11NpHttpTransD0Ev);
    LIB_FUNCTION("rbsJZPsEjN8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZThn16_N3sce2np11NpHttpTransD1Ev);
    LIB_FUNCTION("YudSGKQqqnI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZThn8_N3sce2np11NpHttpTransD0Ev);
    LIB_FUNCTION("mHE2Hk9Ki80", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 _ZThn8_N3sce2np11NpHttpTransD1Ev);
    LIB_FUNCTION("OzKvTvg3ZYU", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpAbortRequest);
    LIB_FUNCTION("JrXA7baBMPQ", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpAsmAbort);
    LIB_FUNCTION("0cn2c-bk8wA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientAbortRequest);
    LIB_FUNCTION("coT6qsU5t9M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientClearNpTitleToken);
    LIB_FUNCTION("zHxRg0AUZm8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientClearNpTitleTokenA);
    LIB_FUNCTION("tOJ-WGFDt-Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientCreateRequest2);
    LIB_FUNCTION("GPRRxFM01r4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientCreateResourceContext);
    LIB_FUNCTION("Auqk+H3qGuo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientCreateResourceContext2);
    LIB_FUNCTION("1wMn3X94WME", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientDeleteRequest);
    LIB_FUNCTION("KA2AITpVTCg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientDeleteResourceContext);
    LIB_FUNCTION("4gi0acCfJL4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientDeleteResourceContext2);
    LIB_FUNCTION("yWcto7E39+k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetAppId);
    LIB_FUNCTION("Q7fnpdkjBp0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetCacheControlMaxAge);
    LIB_FUNCTION("vf+lYeOXdI8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetGameNpTitleInfo);
    LIB_FUNCTION("YQ7-z4zFWok", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetGameNpTitleToken);
    LIB_FUNCTION("6bvqnBIINiY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetGameTitleBanInfo);
    LIB_FUNCTION("cOLn5A3ZoqU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpComInfo2);
    LIB_FUNCTION("P6fkTotWFEg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpComInfo2A);
    LIB_FUNCTION("fX+iM4sZIl0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpComInfo2WithHmac);
    LIB_FUNCTION("uObO1I15Se0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpComInfo3);
    LIB_FUNCTION("u+iH3rRyPEE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpComInfo4);
    LIB_FUNCTION("nuPl4uVXYMM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpTitleId);
    LIB_FUNCTION("HtpGVrVLOlA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpTitleToken);
    LIB_FUNCTION("2GbOPwcNJd0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpTitleToken2);
    LIB_FUNCTION("cugDQBHux8k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetNpTitleTokenA);
    LIB_FUNCTION("rT4NWysyX+g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetRelatedGameNpTitleIds);
    LIB_FUNCTION("scCBvfXGeRM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetRelatedGameNpTitleIdsA);
    LIB_FUNCTION("TtHBV0mH8kY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetRelatedGameNpTitleIdsResult);
    LIB_FUNCTION("O42ZlBvIPMM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceBaseUrl);
    LIB_FUNCTION("iRvaaSfHBc8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceBaseUrlA);
    LIB_FUNCTION("nxpboyvJGf4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceBaseUrlWithNpTitleId);
    LIB_FUNCTION("wXpm75McNZo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceBaseUrlWithNpTitleIdA);
    LIB_FUNCTION("TiC81-OKjpg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceIdInfo);
    LIB_FUNCTION("3rlqy6XxrmI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientGetServiceIdInfoA);
    LIB_FUNCTION("wZy5M6lzip0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientInitialize);
    LIB_FUNCTION("9o4inFK-oWc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientSetNpTitleId);
    LIB_FUNCTION("cu1LlJo+5EY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmClientTerminate);
    LIB_FUNCTION("YDDHD6RP4HQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmCreateConnection);
    LIB_FUNCTION("hIFFMeoLhcY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmCreateRequest);
    LIB_FUNCTION("UxOJvGmy3mA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmDeleteConnection);
    LIB_FUNCTION("RfokKHMuOsE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmDeleteRequest);
    LIB_FUNCTION("ulPuWk7bYCU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGenerateNpTitleToken);
    LIB_FUNCTION("0bCpZmASTm4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGenerateNpTitleToken2);
    LIB_FUNCTION("dSlkmPVTcvk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGetNpCommInfo);
    LIB_FUNCTION("IDXFgpkpDsU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGetNpCommInfo2);
    LIB_FUNCTION("Dkpw9X-HSVA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGetRelatedGameNpTitleIds);
    LIB_FUNCTION("kc-O9XKFRIE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGetServiceBaseUrl);
    LIB_FUNCTION("1Xe+XZ1oI28", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpAsmGetServiceIdInfo);
    LIB_FUNCTION("j2dSNi+SJro", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpAsmInitialize);
    LIB_FUNCTION("Dt2rEe-d5c0", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpAsmTerminate);
    LIB_FUNCTION("3Zl8BePTh9Y", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpCheckCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCheckCallbackForLib);
    LIB_FUNCTION("2rsFmlGWleQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCheckNpAvailability);
    LIB_FUNCTION("8Z2Jc5GvGDI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCheckNpAvailabilityA);
    LIB_FUNCTION("KfGZg2y73oM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCheckNpReachability);
    LIB_FUNCTION("r6MyYJkryz8", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpCheckPlus);
    LIB_FUNCTION("eiqMCt9UshI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCreateAsyncRequest);
    LIB_FUNCTION("GpLQDNKICac", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpCreateRequest);
    LIB_FUNCTION("S7QTn72PrDw", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpDeleteRequest);
    LIB_FUNCTION("+4DegjBqV1g", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetAccountAge);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountry);
    LIB_FUNCTION("JT+t00a3TxA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountryA);
    LIB_FUNCTION("8VBTeRf1ZwI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountDateOfBirth);
    LIB_FUNCTION("q3M7XzBKC3s", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountDateOfBirthA);
    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetAccountId);
    LIB_FUNCTION("rbknaUjpqWo", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetAccountIdA);
    LIB_FUNCTION("KZ1Mj9yEGYc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountLanguage);
    LIB_FUNCTION("3Tcz5bNCfZQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountLanguage2);
    LIB_FUNCTION("TPMbgIxvog0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountLanguageA);
    LIB_FUNCTION("IPb1hd1wAGc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetGamePresenceStatus);
    LIB_FUNCTION("oPO9U42YpgI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetGamePresenceStatusA);
    LIB_FUNCTION("p-o74CnoNzY", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetNpId);
    LIB_FUNCTION("e-ZuhGEoeC4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetNpReachabilityState);
    LIB_FUNCTION("XDncXQIJUSk", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetOnlineId);
    LIB_FUNCTION("ilwLM4zOmu4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetParentalControlInfo);
    LIB_FUNCTION("m9L3O6yst-U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetParentalControlInfoA);
    LIB_FUNCTION("eQH7nWPcAgc", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetState);
    LIB_FUNCTION("VgYczPGB5ss", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetUserIdByAccountId);
    LIB_FUNCTION("F6E4ycq9Dbg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetUserIdByOnlineId);
    LIB_FUNCTION("Oad3rvY-NJQ", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpHasSignedUp);
    LIB_FUNCTION("fJuQuipzW10", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperAbortRequest);
    LIB_FUNCTION("alNLle2vACg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperAccountIdToNpId);
    LIB_FUNCTION("TV3KKXZLUj4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperAccountIdToOnlineId);
    LIB_FUNCTION("lCAYAK4kfkc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperCreateRequest);
    LIB_FUNCTION("Z8nyVQCGCVo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperDeleteRequest);
    LIB_FUNCTION("21FMz6O4B2E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperNpIdToAccountId);
    LIB_FUNCTION("zEZvGyjEhuk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIdMapperOnlineIdToAccountId);
    LIB_FUNCTION("BdykpTwq2bs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageAbortHandle);
    LIB_FUNCTION("lp7vzwISXMg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageCheckCallback);
    LIB_FUNCTION("s4UEa5iBJdc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageCreateHandle);
    LIB_FUNCTION("+anuSx2avHQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageDeleteHandle);
    LIB_FUNCTION("Ubv+fP58W1U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageGetMemoryPoolStatistics);
    LIB_FUNCTION("GFhVUpRmbHE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageInitialize);
    LIB_FUNCTION("Vh1bhUG6mSs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessagePrepare);
    LIB_FUNCTION("IkL62FMpIpo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessagePrepareA);
    LIB_FUNCTION("ON7Sf5XEMmI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageSendData);
    LIB_FUNCTION("PQDFxcnqxtw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageSendDataA);
    LIB_FUNCTION("bMG3cVmUmuk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageTerminate);
    LIB_FUNCTION("GsWjzRU7AWA", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpIntCheckPlus);
    LIB_FUNCTION("H6xqSNWg0wM", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpIntGetAppType);
    LIB_FUNCTION("SdNiYQWjU6E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIntGetGamePresenceStatus);
    LIB_FUNCTION("H0n1QHWdVwQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIntGetNpTitleId);
    LIB_FUNCTION("LtYqw9M23hw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIntGetNpTitleIdSecret);
    LIB_FUNCTION("bZ2mBvP79d8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpIntRegisterGamePresenceCallback);
    LIB_FUNCTION("Ybu6AxV6S0o", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpIsPlusMember);
    LIB_FUNCTION("AUuzKQIwhXY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAbortRequest);
    LIB_FUNCTION("J0MUxuo9H9c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAddActiveSigninStateCallback);
    LIB_FUNCTION("wIX4m0mLfqA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAddOnlineIdChangeCallback);
    LIB_FUNCTION("E6rzFwsDFwE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAddPlusMemberTypeCallback);
    LIB_FUNCTION("S9xDus0Cums", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAddSigninStateCallback);
    LIB_FUNCTION("1aifBDr9oqc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAuthGetAuthorizationCode);
    LIB_FUNCTION("fMWCG0Tqofg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntAuthGetIdToken);
    LIB_FUNCTION("97RAfJch+qE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntBindOfflineAccountId);
    LIB_FUNCTION("Xg7dJekKeHM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCheckGameNpAvailability);
    LIB_FUNCTION("m4JiU8k2PyI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCheckNpAvailability);
    LIB_FUNCTION("d+lmTLvsaRs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCheckNpAvailabilityByPid);
    LIB_FUNCTION("Dvk+xqAqXco", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCheckNpState);
    LIB_FUNCTION("U30AU92fWdU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCheckNpStateA);
    LIB_FUNCTION("r7d8eEp5vJE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntClearGameAccessToken);
    LIB_FUNCTION("5ZoFb+9L7LY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntClearOnlineIdChangeFlag);
    LIB_FUNCTION("6TTRm8KRqbw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntClearTicket);
    LIB_FUNCTION("QZpXoz9wjbE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntClearUsedFlag);
    LIB_FUNCTION("miJIPnB2cfI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntClearVshAccessToken);
    LIB_FUNCTION("6n8NT1pHW9g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCreateLoginContext);
    LIB_FUNCTION("CdQg39qlfgY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCreateLoginRequest);
    LIB_FUNCTION("xZk+QcivrFE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntCreateRequest);
    LIB_FUNCTION("EgmlHG93Tpw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntDeleteLoginContext);
    LIB_FUNCTION("HneC+SpeLwc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntDeleteRequest);
    LIB_FUNCTION("7+uKCMe4SLk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountCountry);
    LIB_FUNCTION("fjJ4xXM+3Tw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountCountryA);
    LIB_FUNCTION("mUcn35JWAvI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountCountrySdk);
    LIB_FUNCTION("CConkVwc7Dc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountDateOfBirthA);
    LIB_FUNCTION("3TbxIy0VEiU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountDateOfBirthSdk);
    LIB_FUNCTION("XS-eY7KRqjQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountId);
    LIB_FUNCTION("1H07-M8fGec", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountIdSdk);
    LIB_FUNCTION("C6xstRBFOio", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountLanguage);
    LIB_FUNCTION("e6rTjFmcQjY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountLanguageA);
    LIB_FUNCTION("HvNrMhlWBSk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountNpEnv);
    LIB_FUNCTION("9lz4fkS+eEk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAccountType);
    LIB_FUNCTION("UAA2-ZTmgJc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetActiveSigninState);
    LIB_FUNCTION("1DMXuE0CbGQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAuthorizationCodeA);
    LIB_FUNCTION("xPvV6oMKOWY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAuthorizationCodeWithPsnoUri);
    LIB_FUNCTION("HkUgFhrpAD4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetAuthServerErrorFlag);
    LIB_FUNCTION("TXzpCgPmXEQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetClientCredentialAccessToken);
    LIB_FUNCTION("A3m-y8VVgqM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetCommunicationRestrictionStatus);
    LIB_FUNCTION("iTXe6EWAHek", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetGameAccessToken);
    LIB_FUNCTION("es6OiIxGiL0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetIssuerId);
    LIB_FUNCTION("jCJEWuExbZg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetLastAccountLanguage);
    LIB_FUNCTION("Oad+nopFTTA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetMAccountId);
    LIB_FUNCTION("BTRVfOx7K1c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetNpEnv);
    LIB_FUNCTION("azEmYv5NqWo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetNpId);
    LIB_FUNCTION("gFB0RmKjyaI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetNpIdByOnlineId);
    LIB_FUNCTION("41CVMRinjWU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetNpIdSdk);
    LIB_FUNCTION("70Swvw7h6ck", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetOfflineAccountId);
    LIB_FUNCTION("QnO8zMmKcGE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetOnlineIdByAccountId);
    LIB_FUNCTION("lYkDUwyzr0s", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetOnlineIdChangeFlag);
    LIB_FUNCTION("jkQKWQTOu8g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetOnlineIdInternal);
    LIB_FUNCTION("sTtvF4QVhjg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetOnlineIdSdk);
    LIB_FUNCTION("FqtDOHUuDNw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetParentalControlFlag);
    LIB_FUNCTION("NS1sEhoj-B0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetParentalControlInfo);
    LIB_FUNCTION("ggj9Qm4XDrU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetParentalControlInfoA);
    LIB_FUNCTION("vrre3KW6OPg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetPlusMemberType);
    LIB_FUNCTION("XRFchqddEVU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetPlusMemberTypeNB);
    LIB_FUNCTION("iDlso2ZrQfA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetServerError);
    LIB_FUNCTION("6miba-pcQt8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetSigninState);
    LIB_FUNCTION("uVAfWmv+cc8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetTicket);
    LIB_FUNCTION("43B0lauksLY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetTicketA);
    LIB_FUNCTION("HsHttp1Ktm0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetTitleTokenWithCheck);
    LIB_FUNCTION("OZTedKNUeFU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByAccountId);
    LIB_FUNCTION("uxLmJ141PmA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByMAccountId);
    LIB_FUNCTION("MDczH3SxE9Q", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByNpId);
    LIB_FUNCTION("Nhxy2NmQhbs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByOfflineAccountId);
    LIB_FUNCTION("uSLgWz8ohak", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByOnlineId);
    LIB_FUNCTION("H33CwgKf4Rs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserIdByOnlineIdSdk);
    LIB_FUNCTION("PL10NiZ0XNA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserList);
    LIB_FUNCTION("etZ84Rf3Urw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetUserNum);
    LIB_FUNCTION("mBTFixSxTzQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetVshAccessToken);
    LIB_FUNCTION("+waQfICfHaw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetVshAccessTokenWithCheck);
    LIB_FUNCTION("3f0ejg9vcE8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntGetVshClientId);
    LIB_FUNCTION("ossvuXednsc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntIsSubAccount);
    LIB_FUNCTION("atgHp5dQi5k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntIsTemporarySignout);
    LIB_FUNCTION("jwOjEhWD6E4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntIsUnregisteredClientError);
    LIB_FUNCTION("aU5QaUCW-Ik", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginAddJsonInfo);
    LIB_FUNCTION("KQYLX4tVLe4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginAuthenticate);
    LIB_FUNCTION("bzf8a7LxtCQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginBind);
    LIB_FUNCTION("xAdGRA3ucDg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGet2svInfo);
    LIB_FUNCTION("-P0LG2EUFBE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetAccessToken);
    LIB_FUNCTION("38cfkczfN08", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetAccessTokenViaImplicitFlow);
    LIB_FUNCTION("dvkqP9KUMfk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetAccountId);
    LIB_FUNCTION("sEZaB9KQ10k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetAuthenticateResponse);
    LIB_FUNCTION("Y+hLqeLseRk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetAuthorizationCode);
    LIB_FUNCTION("EXeJ80p01gs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetDeviceCodeInfo);
    LIB_FUNCTION("yqsFy9yg2rU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetEmail);
    LIB_FUNCTION("wXfHhmzUjK4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetOnlineId);
    LIB_FUNCTION("yWMBHiRdEbk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginGetUserId);
    LIB_FUNCTION("uaCfG0TAPmg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginParseJsonUserInfo);
    LIB_FUNCTION("yHl0pPA3rPQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginResetSsoToken);
    LIB_FUNCTION("0cLPZO1Voe8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginRevalidatePassword);
    LIB_FUNCTION("hmVLIi3pQDE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginSetAccountInfo);
    LIB_FUNCTION("X-WHexCbxcI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginSetSsoToken);
    LIB_FUNCTION("rCnvauevHHc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginSignin);
    LIB_FUNCTION("qmZHHehEDog", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginValidateCredential);
    LIB_FUNCTION("zXukItkUuko", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginValidateKratosAuthCode);
    LIB_FUNCTION("ujtFwWJnv+E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntLoginVerifyDeviceCode);
    LIB_FUNCTION("d0IkWV+u25g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntPfAuth);
    LIB_FUNCTION("SuBDgQswXgo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntRemoveActiveSigninStateCallback);
    LIB_FUNCTION("5nayeu8VK5Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntRemoveOnlineIdChangeCallback);
    LIB_FUNCTION("PafRf+sxnwA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntRemovePlusMemberTypeCallback);
    LIB_FUNCTION("zh2KsQZlAN4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntRemoveSigninStateCallback);
    LIB_FUNCTION("k4M1w5Xstck", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntRevalidatePassword);
    LIB_FUNCTION("C77VnsdaKKI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntSetPlusMemberTypeNB);
    LIB_FUNCTION("PZhz+vjp2CM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntSetTimeout);
    LIB_FUNCTION("64D6V-ADQe0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntSignout);
    LIB_FUNCTION("+IagDajB6AQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntSubmitUserCode);
    LIB_FUNCTION("wUT4cOK0bj0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntTemporarySignout);
    LIB_FUNCTION("IG6ZoGSDaMk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntUnbindOfflineAccountId);
    LIB_FUNCTION("dTvQe2clcNw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntUpdateVshAccessToken);
    LIB_FUNCTION("6AcoqeEhs6E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerIntWebLoginRequired);
    LIB_FUNCTION("LGEIdgILQek", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerPrxStartVsh);
    LIB_FUNCTION("9P8qV9WtgKA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpManagerPrxStopVsh);
    LIB_FUNCTION("Gaxrp3EWY-M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpNotifyPlusFeature);
    LIB_FUNCTION("uqcPJLWL08M", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPollAsync);
    LIB_FUNCTION("QGN2n4c8na4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2CreateUserContext);
    LIB_FUNCTION("HnV+y1xVP2c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2DeleteUserContext);
    LIB_FUNCTION("sDqpKnwnAJQ", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPush2Init);
    LIB_FUNCTION("i1lhp0Wlu+k", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPush2IsInit);
    LIB_FUNCTION("KnOXRM1i6KM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2OptionalCheckCallback);
    LIB_FUNCTION("CsIrEmYADDo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2RegisterDataType);
    LIB_FUNCTION("4ic6Lb+mlfA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2RegisterExtendedDataFilter);
    LIB_FUNCTION("OdRcux-QXm8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2RegisterNotificationExCallback);
    LIB_FUNCTION("KiXYNfe7r9o", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2SendPushStatisticsDataSystemTelemetry);
    LIB_FUNCTION("+rPzLhUKj1Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2SetGlobalMutex);
    LIB_FUNCTION("Y1EmilNpj3Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2SetNpCommunicationId);
    LIB_FUNCTION("KjAjcg3W7F8", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPush2Term);
    LIB_FUNCTION("i9NM4gcpZhk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2TriggerEmptyUserEvent);
    LIB_FUNCTION("rwM99K5fzIk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2UnregisterDataType);
    LIB_FUNCTION("LpfRp+-sitI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2UnregisterExtendedDataFilter);
    LIB_FUNCTION("2q3IIivs72Q", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2UnregisterNotificationExCallback);
    LIB_FUNCTION("tkNfuSDEgYg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2UnsetNpCommunicationId);
    LIB_FUNCTION("c3T1XEYr9MI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPush2WaitCallback);
    LIB_FUNCTION("kdrdY-BEJMw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushCheckCallback);
    LIB_FUNCTION("DkN+WBclFps", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPushInit);
    LIB_FUNCTION("1S2urF24zNQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntBeginInactive);
    LIB_FUNCTION("XyvQv2qjUng", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntEndInactive);
    LIB_FUNCTION("B7bQNq1KPQQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntGetBindUserState);
    LIB_FUNCTION("O-2TTjhWw10", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntGetConnectionState);
    LIB_FUNCTION("Lg5mNqy1zdQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntRegisterNotificationPacketCallback);
    LIB_FUNCTION("RSnzCRbqwDU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushIntUnregisterNotificationPacketCallback);
    LIB_FUNCTION("U9hX5ssnYZ4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushRegisterExtendedDataFilter);
    LIB_FUNCTION("l3dG7h4TlLg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushRegisterNotificationExCallback);
    LIB_FUNCTION("rjatoAGW+Fo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushSetNpCommunicationId);
    LIB_FUNCTION("a7ipJQTfQwo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushStartNotification);
    LIB_FUNCTION("uhSJXVMYQWc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushStartNotificationA);
    LIB_FUNCTION("d695X978Bgw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushStopNotification);
    LIB_FUNCTION("Xa1igyHioag", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushStopNotificationA);
    LIB_FUNCTION("qo5mH49gnDA", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpPushTerm);
    LIB_FUNCTION("VxjXt8G-9Ns", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushUnregisterExtendedDataFilter);
    LIB_FUNCTION("6MuJ-vnDk6A", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushUnregisterNotificationCallback);
    LIB_FUNCTION("j1YsEXl5ta4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpPushUnsetNpCommunicationId);
    LIB_FUNCTION("uFJpaKNBAj4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterGamePresenceCallback);
    LIB_FUNCTION("KswxLxk4c1Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterGamePresenceCallbackA);
    LIB_FUNCTION("hw5KNqAAels", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterNpReachabilityStateCallback);
    LIB_FUNCTION("GImICnh+boA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterPlusEventCallback);
    LIB_FUNCTION("VfRSmPmj8Q8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterStateCallback);
    LIB_FUNCTION("qQJfO8HAiaY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterStateCallbackA);
    LIB_FUNCTION("JHOtNtQ-jmw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpServiceClientInit);
    LIB_FUNCTION("Hhmu86aYI1E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpServiceClientTerm);
    LIB_FUNCTION("41gDrpv1pTw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpSetAdditionalScope);
    LIB_FUNCTION("A2CQ3kgSopQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpSetContentRestriction);
    LIB_FUNCTION("KO+11cgC7N0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpSetGamePresenceOnline);
    LIB_FUNCTION("C0gNCiRIi4U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpSetGamePresenceOnlineA);
    LIB_FUNCTION("Ec63y59l9tw", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpSetNpTitleId);
    LIB_FUNCTION("TJqSgUEzexM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpSetNpTitleIdVsh);
    LIB_FUNCTION("-QglDeRr8D8", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpSetTimeout);
    LIB_FUNCTION("aJZyCcHxzu4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterGamePresenceCallbackA);
    LIB_FUNCTION("cRILAEvn+9M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterNpReachabilityStateCallback);
    LIB_FUNCTION("xViqJdDgKl0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterPlusEventCallback);
    LIB_FUNCTION("mjjTXh+NHWY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterStateCallback);
    LIB_FUNCTION("M3wFXbYQtAA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterStateCallbackA);
    LIB_FUNCTION("jyi5p9XWUSs", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpWaitAsync);
    LIB_FUNCTION("BQA2KNZr2H0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_05003628D66BD87D);
    LIB_FUNCTION("DDiKTyHJivk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_0C388A4F21C98AF9);
    LIB_FUNCTION("DOzHoIo+UK8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_0CECC7A08A3E50AF);
    LIB_FUNCTION("DRcDCh2hjus", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_0D17030A1DA18EEB);
    LIB_FUNCTION("Dw8yC2rYpT0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_0F0F320B6AD8A53D);
    LIB_FUNCTION("ETxHcJD5oXQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_113C477090F9A174);
    LIB_FUNCTION("EtNn1ccn8Ag", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_12D367D5C727F008);
    LIB_FUNCTION("FkASC9R1kx4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_1640120BD475931E);
    LIB_FUNCTION("Gv4cB8leZaU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_1AFE1C07C95E65A5);
    LIB_FUNCTION("HZg8fgworHI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_1D983C7E0C28AC72);
    LIB_FUNCTION("IIlDaVo7WP4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_208943695A3B58FE);
    LIB_FUNCTION("JYo9EMmaQ7s", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_258A3D10C99A43BB);
    LIB_FUNCTION("Jj4yV5S0Eqw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_263E325794B412AC);
    LIB_FUNCTION("K2pL81xeJA0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_2B6A4BF35C5E240D);
    LIB_FUNCTION("K3B--gWssAk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_2B707FFE05ACB009);
    LIB_FUNCTION("LOWrIw66+LQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_2CE5AB230EBAF8B4);
    LIB_FUNCTION("MjfuPDr8GHs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_3237EE3C3AFC187B);
    LIB_FUNCTION("M9TfsqFgO-8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_33D4DFB2A1603BFF);
    LIB_FUNCTION("OCHXnB7YbzM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_3821D79C1ED86F33);
    LIB_FUNCTION("P0MZl8cQW78", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_3F431997C7105BBF);
    LIB_FUNCTION("QZJ5fC0tP8M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_4192797C2D2D3FC3);
    LIB_FUNCTION("Qcfj2Iu7f3U", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_41C7E3D88BBB7F75);
    LIB_FUNCTION("Q49ghYqIP88", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_438F60858A883FCF);
    LIB_FUNCTION("TEoGLlZg+r0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_4C4A062E5660FABD);
    LIB_FUNCTION("TICPek76Nqc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_4C808F7A4EFA36A7);
    LIB_FUNCTION("ThztfmL2j0Y", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_4E1CED7E62F68F46);
    LIB_FUNCTION("UWGkjGphxL8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_5161A48C6A61C4BF);
    LIB_FUNCTION("VCYDmZygruk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_542603999CA0AEE9);
    LIB_FUNCTION("VGkLQcESh5k", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_54690B41C1128799);
    LIB_FUNCTION("VadsfClSH60", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_55A76C7C29521FAD);
    LIB_FUNCTION("VisjSq4l+Aw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_562B234AAE25F80C);
    LIB_FUNCTION("WNGXUCbdhko", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_58D1975026DD864A);
    LIB_FUNCTION("WmA5X4w-4Sg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_5A60395F8C3FE128);
    LIB_FUNCTION("Wzgnd+m18pQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_5B382777E9B5F294);
    LIB_FUNCTION("XbMB+c1klnE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_5DB301F9CD649671);
    LIB_FUNCTION("ZEHVWGnY1vI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_6441D55869D8D6F2);
    LIB_FUNCTION("aQaOGIVChN4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_69068E18854284DE);
    LIB_FUNCTION("b1nDsAsD4Fo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_6F59C3B00B03E05A);
    LIB_FUNCTION("ckzOf3ihNWs", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_724CCE7F78A1356B);
    LIB_FUNCTION("dQ8bBTwkMwg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_750F1B053C243308);
    LIB_FUNCTION("dg8Hm7kd4lg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_760F079BB91DE258);
    LIB_FUNCTION("eGV1IyIVVu8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_78657523221556EF);
    LIB_FUNCTION("gImIi9Nj7aY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8089888BD363EDA6);
    LIB_FUNCTION("gQygKbb3w6E", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_810CA029B6F7C3A1);
    LIB_FUNCTION("glO5Roao0-0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8253B94686A8D3FD);
    LIB_FUNCTION("hmUTinCeFlQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8665138A709E1654);
    LIB_FUNCTION("iC9I+uYJfAw", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_882F48FAE6097C0C);
    LIB_FUNCTION("idvkszA-+Ig", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_89DBE4B3303FF888);
    LIB_FUNCTION("i9PldiC93Dg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8BD3E57620BDDC38);
    LIB_FUNCTION("jwp0ATrWM+w", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8F0A74013AD633EC);
    LIB_FUNCTION("j6YmS-P2zAA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_8FA6264BF3F6CC00);
    LIB_FUNCTION("kpLofCwJceQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_9292E87C2C0971E4);
    LIB_FUNCTION("ksopIxjKA6g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_92CA292318CA03A8);
    LIB_FUNCTION("k0hZbCsX9mI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_9348596C2B17F662);
    LIB_FUNCTION("lQfpsyGl4Nc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_9507E9B321A5E0D7);
    LIB_FUNCTION("mMqV4jGYBzE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_98CA95E231980731);
    LIB_FUNCTION("nmbMS78sGZA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_9E66CC4BBF2C1990);
    LIB_FUNCTION("nmzvcGSJH4Q", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_9E6CEF7064891F84);
    LIB_FUNCTION("p7wseS6VIsU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_A7BC2C792E9522C5);
    LIB_FUNCTION("q7oPgJVIywI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_ABBA0F809548CB02);
    LIB_FUNCTION("sUonpM7eAg8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_B14A27A4CEDE020F);
    LIB_FUNCTION("syPuHCOrl-M", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_B323EE1C23AB97F3);
    LIB_FUNCTION("tCmBna70C6w", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_B429819DAEF40BAC);
    LIB_FUNCTION("tUuVcb6tgsU", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_B54B9571BEAD82C5);
    LIB_FUNCTION("tay1z0pBFKY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_B5ACB5CF4A4114A6);
    LIB_FUNCTION("ukG+D0QVfuQ", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_BA41BE0F44157EE4);
    LIB_FUNCTION("uqHeyEjZlpA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_BAA1DEC848D99690);
    LIB_FUNCTION("u4zM1slIDrI", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_BB8CCCD6C9480EB2);
    LIB_FUNCTION("vsJdqui4uB8", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_BEC25DAAE8B8B81F);
    LIB_FUNCTION("v+6TY5GrDHA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_BFEE936391AB0C70);
    LIB_FUNCTION("wN0tvi6mb3o", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_C0DD2DBE2EA66F7A);
    LIB_FUNCTION("wfhYv1uGwqE", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_C1F858BF5B86C2A1);
    LIB_FUNCTION("wkBhjm-DkgY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_C240618E6FC39206);
    LIB_FUNCTION("wzijRFAxDnk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_C338A34450310E79);
    LIB_FUNCTION("yR7jYD2WaQk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_C91EE3603D966909);
    LIB_FUNCTION("y2cDXtZoz2s", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_CB67035ED668CF6B);
    LIB_FUNCTION("1aWii3NRqb4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_D5A5A28B7351A9BE);
    LIB_FUNCTION("2oQmBZ8dWi0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_DA8426059F1D5A2D);
    LIB_FUNCTION("2o4V3QCvnfg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_DA8E15DD00AF9DF8);
    LIB_FUNCTION("24aYdkO7Xdc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_DB86987643BB5DD7);
    LIB_FUNCTION("3sU9cWXBN98", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_DEC53D7165C137DF);
    LIB_FUNCTION("397uJvLrlrM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_DFDEEE26F2EB96B3);
    LIB_FUNCTION("4gVqbwFkKGY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E2056A6F01642866);
    LIB_FUNCTION("4kDpuFl+5W4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E240E9B8597EE56E);
    LIB_FUNCTION("4yzjO3BvBfc", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E32CE33B706F05F7);
    LIB_FUNCTION("5PZ+-JHIT4c", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E4F67EFC91C84F87);
    LIB_FUNCTION("5vBBomYPg+s", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E6F041A2660F83EB);
    LIB_FUNCTION("6Xm6QTvYTTg", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_E979BA413BD84D38);
    LIB_FUNCTION("7d3y0wXbeGY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_EDDDF2D305DB7866);
    LIB_FUNCTION("8I7HcltC4vk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_F08EC7725B42E2F9);
    LIB_FUNCTION("81ldjv-ybsA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_F3595D8EFFF26EC0);
    LIB_FUNCTION("899ScRQvFV0", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_F3DF5271142F155D);
    LIB_FUNCTION("+JmXFo3Jh6g", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_F89997168DC987A8);
    LIB_FUNCTION("+RtbJcybMNk", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_F91B5B25CC9B30D9);
    LIB_FUNCTION("-DNbcQKlhbM", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_FC335B7102A585B3);
    LIB_FUNCTION("-OrDVMqLIG4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_FCEAC354CA8B206E);
    LIB_FUNCTION("-5ZuQ1HlZNY", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 Func_FF966E4351E564D6);
    LIB_FUNCTION("Ybu6AxV6S0o", "libSceNpManagerIsPlusMember", 1, "libSceNpManager", 1, 1,
                 sceNpIsPlusMember);
    LIB_FUNCTION("2rsFmlGWleQ", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpCheckNpAvailability);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountry);
    LIB_FUNCTION("8VBTeRf1ZwI", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountDateOfBirth);
    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountId);
    LIB_FUNCTION("KZ1Mj9yEGYc", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountLanguage);
    LIB_FUNCTION("IPb1hd1wAGc", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetGamePresenceStatus);
    LIB_FUNCTION("ilwLM4zOmu4", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetParentalControlInfo);
    LIB_FUNCTION("F6E4ycq9Dbg", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetUserIdByOnlineId);
    LIB_FUNCTION("Vh1bhUG6mSs", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessagePrepare);
    LIB_FUNCTION("ON7Sf5XEMmI", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpInGameMessageSendData);
    LIB_FUNCTION("uFJpaKNBAj4", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterGamePresenceCallback);
    LIB_FUNCTION("VfRSmPmj8Q8", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterStateCallback);
    LIB_FUNCTION("KO+11cgC7N0", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpSetGamePresenceOnline);
    LIB_FUNCTION("mjjTXh+NHWY", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterStateCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManagerForToolkit", 1, "libSceNpManager", 1, 1,
                 sceNpCheckCallbackForLib);
    LIB_FUNCTION("0c7HbXRKUt4", "libSceNpManagerForToolkit", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterStateCallbackForToolkit);
    LIB_FUNCTION("YIvqqvJyjEc", "libSceNpManagerForToolkit", 1, "libSceNpManager", 1, 1,
                 sceNpUnregisterStateCallbackForToolkit);
};

} // namespace Libraries::NpManager
