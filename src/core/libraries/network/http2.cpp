// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http2.h"

#include <map>

namespace Libraries::Http2 {

std::map<s32, s32> requests_to_connections{};

s32 PS4_SYSV_ABI sceHttp2AbortRequest(s32 req_id) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpAbortRequest(req_id);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to get abort HTTP request, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2AddCookie() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2AddRequestHeader(s32 template_or_req_id, const char* name,
                                          const char* value, u32 mode) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpAddRequestHeader(template_or_req_id, name, value, mode);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to add HTTP response header, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2AuthCacheFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2CookieExport() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2CookieFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2CookieImport() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2CreateCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2CreateRequestWithURL(s32 tmpl_id, const char* method, const char* url,
                                              u64 content_length) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    // Http1 has a connection thing to go through first
    s32 conn_id = Libraries::Http::sceHttpCreateConnectionWithURL(tmpl_id, url, true);
    if (conn_id < 0) {
        LOG_ERROR(Lib_Http2, "Failed to create HTTP connection, error = {:#x}", conn_id);
        return conn_id;
    }
    s32 req_id =
        Libraries::Http::sceHttpCreateRequestWithURL2(conn_id, method, url, content_length);
    if (req_id < 0) {
        LOG_ERROR(Lib_Http2, "Failed to create HTTP request, error = {:#x}", req_id);
        return req_id;
    }
    requests_to_connections[req_id] = conn_id;
    return req_id;
}

s32 PS4_SYSV_ABI sceHttp2CreateTemplate(s32 ctx_id, const char* user_agent, s32 http_ver,
                                        s32 auto_proxy_conf) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 tmpl_id =
        Libraries::Http::sceHttpCreateTemplate(ctx_id, user_agent, http_ver, auto_proxy_conf);
    if (tmpl_id < 0) {
        LOG_ERROR(Lib_Http2, "Failed to create HTTP template, error = {:#x}", tmpl_id);
    }
    return tmpl_id;
}

s32 PS4_SYSV_ABI sceHttp2DeleteCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2DeleteRequest(s32 req_id) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpDeleteRequest(req_id);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to delete HTTP request, error = {:#x}", result);
        return result;
    }
    s32 conn_id = requests_to_connections[req_id];
    result = Libraries::Http::sceHttpDeleteConnection(conn_id);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to delete HTTP connection, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2DeleteTemplate(s32 tmpl_id) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpDeleteTemplate(tmpl_id);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to delete HTTP template, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2GetAllResponseHeaders(s32 req_id, char** header, u64* header_size) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpGetAllResponseHeaders(req_id, header, header_size);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to get HTTP response headers, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2GetAuthEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetAutoRedirect() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetCookie() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetCookieStats() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetMemoryPoolStats() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetResponseContentLength() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2GetStatusCode(s32 req_id, s32* status_code) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpGetStatusCode(req_id, status_code);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to get HTTP status code, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2Init(s32 net_id, s32 ssl_id, u64 pool_size, s32 max_requests) {
    LOG_ERROR(Lib_Http2, "(DUMMY) called");
    s32 ctx_id = Libraries::Http::sceHttpInit(net_id, ssl_id, pool_size);
    if (ctx_id < 0) {
        LOG_ERROR(Lib_Http2, "Failed to init HTTP context, error = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 PS4_SYSV_ABI sceHttp2ReadData(s32 req_id, void* data, u64 size) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpReadData(req_id, data, size);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to read HTTP data, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2ReadDataAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2RedirectCacheFlush() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2RemoveRequestHeader() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SendRequest(s32 req_id, const void* data, u64 size) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpSendRequest(req_id, data, size);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to send HTTP request, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2SendRequestAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetAuthEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetAuthInfoCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetAutoRedirect() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetConnectionWaitTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetConnectTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieBox() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieMaxNum() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieMaxNumPerDomain() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieMaxSize() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieRecvCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetCookieSendCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetInflateGZIPEnabled() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetMinSslVersion() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetPreSendCallback(s32 template_id, OrbisHttp2PreSendCallback cb_func,
                                            void* user_arg) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetRecvTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetRedirectCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetRequestContentLength(s32 req_id, u64 content_length) {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    s32 result = Libraries::Http::sceHttpSetRequestContentLength(req_id, content_length);
    if (result < 0) {
        LOG_ERROR(Lib_Http2, "Failed to set HTTP request content length, error = {:#x}", result);
    }
    return result;
}

s32 PS4_SYSV_ABI sceHttp2SetResolveRetry() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetResolveTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetSendTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetSslCallback() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SetTimeOut() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SslDisableOption() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2SslEnableOption() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2Term() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHttp2WaitAsync() {
    LOG_ERROR(Lib_Http2, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("IZ-qjhRqvjk", "libSceHttp2", 1, "libSceHttp2", sceHttp2AbortRequest);
    LIB_FUNCTION("flPxnowtvWY", "libSceHttp2", 1, "libSceHttp2", sceHttp2AddCookie);
    LIB_FUNCTION("nrPfOE8TQu0", "libSceHttp2", 1, "libSceHttp2", sceHttp2AddRequestHeader);
    LIB_FUNCTION("WeuDjj5m4YU", "libSceHttp2", 1, "libSceHttp2", sceHttp2AuthCacheFlush);
    LIB_FUNCTION("JlFGR4v50Kw", "libSceHttp2", 1, "libSceHttp2", sceHttp2CookieExport);
    LIB_FUNCTION("5VlQSzXW-SQ", "libSceHttp2", 1, "libSceHttp2", sceHttp2CookieFlush);
    LIB_FUNCTION("B5ibZI5UlzU", "libSceHttp2", 1, "libSceHttp2", sceHttp2CookieImport);
    LIB_FUNCTION("N4UfjvWJsMw", "libSceHttp2", 1, "libSceHttp2", sceHttp2CreateCookieBox);
    LIB_FUNCTION("mmyOCxQMVYQ", "libSceHttp2", 1, "libSceHttp2", sceHttp2CreateRequestWithURL);
    LIB_FUNCTION("+wCt7fCijgk", "libSceHttp2", 1, "libSceHttp2", sceHttp2CreateTemplate);
    LIB_FUNCTION("O9ync3F-JVI", "libSceHttp2", 1, "libSceHttp2", sceHttp2DeleteCookieBox);
    LIB_FUNCTION("c8D9qIjo8EY", "libSceHttp2", 1, "libSceHttp2", sceHttp2DeleteRequest);
    LIB_FUNCTION("pDom5-078DA", "libSceHttp2", 1, "libSceHttp2", sceHttp2DeleteTemplate);
    LIB_FUNCTION("-rdXUi2XW90", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetAllResponseHeaders);
    LIB_FUNCTION("m-OL13q8AI8", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetAuthEnabled);
    LIB_FUNCTION("od5QCZhZSfw", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetAutoRedirect);
    LIB_FUNCTION("GQFGj0rYX+A", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetCookie);
    LIB_FUNCTION("IX23slKvtQI", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetCookieBox);
    LIB_FUNCTION("eij7UzkUqK8", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetCookieStats);
    LIB_FUNCTION("otUQuZa-mv0", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetMemoryPoolStats);
    LIB_FUNCTION("o0DBQpFE13o", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetResponseContentLength);
    LIB_FUNCTION("9XYJwCf3lEA", "libSceHttp2", 1, "libSceHttp2", sceHttp2GetStatusCode);
    LIB_FUNCTION("3JCe3lCbQ8A", "libSceHttp2", 1, "libSceHttp2", sceHttp2Init);
    LIB_FUNCTION("QygCNNmbGss", "libSceHttp2", 1, "libSceHttp2", sceHttp2ReadData);
    LIB_FUNCTION("bGN-6zbo7ms", "libSceHttp2", 1, "libSceHttp2", sceHttp2ReadDataAsync);
    LIB_FUNCTION("klwUy2Wg+q8", "libSceHttp2", 1, "libSceHttp2", sceHttp2RedirectCacheFlush);
    LIB_FUNCTION("jHdP0CS4ZlA", "libSceHttp2", 1, "libSceHttp2", sceHttp2RemoveRequestHeader);
    LIB_FUNCTION("rbqZig38AT8", "libSceHttp2", 1, "libSceHttp2", sceHttp2SendRequest);
    LIB_FUNCTION("A+NVAFu4eCg", "libSceHttp2", 1, "libSceHttp2", sceHttp2SendRequestAsync);
    LIB_FUNCTION("jjFahkBPCYs", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetAuthEnabled);
    LIB_FUNCTION("Wwj6HbB2mOo", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetAuthInfoCallback);
    LIB_FUNCTION("b9AvoIaOuHI", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetAutoRedirect);
    LIB_FUNCTION("n8hMLe31OPA", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetConnectionWaitTimeOut);
    LIB_FUNCTION("-HIO4VT87v8", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetConnectTimeOut);
    LIB_FUNCTION("jrVHsKCXA0g", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieBox);
    LIB_FUNCTION("mPKVhQqh2Es", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieMaxNum);
    LIB_FUNCTION("o7+WXe4WadE", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieMaxNumPerDomain);
    LIB_FUNCTION("6a0N6GPD7RM", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieMaxSize);
    LIB_FUNCTION("zdtXKn9X7no", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieRecvCallback);
    LIB_FUNCTION("McYmUpQ3-DY", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetCookieSendCallback);
    LIB_FUNCTION("uRosf8GQbHQ", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetInflateGZIPEnabled);
    LIB_FUNCTION("09tk+kIA1Ns", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetMinSslVersion);
    LIB_FUNCTION("UL4Fviw+IAM", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetPreSendCallback);
    LIB_FUNCTION("izvHhqgDt44", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetRecvTimeOut);
    LIB_FUNCTION("BJgi0CH7al4", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetRedirectCallback);
    LIB_FUNCTION("FSAFOzi0FpM", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetRequestContentLength);
    LIB_FUNCTION("Gcjh+CisAZM", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetResolveRetry);
    LIB_FUNCTION("ACjtE27aErY", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetResolveTimeOut);
    LIB_FUNCTION("XPtW45xiLHk", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetSendTimeOut);
    LIB_FUNCTION("YrWX+DhPHQY", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetSslCallback);
    LIB_FUNCTION("VYMxTcBqSE0", "libSceHttp2", 1, "libSceHttp2", sceHttp2SetTimeOut);
    LIB_FUNCTION("B37SruheQ5Y", "libSceHttp2", 1, "libSceHttp2", sceHttp2SslDisableOption);
    LIB_FUNCTION("EWcwMpbr5F8", "libSceHttp2", 1, "libSceHttp2", sceHttp2SslEnableOption);
    LIB_FUNCTION("YiBUtz-pGkc", "libSceHttp2", 1, "libSceHttp2", sceHttp2Term);
    LIB_FUNCTION("MOp-AUhdfi8", "libSceHttp2", 1, "libSceHttp2", sceHttp2WaitAsync);
};

} // namespace Libraries::Http2