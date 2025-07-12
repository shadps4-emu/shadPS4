// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http2.h"

namespace Libraries::Http2 {

int PS4_SYSV_ABI _Z5dummyv() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2AbortRequest() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2AddCookie() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2AddRequestHeader() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2AuthCacheFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CookieExport() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CookieFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CookieImport() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CreateCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CreateRequestWithURL() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2CreateTemplate() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2DeleteCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2DeleteRequest() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2DeleteTemplate() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetAllResponseHeaders() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetAuthEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetAutoRedirect() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetCookie() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetCookieStats() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetMemoryPoolStats() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetResponseContentLength() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2GetStatusCode() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2Init(int net_id, int ssl_id, size_t pool_size, int max_requests) {
    LOG_ERROR(Lib_Http2, "(DUMMY) called");
    static int id = 0;
    return ++id;
}

int PS4_SYSV_ABI sceHttp2ReadData() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2ReadDataAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2RedirectCacheFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2RemoveRequestHeader() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SendRequest() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SendRequestAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetAuthEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetAuthInfoCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetAutoRedirect() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetConnectionWaitTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetConnectTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieMaxNum() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieMaxNumPerDomain() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieMaxSize() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieRecvCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetCookieSendCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetInflateGZIPEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetMinSslVersion() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetPreSendCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetRecvTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetRedirectCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetRequestContentLength() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetResolveRetry() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetResolveTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetSendTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetSslCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SetTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SslDisableOption() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2SslEnableOption() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2Term() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttp2WaitAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("AS45QoYHjc4", "libSceHttp2", 1, "libSceHttp2", 1, 1, _Z5dummyv);
    LIB_FUNCTION("IZ-qjhRqvjk", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2AbortRequest);
    LIB_FUNCTION("flPxnowtvWY", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2AddCookie);
    LIB_FUNCTION("nrPfOE8TQu0", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2AddRequestHeader);
    LIB_FUNCTION("WeuDjj5m4YU", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2AuthCacheFlush);
    LIB_FUNCTION("JlFGR4v50Kw", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2CookieExport);
    LIB_FUNCTION("5VlQSzXW-SQ", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2CookieFlush);
    LIB_FUNCTION("B5ibZI5UlzU", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2CookieImport);
    LIB_FUNCTION("N4UfjvWJsMw", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2CreateCookieBox);
    LIB_FUNCTION("mmyOCxQMVYQ", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2CreateRequestWithURL);
    LIB_FUNCTION("+wCt7fCijgk", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2CreateTemplate);
    LIB_FUNCTION("O9ync3F-JVI", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2DeleteCookieBox);
    LIB_FUNCTION("c8D9qIjo8EY", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2DeleteRequest);
    LIB_FUNCTION("pDom5-078DA", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2DeleteTemplate);
    LIB_FUNCTION("-rdXUi2XW90", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2GetAllResponseHeaders);
    LIB_FUNCTION("m-OL13q8AI8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetAuthEnabled);
    LIB_FUNCTION("od5QCZhZSfw", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetAutoRedirect);
    LIB_FUNCTION("GQFGj0rYX+A", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetCookie);
    LIB_FUNCTION("IX23slKvtQI", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetCookieBox);
    LIB_FUNCTION("eij7UzkUqK8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetCookieStats);
    LIB_FUNCTION("otUQuZa-mv0", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetMemoryPoolStats);
    LIB_FUNCTION("o0DBQpFE13o", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2GetResponseContentLength);
    LIB_FUNCTION("9XYJwCf3lEA", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2GetStatusCode);
    LIB_FUNCTION("3JCe3lCbQ8A", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2Init);
    LIB_FUNCTION("QygCNNmbGss", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2ReadData);
    LIB_FUNCTION("bGN-6zbo7ms", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2ReadDataAsync);
    LIB_FUNCTION("klwUy2Wg+q8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2RedirectCacheFlush);
    LIB_FUNCTION("jHdP0CS4ZlA", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2RemoveRequestHeader);
    LIB_FUNCTION("rbqZig38AT8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SendRequest);
    LIB_FUNCTION("A+NVAFu4eCg", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SendRequestAsync);
    LIB_FUNCTION("jjFahkBPCYs", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetAuthEnabled);
    LIB_FUNCTION("Wwj6HbB2mOo", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetAuthInfoCallback);
    LIB_FUNCTION("b9AvoIaOuHI", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetAutoRedirect);
    LIB_FUNCTION("n8hMLe31OPA", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetConnectionWaitTimeOut);
    LIB_FUNCTION("-HIO4VT87v8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetConnectTimeOut);
    LIB_FUNCTION("jrVHsKCXA0g", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetCookieBox);
    LIB_FUNCTION("mPKVhQqh2Es", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetCookieMaxNum);
    LIB_FUNCTION("o7+WXe4WadE", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetCookieMaxNumPerDomain);
    LIB_FUNCTION("6a0N6GPD7RM", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetCookieMaxSize);
    LIB_FUNCTION("zdtXKn9X7no", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetCookieRecvCallback);
    LIB_FUNCTION("McYmUpQ3-DY", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetCookieSendCallback);
    LIB_FUNCTION("uRosf8GQbHQ", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetInflateGZIPEnabled);
    LIB_FUNCTION("09tk+kIA1Ns", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetMinSslVersion);
    LIB_FUNCTION("UL4Fviw+IAM", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetPreSendCallback);
    LIB_FUNCTION("izvHhqgDt44", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetRecvTimeOut);
    LIB_FUNCTION("BJgi0CH7al4", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetRedirectCallback);
    LIB_FUNCTION("FSAFOzi0FpM", "libSceHttp2", 1, "libSceHttp2", 1, 1,
                 sceHttp2SetRequestContentLength);
    LIB_FUNCTION("Gcjh+CisAZM", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetResolveRetry);
    LIB_FUNCTION("ACjtE27aErY", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetResolveTimeOut);
    LIB_FUNCTION("XPtW45xiLHk", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetSendTimeOut);
    LIB_FUNCTION("YrWX+DhPHQY", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetSslCallback);
    LIB_FUNCTION("VYMxTcBqSE0", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SetTimeOut);
    LIB_FUNCTION("B37SruheQ5Y", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SslDisableOption);
    LIB_FUNCTION("EWcwMpbr5F8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2SslEnableOption);
    LIB_FUNCTION("YiBUtz-pGkc", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2Term);
    LIB_FUNCTION("MOp-AUhdfi8", "libSceHttp2", 1, "libSceHttp2", 1, 1, sceHttp2WaitAsync);
};

} // namespace Libraries::Http2