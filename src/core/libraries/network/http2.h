// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Http2 {

int PS4_SYSV_ABI _Z5dummyv();
int PS4_SYSV_ABI sceHttp2AbortRequest();
int PS4_SYSV_ABI sceHttp2AddCookie();
int PS4_SYSV_ABI sceHttp2AddRequestHeader();
int PS4_SYSV_ABI sceHttp2AuthCacheFlush();
int PS4_SYSV_ABI sceHttp2CookieExport();
int PS4_SYSV_ABI sceHttp2CookieFlush();
int PS4_SYSV_ABI sceHttp2CookieImport();
int PS4_SYSV_ABI sceHttp2CreateCookieBox();
int PS4_SYSV_ABI sceHttp2CreateRequestWithURL();
int PS4_SYSV_ABI sceHttp2CreateTemplate();
int PS4_SYSV_ABI sceHttp2DeleteCookieBox();
int PS4_SYSV_ABI sceHttp2DeleteRequest();
int PS4_SYSV_ABI sceHttp2DeleteTemplate();
int PS4_SYSV_ABI sceHttp2GetAllResponseHeaders();
int PS4_SYSV_ABI sceHttp2GetAuthEnabled();
int PS4_SYSV_ABI sceHttp2GetAutoRedirect();
int PS4_SYSV_ABI sceHttp2GetCookie();
int PS4_SYSV_ABI sceHttp2GetCookieBox();
int PS4_SYSV_ABI sceHttp2GetCookieStats();
int PS4_SYSV_ABI sceHttp2GetMemoryPoolStats();
int PS4_SYSV_ABI sceHttp2GetResponseContentLength();
int PS4_SYSV_ABI sceHttp2GetStatusCode();
int PS4_SYSV_ABI sceHttp2Init(int net_id, int ssl_id, size_t pool_size, int max_requests);
int PS4_SYSV_ABI sceHttp2ReadData();
int PS4_SYSV_ABI sceHttp2ReadDataAsync();
int PS4_SYSV_ABI sceHttp2RedirectCacheFlush();
int PS4_SYSV_ABI sceHttp2RemoveRequestHeader();
int PS4_SYSV_ABI sceHttp2SendRequest();
int PS4_SYSV_ABI sceHttp2SendRequestAsync();
int PS4_SYSV_ABI sceHttp2SetAuthEnabled();
int PS4_SYSV_ABI sceHttp2SetAuthInfoCallback();
int PS4_SYSV_ABI sceHttp2SetAutoRedirect();
int PS4_SYSV_ABI sceHttp2SetConnectionWaitTimeOut();
int PS4_SYSV_ABI sceHttp2SetConnectTimeOut();
int PS4_SYSV_ABI sceHttp2SetCookieBox();
int PS4_SYSV_ABI sceHttp2SetCookieMaxNum();
int PS4_SYSV_ABI sceHttp2SetCookieMaxNumPerDomain();
int PS4_SYSV_ABI sceHttp2SetCookieMaxSize();
int PS4_SYSV_ABI sceHttp2SetCookieRecvCallback();
int PS4_SYSV_ABI sceHttp2SetCookieSendCallback();
int PS4_SYSV_ABI sceHttp2SetInflateGZIPEnabled();
int PS4_SYSV_ABI sceHttp2SetMinSslVersion();
int PS4_SYSV_ABI sceHttp2SetPreSendCallback();
int PS4_SYSV_ABI sceHttp2SetRecvTimeOut();
int PS4_SYSV_ABI sceHttp2SetRedirectCallback();
int PS4_SYSV_ABI sceHttp2SetRequestContentLength();
int PS4_SYSV_ABI sceHttp2SetResolveRetry();
int PS4_SYSV_ABI sceHttp2SetResolveTimeOut();
int PS4_SYSV_ABI sceHttp2SetSendTimeOut();
int PS4_SYSV_ABI sceHttp2SetSslCallback();
int PS4_SYSV_ABI sceHttp2SetTimeOut();
int PS4_SYSV_ABI sceHttp2SslDisableOption();
int PS4_SYSV_ABI sceHttp2SslEnableOption();
int PS4_SYSV_ABI sceHttp2Term();
int PS4_SYSV_ABI sceHttp2WaitAsync();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http2