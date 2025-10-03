// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "companion_error.h"
#include "core/libraries/companion/companion_httpd.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::CompanionHttpd {

s32 PS4_SYSV_ABI sceCompanionHttpdAddHeader(const char* key, const char* value,
                                            OrbisCompanionHttpdResponse* response) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceCompanionHttpdGet2ndScreenStatus(Libraries::UserService::OrbisUserServiceUserId) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdGetEvent(OrbisCompanionHttpdEvent* pEvent) {
    pEvent->event = ORBIS_COMPANION_HTTPD_EVENT_DISCONNECT; // disconnected
    LOG_DEBUG(Lib_CompanionHttpd, "device disconnected");
    return ORBIS_COMPANION_HTTPD_ERROR_NO_EVENT; // No events to obtain
}

s32 PS4_SYSV_ABI
sceCompanionHttpdGetUserId(u32 addr, Libraries::UserService::OrbisUserServiceUserId* userId) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdInitialize() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdInitialize2() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdOptParamInitialize() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestBodyReceptionCallback(
    OrbisCompanionHttpdRequestBodyReceptionCallback function, void* param) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceCompanionHttpdRegisterRequestCallback(OrbisCompanionHttpdRequestCallback function, void* param) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestCallback2(
    OrbisCompanionHttpdRequestCallback function, void* param) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdSetBody(const char* body, u64 bodySize,
                                          OrbisCompanionHttpdResponse* response) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdSetStatus(s32 status, OrbisCompanionHttpdResponse* response) {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdStart() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdStop() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdTerminate() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestCallback() {
    LOG_ERROR(Lib_CompanionHttpd, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("8pWltDG7h6A", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdAddHeader);
    LIB_FUNCTION("B-QBMeFdNgY", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdGet2ndScreenStatus);
    LIB_FUNCTION("Vku4big+IYM", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdGetEvent);
    LIB_FUNCTION("0SySxcuVNG0", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdGetUserId);
    LIB_FUNCTION("ykNpWs3ktLY", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdInitialize);
    LIB_FUNCTION("OA6FbORefbo", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdInitialize2);
    LIB_FUNCTION("r-2-a0c7Kfc", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdOptParamInitialize);
    LIB_FUNCTION("fHNmij7kAUM", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdRegisterRequestBodyReceptionCallback);
    LIB_FUNCTION("OaWw+IVEdbI", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdRegisterRequestCallback);
    LIB_FUNCTION("-0c9TCTwnGs", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdRegisterRequestCallback2);
    LIB_FUNCTION("h3OvVxzX4qM", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdSetBody);
    LIB_FUNCTION("w7oz0AWHpT4", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdSetStatus);
    LIB_FUNCTION("k7F0FcDM-Xc", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdStart);
    LIB_FUNCTION("0SCgzfVQHpo", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdStop);
    LIB_FUNCTION("+-du9tWgE9s", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdTerminate);
    LIB_FUNCTION("ZSHiUfYK+QI", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdUnregisterRequestBodyReceptionCallback);
    LIB_FUNCTION("xweOi2QT-BE", "libSceCompanionHttpd", 1, "libSceCompanionHttpd",
                 sceCompanionHttpdUnregisterRequestCallback);
};

} // namespace Libraries::CompanionHttpd