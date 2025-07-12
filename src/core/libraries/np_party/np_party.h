// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NpParty {

s32 PS4_SYSV_ABI sceNpPartyCheckCallback();
s32 PS4_SYSV_ABI sceNpPartyCreate();
s32 PS4_SYSV_ABI sceNpPartyCreateA();
s32 PS4_SYSV_ABI sceNpPartyGetId();
s32 PS4_SYSV_ABI sceNpPartyGetMemberInfo();
s32 PS4_SYSV_ABI sceNpPartyGetMemberInfoA();
s32 PS4_SYSV_ABI sceNpPartyGetMembers();
s32 PS4_SYSV_ABI sceNpPartyGetMembersA();
s32 PS4_SYSV_ABI sceNpPartyGetMemberSessionInfo();
s32 PS4_SYSV_ABI sceNpPartyGetMemberVoiceInfo();
s32 PS4_SYSV_ABI sceNpPartyGetState();
s32 PS4_SYSV_ABI sceNpPartyGetStateAsUser();
s32 PS4_SYSV_ABI sceNpPartyGetStateAsUserA();
s32 PS4_SYSV_ABI sceNpPartyGetVoiceChatPriority();
s32 PS4_SYSV_ABI sceNpPartyInitialize();
s32 PS4_SYSV_ABI sceNpPartyJoin();
s32 PS4_SYSV_ABI sceNpPartyLeave();
s32 PS4_SYSV_ABI sceNpPartyRegisterHandler();
s32 PS4_SYSV_ABI sceNpPartyRegisterHandlerA();
s32 PS4_SYSV_ABI sceNpPartyRegisterPrivateHandler();
s32 PS4_SYSV_ABI sceNpPartySendBinaryMessage();
s32 PS4_SYSV_ABI sceNpPartySetVoiceChatPriority();
s32 PS4_SYSV_ABI sceNpPartyShowInvitationList();
s32 PS4_SYSV_ABI sceNpPartyShowInvitationListA();
s32 PS4_SYSV_ABI sceNpPartyTerminate();
s32 PS4_SYSV_ABI sceNpPartyUnregisterPrivateHandler();
s32 PS4_SYSV_ABI module_start();
s32 PS4_SYSV_ABI module_stop();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::NpParty