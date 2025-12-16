// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>
#include <string>
#include <future>

#include <httplib.h>

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

enum OrbisHttpRequestMethod : s32 {
    ORBIS_HTTP_REQUEST_METHOD_INVALID = -1,
    ORBIS_HTTP_REQUEST_METHOD_GET = 0,
    ORBIS_HTTP_REQUEST_METHOD_POST = 1,
};

class RequestTemplate {
public:
    int id;
    std::map<std::string, std::string> headers;
    std::string user_agent = {};
    bool is_async = false;

    RequestTemplate() : id(0) {}
    explicit RequestTemplate(int tmpl_id, std::string user_agent = "") : id(tmpl_id), user_agent(user_agent) {}
};

class RequestObj {
public:
    int id;
    RequestTemplate* req_template = nullptr;

    char* result_body = nullptr;
    u32 result_read_chunk_index = 0;
    u32 result_body_size = 0;

    void SendRequest() {
        request_future = std::async(std::launch::async, [this] { _SendRequest(); });
        
        if (!req_template->is_async) {
            WaitForRequest();
        }
    }

    bool IsRequestComplete() {
        if (!request_future.valid())
            return true;
        return request_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void WaitForRequest() {
        if (request_future.valid()) {
            request_future.get();
        }
    }

    u64 GetContentLength() const {
        return static_cast<u64>(result_body_size);
    }

    bool IsSent() const {
        return status_code != -1;
    }

    void SetUrl(const std::string& new_url) {

        //TODO checks
        url = new_url;

        size_t scheme_end = url.find("://");
        size_t path_start = url.find('/', scheme_end + 3);

        if (path_start == std::string::npos) {
            host = url;
            path = "/";
        } else {
            host = url.substr(0, path_start);
            path = url.substr(path_start);
        }
    }

    void SetPostData(const void* data, u64 size) {
        
        post_data_size = size;

        post_data = new u8[post_data_size];
        std::memcpy(post_data, data, post_data_size);
    }

    void AddHeader(const char* name, const char* value) {

        req_headers[std::string(name)] = std::string(value);
    }

    bool IsSuccessful() const {
        
        return status_code / 100 == 2;
    }

    u32 GetStatusCode() const {
        
        return status_code;
    }

    RequestObj()
        : id(0), req_template(nullptr), method(ORBIS_HTTP_REQUEST_METHOD_INVALID), url(""),
          content_length(0), status_code(-1), result_body(nullptr), result_body_size(-1),
          result_read_chunk_index(0), post_data(nullptr) {}
    explicit RequestObj(s32 req_id, RequestTemplate* req_template, s32 method, std::string url_str, u64 cntLen)
        : id(req_id), req_template(req_template), method(static_cast<OrbisHttpRequestMethod>(method)),
          content_length(cntLen), 
          status_code(-1), result_body(nullptr), result_body_size(-1),
          result_read_chunk_index(0), post_data(nullptr) {

        SetUrl(url_str);
    }

private:
    std::future<void> request_future = {};

    OrbisHttpRequestMethod method = ORBIS_HTTP_REQUEST_METHOD_INVALID;
    std::string host = {};
    std::string path = {};
    u64 content_length = 0;

    u32 status_code = -1; // check
    
    std::map<std::string, std::string> req_headers;
    std::string url = {};
    void* post_data = nullptr;
    u64 post_data_size = 0;

    void _SendRequest() {

        httplib::Client cli(host);

        httplib::Result response = {};

        auto templ_headers = req_template->headers;

        httplib::Headers headers;
        for (const auto& pair : templ_headers) {
            headers.emplace(pair.first, pair.second);
        }

        for (const auto& pair : req_headers) {
            headers.emplace(pair.first, pair.second);
        }

        switch (method) {
        case ORBIS_HTTP_REQUEST_METHOD_GET:

            response = cli.Get(path, headers);
            break;
        case ORBIS_HTTP_REQUEST_METHOD_POST:

            response = cli.Post(path, headers, static_cast<char*>(post_data),
                                static_cast<size_t>(post_data_size), "application/octet-stream");
            break;

        default:
        case ORBIS_HTTP_REQUEST_METHOD_INVALID:
            LOG_ERROR(Lib_Http, "Invalid HTTP method");
            return;
        }

        if (!response) {

            return;
        }

        status_code = response->status;

        if (response && response->status / 100 == 2) {

            result_body_size = static_cast<u32>(response->body.size());
            result_body = new char[result_body_size];
            std::memcpy(result_body, response->body.data(), result_body_size);
        }
    }
};

struct HttpRequestInternal {
    int state;          // +0x20
    int errorCode;      // +0x28
    int httpStatusCode; // +0x20C
    std::mutex m_mutex;
};
using OrbisHttpsCaList = Libraries::Ssl::OrbisSslCaList;

int PS4_SYSV_ABI sceHttpAbortRequest();
int PS4_SYSV_ABI sceHttpAbortRequestForce();
int PS4_SYSV_ABI sceHttpAbortWaitRequest();
int PS4_SYSV_ABI sceHttpAddCookie();
int PS4_SYSV_ABI sceHttpAddQuery();
int PS4_SYSV_ABI sceHttpAddRequestHeader(int id, const char* name, const char* value, s32 mode);
int PS4_SYSV_ABI sceHttpAddRequestHeaderRaw();
int PS4_SYSV_ABI sceHttpAuthCacheExport();
int PS4_SYSV_ABI sceHttpAuthCacheFlush();
int PS4_SYSV_ABI sceHttpAuthCacheImport();
int PS4_SYSV_ABI sceHttpCacheRedirectedConnectionEnabled();
int PS4_SYSV_ABI sceHttpCookieExport();
int PS4_SYSV_ABI sceHttpCookieFlush();
int PS4_SYSV_ABI sceHttpCookieImport();
int PS4_SYSV_ABI sceHttpCreateConnection();
int PS4_SYSV_ABI sceHttpCreateConnectionWithURL(int tmplId, const char* url, bool enableKeepalive);
int PS4_SYSV_ABI sceHttpCreateEpoll();
int PS4_SYSV_ABI sceHttpCreateRequest();
int PS4_SYSV_ABI sceHttpCreateRequest2();
int PS4_SYSV_ABI sceHttpCreateRequestWithURL(s32 conn_id, s32 method, const char* url,
                                             u64 content_length);
int PS4_SYSV_ABI sceHttpCreateRequestWithURL2();
int PS4_SYSV_ABI sceHttpCreateTemplate(s32 conn_id, const char* user_agent, s32 http_v, s32 flags);
int PS4_SYSV_ABI sceHttpDbgEnableProfile();
int PS4_SYSV_ABI sceHttpDbgGetConnectionStat();
int PS4_SYSV_ABI sceHttpDbgGetRequestStat();
int PS4_SYSV_ABI sceHttpDbgSetPrintf();
int PS4_SYSV_ABI sceHttpDbgShowConnectionStat();
int PS4_SYSV_ABI sceHttpDbgShowMemoryPoolStat();
int PS4_SYSV_ABI sceHttpDbgShowRequestStat();
int PS4_SYSV_ABI sceHttpDbgShowStat();
int PS4_SYSV_ABI sceHttpDeleteConnection();
int PS4_SYSV_ABI sceHttpDeleteRequest(s32 req_id);
int PS4_SYSV_ABI sceHttpDeleteTemplate();
int PS4_SYSV_ABI sceHttpDestroyEpoll();
int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled();
int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int reqId, char** header, u64* headerSize);
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
int PS4_SYSV_ABI sceHttpGetResponseContentLength(u32 req_id, u64* out_content_length, u32* _flag);
int PS4_SYSV_ABI sceHttpGetStatusCode(s32 req_id, s32* status_code);
int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, u64 poolSize);
int PS4_SYSV_ABI sceHttpParseResponseHeader(const char* header, u64 headerLen, const char* fieldStr,
                                            const char** fieldValue, u64* valueLen);
int PS4_SYSV_ABI sceHttpParseStatusLine(const char* statusLine, u64 lineLen, int32_t* httpMajorVer,
                                        int32_t* httpMinorVer, int32_t* responseCode,
                                        const char** reasonPhrase, u64* phraseLen);
int PS4_SYSV_ABI sceHttpReadData(u32 req_id, char* dest, u32 size);
int PS4_SYSV_ABI sceHttpRedirectCacheFlush();
int PS4_SYSV_ABI sceHttpRemoveRequestHeader();
int PS4_SYSV_ABI sceHttpRequestGetAllHeaders();
int PS4_SYSV_ABI sceHttpsDisableOption();
int PS4_SYSV_ABI sceHttpsDisableOptionPrivate();
int PS4_SYSV_ABI sceHttpsEnableOption(u32 options);
int PS4_SYSV_ABI sceHttpsEnableOptionPrivate();
int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size);
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
int PS4_SYSV_ABI sceHttpSetNonblock(s32 tmpl_id, bool enable);
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
int PS4_SYSV_ABI sceHttpsGetCaList(int httpCtxId, OrbisHttpsCaList* list);
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
int PS4_SYSV_ABI sceHttpUriBuild(char* out, u64* require, u64 prepare,
                                 const OrbisHttpUriElement* srcElement, u32 option);
int PS4_SYSV_ABI sceHttpUriCopy();
int PS4_SYSV_ABI sceHttpUriEscape(char* out, u64* require, u64 prepare, const char* in);
int PS4_SYSV_ABI sceHttpUriMerge(char* mergedUrl, char* url, char* relativeUri, u64* require,
                                 u64 prepare, u32 option);
int PS4_SYSV_ABI sceHttpUriParse(OrbisHttpUriElement* out, const char* srcUri, void* pool,
                                 u64* require, u64 prepare);
int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, u64 srcSize);
int PS4_SYSV_ABI sceHttpUriUnescape(char* out, u64* require, u64 prepare, const char* in);
int PS4_SYSV_ABI sceHttpWaitRequest();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http
