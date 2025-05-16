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
s32 PS4_SYSV_ABI sceCompanionHttpdAddHeader();
s32 PS4_SYSV_ABI sceCompanionHttpdGet2ndScreenStatus();
s32 PS4_SYSV_ABI sceCompanionHttpdGetEvent(OrbisCompanionHttpdEvent* pEvent);
s32 PS4_SYSV_ABI sceCompanionHttpdGetUserId();
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize2();
s32 PS4_SYSV_ABI sceCompanionHttpdOptParamInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestBodyReceptionCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestCallback2();
s32 PS4_SYSV_ABI sceCompanionHttpdSetBody();
s32 PS4_SYSV_ABI sceCompanionHttpdSetStatus();
s32 PS4_SYSV_ABI sceCompanionHttpdStart();
s32 PS4_SYSV_ABI sceCompanionHttpdStop();
s32 PS4_SYSV_ABI sceCompanionHttpdTerminate();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestCallback();

void RegisterlibSceCompanionHttpd(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CompanionHttpd