// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np_auth/np_auth.h"

namespace Libraries::NpAuth {

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCode() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthGetIdToken() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthCreateRequest() {
    LOG_WARNING(Lib_NpAuth, "(DUMMY) called");
    return 1;
}

s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 id) {
    LOG_WARNING(Lib_NpAuth, "(DUMMY) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeA() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeV3() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenA() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenV3() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthSetTimeout() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync() {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceNpAuth(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("KxGkOrQJTqY", "libSceNpAuthCompat", 1, "libSceNpAuth", 1, 1,
                 sceNpAuthGetAuthorizationCode);
    LIB_FUNCTION("uaB-LoJqHis", "libSceNpAuthCompat", 1, "libSceNpAuth", 1, 1, sceNpAuthGetIdToken);
    LIB_FUNCTION("cE7wIsqXdZ8", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthAbortRequest);
    LIB_FUNCTION("N+mr7GjTvr8", "libSceNpAuth", 1, "libSceNpAuth", 1, 1,
                 sceNpAuthCreateAsyncRequest);
    LIB_FUNCTION("6bwFkosYRQg", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthCreateRequest);
    LIB_FUNCTION("H8wG9Bk-nPc", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthDeleteRequest);
    LIB_FUNCTION("KxGkOrQJTqY", "libSceNpAuth", 1, "libSceNpAuth", 1, 1,
                 sceNpAuthGetAuthorizationCode);
    LIB_FUNCTION("qAUXQ9GdWp8", "libSceNpAuth", 1, "libSceNpAuth", 1, 1,
                 sceNpAuthGetAuthorizationCodeA);
    LIB_FUNCTION("KI4dHLlTNl0", "libSceNpAuth", 1, "libSceNpAuth", 1, 1,
                 sceNpAuthGetAuthorizationCodeV3);
    LIB_FUNCTION("uaB-LoJqHis", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthGetIdToken);
    LIB_FUNCTION("CocbHVIKPE8", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthGetIdTokenA);
    LIB_FUNCTION("RdsFVsgSpZY", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthGetIdTokenV3);
    LIB_FUNCTION("gjSyfzSsDcE", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthPollAsync);
    LIB_FUNCTION("PM3IZCw-7m0", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthSetTimeout);
    LIB_FUNCTION("SK-S7daqJSE", "libSceNpAuth", 1, "libSceNpAuth", 1, 1, sceNpAuthWaitAsync);
};

} // namespace Libraries::NpAuth