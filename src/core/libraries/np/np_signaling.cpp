// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_signaling.h"

namespace Libraries::Np::NpSignaling {

s32 PS4_SYSV_ABI sceNpSignalingActivateConnection() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingActivateConnectionA() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCancelPeerNetInfo() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContext(s32 param_1, void* param_2, void* param_3,
                                             s32* context_id) {

    static s32 context_id_counter = 0;
    *context_id = ++context_id_counter;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContextA() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingDeactivateConnection() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingDeleteContext() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromNpId() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddress() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddressA() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfo() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfoA() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatistics() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatus() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetContextOption() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetMemoryInfo() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfo() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoA() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoResult() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingInitialize() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingSetContextOption() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingTerminate() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingTerminateConnection() {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0UvTFeomAUM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnection);
    LIB_FUNCTION("ZPLavCKqAB0", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingActivateConnectionA);
    LIB_FUNCTION("X1G4kkN2R-8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCancelPeerNetInfo);
    LIB_FUNCTION("5yYjEdd4t8Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContext);
    LIB_FUNCTION("dDLNFdY8dws", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContextA);
    LIB_FUNCTION("6UEembipgrM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeactivateConnection);
    LIB_FUNCTION("hx+LIg-1koI", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingDeleteContext);
    LIB_FUNCTION("GQ0hqmzj0F4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromNpId);
    LIB_FUNCTION("CkPxQjSm018", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddress);
    LIB_FUNCTION("B7cT9aVby7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionFromPeerAddressA);
    LIB_FUNCTION("AN3h0EBSX7A", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfo);
    LIB_FUNCTION("rcylknsUDwg", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionInfoA);
    LIB_FUNCTION("C6ZNCDTj00Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatistics);
    LIB_FUNCTION("bD-JizUb3JM", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetConnectionStatus);
    LIB_FUNCTION("npU5V56id34", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetContextOption);
    LIB_FUNCTION("U8AQMlOFBc8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetLocalNetInfo);
    LIB_FUNCTION("tOpqyDyMje4", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetMemoryInfo);
    LIB_FUNCTION("zFgFHId7vAE", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfo);
    LIB_FUNCTION("Shr7bZq8QHY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoA);
    LIB_FUNCTION("2HajCEGgG4s", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetPeerNetInfoResult);
    LIB_FUNCTION("3KOuC4RmZZU", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingInitialize);
    LIB_FUNCTION("IHRDvZodPYY", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingSetContextOption);
    LIB_FUNCTION("NPhw0UXaNrk", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminate);
    LIB_FUNCTION("b4qaXPzMJxo", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingTerminateConnection);
};

} // namespace Libraries::Np::NpSignaling