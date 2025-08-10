// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np_party/np_party.h"
#include "core/libraries/np_party/np_party_error.h"

namespace Libraries::NpParty {

s32 PS4_SYSV_ABI sceNpPartyCheckCallback() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyCreate() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyCreateA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetId() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetMemberInfo() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_NP_PARTY_ERROR_NOT_IN_PARTY;
}

s32 PS4_SYSV_ABI sceNpPartyGetMemberInfoA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_NP_PARTY_ERROR_NOT_IN_PARTY;
}

s32 PS4_SYSV_ABI sceNpPartyGetMembers() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_NP_PARTY_ERROR_NOT_IN_PARTY;
}

s32 PS4_SYSV_ABI sceNpPartyGetMembersA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_NP_PARTY_ERROR_NOT_IN_PARTY;
}

s32 PS4_SYSV_ABI sceNpPartyGetMemberSessionInfo() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetMemberVoiceInfo() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetState(OrbisNpPartyState* state) {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    *state = OrbisNpPartyState::NotInParty;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetStateAsUser() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetStateAsUserA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyGetVoiceChatPriority() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyInitialize() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyJoin() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyLeave() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyRegisterHandler() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyRegisterHandlerA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyRegisterPrivateHandler() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartySendBinaryMessage() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartySetVoiceChatPriority() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyShowInvitationList() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyShowInvitationListA() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyTerminate() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPartyUnregisterPrivateHandler() {
    LOG_ERROR(Lib_NpParty, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("3e4k2mzLkmc", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyCheckCallback);
    LIB_FUNCTION("nOZRy-slBoA", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyCreate);
    LIB_FUNCTION("XQSUbbnpPBA", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyCreateA);
    LIB_FUNCTION("DRA3ay-1DFQ", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyGetId);
    LIB_FUNCTION("F1P+-wpxQow", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyGetMemberInfo);
    LIB_FUNCTION("v2RYVGrJDkM", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetMemberInfoA);
    LIB_FUNCTION("T2UOKf00ZN0", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyGetMembers);
    LIB_FUNCTION("TaNw7W25QJw", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyGetMembersA);
    LIB_FUNCTION("4gOMfNYzllw", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetMemberSessionInfo);
    LIB_FUNCTION("EKi1jx59SP4", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetMemberVoiceInfo);
    LIB_FUNCTION("aEzKdJzATZ0", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyGetState);
    LIB_FUNCTION("o7grRhiGHYI", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetStateAsUser);
    LIB_FUNCTION("EjyAI+QNgFw", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetStateAsUserA);
    LIB_FUNCTION("-lc6XZnQXvM", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetVoiceChatPriority);
    LIB_FUNCTION("lhYCTQmBkds", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyInitialize);
    LIB_FUNCTION("RXNCDw2GDEg", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyJoin);
    LIB_FUNCTION("J8jAi-tfJHc", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyLeave);
    LIB_FUNCTION("kA88gbv71ao", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyRegisterHandler);
    LIB_FUNCTION("+v4fVHMwFWc", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyRegisterHandlerA);
    LIB_FUNCTION("zo4G5WWYpKg", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyRegisterPrivateHandler);
    LIB_FUNCTION("U6VdUe-PNAY", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartySendBinaryMessage);
    LIB_FUNCTION("nazKyHygHhY", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartySetVoiceChatPriority);
    LIB_FUNCTION("-MFiL7hEnPE", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyShowInvitationList);
    LIB_FUNCTION("yARHEYLajs0", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyShowInvitationListA);
    LIB_FUNCTION("oLYkibiHqRA", "libSceNpParty", 1, "libSceNpParty", 1, 1, sceNpPartyTerminate);
    LIB_FUNCTION("zQ7gIvt11Pc", "libSceNpParty", 1, "libSceNpParty", 1, 1,
                 sceNpPartyUnregisterPrivateHandler);
    LIB_FUNCTION("nOZRy-slBoA", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1, sceNpPartyCreate);
    LIB_FUNCTION("F1P+-wpxQow", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetMemberInfo);
    LIB_FUNCTION("T2UOKf00ZN0", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetMembers);
    LIB_FUNCTION("o7grRhiGHYI", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1,
                 sceNpPartyGetStateAsUser);
    LIB_FUNCTION("kA88gbv71ao", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1,
                 sceNpPartyRegisterHandler);
    LIB_FUNCTION("-MFiL7hEnPE", "libSceNpPartyCompat", 1, "libSceNpParty", 1, 1,
                 sceNpPartyShowInvitationList);
};

} // namespace Libraries::NpParty