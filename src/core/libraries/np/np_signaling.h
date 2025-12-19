// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpSignaling {

s32 PS4_SYSV_ABI sceNpSignalingActivateConnection();
s32 PS4_SYSV_ABI sceNpSignalingActivateConnectionA();
s32 PS4_SYSV_ABI sceNpSignalingCancelPeerNetInfo();
s32 PS4_SYSV_ABI sceNpSignalingCreateContext(s32 param_1, void* param_2, void* param_3,
                                             s32* context_id);
s32 PS4_SYSV_ABI sceNpSignalingCreateContextA();
s32 PS4_SYSV_ABI sceNpSignalingDeactivateConnection();
s32 PS4_SYSV_ABI sceNpSignalingDeleteContext();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromNpId();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddress();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddressA();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfo();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfoA();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatistics();
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatus();
s32 PS4_SYSV_ABI sceNpSignalingGetContextOption();
s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo();
s32 PS4_SYSV_ABI sceNpSignalingGetMemoryInfo();
s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfo();
s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoA();
s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoResult();
s32 PS4_SYSV_ABI sceNpSignalingInitialize();
s32 PS4_SYSV_ABI sceNpSignalingSetContextOption();
s32 PS4_SYSV_ABI sceNpSignalingTerminate();
s32 PS4_SYSV_ABI sceNpSignalingTerminateConnection();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpSignaling