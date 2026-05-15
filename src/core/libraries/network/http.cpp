// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "http_error.h"

namespace Libraries::Http {

static bool g_isHttpInitialized = true; // TODO temp always inited

void NormalizeAndAppendPath(char* dest, char* src) {
    char* lastSlash;
    u64 length;

    lastSlash = strrchr(dest, '/');
    if (lastSlash == NULL) {
        length = strlen(dest);
        dest[length] = '/';
        dest[length + 1] = '\0';
    } else {
        lastSlash[1] = '\0';
    }
    if (*src == '/') {
        dest[0] = '\0';
    }
    length = strnlen(dest, 0x3fff);
    strncat(dest, src, 0x3fff - length);
    return;
}

int PS4_SYSV_ABI sceHttpAbortRequest(int reqId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}", reqId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortRequestForce(int reqId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}", reqId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortWaitRequest(OrbisHttpEpollHandle eh) {
    LOG_ERROR(Lib_Http, "(STUBBED) called eh={}", fmt::ptr(eh));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddCookie(int libhttpCtxId, const char* url, const char* cookie,
                                  u64 cookieLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, url={}, cookie={}, cookieLength={}",
              libhttpCtxId, url ? url : "(null)", cookie ? cookie : "(null)", cookieLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddQuery() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddRequestHeader(int id, const char* name, const char* value, s32 mode) {
    LOG_INFO(Lib_Http, "(STUBBED) called id={}, name={}, value={}, mode={}", id,
             name ? name : "(null)", value ? value : "(null)", mode);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddRequestHeaderRaw() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheExport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheFlush(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAuthCacheImport() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCacheRedirectedConnectionEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieExport(int libhttpCtxId, void* buffer, u64 bufferSize,
                                     u64* exportSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, buffer={}, bufferSize={}, exportSize={}",
              libhttpCtxId, fmt::ptr(buffer), bufferSize, fmt::ptr(exportSize));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieFlush(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCookieImport(int libhttpCtxId, const void* buffer, u64 bufferSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, buffer={}, bufferSize={}", libhttpCtxId,
              fmt::ptr(buffer), bufferSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateConnection(int tmplId, const char* serverName, const char* scheme,
                                         u16 port, int isEnableKeepalive) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called tmplId={}, serverName={}, scheme={}, port={}, isEnableKeepalive={}",
              tmplId, serverName ? serverName : "(null)", scheme ? scheme : "(null)", port,
              isEnableKeepalive);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateConnectionWithURL(int tmplId, const char* url, bool enableKeepalive) {
    LOG_INFO(Lib_Http, "(STUBBED) called tmplId={}, url={}, enableKeepalive={}", tmplId,
             url ? url : "(null)", enableKeepalive);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateEpoll(int libhttpCtxId, OrbisHttpEpollHandle* eh) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequest(int connId, int method, const char* path, u64 contentLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called connId={}, method={}, path={}, contentLength={}", connId,
              method, path ? path : "(null)", contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequest2(int connId, const char* method, const char* path,
                                       u64 contentLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called connId={}, method={}, path={}, contentLength={}", connId,
              method ? method : "(null)", path ? path : "(null)", contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL(int connId, s32 method, const char* url,
                                             u64 contentLength) {
    LOG_INFO(Lib_Http, "(STUBBED) called connId={}, method={}, url={}, contentLength={}", connId,
             method, url ? url : "(null)", contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int connId, const char* method, const char* url,
                                              u64 contentLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called connId={}, method={}, url={}, contentLength={}", connId,
              method ? method : "(null)", url ? url : "(null)", contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateTemplate(int libhttpCtxId, const char* userAgent, int httpVer,
                                       int isAutoProxyConf) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called libhttpCtxId={}, userAgent={}, httpVer={}, isAutoProxyConf={}",
              libhttpCtxId, userAgent ? userAgent : "(null)", httpVer, isAutoProxyConf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgEnableProfile() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgGetConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgGetRequestStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgSetPrintf() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowMemoryPoolStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowRequestStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDbgShowStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteConnection(int connId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called connId={}", connId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteRequest(int reqId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}", reqId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDeleteTemplate(int tmplId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called tmplId={}", tmplId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDestroyEpoll(int libhttpCtxId, OrbisHttpEpollHandle eh) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int reqId, char** header, u64* headerSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}, header={}, headerSize={}", reqId,
              fmt::ptr(header), fmt::ptr(headerSize));
    return ORBIS_FAIL;
}

int PS4_SYSV_ABI sceHttpGetAuthEnabled(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAutoRedirect(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetConnectionStat() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookie(int libhttpCtxId, const char* url, char* cookie, u64* required,
                                  u64 prepared, int isSecure) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called libhttpCtxId={}, url={}, cookie={}, required={}, prepared={}, "
              "isSecure={}",
              libhttpCtxId, url ? url : "(null)", fmt::ptr(cookie), fmt::ptr(required), prepared,
              isSecure);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookieEnabled(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetCookieStats(int libhttpCtxId, OrbisHttpCookieStats* stats) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, stats={}", libhttpCtxId,
              fmt::ptr(stats));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpoll(int id, OrbisHttpEpollHandle* eh, void** userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, eh={}, userArg={}", id, fmt::ptr(eh),
              fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetLastErrno(int reqId, int* errNum) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}, errNum={}", reqId, fmt::ptr(errNum));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetMemoryPoolStats(int libhttpCtxId,
                                           OrbisHttpMemoryPoolStats* currentStat) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, currentStat={}", libhttpCtxId,
              fmt::ptr(currentStat));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetNonblock(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetRegisteredCtxIds() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetResponseContentLength(int reqId, int* result, u64* contentLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}, result={}, contentLength={}", reqId,
              fmt::ptr(result), fmt::ptr(contentLength));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetStatusCode(int reqId, int* statusCode) {
    LOG_INFO(Lib_Http, "(STUBBED) called reqId={}, statusCode={}", reqId, fmt::ptr(statusCode));
#if 0
    if (!g_isHttpInitialized)
        return ORBIS_HTTP_ERROR_BEFORE_INIT;

    if (statusCode == nullptr)
        return ORBIS_HTTP_ERROR_INVALID_VALUE;

    int ret = 0;
    // Lookup HttpRequestInternal by reqId
    HttpRequestInternal* request = nullptr;
    ret = HttpRequestInternal_Acquire(&request, reqId);
    if (ret < 0)
        return ret;
    request->m_mutex.lock();
    if (request->state > 0x11) {
        if (request->state == 0x16) {
            ret = request->errorCode;
        } else {
            *statusCode = request->httpStatusCode;
            ret = 0;
        }
    } else {
        ret = ORBIS_HTTP_ERROR_BEFORE_SEND;
    }
    request->m_mutex.unlock();
    HttpRequestInternal_Release(request);

    return ret;
#else
    return ORBIS_HTTP_ERROR_BEFORE_SEND;
#endif
}

int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, u64 poolSize) {
    LOG_INFO(Lib_Http, "called libnetMemId={}, libsslCtxId={}, poolSize={}", libnetMemId,
             libsslCtxId, poolSize);
    LOG_ERROR(Lib_Http, "(DUMMY) returning incrementing context id");
    // return a value >1
    static int id = 0;
    return ++id;
}

int PS4_SYSV_ABI sceHttpParseResponseHeader(const char* header, u64 headerLen, const char* fieldStr,
                                            const char** fieldValue, u64* valueLen) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called header={}, headerLen={}, fieldStr={}, fieldValue={}, valueLen={}",
              fmt::ptr(header), headerLen, fieldStr ? fieldStr : "(null)", fmt::ptr(fieldValue),
              fmt::ptr(valueLen));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpParseStatusLine(const char* statusLine, u64 lineLen, int32_t* httpMajorVer,
                                        int32_t* httpMinorVer, int32_t* responseCode,
                                        const char** reasonPhrase, u64* phraseLen) {
    LOG_INFO(Lib_Http,
             "called statusLine={}, lineLen={}, httpMajorVer={}, httpMinorVer={}, responseCode={}, "
             "reasonPhrase={}, phraseLen={}",
             fmt::ptr(statusLine), lineLen, fmt::ptr(httpMajorVer), fmt::ptr(httpMinorVer),
             fmt::ptr(responseCode), fmt::ptr(reasonPhrase), fmt::ptr(phraseLen));

    if (!statusLine) {
        LOG_ERROR(Lib_Http, "Invalid response: statusLine is null");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    if (!httpMajorVer || !httpMinorVer || !responseCode || !reasonPhrase || !phraseLen) {
        LOG_ERROR(Lib_Http, "Invalid value: one or more output pointers are null");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE;
    }
    *httpMajorVer = 0;
    *httpMinorVer = 0;
    if (lineLen < 8) {
        LOG_ERROR(Lib_Http, "lineLen ({}) is smaller than 8", lineLen);
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    if (strncmp(statusLine, "HTTP/", 5) != 0) {
        LOG_ERROR(Lib_Http, "statusLine doesn't start with HTTP/");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    u64 index = 5;

    if (!isdigit(statusLine[index])) {
        LOG_ERROR(Lib_Http, "Invalid response: expected digit after HTTP/");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    while (isdigit(statusLine[index])) {
        *httpMajorVer = *httpMajorVer * 10 + (statusLine[index] - '0');
        index++;
    }

    if (statusLine[index] != '.') {
        LOG_ERROR(Lib_Http, "Invalid response: expected '.' after major version");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    index++;

    if (!isdigit(statusLine[index])) {
        LOG_ERROR(Lib_Http, "Invalid response: expected digit for minor version");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    while (isdigit(statusLine[index])) {
        *httpMinorVer = *httpMinorVer * 10 + (statusLine[index] - '0');
        index++;
    }

    if (statusLine[index] != ' ') {
        LOG_ERROR(Lib_Http, "Invalid response: expected ' ' after minor version");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    index++;

    // Validate and parse the 3-digit HTTP response code
    if (lineLen - index < 3 || !isdigit(statusLine[index]) || !isdigit(statusLine[index + 1]) ||
        !isdigit(statusLine[index + 2])) {
        LOG_ERROR(Lib_Http, "Invalid response: malformed 3-digit response code");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    *responseCode = (statusLine[index] - '0') * 100 + (statusLine[index + 1] - '0') * 10 +
                    (statusLine[index + 2] - '0');
    index += 3;

    if (statusLine[index] != ' ') {
        LOG_ERROR(Lib_Http, "Invalid response: expected ' ' after response code");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    index++;

    // Set the reason phrase start position
    *reasonPhrase = &statusLine[index];
    u64 phraseStart = index;

    while (index < lineLen && statusLine[index] != '\n') {
        index++;
    }

    // Determine the length of the reason phrase, excluding trailing \r if present
    if (index == phraseStart) {
        *phraseLen = 0;
    } else {
        *phraseLen =
            (statusLine[index - 1] == '\r') ? (index - phraseStart - 1) : (index - phraseStart);
    }

    LOG_INFO(Lib_Http, "parsed HTTP/{}.{} {}, phraseLen={}, bytes consumed={}", *httpMajorVer,
             *httpMinorVer, *responseCode, *phraseLen, index + 1);

    // Return the number of bytes processed
    return index + 1;
}

int PS4_SYSV_ABI sceHttpReadData(s32 reqId, void* data, u64 size) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}, data={}, size={}", reqId, fmt::ptr(data), size);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRedirectCacheFlush(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRemoveRequestHeader(int id, const char* name) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, name={}", id, name ? name : "(null)");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRequestGetAllHeaders() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsDisableOption(int id, u32 sslFlags) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, sslFlags={:#x}", id, sslFlags);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsDisableOptionPrivate(int id, u32 sslFlags) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, sslFlags={:#x}", id, sslFlags);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOption(int id, u32 sslFlags) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, sslFlags={:#x}", id, sslFlags);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOptionPrivate(int id, u32 sslFlags) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, sslFlags={:#x}", id, sslFlags);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size) {
    LOG_ERROR(Lib_Http, "(STUBBED) called reqId={}, postData={}, size={}", reqId,
              fmt::ptr(postData), size);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAuthEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAuthInfoCallback(int id, OrbisHttpAuthInfoCallback cbfunc,
                                            void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAutoRedirect(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetChunkedTransferEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetConnectTimeOut(int id, u32 usec) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, usec={}", id, usec);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxNum(int libhttpCtxId, u32 num) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, num={}", libhttpCtxId, num);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxNumPerDomain(int libhttpCtxId, u32 num) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, num={}", libhttpCtxId, num);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieMaxSize(int libhttpCtxId, u32 size) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, size={}", libhttpCtxId, size);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieRecvCallback(int id, OrbisHttpCookieRecvCallback cbfunc,
                                              void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieSendCallback(int id, OrbisHttpCookieSendCallback cbfunc,
                                              void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetCookieTotalMaxSize(int libhttpCtxId, u32 size) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, size={}", libhttpCtxId, size);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int libhttpCtxId, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, isEnable={}", libhttpCtxId, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetDelayBuildRequestEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpoll(int id, OrbisHttpEpollHandle eh, void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, eh={}, userArg={}", id, fmt::ptr(eh),
              fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetHttp09Enabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetInflateGZIPEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetNonblock(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetPolicyOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetPriorityOption() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetProxy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRecvBlockSize(int id, u32 blockSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, blockSize={}", id, blockSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRecvTimeOut(int id, u32 usec) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, usec={}", id, usec);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRedirectCallback(int id, OrbisHttpRedirectCallback cbfunc,
                                            void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRequestContentLength(int id, u64 contentLength) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, contentLength={}", id, contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRequestStatusCallback(int id, OrbisHttpRequestStatusCallback cbfunc,
                                                 void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResolveRetry(int id, int retry) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, retry={}", id, retry);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResolveTimeOut(int id, u32 usec) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, usec={}", id, usec);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize(int id, u64 headerSize) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, headerSize={}", id, headerSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetSendTimeOut(int id, u32 usec) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, usec={}", id, usec);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetSocketCreationCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsFreeCaList(int libhttpCtxId, OrbisHttpsCaList* caList) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, caList={}", libhttpCtxId,
              fmt::ptr(caList));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetCaList(int httpCtxId, OrbisHttpsCaList* list) {
    LOG_INFO(Lib_Http, "called httpCtxId={}, list={}", httpCtxId, fmt::ptr(list));
    LOG_ERROR(Lib_Http, "(DUMMY) returning empty CA list");
    list->certsNum = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetSslError(int id, int* errNum, u32* detail) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, errNum={}, detail={}", id, fmt::ptr(errNum),
              fmt::ptr(detail));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsLoadCert(int libhttpCtxId, int caCertNum, const void** caList,
                                  const void* cert, const void* privKey) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called libhttpCtxId={}, caCertNum={}, caList={}, cert={}, privKey={}",
              libhttpCtxId, caCertNum, fmt::ptr(caList), fmt::ptr(cert), fmt::ptr(privKey));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetMinSslVersion(int id, int version) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, version={}", id, version);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetSslCallback(int id, OrbisHttpsCallback cbfunc, void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsSetSslVersion(int id, int version) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, version={}", id, version);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsUnloadCert(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTerm(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTryGetNonblock(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTrySetNonblock(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUnsetEpoll(int id) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}", id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpWaitRequest(OrbisHttpEpollHandle eh, OrbisHttpNBEvent* nbev, int maxevents,
                                    int timeout) {
    LOG_ERROR(Lib_Http, "(STUBBED) called eh={}, nbev={}, maxevents={}, timeout={}", fmt::ptr(eh),
              fmt::ptr(nbev), maxevents, timeout);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriCopy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

//***********************************
// URI functions
//***********************************
int PS4_SYSV_ABI sceHttpUriBuild(char* out, u64* require, u64 prepare,
                                 const OrbisHttpUriElement* srcElement, u32 option) {
    LOG_INFO(Lib_Http,
             "sceHttpUriBuild: called out={}, require={}, prepare={}, "
             "srcElement={}, option=0x{:x}",
             fmt::ptr(out), fmt::ptr(require), prepare, fmt::ptr(srcElement), option);

    if (srcElement == nullptr) {
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    auto field = [](const char* p) -> std::string_view {
        return p ? std::string_view(p) : std::string_view{};
    };

    const std::string_view scheme = field(srcElement->scheme);
    const std::string_view username = field(srcElement->username);
    const std::string_view password = field(srcElement->password);
    const std::string_view hostname = field(srcElement->hostname);
    const std::string_view path = field(srcElement->path);
    const std::string_view query = field(srcElement->query);
    const std::string_view fragment = field(srcElement->fragment);

    std::string built;
    built.reserve(256);

    // Scheme
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_SCHEME) && !scheme.empty()) {
        built.append(scheme);
        built.append("://");
    }

    // Userinfo (username[:password]@)
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_USERNAME) && !username.empty()) {
        built.append(username);
        if ((option & ORBIS_HTTP_URI_BUILD_WITH_PASSWORD) && !password.empty()) {
            built.push_back(':');
            built.append(password);
        }
        built.push_back('@');
    }

    // Host
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_HOSTNAME) && !hostname.empty()) {
        built.append(hostname);
    }

    // Port (only if not the scheme's default)
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_PORT) && srcElement->port != 0) {
        const bool is_default_https = (scheme == "https" && srcElement->port == 443);
        const bool is_default_http = (scheme == "http" && srcElement->port == 80);
        if (!is_default_https && !is_default_http) {
            built.push_back(':');
            built.append(std::to_string(srcElement->port));
        }
    }

    // Path
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_PATH) && !path.empty()) {
        if (path.front() != '/')
            built.push_back('/');
        built.append(path);
    }

    // Query
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_QUERY) && !query.empty()) {
        if (query.front() != '?') {
            built.push_back('?');
        }
        built.append(query);
    }

    // Fragment
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_FRAGMENT) && !fragment.empty()) {
        if (fragment.front() != '#') {
            built.push_back('#');
        }
        built.append(fragment);
    }

    // include null terminator in the required size.
    const size_t need = built.size() + 1;
    if (require) {
        *require = need;
    }

    if (out == nullptr) {
        // Size query mode (no buffer provided).
        return ORBIS_OK;
    }

    if (prepare < need) {
        return ORBIS_HTTP_ERROR_OUT_OF_SIZE; // buffer too small
    }

    std::memcpy(out, built.c_str(), need);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriEscape(char* out, u64* require, u64 prepare, const char* in) {
    LOG_TRACE(Lib_Http, "called out={}, require={}, prepare={}, in={}", fmt::ptr(out),
              fmt::ptr(require), prepare, in ? in : "(null)");

    if (!in) {
        LOG_ERROR(Lib_Http, "Invalid input string");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    auto IsUnreserved = [](unsigned char c) -> bool {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '-' || c == '_' || c == '.' || c == '~';
    };

    u64 needed = 0;
    const char* src = in;
    while (*src) {
        unsigned char c = static_cast<unsigned char>(*src);
        if (IsUnreserved(c)) {
            needed++;
        } else {
            needed += 3; // %XX format
        }
        src++;
    }
    needed++; // null terminator

    if (require) {
        *require = needed;
    }

    if (!out) {
        LOG_INFO(Lib_Http, "out is null, only computing required size: {}", needed);
        return ORBIS_OK;
    }

    if (prepare < needed) {
        LOG_ERROR(Lib_Http, "Buffer too small: need {} but only {} available", needed, prepare);
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
    }

    static const char hex_chars[] = "0123456789ABCDEF";
    src = in;
    char* dst = out;
    while (*src) {
        unsigned char c = static_cast<unsigned char>(*src);
        if (IsUnreserved(c)) {
            *dst++ = *src;
        } else {
            *dst++ = '%';
            *dst++ = hex_chars[(c >> 4) & 0x0F];
            *dst++ = hex_chars[c & 0x0F];
        }
        src++;
    }
    *dst = '\0';

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriMerge(char* mergedUrl, char* url, char* relativeUri, u64* require,
                                 u64 prepare, u32 option) {
    LOG_TRACE(Lib_Http,
              "called mergedUrl={}, url={}, relativeUri={}, require={}, prepare={}, option={:#x}",
              fmt::ptr(mergedUrl), url ? url : "(null)", relativeUri ? relativeUri : "(null)",
              fmt::ptr(require), prepare, option);

    u64 requiredLength;
    int returnValue;
    u64 baseUrlLength;
    u64 relativeUriLength;
    u64 totalLength;
    u64 combinedLength;
    int parseResult;
    u64 localSizeRelativeUri;
    u64 localSizeBaseUrl;
    OrbisHttpUriElement parsedUriElement;

    if (option != 0 || url == NULL || relativeUri == NULL) {
        LOG_ERROR(Lib_Http, "Invalid value: option={:#x}, url={}, relativeUri={}", option,
                  fmt::ptr(url), fmt::ptr(relativeUri));
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    returnValue = sceHttpUriParse(NULL, url, NULL, &localSizeBaseUrl, 0);
    if (returnValue < 0) {
        LOG_ERROR(Lib_Http, "sceHttpUriParse(url) returned {:#x}", returnValue);
        return returnValue;
    }

    returnValue = sceHttpUriParse(NULL, relativeUri, NULL, &localSizeRelativeUri, 0);
    if (returnValue < 0) {
        LOG_ERROR(Lib_Http, "sceHttpUriParse(relativeUri) returned {:#x}", returnValue);
        return returnValue;
    }

    baseUrlLength = strnlen(url, 0x3fff);
    relativeUriLength = strnlen(relativeUri, 0x3fff);
    requiredLength = localSizeBaseUrl + 2 + (relativeUriLength + baseUrlLength) * 2;

    if (require) {
        *require = requiredLength;
    }

    if (mergedUrl == NULL) {
        LOG_INFO(Lib_Http, "mergedUrl is null, only returning required size: {}", requiredLength);
        return ORBIS_OK;
    }

    if (prepare < requiredLength) {
        LOG_ERROR(Lib_Http, "Out of memory: need {} but only {} available", requiredLength,
                  prepare);
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
    }

    totalLength = strnlen(url, 0x3fff);
    baseUrlLength = strnlen(relativeUri, 0x3fff);
    combinedLength = totalLength + 1 + baseUrlLength;
    relativeUriLength = prepare - combinedLength;

    returnValue =
        sceHttpUriParse(&parsedUriElement, relativeUri, mergedUrl + totalLength + baseUrlLength + 1,
                        &localSizeRelativeUri, relativeUriLength);
    if (returnValue < 0) {
        LOG_ERROR(Lib_Http, "second sceHttpUriParse(relativeUri) returned {:#x}", returnValue);
        return returnValue;
    }
    if (parsedUriElement.scheme == NULL) {
        strncpy(mergedUrl, relativeUri, requiredLength);
        if (require) {
            *require = strnlen(relativeUri, 0x3fff) + 1;
        }
        return ORBIS_OK;
    }

    returnValue =
        sceHttpUriParse(&parsedUriElement, url, mergedUrl + totalLength + baseUrlLength + 1,
                        &localSizeBaseUrl, relativeUriLength);
    if (returnValue < 0) {
        LOG_ERROR(Lib_Http, "second sceHttpUriParse(url) returned {:#x}", returnValue);
        return returnValue;
    }

    combinedLength += localSizeBaseUrl;
    strncpy(mergedUrl + combinedLength, parsedUriElement.path, prepare - combinedLength);
    NormalizeAndAppendPath(mergedUrl + combinedLength, relativeUri);

    returnValue = sceHttpUriBuild(mergedUrl, 0, ~(baseUrlLength + totalLength) + prepare,
                                  &parsedUriElement, 0x3f);
    if (returnValue >= 0) {
        return ORBIS_OK;
    } else {
        LOG_ERROR(Lib_Http, "sceHttpUriBuild returned {:#x}", returnValue);
        return returnValue;
    }
}

int PS4_SYSV_ABI sceHttpUriParse(OrbisHttpUriElement* out, const char* srcUri, void* pool,
                                 u64* require, u64 prepare) {
    LOG_TRACE(Lib_Http, "called out={}, srcUri={}, pool={}, require={}, prepare={}", fmt::ptr(out),
              srcUri ? srcUri : "(null)", fmt::ptr(pool), fmt::ptr(require), prepare);
    if (!srcUri) {
        LOG_ERROR(Lib_Http, "invalid url: srcUri is null");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }
    if (!out && !pool && !require) {
        LOG_ERROR(Lib_Http, "invalid values: all output parameters are null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    if (out && pool) {
        memset(out, 0, sizeof(OrbisHttpUriElement));
        char* empty = (char*)pool;
        *empty = '\0';
        out->scheme = (char*)pool + 1; // scheme storage follows the sentinel
        out->username = empty;
        out->password = empty;
        out->hostname = empty;
        out->path = empty;
        out->query = empty;
        out->fragment = empty;
    }

    u64 requiredSize = 1;

    // Parse the scheme (e.g., "http:", "https:", "file:")
    u64 schemeLength = 0;
    while (srcUri[schemeLength] && srcUri[schemeLength] != ':') {
        if (!isalnum(srcUri[schemeLength])) {
            LOG_ERROR(Lib_Http, "invalid url: non-alphanumeric character in scheme");
            return ORBIS_HTTP_ERROR_INVALID_URL;
        }
        schemeLength++;
    }

    if (pool && prepare < requiredSize + schemeLength + 1) {
        LOG_ERROR(Lib_Http, "out of memory while writing scheme");
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
    }

    if (out && pool) {
        memcpy(out->scheme, srcUri, schemeLength);
        out->scheme[schemeLength] = '\0';
    }

    requiredSize += schemeLength + 1;

    // Move past the scheme and ':' character
    u64 offset = schemeLength + 1;

    // Check if "//" appears after the scheme
    if (strncmp(srcUri + offset, "//", 2) == 0) {
        // "//" is present
        if (out) {
            out->opaque = false;
        }
        offset += 2; // Move past "//"
    } else {
        // "//" is not present
        if (out) {
            out->opaque = true;
        }
    }

    // Handle "file" scheme
    if (strncmp(srcUri, "file", 4) == 0) {
        // File URIs typically start with "file://"
        if (out && !out->opaque) {
            // Skip additional slashes (e.g., "////")
            while (srcUri[offset] == '/') {
                offset++;
            }

            // Parse the path (everything after the slashes)
            char* pathStart = (char*)srcUri + offset;
            u64 pathLength = 0;
            while (pathStart[pathLength] && pathStart[pathLength] != '?' &&
                   pathStart[pathLength] != '#') {
                pathLength++;
            }

            if (pathLength > 0) {
                // Prepend '/' to the path
                requiredSize += pathLength + 2; // Include '/' and null terminator

                if (pool && prepare < requiredSize) {
                    LOG_ERROR(Lib_Http, "out of memory, provided size: {}, required size: {}",
                              prepare, requiredSize);
                    return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
                }

                if (out && pool) {
                    out->path = (char*)pool + (requiredSize - pathLength - 2);
                    out->username = (char*)pool + (requiredSize - pathLength - 3);
                    out->password = (char*)pool + (requiredSize - pathLength - 3);
                    out->hostname = (char*)pool + (requiredSize - pathLength - 3);
                    out->query = (char*)pool + (requiredSize - pathLength - 3);
                    out->fragment = (char*)pool + (requiredSize - pathLength - 3);
                    out->username[0] = '\0';
                    out->path[0] = '/'; // Add leading '/'
                    memcpy(out->path + 1, pathStart, pathLength);
                    out->path[pathLength + 1] = '\0';
                }
            } else {
                // Path already starts with '/'
                requiredSize += pathLength + 1;

                if (pool && prepare < requiredSize) {
                    LOG_ERROR(Lib_Http, "out of memory writing file scheme path");
                    return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
                }

                if (out && pool) {
                    memcpy((char*)pool + (requiredSize - pathLength - 1), pathStart, pathLength);
                    out->path = (char*)pool + (requiredSize - pathLength - 1);
                    out->path[pathLength] = '\0';
                }
            }

            // Move past the path
            offset += pathLength;
        } else {
            // Parse the path (everything after the slashes)
            char* pathStart = (char*)srcUri + offset;
            u64 pathLength = 0;
            while (pathStart[pathLength] && pathStart[pathLength] != '?' &&
                   pathStart[pathLength] != '#') {
                pathLength++;
            }

            if (pathLength > 0) {
                requiredSize += pathLength + 3; // Add '/' and null terminator, and the dummy
                                                // null character for the other fields
            }
        }
    }

    // Handle non-file schemes (e.g., "http", "https")
    else {
        // Parse the host and port
        char* hostStart = (char*)srcUri + offset;
        while (*hostStart == '/') {
            hostStart++;
        }

        u64 hostLength = 0;
        while (hostStart[hostLength] && hostStart[hostLength] != '/' &&
               hostStart[hostLength] != '?' && hostStart[hostLength] != ':') {
            hostLength++;
        }

        requiredSize += hostLength + 1;

        if (pool && prepare < requiredSize) {
            LOG_ERROR(Lib_Http, "out of memory while writing hostname");
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }

        if (out && pool) {
            memcpy((char*)pool + (requiredSize - hostLength - 1), hostStart, hostLength);
            out->hostname = (char*)pool + (requiredSize - hostLength - 1);
            out->hostname[hostLength] = '\0';
        }

        // Move past the host
        offset += hostLength;

        // Parse the port (if present)
        if (hostStart[hostLength] == ':') {
            char* portStart = hostStart + hostLength + 1;
            u64 portLength = 0;
            while (portStart[portLength] && isdigit(portStart[portLength])) {
                portLength++;
            }

            requiredSize += portLength + 1;

            if (pool && prepare < requiredSize) {
                LOG_ERROR(Lib_Http, "out of memory while writing port");
                return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
            }

            // Convert the port string to a uint16_t
            char portStr[6]; // Max length for a port number (65535)
            if (portLength > 5) {
                LOG_ERROR(Lib_Http, "invalid url: port length {} exceeds 5 chars", portLength);
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
            memcpy(portStr, portStart, portLength);
            portStr[portLength] = '\0';

            uint16_t port = (uint16_t)atoi(portStr);
            if (port == 0 && portStr[0] != '0') {
                LOG_ERROR(Lib_Http, "invalid url: failed to parse port '{}'", portStr);
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }

            // Set the port in the output structure
            if (out) {
                out->port = port;
            }

            // Move past the port
            offset += portLength + 1;
        }
    }

    // Parse the path (if present)
    if (srcUri[offset] == '/') {
        char* pathStart = (char*)srcUri + offset;
        u64 pathLength = 0;
        while (pathStart[pathLength] && pathStart[pathLength] != '?' &&
               pathStart[pathLength] != '#') {
            pathLength++;
        }

        requiredSize += pathLength + 1;

        if (pool && prepare < requiredSize) {
            LOG_ERROR(Lib_Http, "out of memory while writing path");
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }

        if (out && pool) {
            memcpy((char*)pool + (requiredSize - pathLength - 1), pathStart, pathLength);
            out->path = (char*)pool + (requiredSize - pathLength - 1);
            out->path[pathLength] = '\0';
        }

        // Move past the path
        offset += pathLength;
    }

    if (srcUri[offset] == '?') {
        char* queryStart = (char*)srcUri + offset;
        u64 queryLength = 0;
        while (queryStart[queryLength + 1] && queryStart[queryLength + 1] != '#') {
            queryLength++;
        }
        queryLength++;

        requiredSize += queryLength + 1;

        if (pool && prepare < requiredSize) {
            LOG_ERROR(Lib_Http, "out of memory while writing query");
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }

        if (out && pool) {
            memcpy((char*)pool + (requiredSize - queryLength - 1), queryStart, queryLength);
            out->query = (char*)pool + (requiredSize - queryLength - 1);
            out->query[queryLength] = '\0';
        }

        offset += queryLength;
    }

    if (srcUri[offset] == '#') {
        char* fragmentStart = (char*)srcUri + offset;
        u64 fragmentLength = 0;
        while (fragmentStart[fragmentLength + 1]) {
            fragmentLength++;
        }
        fragmentLength++;

        requiredSize += fragmentLength + 1;

        if (pool && prepare < requiredSize) {
            LOG_ERROR(Lib_Http, "out of memory while writing fragment");
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }

        if (out && pool) {
            memcpy((char*)pool + (requiredSize - fragmentLength - 1), fragmentStart,
                   fragmentLength);
            out->fragment = (char*)pool + (requiredSize - fragmentLength - 1);
            out->fragment[fragmentLength] = '\0';
        }
    }

    // Calculate the total required buffer size
    if (require) {
        *require = requiredSize; // Update with actual required size
    }

    LOG_TRACE(Lib_Http, "parsed successfully, requiredSize={}", requiredSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, u64 srcSize) {
    LOG_TRACE(Lib_Http, "called dst={}, src={}, srcSize={}", fmt::ptr(dst), src ? src : "(null)",
              srcSize);

    if (!dst || !src) {
        LOG_ERROR(Lib_Http, "Invalid parameters: dst={}, src={}", fmt::ptr(dst), fmt::ptr(src));
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    if (srcSize == 0) {
        dst[0] = '\0';
        return ORBIS_OK;
    }

    u64 len = 0;
    while (len < srcSize && src[len] != '\0') {
        len++;
    }

    for (u64 i = 0; i < len; i++) {
        dst[i] = src[i];
    }
    dst[len] = '\0';

    char* read = dst;
    char* write = dst;

    while (*read) {
        if (read[0] == '.' && read[1] == '.' && read[2] == '/') {
            read += 3;
            continue;
        }

        if (read[0] == '.' && read[1] == '/') {
            read += 2;
            continue;
        }

        if (read[0] == '/' && read[1] == '.' && read[2] == '/') {
            read += 2;
            continue;
        }

        if (read[0] == '/' && read[1] == '.' && read[2] == '\0') {
            if (write == dst) {
                *write++ = '/';
            }
            break;
        }

        bool is_dotdot_mid = (read[0] == '/' && read[1] == '.' && read[2] == '.' && read[3] == '/');
        bool is_dotdot_end =
            (read[0] == '/' && read[1] == '.' && read[2] == '.' && read[3] == '\0');

        if (is_dotdot_mid || is_dotdot_end) {
            if (write > dst) {
                if (*(write - 1) == '/') {
                    write--;
                }
                while (write > dst && *(write - 1) != '/') {
                    write--;
                }

                if (is_dotdot_mid && write > dst) {
                    write--;
                }
            }

            if (is_dotdot_mid) {
                read += 3;
            } else {
                break;
            }
            continue;
        }

        if ((read[0] == '.' && read[1] == '\0') ||
            (read[0] == '.' && read[1] == '.' && read[2] == '\0')) {
            break;
        }

        if (read[0] == '/') {
            *write++ = *read++;
        }
        while (*read && *read != '/') {
            *write++ = *read++;
        }
    }

    *write = '\0';
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriUnescape(char* out, u64* require, u64 prepare, const char* in) {
    LOG_TRACE(Lib_Http, "called out={}, require={}, prepare={}, in={}", fmt::ptr(out),
              fmt::ptr(require), prepare, in ? in : "(null)");

    if (!in) {
        LOG_ERROR(Lib_Http, "Invalid input string");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    // Locale-independent hex digit check
    auto IsHex = [](char c) -> bool {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    };

    // Convert hex char to int value
    auto HexToInt = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return 0;
    };

    // Check for valid percent-encoded sequence (%XX)
    auto IsValidPercentSequence = [&](const char* s) -> bool {
        return s[0] == '%' && s[1] != '\0' && s[2] != '\0' && IsHex(s[1]) && IsHex(s[2]);
    };

    u64 needed = 0;
    const char* src = in;
    while (*src) {
        if (IsValidPercentSequence(src)) {
            src += 3;
        } else {
            src++;
        }
        needed++;
    }
    needed++; // null terminator

    if (require) {
        *require = needed;
    }

    if (!out) {
        LOG_INFO(Lib_Http, "out is null, only computing required size: {}", needed);
        return ORBIS_OK;
    }

    if (prepare < needed) {
        LOG_ERROR(Lib_Http, "Buffer too small: need {} but only {} available", needed, prepare);
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
    }

    src = in;
    char* dst = out;
    while (*src) {
        if (IsValidPercentSequence(src)) {
            *dst++ = static_cast<char>((HexToInt(src[1]) << 4) | HexToInt(src[2]));
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hvG6GfBMXg8", "libSceHttp", 1, "libSceHttp", sceHttpAbortRequest);
    LIB_FUNCTION("JKl06ZIAl6A", "libSceHttp", 1, "libSceHttp", sceHttpAbortRequestForce);
    LIB_FUNCTION("sWQiqKvYTVA", "libSceHttp", 1, "libSceHttp", sceHttpAbortWaitRequest);
    LIB_FUNCTION("mNan6QSnpeY", "libSceHttp", 1, "libSceHttp", sceHttpAddCookie);
    LIB_FUNCTION("JM58a21mtrQ", "libSceHttp", 1, "libSceHttp", sceHttpAddQuery);
    LIB_FUNCTION("EY28T2bkN7k", "libSceHttp", 1, "libSceHttp", sceHttpAddRequestHeader);
    LIB_FUNCTION("lGAjftanhFs", "libSceHttp", 1, "libSceHttp", sceHttpAddRequestHeaderRaw);
    LIB_FUNCTION("Y1DCjN-s2BA", "libSceHttp", 1, "libSceHttp", sceHttpAuthCacheExport);
    LIB_FUNCTION("zzB0StvRab4", "libSceHttp", 1, "libSceHttp", sceHttpAuthCacheFlush);
    LIB_FUNCTION("wF0KcxK20BE", "libSceHttp", 1, "libSceHttp", sceHttpAuthCacheImport);
    LIB_FUNCTION("A7n9nNg7NBg", "libSceHttp", 1, "libSceHttp",
                 sceHttpCacheRedirectedConnectionEnabled);
    LIB_FUNCTION("nOkViL17ZOo", "libSceHttp", 1, "libSceHttp", sceHttpCookieExport);
    LIB_FUNCTION("seCvUt91WHY", "libSceHttp", 1, "libSceHttp", sceHttpCookieFlush);
    LIB_FUNCTION("pFnXDxo3aog", "libSceHttp", 1, "libSceHttp", sceHttpCookieImport);
    LIB_FUNCTION("Kiwv9r4IZCc", "libSceHttp", 1, "libSceHttp", sceHttpCreateConnection);
    LIB_FUNCTION("qgxDBjorUxs", "libSceHttp", 1, "libSceHttp", sceHttpCreateConnectionWithURL);
    LIB_FUNCTION("6381dWF+xsQ", "libSceHttp", 1, "libSceHttp", sceHttpCreateEpoll);
    LIB_FUNCTION("tsGVru3hCe8", "libSceHttp", 1, "libSceHttp", sceHttpCreateRequest);
    LIB_FUNCTION("rGNm+FjIXKk", "libSceHttp", 1, "libSceHttp", sceHttpCreateRequest2);
    LIB_FUNCTION("Aeu5wVKkF9w", "libSceHttp", 1, "libSceHttp", sceHttpCreateRequestWithURL);
    LIB_FUNCTION("Cnp77podkCU", "libSceHttp", 1, "libSceHttp", sceHttpCreateRequestWithURL2);
    LIB_FUNCTION("0gYjPTR-6cY", "libSceHttp", 1, "libSceHttp", sceHttpCreateTemplate);
    LIB_FUNCTION("Lffcxao-QMM", "libSceHttp", 1, "libSceHttp", sceHttpDbgEnableProfile);
    LIB_FUNCTION("6gyx-I0Oob4", "libSceHttp", 1, "libSceHttp", sceHttpDbgGetConnectionStat);
    LIB_FUNCTION("fzzBpJjm9Kw", "libSceHttp", 1, "libSceHttp", sceHttpDbgGetRequestStat);
    LIB_FUNCTION("VmqSnjZ5mE4", "libSceHttp", 1, "libSceHttp", sceHttpDbgSetPrintf);
    LIB_FUNCTION("KJtUHtp6y0U", "libSceHttp", 1, "libSceHttp", sceHttpDbgShowConnectionStat);
    LIB_FUNCTION("oEuPssSYskA", "libSceHttp", 1, "libSceHttp", sceHttpDbgShowMemoryPoolStat);
    LIB_FUNCTION("L2gM3qptqHs", "libSceHttp", 1, "libSceHttp", sceHttpDbgShowRequestStat);
    LIB_FUNCTION("pxBsD-X9eH0", "libSceHttp", 1, "libSceHttp", sceHttpDbgShowStat);
    LIB_FUNCTION("P6A3ytpsiYc", "libSceHttp", 1, "libSceHttp", sceHttpDeleteConnection);
    LIB_FUNCTION("qe7oZ+v4PWA", "libSceHttp", 1, "libSceHttp", sceHttpDeleteRequest);
    LIB_FUNCTION("4I8vEpuEhZ8", "libSceHttp", 1, "libSceHttp", sceHttpDeleteTemplate);
    LIB_FUNCTION("wYhXVfS2Et4", "libSceHttp", 1, "libSceHttp", sceHttpDestroyEpoll);
    LIB_FUNCTION("1rpZqxdMRwQ", "libSceHttp", 1, "libSceHttp", sceHttpGetAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("aCYPMSUIaP8", "libSceHttp", 1, "libSceHttp", sceHttpGetAllResponseHeaders);
    LIB_FUNCTION("9m8EcOGzcIQ", "libSceHttp", 1, "libSceHttp", sceHttpGetAuthEnabled);
    LIB_FUNCTION("mmLexUbtnfY", "libSceHttp", 1, "libSceHttp", sceHttpGetAutoRedirect);
    LIB_FUNCTION("L-DwVoHXLtU", "libSceHttp", 1, "libSceHttp", sceHttpGetConnectionStat);
    LIB_FUNCTION("+G+UsJpeXPc", "libSceHttp", 1, "libSceHttp", sceHttpGetCookie);
    LIB_FUNCTION("iSZjWw1TGiA", "libSceHttp", 1, "libSceHttp", sceHttpGetCookieEnabled);
    LIB_FUNCTION("xkymWiGdMiI", "libSceHttp", 1, "libSceHttp", sceHttpGetCookieStats);
    LIB_FUNCTION("7j9VcwnrZo4", "libSceHttp", 1, "libSceHttp", sceHttpGetEpoll);
    LIB_FUNCTION("IQOP6McWJcY", "libSceHttp", 1, "libSceHttp", sceHttpGetEpollId);
    LIB_FUNCTION("0onIrKx9NIE", "libSceHttp", 1, "libSceHttp", sceHttpGetLastErrno);
    LIB_FUNCTION("16sMmVuOvgU", "libSceHttp", 1, "libSceHttp", sceHttpGetMemoryPoolStats);
    LIB_FUNCTION("Wq4RNB3snSQ", "libSceHttp", 1, "libSceHttp", sceHttpGetNonblock);
    LIB_FUNCTION("hkcfqAl+82w", "libSceHttp", 1, "libSceHttp", sceHttpGetRegisteredCtxIds);
    LIB_FUNCTION("yuO2H2Uvnos", "libSceHttp", 1, "libSceHttp", sceHttpGetResponseContentLength);
    LIB_FUNCTION("0a2TBNfE3BU", "libSceHttp", 1, "libSceHttp", sceHttpGetStatusCode);
    LIB_FUNCTION("A9cVMUtEp4Y", "libSceHttp", 1, "libSceHttp", sceHttpInit);
    LIB_FUNCTION("hPTXo3bICzI", "libSceHttp", 1, "libSceHttp", sceHttpParseResponseHeader);
    LIB_FUNCTION("Qq8SfuJJJqE", "libSceHttp", 1, "libSceHttp", sceHttpParseStatusLine);
    LIB_FUNCTION("P5pdoykPYTk", "libSceHttp", 1, "libSceHttp", sceHttpReadData);
    LIB_FUNCTION("u05NnI+P+KY", "libSceHttp", 1, "libSceHttp", sceHttpRedirectCacheFlush);
    LIB_FUNCTION("zNGh-zoQTD0", "libSceHttp", 1, "libSceHttp", sceHttpRemoveRequestHeader);
    LIB_FUNCTION("4fgkfVeVsGU", "libSceHttp", 1, "libSceHttp", sceHttpRequestGetAllHeaders);
    LIB_FUNCTION("mSQCxzWTwVI", "libSceHttp", 1, "libSceHttp", sceHttpsDisableOption);
    LIB_FUNCTION("zJYi5br6ZiQ", "libSceHttp", 1, "libSceHttp", sceHttpsDisableOptionPrivate);
    LIB_FUNCTION("f42K37mm5RM", "libSceHttp", 1, "libSceHttp", sceHttpsEnableOption);
    LIB_FUNCTION("I4+4hKttt1w", "libSceHttp", 1, "libSceHttp", sceHttpsEnableOptionPrivate);
    LIB_FUNCTION("1e2BNwI-XzE", "libSceHttp", 1, "libSceHttp", sceHttpSendRequest);
    LIB_FUNCTION("HRX1iyDoKR8", "libSceHttp", 1, "libSceHttp", sceHttpSetAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("qFg2SuyTJJY", "libSceHttp", 1, "libSceHttp", sceHttpSetAuthEnabled);
    LIB_FUNCTION("jf4TB2nUO40", "libSceHttp", 1, "libSceHttp", sceHttpSetAuthInfoCallback);
    LIB_FUNCTION("T-mGo9f3Pu4", "libSceHttp", 1, "libSceHttp", sceHttpSetAutoRedirect);
    LIB_FUNCTION("PDxS48xGQLs", "libSceHttp", 1, "libSceHttp", sceHttpSetChunkedTransferEnabled);
    LIB_FUNCTION("0S9tTH0uqTU", "libSceHttp", 1, "libSceHttp", sceHttpSetConnectTimeOut);
    LIB_FUNCTION("XNUoD2B9a6A", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieEnabled);
    LIB_FUNCTION("pM--+kIeW-8", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieMaxNum);
    LIB_FUNCTION("Kp6juCJUJGQ", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieMaxNumPerDomain);
    LIB_FUNCTION("7Y4364GBras", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieMaxSize);
    LIB_FUNCTION("Kh6bS2HQKbo", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieRecvCallback);
    LIB_FUNCTION("GnVDzYfy-KI", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieSendCallback);
    LIB_FUNCTION("pHc3bxUzivU", "libSceHttp", 1, "libSceHttp", sceHttpSetCookieTotalMaxSize);
    LIB_FUNCTION("8kzIXsRy1bY", "libSceHttp", 1, "libSceHttp",
                 sceHttpSetDefaultAcceptEncodingGZIPEnabled);
    LIB_FUNCTION("22buO-UufJY", "libSceHttp", 1, "libSceHttp", sceHttpSetDelayBuildRequestEnabled);
    LIB_FUNCTION("-xm7kZQNpHI", "libSceHttp", 1, "libSceHttp", sceHttpSetEpoll);
    LIB_FUNCTION("LG1YW1Uhkgo", "libSceHttp", 1, "libSceHttp", sceHttpSetEpollId);
    LIB_FUNCTION("pk0AuomQM1o", "libSceHttp", 1, "libSceHttp", sceHttpSetHttp09Enabled);
    LIB_FUNCTION("i9mhafzkEi8", "libSceHttp", 1, "libSceHttp", sceHttpSetInflateGZIPEnabled);
    LIB_FUNCTION("s2-NPIvz+iA", "libSceHttp", 1, "libSceHttp", sceHttpSetNonblock);
    LIB_FUNCTION("gZ9TpeFQ7Gk", "libSceHttp", 1, "libSceHttp", sceHttpSetPolicyOption);
    LIB_FUNCTION("2NeZnMEP3-0", "libSceHttp", 1, "libSceHttp", sceHttpSetPriorityOption);
    LIB_FUNCTION("i+quCZCL+D8", "libSceHttp", 1, "libSceHttp", sceHttpSetProxy);
    LIB_FUNCTION("mMcB2XIDoV4", "libSceHttp", 1, "libSceHttp", sceHttpSetRecvBlockSize);
    LIB_FUNCTION("yigr4V0-HTM", "libSceHttp", 1, "libSceHttp", sceHttpSetRecvTimeOut);
    LIB_FUNCTION("h9wmFZX4i-4", "libSceHttp", 1, "libSceHttp", sceHttpSetRedirectCallback);
    LIB_FUNCTION("PTiFIUxCpJc", "libSceHttp", 1, "libSceHttp", sceHttpSetRequestContentLength);
    LIB_FUNCTION("vO4B-42ef-k", "libSceHttp", 1, "libSceHttp", sceHttpSetRequestStatusCallback);
    LIB_FUNCTION("K1d1LqZRQHQ", "libSceHttp", 1, "libSceHttp", sceHttpSetResolveRetry);
    LIB_FUNCTION("Tc-hAYDKtQc", "libSceHttp", 1, "libSceHttp", sceHttpSetResolveTimeOut);
    LIB_FUNCTION("a4VsZ4oqn68", "libSceHttp", 1, "libSceHttp", sceHttpSetResponseHeaderMaxSize);
    LIB_FUNCTION("xegFfZKBVlw", "libSceHttp", 1, "libSceHttp", sceHttpSetSendTimeOut);
    LIB_FUNCTION("POJ0azHZX3w", "libSceHttp", 1, "libSceHttp", sceHttpSetSocketCreationCallback);
    LIB_FUNCTION("7WcNoAI9Zcw", "libSceHttp", 1, "libSceHttp", sceHttpsFreeCaList);
    LIB_FUNCTION("gcUjwU3fa0M", "libSceHttp", 1, "libSceHttp", sceHttpsGetCaList);
    LIB_FUNCTION("JBN6N-EY+3M", "libSceHttp", 1, "libSceHttp", sceHttpsGetSslError);
    LIB_FUNCTION("DK+GoXCNT04", "libSceHttp", 1, "libSceHttp", sceHttpsLoadCert);
    LIB_FUNCTION("jUjp+yqMNdQ", "libSceHttp", 1, "libSceHttp", sceHttpsSetMinSslVersion);
    LIB_FUNCTION("htyBOoWeS58", "libSceHttp", 1, "libSceHttp", sceHttpsSetSslCallback);
    LIB_FUNCTION("U5ExQGyyx9s", "libSceHttp", 1, "libSceHttp", sceHttpsSetSslVersion);
    LIB_FUNCTION("zXqcE0fizz0", "libSceHttp", 1, "libSceHttp", sceHttpsUnloadCert);
    LIB_FUNCTION("Ik-KpLTlf7Q", "libSceHttp", 1, "libSceHttp", sceHttpTerm);
    LIB_FUNCTION("V-noPEjSB8c", "libSceHttp", 1, "libSceHttp", sceHttpTryGetNonblock);
    LIB_FUNCTION("fmOs6MzCRqk", "libSceHttp", 1, "libSceHttp", sceHttpTrySetNonblock);
    LIB_FUNCTION("59tL1AQBb8U", "libSceHttp", 1, "libSceHttp", sceHttpUnsetEpoll);
    LIB_FUNCTION("5LZA+KPISVA", "libSceHttp", 1, "libSceHttp", sceHttpUriBuild);
    LIB_FUNCTION("CR-l-yI-o7o", "libSceHttp", 1, "libSceHttp", sceHttpUriCopy);
    LIB_FUNCTION("YuOW3dDAKYc", "libSceHttp", 1, "libSceHttp", sceHttpUriEscape);
    LIB_FUNCTION("3lgQ5Qk42ok", "libSceHttp", 1, "libSceHttp", sceHttpUriMerge);
    LIB_FUNCTION("IWalAn-guFs", "libSceHttp", 1, "libSceHttp", sceHttpUriParse);
    LIB_FUNCTION("mUU363n4yc0", "libSceHttp", 1, "libSceHttp", sceHttpUriSweepPath);
    LIB_FUNCTION("thTS+57zoLM", "libSceHttp", 1, "libSceHttp", sceHttpUriUnescape);
    LIB_FUNCTION("qISjDHrxONc", "libSceHttp", 1, "libSceHttp", sceHttpWaitRequest);
};

} // namespace Libraries::Http
