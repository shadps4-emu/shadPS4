// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::CompanionHttpd {

// OrbisCompanionHttpdEvent event codes
constexpr int ORBIS_COMPANION_HTTPD_EVENT_CONNECT = 0x10000001;
constexpr int ORBIS_COMPANION_HTTPD_EVENT_DISCONNECT = 0x10000002;

struct OrbisCompanionHttpdHeader {
    char* key;
    char* value;
    struct OrbisCompanionHttpdHeader* header;
};

struct OrbisCompanionHttpdRequest {
    s32 method;
    char* url;
    OrbisCompanionHttpdHeader* header;
    char* body;
    u64 bodySize;
};

struct OrbisCompanionHttpdResponse {
    s32 status;
    OrbisCompanionHttpdHeader* header;
    char* body;
    u64 bodySize;
};

using OrbisCompanionHttpdRequestBodyReceptionCallback =
    PS4_SYSV_ABI s32 (*)(s32 event, Libraries::UserService::OrbisUserServiceUserId userId,
                         const OrbisCompanionHttpdRequest* httpRequest, void* param);

using OrbisCompanionHttpdRequestCallback =
    PS4_SYSV_ABI s32 (*)(Libraries::UserService::OrbisUserServiceUserId userId,
                         const OrbisCompanionHttpdRequest* httpRequest,
                         OrbisCompanionHttpdResponse* httpResponse, void* param);

struct OrbisCompanionUtilDeviceInfo {
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Net::OrbisNetSockaddrIn addr;
    char reserved[236];
};

struct OrbisCompanionHttpdEvent {
    s32 event;
    union {
        OrbisCompanionUtilDeviceInfo deviceInfo;
        Libraries::UserService::OrbisUserServiceUserId userId;
        char reserved[256];
    } data;
};

s32 PS4_SYSV_ABI sceCompanionHttpdAddHeader(const char* key, const char* value,
                                            OrbisCompanionHttpdResponse* response);
s32 PS4_SYSV_ABI
sceCompanionHttpdGet2ndScreenStatus(Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceCompanionHttpdGetEvent(OrbisCompanionHttpdEvent* pEvent);
s32 PS4_SYSV_ABI sceCompanionHttpdGetUserId(u32 addr,
                                            Libraries::UserService::OrbisUserServiceUserId* userId);
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize2();
s32 PS4_SYSV_ABI sceCompanionHttpdOptParamInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestBodyReceptionCallback(
    OrbisCompanionHttpdRequestBodyReceptionCallback function, void* param);
s32 PS4_SYSV_ABI
sceCompanionHttpdRegisterRequestCallback(OrbisCompanionHttpdRequestCallback function, void* param);
s32 PS4_SYSV_ABI
sceCompanionHttpdRegisterRequestCallback2(OrbisCompanionHttpdRequestCallback function, void* param);
s32 PS4_SYSV_ABI sceCompanionHttpdSetBody(const char* body, u64 bodySize,
                                          OrbisCompanionHttpdResponse* response);
s32 PS4_SYSV_ABI sceCompanionHttpdSetStatus(s32 status, OrbisCompanionHttpdResponse* response);
s32 PS4_SYSV_ABI sceCompanionHttpdStart();
s32 PS4_SYSV_ABI sceCompanionHttpdStop();
s32 PS4_SYSV_ABI sceCompanionHttpdTerminate();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestCallback();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CompanionHttpd