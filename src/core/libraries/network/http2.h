// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/ssl2.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Http2 {

enum OrbisHttp2HttpVersion : u32 {
    ORBIS_HTTP2_VERSION_1_0 = 1,
    ORBIS_HTTP2_VERSION_1_1 = 2,
    ORBIS_HTTP2_VERSION_2_0 = 3,
};

// TODO: Figure out the whole struct. What's here is based on libSceNpWebApi2 use.
struct OrbisHttp2PreSendCallbackData {
    void* unk;
    void* cert;
};
using OrbisHttp2PreSendCallback = PS4_SYSV_ABI s32 (*)(s32 request_id, s32 ssl_id,
                                                       OrbisHttp2PreSendCallbackData* data,
                                                       void* user_arg);

s32 PS4_SYSV_ABI sceHttp2AbortRequest();
s32 PS4_SYSV_ABI sceHttp2AddCookie();
s32 PS4_SYSV_ABI sceHttp2AddRequestHeader();
s32 PS4_SYSV_ABI sceHttp2AuthCacheFlush();
s32 PS4_SYSV_ABI sceHttp2CookieExport();
s32 PS4_SYSV_ABI sceHttp2CookieFlush();
s32 PS4_SYSV_ABI sceHttp2CookieImport();
s32 PS4_SYSV_ABI sceHttp2CreateCookieBox();
s32 PS4_SYSV_ABI sceHttp2CreateRequestWithURL();
s32 PS4_SYSV_ABI sceHttp2CreateTemplate(s32 ctx_id, char* user_agent, s32 http_ver,
                                        s32 auto_proxy_conf);
s32 PS4_SYSV_ABI sceHttp2DeleteCookieBox();
s32 PS4_SYSV_ABI sceHttp2DeleteRequest();
s32 PS4_SYSV_ABI sceHttp2DeleteTemplate();
s32 PS4_SYSV_ABI sceHttp2GetAllResponseHeaders();
s32 PS4_SYSV_ABI sceHttp2GetAuthEnabled();
s32 PS4_SYSV_ABI sceHttp2GetAutoRedirect();
s32 PS4_SYSV_ABI sceHttp2GetCookie();
s32 PS4_SYSV_ABI sceHttp2GetCookieBox();
s32 PS4_SYSV_ABI sceHttp2GetCookieStats();
s32 PS4_SYSV_ABI sceHttp2GetMemoryPoolStats();
s32 PS4_SYSV_ABI sceHttp2GetResponseContentLength();
s32 PS4_SYSV_ABI sceHttp2GetStatusCode();
s32 PS4_SYSV_ABI sceHttp2Init(s32 net_id, s32 ssl_id, u64 pool_size, s32 max_requests);
s32 PS4_SYSV_ABI sceHttp2ReadData();
s32 PS4_SYSV_ABI sceHttp2ReadDataAsync();
s32 PS4_SYSV_ABI sceHttp2RedirectCacheFlush();
s32 PS4_SYSV_ABI sceHttp2RemoveRequestHeader();
s32 PS4_SYSV_ABI sceHttp2SendRequest();
s32 PS4_SYSV_ABI sceHttp2SendRequestAsync();
s32 PS4_SYSV_ABI sceHttp2SetAuthEnabled();
s32 PS4_SYSV_ABI sceHttp2SetAuthInfoCallback();
s32 PS4_SYSV_ABI sceHttp2SetAutoRedirect();
s32 PS4_SYSV_ABI sceHttp2SetConnectionWaitTimeOut();
s32 PS4_SYSV_ABI sceHttp2SetConnectTimeOut();
s32 PS4_SYSV_ABI sceHttp2SetCookieBox();
s32 PS4_SYSV_ABI sceHttp2SetCookieMaxNum();
s32 PS4_SYSV_ABI sceHttp2SetCookieMaxNumPerDomain();
s32 PS4_SYSV_ABI sceHttp2SetCookieMaxSize();
s32 PS4_SYSV_ABI sceHttp2SetCookieRecvCallback();
s32 PS4_SYSV_ABI sceHttp2SetCookieSendCallback();
s32 PS4_SYSV_ABI sceHttp2SetInflateGZIPEnabled();
s32 PS4_SYSV_ABI sceHttp2SetMinSslVersion();
s32 PS4_SYSV_ABI sceHttp2SetPreSendCallback(s32 template_id, OrbisHttp2PreSendCallback cb_func,
                                            void* user_arg);
s32 PS4_SYSV_ABI sceHttp2SetRecvTimeOut();
s32 PS4_SYSV_ABI sceHttp2SetRedirectCallback();
s32 PS4_SYSV_ABI sceHttp2SetRequestContentLength();
s32 PS4_SYSV_ABI sceHttp2SetResolveRetry();
s32 PS4_SYSV_ABI sceHttp2SetResolveTimeOut();
s32 PS4_SYSV_ABI sceHttp2SetSendTimeOut();
s32 PS4_SYSV_ABI sceHttp2SetSslCallback();
s32 PS4_SYSV_ABI sceHttp2SetTimeOut();
s32 PS4_SYSV_ABI sceHttp2SslDisableOption();
s32 PS4_SYSV_ABI sceHttp2SslEnableOption();
s32 PS4_SYSV_ABI sceHttp2Term();
s32 PS4_SYSV_ABI sceHttp2WaitAsync();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http2