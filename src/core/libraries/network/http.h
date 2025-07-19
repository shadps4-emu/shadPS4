// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Http {

struct OrbisHttpUriElement {
    bool opaque;
    char* scheme;
    char* username;
    char* password;
    char* hostname;
    char* path;
    char* query;
    char* fragment;
    u16 port;
    u8 reserved[10];
};

int PS4_SYSV_ABI sceHttpAbortRequest();
int PS4_SYSV_ABI sceHttpAbortRequestForce();
int PS4_SYSV_ABI sceHttpAbortWaitRequest();
int PS4_SYSV_ABI sceHttpAddCookie();
int PS4_SYSV_ABI sceHttpAddQuery();
int PS4_SYSV_ABI sceHttpAddRequestHeader();
int PS4_SYSV_ABI sceHttpAddRequestHeaderRaw();
int PS4_SYSV_ABI sceHttpAuthCacheExport();
int PS4_SYSV_ABI sceHttpAuthCacheFlush();
int PS4_SYSV_ABI sceHttpAuthCacheImport();
int PS4_SYSV_ABI sceHttpCacheRedirectedConnectionEnabled();
int PS4_SYSV_ABI sceHttpCookieExport();
int PS4_SYSV_ABI sceHttpCookieFlush();
int PS4_SYSV_ABI sceHttpCookieImport();
int PS4_SYSV_ABI sceHttpCreateConnection();
int PS4_SYSV_ABI sceHttpCreateConnectionWithURL();
int PS4_SYSV_ABI sceHttpCreateEpoll();
int PS4_SYSV_ABI sceHttpCreateRequest();
int PS4_SYSV_ABI sceHttpCreateRequest2();
int PS4_SYSV_ABI sceHttpCreateRequestWithURL();
int PS4_SYSV_ABI sceHttpCreateRequestWithURL2();
int PS4_SYSV_ABI sceHttpCreateTemplate();
int PS4_SYSV_ABI sceHttpDbgEnableProfile();
int PS4_SYSV_ABI sceHttpDbgGetConnectionStat();
int PS4_SYSV_ABI sceHttpDbgGetRequestStat();
int PS4_SYSV_ABI sceHttpDbgSetPrintf();
int PS4_SYSV_ABI sceHttpDbgShowConnectionStat();
int PS4_SYSV_ABI sceHttpDbgShowMemoryPoolStat();
int PS4_SYSV_ABI sceHttpDbgShowRequestStat();
int PS4_SYSV_ABI sceHttpDbgShowStat();
int PS4_SYSV_ABI sceHttpDeleteConnection();
int PS4_SYSV_ABI sceHttpDeleteRequest();
int PS4_SYSV_ABI sceHttpDeleteTemplate();
int PS4_SYSV_ABI sceHttpDestroyEpoll();
int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled();
int PS4_SYSV_ABI sceHttpGetAllResponseHeaders();
int PS4_SYSV_ABI sceHttpGetAuthEnabled();
int PS4_SYSV_ABI sceHttpGetAutoRedirect();
int PS4_SYSV_ABI sceHttpGetConnectionStat();
int PS4_SYSV_ABI sceHttpGetCookie();
int PS4_SYSV_ABI sceHttpGetCookieEnabled();
int PS4_SYSV_ABI sceHttpGetCookieStats();
int PS4_SYSV_ABI sceHttpGetEpoll();
int PS4_SYSV_ABI sceHttpGetEpollId();
int PS4_SYSV_ABI sceHttpGetLastErrno();
int PS4_SYSV_ABI sceHttpGetMemoryPoolStats();
int PS4_SYSV_ABI sceHttpGetNonblock();
int PS4_SYSV_ABI sceHttpGetRegisteredCtxIds();
int PS4_SYSV_ABI sceHttpGetResponseContentLength();
int PS4_SYSV_ABI sceHttpGetStatusCode();
int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, std::size_t poolSize);
int PS4_SYSV_ABI sceHttpParseResponseHeader();
int PS4_SYSV_ABI sceHttpParseStatusLine();
int PS4_SYSV_ABI sceHttpReadData();
int PS4_SYSV_ABI sceHttpRedirectCacheFlush();
int PS4_SYSV_ABI sceHttpRemoveRequestHeader();
int PS4_SYSV_ABI sceHttpRequestGetAllHeaders();
int PS4_SYSV_ABI sceHttpsDisableOption();
int PS4_SYSV_ABI sceHttpsDisableOptionPrivate();
int PS4_SYSV_ABI sceHttpsEnableOption();
int PS4_SYSV_ABI sceHttpsEnableOptionPrivate();
int PS4_SYSV_ABI sceHttpSendRequest();
int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled();
int PS4_SYSV_ABI sceHttpSetAuthEnabled();
int PS4_SYSV_ABI sceHttpSetAuthInfoCallback();
int PS4_SYSV_ABI sceHttpSetAutoRedirect();
int PS4_SYSV_ABI sceHttpSetChunkedTransferEnabled();
int PS4_SYSV_ABI sceHttpSetConnectTimeOut();
int PS4_SYSV_ABI sceHttpSetCookieEnabled();
int PS4_SYSV_ABI sceHttpSetCookieMaxNum();
int PS4_SYSV_ABI sceHttpSetCookieMaxNumPerDomain();
int PS4_SYSV_ABI sceHttpSetCookieMaxSize();
int PS4_SYSV_ABI sceHttpSetCookieRecvCallback();
int PS4_SYSV_ABI sceHttpSetCookieSendCallback();
int PS4_SYSV_ABI sceHttpSetCookieTotalMaxSize();
int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled();
int PS4_SYSV_ABI sceHttpSetDelayBuildRequestEnabled();
int PS4_SYSV_ABI sceHttpSetEpoll();
int PS4_SYSV_ABI sceHttpSetEpollId();
int PS4_SYSV_ABI sceHttpSetHttp09Enabled();
int PS4_SYSV_ABI sceHttpSetInflateGZIPEnabled();
int PS4_SYSV_ABI sceHttpSetNonblock();
int PS4_SYSV_ABI sceHttpSetPolicyOption();
int PS4_SYSV_ABI sceHttpSetPriorityOption();
int PS4_SYSV_ABI sceHttpSetProxy();
int PS4_SYSV_ABI sceHttpSetRecvBlockSize();
int PS4_SYSV_ABI sceHttpSetRecvTimeOut();
int PS4_SYSV_ABI sceHttpSetRedirectCallback();
int PS4_SYSV_ABI sceHttpSetRequestContentLength();
int PS4_SYSV_ABI sceHttpSetRequestStatusCallback();
int PS4_SYSV_ABI sceHttpSetResolveRetry();
int PS4_SYSV_ABI sceHttpSetResolveTimeOut();
int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize();
int PS4_SYSV_ABI sceHttpSetSendTimeOut();
int PS4_SYSV_ABI sceHttpSetSocketCreationCallback();
int PS4_SYSV_ABI sceHttpsFreeCaList();
int PS4_SYSV_ABI sceHttpsGetCaList();
int PS4_SYSV_ABI sceHttpsGetSslError();
int PS4_SYSV_ABI sceHttpsLoadCert();
int PS4_SYSV_ABI sceHttpsSetMinSslVersion();
int PS4_SYSV_ABI sceHttpsSetSslCallback();
int PS4_SYSV_ABI sceHttpsSetSslVersion();
int PS4_SYSV_ABI sceHttpsUnloadCert();
int PS4_SYSV_ABI sceHttpTerm();
int PS4_SYSV_ABI sceHttpTryGetNonblock();
int PS4_SYSV_ABI sceHttpTrySetNonblock();
int PS4_SYSV_ABI sceHttpUnsetEpoll();
int PS4_SYSV_ABI sceHttpUriBuild();
int PS4_SYSV_ABI sceHttpUriCopy();
int PS4_SYSV_ABI sceHttpUriEscape();
int PS4_SYSV_ABI sceHttpUriMerge();
int PS4_SYSV_ABI sceHttpUriParse(OrbisHttpUriElement* out, const char* srcUri, void* pool,
                                 size_t* require, size_t prepare);
int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, size_t srcSize);
int PS4_SYSV_ABI sceHttpUriUnescape(char* out, size_t* require, size_t prepare, const char* in);
int PS4_SYSV_ABI sceHttpWaitRequest();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http
