// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpWebApi {

s32 PS4_SYSV_ABI sceNpWebApiCreateContext();
s32 PS4_SYSV_ABI sceNpWebApiCreatePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiCreateServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiDeletePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiDeleteServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterNotificationCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterNotificationCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiAbortHandle();
s32 PS4_SYSV_ABI sceNpWebApiAbortRequest();
s32 PS4_SYSV_ABI sceNpWebApiAddHttpRequestHeader();
s32 PS4_SYSV_ABI sceNpWebApiAddMultipartPart();
s32 PS4_SYSV_ABI sceNpWebApiCheckTimeout();
s32 PS4_SYSV_ABI sceNpWebApiClearAllUnusedConnection();
s32 PS4_SYSV_ABI sceNpWebApiClearUnusedConnection();
s32 PS4_SYSV_ABI sceNpWebApiCreateContextA();
s32 PS4_SYSV_ABI sceNpWebApiCreateExtdPushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiCreateHandle();
s32 PS4_SYSV_ABI sceNpWebApiCreateMultipartRequest();
s32 PS4_SYSV_ABI sceNpWebApiCreateRequest();
s32 PS4_SYSV_ABI sceNpWebApiDeleteContext();
s32 PS4_SYSV_ABI sceNpWebApiDeleteExtdPushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiDeleteHandle();
s32 PS4_SYSV_ABI sceNpWebApiDeleteRequest();
s32 PS4_SYSV_ABI sceNpWebApiGetConnectionStats();
s32 PS4_SYSV_ABI sceNpWebApiGetErrorCode();
s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValue();
s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValueLength();
s32 PS4_SYSV_ABI sceNpWebApiGetHttpStatusCode();
s32 PS4_SYSV_ABI sceNpWebApiGetMemoryPoolStats();
s32 PS4_SYSV_ABI sceNpWebApiInitialize();
s32 PS4_SYSV_ABI sceNpWebApiInitializeForPresence();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateCtxIndExtdPushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateRequest();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiIntInitialize();
s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallbackA();
s32 PS4_SYSV_ABI sceNpWebApiReadData();
s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallbackA();
s32 PS4_SYSV_ABI sceNpWebApiSendMultipartRequest();
s32 PS4_SYSV_ABI sceNpWebApiSendMultipartRequest2();
s32 PS4_SYSV_ABI sceNpWebApiSendRequest();
s32 PS4_SYSV_ABI sceNpWebApiSendRequest2();
s32 PS4_SYSV_ABI sceNpWebApiSetHandleTimeout();
s32 PS4_SYSV_ABI sceNpWebApiSetMaxConnection();
s32 PS4_SYSV_ABI sceNpWebApiSetMultipartContentType();
s32 PS4_SYSV_ABI sceNpWebApiSetRequestTimeout();
s32 PS4_SYSV_ABI sceNpWebApiTerminate();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterExtdPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiUtilityParseNpId();
s32 PS4_SYSV_ABI sceNpWebApiVshInitialize();
s32 PS4_SYSV_ABI Func_064C4ED1EDBEB9E8();
s32 PS4_SYSV_ABI Func_0783955D4E9563DA();
s32 PS4_SYSV_ABI Func_1A6D77F3FD8323A8();
s32 PS4_SYSV_ABI Func_1E0693A26FE0F954();
s32 PS4_SYSV_ABI Func_24A9B5F1D77000CF();
s32 PS4_SYSV_ABI Func_24AAA6F50E4C2361();
s32 PS4_SYSV_ABI Func_24D8853D6B47FC79();
s32 PS4_SYSV_ABI Func_279B3E9C7C4A9DC5();
s32 PS4_SYSV_ABI Func_28461E29E9F8D697();
s32 PS4_SYSV_ABI Func_3C29624704FAB9E0();
s32 PS4_SYSV_ABI Func_3F027804ED2EC11E();
s32 PS4_SYSV_ABI Func_4066C94E782997CD();
s32 PS4_SYSV_ABI Func_47C85356815DBE90();
s32 PS4_SYSV_ABI Func_4FCE8065437E3B87();
s32 PS4_SYSV_ABI Func_536280BE3DABB521();
s32 PS4_SYSV_ABI Func_57A0E1BC724219F3();
s32 PS4_SYSV_ABI Func_5819749C040B6637();
s32 PS4_SYSV_ABI Func_6198D0C825E86319();
s32 PS4_SYSV_ABI Func_61F2B9E8AB093743();
s32 PS4_SYSV_ABI Func_6BC388E6113F0D44();
s32 PS4_SYSV_ABI Func_7500F0C4F8DC2D16();
s32 PS4_SYSV_ABI Func_75A03814C7E9039F();
s32 PS4_SYSV_ABI Func_789D6026C521416E();
s32 PS4_SYSV_ABI Func_7DED63D06399EFFF();
s32 PS4_SYSV_ABI Func_7E55A2DCC03D395A();
s32 PS4_SYSV_ABI Func_7E6C8F9FB86967F4();
s32 PS4_SYSV_ABI Func_7F04B7D4A7D41E80();
s32 PS4_SYSV_ABI Func_8E167252DFA5C957();
s32 PS4_SYSV_ABI Func_95D0046E504E3B09();
s32 PS4_SYSV_ABI Func_97284BFDA4F18FDF();
s32 PS4_SYSV_ABI Func_99E32C1F4737EAB4();
s32 PS4_SYSV_ABI Func_9CFF661EA0BCBF83();
s32 PS4_SYSV_ABI Func_9EB0E1F467AC3B29();
s32 PS4_SYSV_ABI Func_A2318FE6FBABFAA3();
s32 PS4_SYSV_ABI Func_BA07A2E1BF7B3971();
s32 PS4_SYSV_ABI Func_BD0803EEE0CC29A0();
s32 PS4_SYSV_ABI Func_BE6F4E5524BB135F();
s32 PS4_SYSV_ABI Func_C0D490EB481EA4D0();
s32 PS4_SYSV_ABI Func_C175D392CA6D084A();
s32 PS4_SYSV_ABI Func_CD0136AF165D2F2F();
s32 PS4_SYSV_ABI Func_D1C0ADB7B52FEAB5();
s32 PS4_SYSV_ABI Func_E324765D18EE4D12();
s32 PS4_SYSV_ABI Func_E789F980D907B653();
s32 PS4_SYSV_ABI Func_F9A32E8685627436();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpWebApi