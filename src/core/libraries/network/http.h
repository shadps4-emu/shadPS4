// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"
#include "core/libraries/network/ssl.h"

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

struct HttpRequestInternal {
    int state;          // +0x20
    int errorCode;      // +0x28
    int httpStatusCode; // +0x20C
    std::mutex m_mutex;
};

// Memory pool statistics
struct OrbisHttpMemoryPoolStats {
    u64 poolSize;
    u64 maxInuseSize;
    u64 currentInuseSize;
    s32 reserved;
};

// Cookie statistics
struct OrbisHttpCookieStats {
    u64 currentInuseSize;
    u32 currentInuseNum;
    u64 maxInuseSize;
    u32 maxInuseNum;
    u32 removedNum;
    s32 reserved;
};

using OrbisHttpEpollHandle = void*;

// Non-blocking event reported by sceHttpWaitRequest.
struct OrbisHttpNBEvent {
    u32 events;
    u32 eventDetail;
    int id;
    void* userArg;
};

// Callbacks
using OrbisHttpAuthInfoCallback = int(PS4_SYSV_ABI*)(int request, int authType, const char* realm,
                                                     char* username, char* password,
                                                     int isNeedEntity, u8** entityBody,
                                                     u64* entitySize, int* isSave, void* userArg);

using OrbisHttpRedirectCallback = int(PS4_SYSV_ABI*)(int request, s32 statusCode, s32* method,
                                                     const char* location, void* userArg);

using OrbisHttpRequestStatusCallback = void(PS4_SYSV_ABI*)(int request, int requestStatus,
                                                           void* userArg);

using OrbisHttpCookieRecvCallback = int(PS4_SYSV_ABI*)(int request, const char* url,
                                                       const char* cookieHeader, u64 headerLen,
                                                       void* userArg);

using OrbisHttpCookieSendCallback = int(PS4_SYSV_ABI*)(int request, const char* url,
                                                       const char* cookieHeader, void* userArg);

using OrbisHttpsCallback = int(PS4_SYSV_ABI*)(int libsslCtxId, u32 verifyErr, void* const sslCert[],
                                              int certNum, void* userArg);

using OrbisHttpsCaList = Libraries::Ssl::OrbisSslCaList;

// Functions
int PS4_SYSV_ABI sceHttpAbortRequest(int reqId);
int PS4_SYSV_ABI sceHttpAbortRequestForce(int reqId);
int PS4_SYSV_ABI sceHttpAbortWaitRequest(OrbisHttpEpollHandle eh);
int PS4_SYSV_ABI sceHttpAddCookie(int libhttpCtxId, const char* url, const char* cookie,
                                  u64 cookieLength);
int PS4_SYSV_ABI sceHttpAddQuery();
int PS4_SYSV_ABI sceHttpAddRequestHeader(int id, const char* name, const char* value, s32 mode);
int PS4_SYSV_ABI sceHttpAddRequestHeaderRaw();
int PS4_SYSV_ABI sceHttpAuthCacheExport();
int PS4_SYSV_ABI sceHttpAuthCacheFlush(int libhttpCtxId);
int PS4_SYSV_ABI sceHttpAuthCacheImport();
int PS4_SYSV_ABI sceHttpCacheRedirectedConnectionEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpCookieExport(int libhttpCtxId, void* buffer, u64 bufferSize,
                                     u64* exportSize);
int PS4_SYSV_ABI sceHttpCookieFlush(int libhttpCtxId);
int PS4_SYSV_ABI sceHttpCookieImport(int libhttpCtxId, const void* buffer, u64 bufferSize);
int PS4_SYSV_ABI sceHttpCreateConnection(int tmplId, const char* serverName, const char* scheme,
                                         u16 port, int isEnableKeepalive);
int PS4_SYSV_ABI sceHttpCreateConnectionWithURL(int tmplId, const char* url, bool enableKeepalive);
int PS4_SYSV_ABI sceHttpCreateEpoll(int libhttpCtxId, OrbisHttpEpollHandle* eh);
int PS4_SYSV_ABI sceHttpCreateRequest(int connId, int method, const char* path, u64 contentLength);
int PS4_SYSV_ABI sceHttpCreateRequest2(int connId, const char* method, const char* path,
                                       u64 contentLength);
int PS4_SYSV_ABI sceHttpCreateRequestWithURL(int connId, s32 method, const char* url,
                                             u64 contentLength);
int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int connId, const char* method, const char* url,
                                              u64 contentLength);
int PS4_SYSV_ABI sceHttpCreateTemplate(int libhttpCtxId, const char* userAgent, int httpVer,
                                       int isAutoProxyConf);
int PS4_SYSV_ABI sceHttpDbgEnableProfile();
int PS4_SYSV_ABI sceHttpDbgGetConnectionStat();
int PS4_SYSV_ABI sceHttpDbgGetRequestStat();
int PS4_SYSV_ABI sceHttpDbgSetPrintf();
int PS4_SYSV_ABI sceHttpDbgShowConnectionStat();
int PS4_SYSV_ABI sceHttpDbgShowMemoryPoolStat();
int PS4_SYSV_ABI sceHttpDbgShowRequestStat();
int PS4_SYSV_ABI sceHttpDbgShowStat();
int PS4_SYSV_ABI sceHttpDeleteConnection(int connId);
int PS4_SYSV_ABI sceHttpDeleteRequest(int reqId);
int PS4_SYSV_ABI sceHttpDeleteTemplate(int tmplId);
int PS4_SYSV_ABI sceHttpDestroyEpoll(int libhttpCtxId, OrbisHttpEpollHandle eh);
int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int reqId, char** header, u64* headerSize);
int PS4_SYSV_ABI sceHttpGetAuthEnabled(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpGetAutoRedirect(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpGetConnectionStat();
int PS4_SYSV_ABI sceHttpGetCookie(int libhttpCtxId, const char* url, char* cookie, u64* required,
                                  u64 prepared, int isSecure);
int PS4_SYSV_ABI sceHttpGetCookieEnabled(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpGetCookieStats(int libhttpCtxId, OrbisHttpCookieStats* stats);
int PS4_SYSV_ABI sceHttpGetEpoll(int id, OrbisHttpEpollHandle* eh, void** userArg);
int PS4_SYSV_ABI sceHttpGetEpollId();
int PS4_SYSV_ABI sceHttpGetLastErrno(int reqId, int* errNum);
int PS4_SYSV_ABI sceHttpGetMemoryPoolStats(int libhttpCtxId, OrbisHttpMemoryPoolStats* currentStat);
int PS4_SYSV_ABI sceHttpGetNonblock(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpGetRegisteredCtxIds();
int PS4_SYSV_ABI sceHttpGetResponseContentLength(int reqId, int* result, u64* contentLength);
int PS4_SYSV_ABI sceHttpGetStatusCode(int reqId, int* statusCode);
int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, u64 poolSize);
int PS4_SYSV_ABI sceHttpParseResponseHeader(const char* header, u64 headerLen, const char* fieldStr,
                                            const char** fieldValue, u64* valueLen);
int PS4_SYSV_ABI sceHttpParseStatusLine(const char* statusLine, u64 lineLen, int32_t* httpMajorVer,
                                        int32_t* httpMinorVer, int32_t* responseCode,
                                        const char** reasonPhrase, u64* phraseLen);
int PS4_SYSV_ABI sceHttpReadData(s32 reqId, void* data, u64 size);
int PS4_SYSV_ABI sceHttpRedirectCacheFlush(int libhttpCtxId);
int PS4_SYSV_ABI sceHttpRemoveRequestHeader(int id, const char* name);
int PS4_SYSV_ABI sceHttpRequestGetAllHeaders();
int PS4_SYSV_ABI sceHttpsDisableOption(int id, u32 sslFlags);
int PS4_SYSV_ABI sceHttpsDisableOptionPrivate(int id, u32 sslFlags);
int PS4_SYSV_ABI sceHttpsEnableOption(int id, u32 sslFlags);
int PS4_SYSV_ABI sceHttpsEnableOptionPrivate(int id, u32 sslFlags);
int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size);
int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetAuthEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetAuthInfoCallback(int id, OrbisHttpAuthInfoCallback cbfunc,
                                            void* userArg);
int PS4_SYSV_ABI sceHttpSetAutoRedirect(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetChunkedTransferEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetConnectTimeOut(int id, u32 usec);
int PS4_SYSV_ABI sceHttpSetCookieEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetCookieMaxNum(int libhttpCtxId, u32 num);
int PS4_SYSV_ABI sceHttpSetCookieMaxNumPerDomain(int libhttpCtxId, u32 num);
int PS4_SYSV_ABI sceHttpSetCookieMaxSize(int libhttpCtxId, u32 size);
int PS4_SYSV_ABI sceHttpSetCookieRecvCallback(int id, OrbisHttpCookieRecvCallback cbfunc,
                                              void* userArg);
int PS4_SYSV_ABI sceHttpSetCookieSendCallback(int id, OrbisHttpCookieSendCallback cbfunc,
                                              void* userArg);
int PS4_SYSV_ABI sceHttpSetCookieTotalMaxSize(int libhttpCtxId, u32 size);
int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int libhttpCtxId, int isEnable);
int PS4_SYSV_ABI sceHttpSetDelayBuildRequestEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetEpoll(int id, OrbisHttpEpollHandle eh, void* userArg);
int PS4_SYSV_ABI sceHttpSetEpollId();
int PS4_SYSV_ABI sceHttpSetHttp09Enabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetInflateGZIPEnabled(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetNonblock(int id, int isEnable);
int PS4_SYSV_ABI sceHttpSetPolicyOption();
int PS4_SYSV_ABI sceHttpSetPriorityOption();
int PS4_SYSV_ABI sceHttpSetProxy();
int PS4_SYSV_ABI sceHttpSetRecvBlockSize(int id, u32 blockSize);
int PS4_SYSV_ABI sceHttpSetRecvTimeOut(int id, u32 usec);
int PS4_SYSV_ABI sceHttpSetRedirectCallback(int id, OrbisHttpRedirectCallback cbfunc,
                                            void* userArg);
int PS4_SYSV_ABI sceHttpSetRequestContentLength(int id, u64 contentLength);
int PS4_SYSV_ABI sceHttpSetRequestStatusCallback(int id, OrbisHttpRequestStatusCallback cbfunc,
                                                 void* userArg);
int PS4_SYSV_ABI sceHttpSetResolveRetry(int id, int retry);
int PS4_SYSV_ABI sceHttpSetResolveTimeOut(int id, u32 usec);
int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize(int id, u64 headerSize);
int PS4_SYSV_ABI sceHttpSetSendTimeOut(int id, u32 usec);
int PS4_SYSV_ABI sceHttpSetSocketCreationCallback();
int PS4_SYSV_ABI sceHttpsFreeCaList(int libhttpCtxId, OrbisHttpsCaList* caList);
int PS4_SYSV_ABI sceHttpsGetCaList(int httpCtxId, OrbisHttpsCaList* list);
int PS4_SYSV_ABI sceHttpsGetSslError(int id, int* errNum, u32* detail);
int PS4_SYSV_ABI sceHttpsLoadCert(int libhttpCtxId, int caCertNum, const void** caList,
                                  const void* cert, const void* privKey);
int PS4_SYSV_ABI sceHttpsSetMinSslVersion(int id, int version);
int PS4_SYSV_ABI sceHttpsSetSslCallback(int id, OrbisHttpsCallback cbfunc, void* userArg);
int PS4_SYSV_ABI sceHttpsSetSslVersion(int id, int version);
int PS4_SYSV_ABI sceHttpsUnloadCert(int libhttpCtxId);
int PS4_SYSV_ABI sceHttpTerm(int libhttpCtxId);
int PS4_SYSV_ABI sceHttpTryGetNonblock(int id, int* isEnable);
int PS4_SYSV_ABI sceHttpTrySetNonblock(int id, int isEnable);
int PS4_SYSV_ABI sceHttpUnsetEpoll(int id);
int PS4_SYSV_ABI sceHttpWaitRequest(OrbisHttpEpollHandle eh, OrbisHttpNBEvent* nbev, int maxevents,
                                    int timeout);
int PS4_SYSV_ABI sceHttpUriBuild(char* out, u64* require, u64 prepare,
                                 const OrbisHttpUriElement* srcElement, u32 option);
int PS4_SYSV_ABI sceHttpUriCopy();
//***********************************
// URI functions
//***********************************
int PS4_SYSV_ABI sceHttpUriEscape(char* out, u64* require, u64 prepare, const char* in);
int PS4_SYSV_ABI sceHttpUriMerge(char* mergedUrl, char* url, char* relativeUri, u64* require,
                                 u64 prepare, u32 option);
int PS4_SYSV_ABI sceHttpUriParse(OrbisHttpUriElement* out, const char* srcUri, void* pool,
                                 u64* require, u64 prepare);
int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, u64 srcSize);
int PS4_SYSV_ABI sceHttpUriUnescape(char* out, u64* require, u64 prepare, const char* in);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http
