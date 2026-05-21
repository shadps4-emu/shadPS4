// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "http_error.h"

namespace Libraries::Http {

enum class HttpRequestState {
    Created,
    Sending,
    Sent,
    Aborted,
};

struct HttpSettings {
    u32 connect_timeout_us = 0;
    u32 send_timeout_us = 0;
    u32 recv_timeout_us = 0;
    bool auto_redirect = true;
    bool inflate_gzip = true;
    u32 ssl_flags = ORBIS_HTTPS_FLAG_SDK_DEFAULT; // SSL flag mask. Bitmask of OrbisHttpsFlags.
    bool nonblock = false; // false = blocking (default), true = nonblock (EAGAIN)
};

struct Epoll {
    int ctx_id = 0;
    std::deque<OrbisHttpNBEvent> events;
    std::condition_variable cv;
    bool destroyed = false;
    bool abort_requested = false;
};

struct HttpTemplate {
    std::string user_agent;
    int http_version;
    int auto_proxy_conf;
    HttpSettings settings;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
};

struct HttpConnection {
    int tmpl_id;             // Owning template (looked up to inherit user_agent etc.)
    std::string url;         // Reconstructed "scheme://host:port" or the raw URL the
                             // game gave us via sceHttpCreateConnectionWithURL.
    std::string scheme;      // "http" or "https".
    std::string hostname;    // Host portion only. Used for the Host: request header.
    u16 port = 0;            // Numeric port. 80 / 443 if scheme-default.
    bool keep_alive = false; // Game-controlled keep-alive intent.
    bool is_secure = false;  // True if scheme == "https".
    HttpSettings settings;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
};

struct HttpResponse {
    int status_code = 0;
    u64 content_length = 0;
    int content_length_result = ORBIS_HTTP_ERROR_NO_CONTENT_LENGTH;
    std::vector<u8> body;
    u64 read_cursor = 0;
    std::string all_headers_blob; // Pre-formatted "Name: Value\r\n..." string.
};

struct HttpRequest {
    int conn_id = 0;
    int method = 0;
    std::string method_str;
    std::string url;
    u64 content_length = 0;
    HttpRequestState state = HttpRequestState::Created;
    bool deleted = false;
    s32 last_errno = 0; // populated by SendRequest, read by GetLastErrno
    HttpSettings settings;
    HttpResponse res;
    std::condition_variable cv; // waiters in blocking getters block on this
                                // notified when state leaves Sending.
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
};

struct HttpState {
    std::mutex m_mutex;
    bool inited = false;
    int next_ctx_id = 0;
    int next_obj_id = 0;
    std::unordered_set<int> active_contexts;
    std::unordered_map<int, HttpTemplate> templates;
    std::unordered_map<int, HttpConnection> connections;
    std::unordered_map<int, std::shared_ptr<HttpRequest>> requests;
    std::unordered_map<int, std::shared_ptr<Epoll>> epolls;
    std::atomic<bool> shutting_down{false};
};

static HttpState g_state;

//***********************************
// Helper functions
//***********************************

static HttpSettings* ResolveSettings(int id, const char*& level) {
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        level = "template";
        return &it->second.settings;
    }
    if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        level = "connection";
        return &it->second.settings;
    }
    if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        level = "request";
        return &it->second->settings;
    }
    level = "";
    return nullptr;
}

static OrbisHttpEpollHandle EncodeEpollHandle(int id) {
    return reinterpret_cast<OrbisHttpEpollHandle>(static_cast<intptr_t>(id));
}

static int DecodeEpollHandle(OrbisHttpEpollHandle eh) {
    return static_cast<int>(reinterpret_cast<intptr_t>(eh));
}

// Resolve the (epoll_id*, epoll_user_arg*) pair on a template/connection/request
static bool ResolveEpollBinding(int id, int*& epoll_id_out, void**& user_arg_out,
                                const char*& level) {
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        epoll_id_out = &it->second.epoll_id;
        user_arg_out = &it->second.epoll_user_arg;
        level = "template";
        return true;
    }
    if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        epoll_id_out = &it->second.epoll_id;
        user_arg_out = &it->second.epoll_user_arg;
        level = "connection";
        return true;
    }
    if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        epoll_id_out = &it->second->epoll_id;
        user_arg_out = &it->second->epoll_user_arg;
        level = "request";
        return true;
    }
    return false;
}

// Populate a response object with the shape a transport-level failure produces:
// no status line, no headers, no body. Used by the no-internet path.
static void SynthesizeTransportFailureResponse(HttpResponse& res) {
    res.status_code = 0;
    res.content_length = 0;
    res.content_length_result = ORBIS_HTTP_ERROR_NO_CONTENT_LENGTH;
    res.body.clear();
    res.read_cursor = 0;
    res.all_headers_blob.clear();
}

// Map common HTTP status codes to strings for logs.
static std::string HttpStatusLabel(int sc) {
    switch (sc) {
    case 0:
        return "0 (no status)";
    case 200:
        return "200 OK";
    case 204:
        return "204 No Content";
    case 301:
        return "301 Moved Permanently";
    case 302:
        return "302 Found";
    case 304:
        return "304 Not Modified";
    case 400:
        return "400 Bad Request";
    case 401:
        return "401 Unauthorized";
    case 403:
        return "403 Forbidden";
    case 404:
        return "404 Not Found";
    case 500:
        return "500 Internal Server Error";
    case 502:
        return "502 Bad Gateway";
    case 503:
        return "503 Service Unavailable";
    default:
        return std::to_string(sc);
    }
}

static int WaitForResponseReady(HttpRequest& req, std::unique_lock<std::mutex>& lock) {
    if (req.state == HttpRequestState::Aborted) {
        return ORBIS_HTTP_ERROR_ABORTED;
    }
    if (req.state == HttpRequestState::Created) {
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }
    if (req.state == HttpRequestState::Sent) {
        return ORBIS_OK;
    }
    // state == Sending. Honor nonblock: return EAGAIN instead of blocking.
    if (req.settings.nonblock) {
        return ORBIS_HTTP_ERROR_EAGAIN;
    }
    req.cv.wait(lock, [&req]() {
        return req.state != HttpRequestState::Sending || g_state.shutting_down.load();
    });
    if (g_state.shutting_down.load() || req.state == HttpRequestState::Aborted) {
        return ORBIS_HTTP_ERROR_ABORTED;
    }
    return ORBIS_OK;
}

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

// Returns ORBIS_OK on success or one of {INVALID_VALUE, UNKNOWN_SCHEME}.
static int CheckScheme(const char* scheme, bool& is_secure) {
    if (!scheme) {
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto match_ci = [](const char* s, const char* lit, size_t cap) -> bool {
        for (size_t i = 0; i < cap; ++i) {
            const char a = s[i];
            const char b = lit[i];
            if (a == '\0' && b == '\0')
                return true;
            if (a == '\0' || b == '\0')
                return false;
            const char fa = (a >= 'A' && a <= 'Z') ? char(a - 'A' + 'a') : a;
            const char fb = (b >= 'A' && b <= 'Z') ? char(b - 'A' + 'a') : b;
            if (fa != fb)
                return false;
        }
        return s[cap] == '\0' && lit[cap] == '\0';
    };
    if (match_ci(scheme, "HTTP", 0x20)) {
        is_secure = false;
        return ORBIS_OK;
    }
    if (match_ci(scheme, "HTTPS", 0x20)) {
        is_secure = true;
        return ORBIS_OK;
    }
    return ORBIS_HTTP_ERROR_UNKNOWN_SCHEME;
}

static bool ContainsCrLf(const char* s) {
    if (!s)
        return false;
    for (; *s; ++s) {
        if (*s == '\r' || *s == '\n')
            return true;
    }
    return false;
}

//***********************************
// TODO/WIP/Stubbed functions
//***********************************

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
    LOG_INFO(Lib_Http, "called tmplId={}, serverName={}, scheme={}, port={}, isEnableKeepalive={}",
             tmplId, serverName ? serverName : "(null)", scheme ? scheme : "(null)", port,
             isEnableKeepalive);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.templates.contains(tmplId)) {
        LOG_ERROR(Lib_Http, "Invalid tmplId={}", tmplId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    if (!serverName) {
        LOG_ERROR(Lib_Http, "serverName is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    bool is_secure = false;
    if (int sc = CheckScheme(scheme, is_secure); sc < 0) {
        LOG_ERROR(Lib_Http, "scheme rejected: '{}' -> {:#x}", scheme ? scheme : "(null)", sc);
        return sc;
    }

    const std::string scheme_str = is_secure ? "https" : "http";
    const int conn_id = ++g_state.next_obj_id;
    HttpConnection conn;
    conn.tmpl_id = tmplId;
    conn.scheme = scheme_str;
    conn.hostname = serverName;
    conn.port = port;
    conn.keep_alive = (isEnableKeepalive != 0);
    conn.is_secure = is_secure;
    conn.url = scheme_str + "://" + serverName + ":" + std::to_string(port);
    if (auto tmpl_it = g_state.templates.find(tmplId); tmpl_it != g_state.templates.end()) {
        conn.settings = tmpl_it->second.settings;
        conn.epoll_id = tmpl_it->second.epoll_id;
        conn.epoll_user_arg = tmpl_it->second.epoll_user_arg;
    }
    g_state.connections.emplace(conn_id, std::move(conn));
    LOG_INFO(Lib_Http, "created connection connId={} url={}", conn_id,
             g_state.connections[conn_id].url);
    return conn_id;
}

int PS4_SYSV_ABI sceHttpCreateConnectionWithURL(int tmplId, const char* url, bool enableKeepalive) {
    LOG_INFO(Lib_Http, "called tmplId={}, url={}, enableKeepalive={}", tmplId, url ? url : "(null)",
             enableKeepalive);
    {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (!g_state.inited) {
            LOG_ERROR(Lib_Http, "Not initialized");
            return ORBIS_HTTP_ERROR_BEFORE_INIT;
        }
        if (!g_state.templates.contains(tmplId)) {
            LOG_ERROR(Lib_Http, "Invalid tmplId={}", tmplId);
            return ORBIS_HTTP_ERROR_INVALID_ID;
        }
    }
    if (!url) {
        LOG_ERROR(Lib_Http, "url is null");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }

    u64 required = 0;
    int parse_ret = sceHttpUriParse(nullptr, url, nullptr, &required, 0);
    if (parse_ret < 0) {
        LOG_ERROR(Lib_Http, "URI pre-parse failed: {:#x}", parse_ret);
        return parse_ret; // already an ORBIS_HTTP_ERROR_*
    }
    std::vector<char> pool(required);
    OrbisHttpUriElement parsed{};
    parse_ret = sceHttpUriParse(&parsed, url, pool.data(), &required, required);
    if (parse_ret < 0) {
        LOG_ERROR(Lib_Http, "URI parse failed: {:#x}", parse_ret);
        return parse_ret;
    }

    std::string scheme_str = parsed.scheme ? parsed.scheme : "";
    bool is_secure = false;
    if (int sc = CheckScheme(parsed.scheme, is_secure); sc < 0) {
        LOG_ERROR(Lib_Http, "URL scheme rejected: '{}' -> {:#x}", scheme_str, sc);
        return sc;
    }
    if (!parsed.hostname || !parsed.hostname[0]) {
        LOG_ERROR(Lib_Http, "URL has no hostname");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }

    scheme_str = is_secure ? "https" : "http";
    u16 port = parsed.port;

    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized (raced with Term?)");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.templates.contains(tmplId)) {
        LOG_ERROR(Lib_Http, "Invalid tmplId={} (raced with DeleteTemplate?)", tmplId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    int conn_id = ++g_state.next_obj_id;
    HttpConnection conn;
    conn.tmpl_id = tmplId;
    conn.url = url;
    conn.scheme = scheme_str;
    conn.hostname = parsed.hostname;
    conn.port = port;
    conn.keep_alive = enableKeepalive;
    conn.is_secure = is_secure;
    if (auto tmpl_it = g_state.templates.find(tmplId); tmpl_it != g_state.templates.end()) {
        conn.settings = tmpl_it->second.settings;
        conn.epoll_id = tmpl_it->second.epoll_id;
        conn.epoll_user_arg = tmpl_it->second.epoll_user_arg;
    }
    g_state.connections.emplace(conn_id, std::move(conn));
    LOG_INFO(Lib_Http, "created connection connId={} host={} port={} scheme={}", conn_id,
             g_state.connections[conn_id].hostname, port, scheme_str);
    return conn_id;
}

int PS4_SYSV_ABI sceHttpCreateRequest(int connId, int method, const char* path, u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, path={}, contentLength={}", connId, method,
             path ? path : "(null)", contentLength);
    std::string url;
    {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (!g_state.inited) {
            LOG_ERROR(Lib_Http, "Not initialized");
            return ORBIS_HTTP_ERROR_BEFORE_INIT;
        }
        auto it = g_state.connections.find(connId);
        if (it == g_state.connections.end()) {
            LOG_ERROR(Lib_Http, "Invalid connId={}", connId);
            return ORBIS_HTTP_ERROR_INVALID_ID;
        }
        if (!path) {
            LOG_ERROR(Lib_Http, "path is null");
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        if (ContainsCrLf(path)) {
            LOG_ERROR(Lib_Http, "path contains CR/LF (CRLF-injection rejected): {}", path);
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        const auto& conn = it->second;
        url = conn.scheme + "://" + conn.hostname + ":" + std::to_string(conn.port);
        if (path[0] != '\0') {
            if (path[0] != '/') {
                url.push_back('/');
            }
            url.append(path);
        }
    }
    return sceHttpCreateRequestWithURL(connId, method, url.c_str(), contentLength);
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL(int connId, s32 method, const char* url,
                                             u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, url={}, contentLength={}", connId, method,
             url ? url : "(null)", contentLength);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto conn_it = g_state.connections.find(connId);
    if (conn_it == g_state.connections.end()) {
        LOG_ERROR(Lib_Http, "Invalid connId={}", connId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (!url) {
        LOG_ERROR(Lib_Http, "url is null");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }

    if (method > 8 || ((0x177u >> (static_cast<u32>(method) & 0x1f)) & 1u) == 0) {
        LOG_ERROR(Lib_Http, "method {} not accepted", method);
        return ORBIS_HTTP_ERROR_UNKNOWN_METHOD;
    }
    const int req_id = ++g_state.next_obj_id;
    auto req = std::make_shared<HttpRequest>();
    req->conn_id = connId;
    req->method = method;
    req->url = url;
    req->content_length = contentLength;
    req->settings = conn_it->second.settings;
    req->epoll_id = conn_it->second.epoll_id;
    req->epoll_user_arg = conn_it->second.epoll_user_arg;
    g_state.requests.emplace(req_id, std::move(req));
    LOG_INFO(Lib_Http, "created request reqId={}", req_id);
    return req_id;
}

int PS4_SYSV_ABI sceHttpCreateTemplate(int libhttpCtxId, const char* userAgent, int httpVer,
                                       int isAutoProxyConf) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, userAgent={}, httpVer={}, isAutoProxyConf={}",
             libhttpCtxId, userAgent ? userAgent : "(null)", httpVer, isAutoProxyConf);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    const int tmpl_id = ++g_state.next_obj_id;
    HttpTemplate tmpl;
    tmpl.user_agent = userAgent ? userAgent : "";
    tmpl.http_version = httpVer;
    tmpl.auto_proxy_conf = isAutoProxyConf;
    g_state.templates.emplace(tmpl_id, std::move(tmpl));
    LOG_INFO(Lib_Http, "created template tmplId={}", tmpl_id);
    return tmpl_id;
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

int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int id, int* isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAuthEnabled(int id, int* isEnable) {
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

int PS4_SYSV_ABI sceHttpGetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetMemoryPoolStats(int libhttpCtxId,
                                           OrbisHttpMemoryPoolStats* currentStat) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}, currentStat={}", libhttpCtxId,
              fmt::ptr(currentStat));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetRegisteredCtxIds() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, u64 poolSize) {
    LOG_INFO(Lib_Http, "called libnetMemId={}, libsslCtxId={}, poolSize={}", libnetMemId,
             libsslCtxId, poolSize);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (poolSize == 0) {
        LOG_ERROR(Lib_Http, "poolSize is zero");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const int ctx_id = ++g_state.next_ctx_id;
    g_state.active_contexts.insert(ctx_id);
    g_state.inited = true; // true while at least one context is alive
    g_state.shutting_down.store(false);
    LOG_INFO(Lib_Http, "initialized -> ctxId={} (active contexts: {})", ctx_id,
             g_state.active_contexts.size());
    return ctx_id;
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

int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size) {
    LOG_INFO(Lib_Http, "called reqId={}, postData={}, size={}", reqId, fmt::ptr(postData), size);
    std::shared_ptr<HttpRequest> req_ptr;
    {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (!g_state.inited) {
            LOG_ERROR(Lib_Http, "Not initialized");
            return ORBIS_HTTP_ERROR_BEFORE_INIT;
        }
        auto it = g_state.requests.find(reqId);
        if (it == g_state.requests.end()) {
            LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
            return ORBIS_HTTP_ERROR_INVALID_ID;
        }
        auto& req = *it->second;
        if (req.state == HttpRequestState::Sending || req.state == HttpRequestState::Sent) {
            LOG_ERROR(Lib_Http, "Request already sent (reqId={})", reqId);
            return ORBIS_HTTP_ERROR_AFTER_SEND;
        }
        if (req.state == HttpRequestState::Aborted) {
            LOG_ERROR(Lib_Http, "Request was aborted (reqId={})", reqId);
            return ORBIS_HTTP_ERROR_ABORTED;
        }
        // Created to Sending. Worker thread will move to Sent.
        req.state = HttpRequestState::Sending;
        req_ptr = it->second;
    }

    LOG_INFO(Lib_Http, "reqId={} dispatched to async worker [{}]", reqId,
             EmulatorSettings.IsConnectedToNetwork() ? "ONLINE (TODO real I/O)"
                                                     : "OFFLINE no-internet path");
    std::thread([req_ptr, reqId]() {
        HttpResponse local_res;
        if (!EmulatorSettings.IsConnectedToNetwork()) {
            SynthesizeTransportFailureResponse(local_res);
        } else {
            // TODO: real network I/O path but for now return the same so switching doesn't affect
            // something
            SynthesizeTransportFailureResponse(local_res);
        }
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (g_state.shutting_down.load() || req_ptr->deleted ||
            req_ptr->state == HttpRequestState::Aborted) {
            req_ptr->cv.notify_all();
            return;
        }
        req_ptr->res = std::move(local_res);
        req_ptr->state = HttpRequestState::Sent;
        req_ptr->last_errno = ORBIS_HTTP_ERROR_RESOLVER_ENODNS;
        LOG_INFO(Lib_Http, "(TRANSPORT FAIL) reqId={} -> 0 (body 0 bytes, errno={:#x})", reqId,
                 static_cast<u32>(req_ptr->last_errno));
        req_ptr->cv.notify_all();
        // If this request is bound to an epoll, push a failure-shaped event so
        // sceHttpWaitRequest callers see the completion (with errored bits).
        if (req_ptr->epoll_id != 0) {
            auto epoll_it = g_state.epolls.find(req_ptr->epoll_id);
            if (epoll_it != g_state.epolls.end() && !epoll_it->second->destroyed) {
                constexpr u32 FailureEventBits =
                    ORBIS_HTTP_NB_EVENT_RESOLVER_ERR | ORBIS_HTTP_NB_EVENT_HUP;
                OrbisHttpNBEvent ev{};
                ev.events = FailureEventBits;
                ev.eventDetail = FailureEventBits;
                ev.id = reqId;
                ev.userArg = req_ptr->epoll_user_arg;
                epoll_it->second->events.push_back(ev);
                epoll_it->second->cv.notify_all();
                LOG_DEBUG(Lib_Http, "pushed failure epoll event for reqId={} on epoll={}", reqId,
                          req_ptr->epoll_id);
            }
        }
    }).detach();

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

int PS4_SYSV_ABI sceHttpSetChunkedTransferEnabled(int id, int isEnable) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, isEnable={}", id, isEnable);
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

int PS4_SYSV_ABI sceHttpSetEpollId() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetHttp09Enabled(int id, int isEnable) {
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

int PS4_SYSV_ABI sceHttpSetRedirectCallback(int id, OrbisHttpRedirectCallback cbfunc,
                                            void* userArg) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, cbfunc={}, userArg={}", id,
              fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
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
    LOG_INFO(Lib_Http, "called libhttpCtxId={}", libhttpCtxId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (g_state.active_contexts.erase(libhttpCtxId) == 0) {
        LOG_ERROR(Lib_Http, "Invalid or already-terminated ctxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (g_state.active_contexts.empty()) {
        // Last context torn down - wipe all dependent objects.
        LOG_INFO(Lib_Http, "last context terminated, clearing state");
        g_state.shutting_down.store(true);
        for (auto& [id, req_ptr] : g_state.requests) {
            req_ptr->deleted = true;
            req_ptr->state = HttpRequestState::Aborted;
            req_ptr->cv.notify_all(); // wake blocked waiters before wiping the map
        }
        for (auto& [id, epoll_ptr] : g_state.epolls) {
            epoll_ptr->destroyed = true;
            epoll_ptr->cv.notify_all(); // wake any sceHttpWaitRequest blocker
        }
        g_state.requests.clear();
        g_state.connections.clear();
        g_state.templates.clear();
        g_state.epolls.clear();
        g_state.inited = false;
    } else {
        LOG_INFO(Lib_Http, "ctxId={} terminated, {} contexts still active", libhttpCtxId,
                 g_state.active_contexts.size());
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpWaitRequest(OrbisHttpEpollHandle eh, OrbisHttpNBEvent* nbev, int maxevents,
                                    int timeout) {
    LOG_DEBUG(Lib_Http, "called eh={}, nbev={}, maxevents={}, timeout={}", fmt::ptr(eh),
              fmt::ptr(nbev), maxevents, timeout);
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (maxevents <= 0 || !eh || !nbev) {
        LOG_ERROR(Lib_Http, "InvalidValue (maxevents={}, eh={}, nbev={})", maxevents, fmt::ptr(eh),
                  fmt::ptr(nbev));
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int epoll_id = DecodeEpollHandle(eh);
    auto it = g_state.epolls.find(epoll_id);
    if (it == g_state.epolls.end()) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto epoll_ptr = it->second;

    // if the epoll's abort flag is
    // already set, return Aborted immediately without draining or waiting.
    if (epoll_ptr->abort_requested) {
        LOG_INFO(Lib_Http, "epoll id={} already aborted, returning ABORTED", epoll_id);
        return ORBIS_HTTP_ERROR_ABORTED;
    }

    auto drain_into_output = [&]() -> int {
        int count = 0;
        while (count < maxevents && !epoll_ptr->events.empty()) {
            nbev[count] = epoll_ptr->events.front();
            epoll_ptr->events.pop_front();
            ++count;
        }
        return count;
    };

    // Events already queued: drain and return immediately.
    int already = drain_into_output();
    if (already > 0) {
        LOG_INFO(Lib_Http, "epoll id={} returned {} events (no wait)", epoll_id, already);
        return already;
    }

    // No events queued. Behavior depends on timeout.
    if (timeout == 0) {
        // Poll mode: don't wait, just return 0.
        return 0;
    }
    if (epoll_ptr->destroyed || g_state.shutting_down.load()) {
        // Don't wait if we already know we'd be woken right away.
        return 0;
    }

    auto predicate = [&]() {
        return !epoll_ptr->events.empty() || epoll_ptr->destroyed || epoll_ptr->abort_requested ||
               g_state.shutting_down.load();
    };
    if (timeout < 0) {
        epoll_ptr->cv.wait(lock, predicate);
    } else {
        epoll_ptr->cv.wait_for(lock, std::chrono::microseconds(timeout), predicate);
    }

    // If AbortWaitRequest fired during the wait, return ABORTED
    if (epoll_ptr->abort_requested) {
        LOG_INFO(Lib_Http, "epoll id={} woken by abort, returning ABORTED", epoll_id);
        return ORBIS_HTTP_ERROR_ABORTED;
    }
    int count = drain_into_output();
    if (epoll_ptr->destroyed) {
        LOG_INFO(Lib_Http, "epoll id={} woken because destroyed; returning {} events", epoll_id,
                 count);
    } else {
        LOG_INFO(Lib_Http, "epoll id={} returned {} events after wait", epoll_id, count);
    }
    return count;
}

int PS4_SYSV_ABI sceHttpUriCopy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

//***********************************
// Non-blocking processing functions
//***********************************
int PS4_SYSV_ABI sceHttpCreateEpoll(int libhttpCtxId, OrbisHttpEpollHandle* eh) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (!eh) {
        LOG_ERROR(Lib_Http, "eh output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int epoll_id = ++g_state.next_obj_id;
    auto epoll = std::make_shared<Epoll>();
    epoll->ctx_id = libhttpCtxId;
    g_state.epolls.emplace(epoll_id, std::move(epoll));
    *eh = EncodeEpollHandle(epoll_id);
    LOG_INFO(Lib_Http, "created epoll id={} (handle={})", epoll_id, fmt::ptr(*eh));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpDestroyEpoll(int libhttpCtxId, OrbisHttpEpollHandle eh) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (!eh) {
        LOG_ERROR(Lib_Http, "eh is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int epoll_id = DecodeEpollHandle(eh);
    auto it = g_state.epolls.find(epoll_id);
    if (it == g_state.epolls.end()) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (it->second->ctx_id != libhttpCtxId) {
        LOG_ERROR(Lib_Http, "ctxId mismatch: epoll ctx_id={} but caller passed {}",
                  it->second->ctx_id, libhttpCtxId);
    }
    auto epoll_ptr = it->second;
    epoll_ptr->destroyed = true;
    epoll_ptr->cv.notify_all();
    g_state.epolls.erase(it);
    LOG_INFO(Lib_Http, "destroyed epoll id={}", epoll_id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpoll(int id, OrbisHttpEpollHandle* eh, void** userArg) {
    LOG_INFO(Lib_Http, "called id={}, eh={}, userArg={}", id, fmt::ptr(eh), fmt::ptr(userArg));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!eh) {
        LOG_ERROR(Lib_Http, "eh output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int* src_epoll_id = nullptr;
    void** src_user_arg = nullptr;
    const char* level = "";
    if (!ResolveEpollBinding(id, src_epoll_id, src_user_arg, level)) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *eh = EncodeEpollHandle(*src_epoll_id);
    if (userArg) {
        *userArg = *src_user_arg;
    }
    LOG_INFO(Lib_Http, "got epoll id={} userArg={} from {} id={}", *src_epoll_id,
             fmt::ptr(*src_user_arg), level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpoll(int id, OrbisHttpEpollHandle eh, void* userArg) {
    LOG_INFO(Lib_Http, "called id={}, eh={}, userArg={}", id, fmt::ptr(eh), fmt::ptr(userArg));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    int epoll_id = DecodeEpollHandle(eh);
    if (!g_state.epolls.contains(epoll_id)) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    int* target_epoll_id = nullptr;
    void** target_user_arg = nullptr;
    const char* level = "";
    if (!ResolveEpollBinding(id, target_epoll_id, target_user_arg, level)) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *target_epoll_id = epoll_id;
    *target_user_arg = userArg;
    LOG_INFO(Lib_Http, "set epoll={} userArg={} at {} level (id={})", epoll_id, fmt::ptr(userArg),
             level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUnsetEpoll(int id) {
    LOG_INFO(Lib_Http, "called id={}", id);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.requests.find(id);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    it->second->epoll_id = 0;
    it->second->epoll_user_arg = nullptr;
    LOG_INFO(Lib_Http, "cleared epoll binding from reqId={}", id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortWaitRequest(OrbisHttpEpollHandle eh) {
    LOG_INFO(Lib_Http, "called eh={}", fmt::ptr(eh));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!eh) {
        LOG_ERROR(Lib_Http, "eh is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int epoll_id = DecodeEpollHandle(eh);
    auto it = g_state.epolls.find(epoll_id);
    if (it == g_state.epolls.end()) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    it->second->abort_requested = true;
    it->second->cv.notify_all();
    LOG_INFO(Lib_Http, "epoll id={} abort requested", epoll_id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetNonblock(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, fmt::ptr(isEnable));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!isEnable) {
        LOG_ERROR(Lib_Http, "isEnable output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *isEnable = s->nonblock ? 1 : 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetNonblock(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->nonblock = (isEnable != 0);
    LOG_INFO(Lib_Http, "set {} id={} nonblock={}", level, id, s->nonblock);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpTryGetNonblock(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return sceHttpGetNonblock(id, isEnable);
}

int PS4_SYSV_ABI sceHttpTrySetNonblock(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    return sceHttpSetNonblock(id, isEnable);
}

//***********************************
// Http Communication functions
//***********************************
int PS4_SYSV_ABI sceHttpReadData(s32 reqId, void* data, u64 size) {
    LOG_INFO(Lib_Http, "called reqId={}, data={}, size={}", reqId, fmt::ptr(data), size);
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!data) {
        LOG_ERROR(Lib_Http, "data output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto& req = *it->second;
    int wr = WaitForResponseReady(req, lock);
    if (wr != ORBIS_OK) {
        if (wr == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, static_cast<u32>(wr));
        }
        return wr;
    }
    u64 remaining = req.res.body.size() - req.res.read_cursor;
    u64 to_copy = std::min(size, remaining);
    if (to_copy > 0) {
        std::memcpy(data, req.res.body.data() + req.res.read_cursor, to_copy);
        req.res.read_cursor += to_copy;
    }
    LOG_INFO(Lib_Http, "reqId={} copied {} bytes (cursor {}/{}) ", reqId, to_copy,
             req.res.read_cursor, req.res.body.size());
    return static_cast<int>(to_copy);
}

int PS4_SYSV_ABI sceHttpAbortRequest(int reqId) {
    LOG_INFO(Lib_Http, "called reqId={}", reqId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto& req = *it->second;

    if (req.state == HttpRequestState::Created || req.state == HttpRequestState::Sending) {
        req.state = HttpRequestState::Aborted;
        req.cv.notify_all();
        LOG_INFO(Lib_Http, "reqId={} marked Aborted", reqId);
    } else {
        LOG_INFO(Lib_Http, "reqId={} already Sent/Aborted, no-op", reqId);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortRequestForce(int reqId) {
    LOG_INFO(Lib_Http, "called reqId={}", reqId);
    return sceHttpAbortRequest(reqId);
}

//***********************************
// Https Option setting functions
//***********************************
int PS4_SYSV_ABI sceHttpsDisableOption(int id, u32 sslFlags) {
    LOG_INFO(Lib_Http, "called id={}, sslFlags={:#x}", id, sslFlags);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if ((sslFlags & ~ORBIS_HTTPS_FLAG_PUBLIC_VALID) != 0) {
        LOG_ERROR(Lib_Http, "sslFlags=0x{:x} contains unknown bits 0x{:x}", sslFlags,
                  sslFlags & ~ORBIS_HTTPS_FLAG_PUBLIC_VALID);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->ssl_flags &= ~sslFlags;
    LOG_INFO(Lib_Http, "ssl_flags now 0x{:x} at {} level (id={})", s->ssl_flags, level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsDisableOptionPrivate(int id, u32 sslFlags) {
    LOG_INFO(Lib_Http, "called id={}, sslFlags={:#x}", id, sslFlags);
    // Same as sceHttpsDisableOption but accepts a wider bit-mask
    // ORBIS_HTTPS_FLAG_PRIVATE_VALID
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if ((sslFlags & ~ORBIS_HTTPS_FLAG_PRIVATE_VALID) != 0) {
        LOG_ERROR(Lib_Http, "sslFlags=0x{:x} contains unknown bits 0x{:x}", sslFlags,
                  sslFlags & ~ORBIS_HTTPS_FLAG_PRIVATE_VALID);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->ssl_flags &= ~sslFlags;
    LOG_INFO(Lib_Http, "ssl_flags now 0x{:x} at {} level (id={})", s->ssl_flags, level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOption(int id, u32 sslFlags) {
    LOG_INFO(Lib_Http, "called id={}, sslFlags={:#x}", id, sslFlags);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if ((sslFlags & ~ORBIS_HTTPS_FLAG_PUBLIC_VALID) != 0) {
        LOG_ERROR(Lib_Http, "sslFlags=0x{:x} contains unknown bits 0x{:x}", sslFlags,
                  sslFlags & ~ORBIS_HTTPS_FLAG_PUBLIC_VALID);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->ssl_flags |= sslFlags;
    LOG_INFO(Lib_Http, "ssl_flags now 0x{:x} at {} level (id={})", s->ssl_flags, level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsEnableOptionPrivate(int id, u32 sslFlags) {
    LOG_INFO(Lib_Http, "called id={}, sslFlags={:#x}", id, sslFlags);
    // Same as sceHttpsEnableOption but accepts the wider Private
    // bit-mask (ORBIS_HTTPS_FLAG_PRIVATE_VALID).
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if ((sslFlags & ~ORBIS_HTTPS_FLAG_PRIVATE_VALID) != 0) {
        LOG_ERROR(Lib_Http, "sslFlags=0x{:x} contains unknown bits 0x{:x}", sslFlags,
                  sslFlags & ~ORBIS_HTTPS_FLAG_PRIVATE_VALID);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->ssl_flags |= sslFlags;
    LOG_INFO(Lib_Http, "ssl_flags now 0x{:x} at {} level (id={})", s->ssl_flags, level, id);
    return ORBIS_OK;
}

//***********************************
// Response Information functions
//***********************************
int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int reqId, char** header, u64* headerSize) {
    LOG_INFO(Lib_Http, "called reqId={}, header={}, headerSize={}", reqId, fmt::ptr(header),
             fmt::ptr(headerSize));
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!header || !headerSize) {
        LOG_ERROR(Lib_Http, "header or headerSize output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto& req = *it->second;
    int wr = WaitForResponseReady(req, lock);
    if (wr != ORBIS_OK) {
        if (wr == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, static_cast<u32>(wr));
        }
        return wr;
    }
    if (req.res.all_headers_blob.empty()) {
        *header = nullptr;
        *headerSize = 0;
    } else {
        *header = const_cast<char*>(req.res.all_headers_blob.c_str());
        *headerSize = req.res.all_headers_blob.size();
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetResponseContentLength(int reqId, int* result, u64* contentLength) {
    LOG_INFO(Lib_Http, "called reqId={}, result={}, contentLength={}", reqId, fmt::ptr(result),
             fmt::ptr(contentLength));
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!result || !contentLength) {
        LOG_ERROR(Lib_Http, "result or contentLength output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto& req = *it->second;
    int wr = WaitForResponseReady(req, lock);
    if (wr == ORBIS_HTTP_ERROR_EAGAIN) {
        LOG_DEBUG(Lib_Http, "reqId={}: response not yet ready, returning BEFORE_SEND", reqId);
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }
    if (wr != ORBIS_OK) {
        LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, static_cast<u32>(wr));
        return wr;
    }
    *result = req.res.content_length_result;
    *contentLength = req.res.content_length;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetStatusCode(int reqId, int* statusCode) {
    LOG_INFO(Lib_Http, "called reqId={}", reqId);
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!statusCode) {
        LOG_ERROR(Lib_Http, "statusCode output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto& req = *it->second;
    int wr = WaitForResponseReady(req, lock);
    if (wr == ORBIS_HTTP_ERROR_EAGAIN) {
        LOG_DEBUG(Lib_Http, "reqId={}: response not yet ready, returning BEFORE_SEND", reqId);
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }
    if (wr != ORBIS_OK) {
        LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, static_cast<u32>(wr));
        return wr;
    }
    // Transport failure: no status line was ever received from a server.
    if (req.res.status_code == 0 && req.last_errno != 0) {
        LOG_INFO(Lib_Http, "reqId={} transport failure, errno={:#x}, returning BEFORE_SEND", reqId,
                 static_cast<u32>(req.last_errno));
        return ORBIS_HTTP_ERROR_BEFORE_SEND;
    }
    *statusCode = req.res.status_code;
    LOG_INFO(Lib_Http, "reqId={} status={}", reqId, HttpStatusLabel(req.res.status_code));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetInflateGZIPEnabled(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (static_cast<u32>(isEnable) >= 2) {
        LOG_ERROR(Lib_Http, "isEnable={} is not 0 or 1", isEnable);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->inflate_gzip = (isEnable != 0);
    LOG_INFO(Lib_Http, "inflate_gzip={} at {} level (id={})", s->inflate_gzip, level, id);
    return ORBIS_OK;
}

//***********************************
// Http Header setting functions
//***********************************
int PS4_SYSV_ABI sceHttpSetRequestContentLength(int id, u64 contentLength) {
    LOG_INFO(Lib_Http, "called id={}, contentLength={}", id, contentLength);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.requests.find(id);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (it->second->state != HttpRequestState::Created) {
        LOG_ERROR(Lib_Http, "reqId={} already sent or in-flight; cannot set content length", id);
        return ORBIS_HTTP_ERROR_BUSY;
    }
    it->second->content_length = contentLength;
    return ORBIS_OK;
}

//***********************************
// Redirection setting functions
//***********************************
int PS4_SYSV_ABI sceHttpGetAutoRedirect(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, fmt::ptr(isEnable));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!isEnable) {
        LOG_ERROR(Lib_Http, "Invalid Value");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *isEnable = s->auto_redirect ? 1 : 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAutoRedirect(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->auto_redirect = (isEnable != 0);
    LOG_INFO(Lib_Http, "auto_redirect={} at {} level (id={})", s->auto_redirect, level, id);
    return ORBIS_OK;
}

//***********************************
// Timeout setting functions
//***********************************
int PS4_SYSV_ABI sceHttpSetConnectTimeOut(int id, u32 usec) {
    LOG_INFO(Lib_Http, "called id={}, usec={}", id, usec);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->connect_timeout_us = usec;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetSendTimeOut(int id, u32 usec) {
    LOG_INFO(Lib_Http, "called id={}, usec={}", id, usec);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->send_timeout_us = usec;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRecvTimeOut(int id, u32 usec) {
    LOG_INFO(Lib_Http, "called id={}, usec={}", id, usec);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid Id");
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->recv_timeout_us = usec;
    return ORBIS_OK;
}

//***********************************
// Connection functions
//***********************************
int PS4_SYSV_ABI sceHttpDeleteConnection(int connId) {
    LOG_INFO(Lib_Http, "called connId={}", connId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (g_state.connections.erase(connId) == 0) {
        LOG_ERROR(Lib_Http, "Invalid connId={}", connId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    return ORBIS_OK;
}

//***********************************
// Template functions
//***********************************
int PS4_SYSV_ABI sceHttpDeleteTemplate(int tmplId) {
    LOG_INFO(Lib_Http, "called tmplId={}", tmplId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (g_state.templates.erase(tmplId) == 0) {
        LOG_ERROR(Lib_Http, "Invalid tmplId={}", tmplId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    return ORBIS_OK;
}

//***********************************
// Request functions
//***********************************
int PS4_SYSV_ABI sceHttpDeleteRequest(int reqId) {
    LOG_INFO(Lib_Http, "called reqId={}", reqId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto req_ptr = it->second;
    req_ptr->deleted = true;
    req_ptr->state = HttpRequestState::Aborted;
    req_ptr->cv.notify_all();
    g_state.requests.erase(it);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpCreateRequest2(int connId, const char* method, const char* path,
                                       u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, path={}, contentLength={}", connId,
             method ? method : "(null)", path ? path : "(null)", contentLength);
    auto map_method = [](const char* m) -> int {
        if (!m)
            return -1;
        std::string s = m;
        if (s == "GET")
            return ORBIS_HTTP_METHOD_GET;
        if (s == "POST")
            return ORBIS_HTTP_METHOD_POST;
        if (s == "HEAD")
            return ORBIS_HTTP_METHOD_HEAD;
        if (s == "PUT")
            return ORBIS_HTTP_METHOD_PUT;
        if (s == "DELETE")
            return ORBIS_HTTP_METHOD_DELETE;
        if (s == "TRACE")
            return ORBIS_HTTP_METHOD_TRACE;
        return -1;
    };
    // Resolve the connection's URL under the lock, then drop the lock before
    // delegating to sceHttpCreateRequestWithURL
    std::string url;
    int int_method;
    {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (!g_state.inited) {
            LOG_ERROR(Lib_Http, "Not initialized");
            return ORBIS_HTTP_ERROR_BEFORE_INIT;
        }
        auto it = g_state.connections.find(connId);
        if (it == g_state.connections.end()) {
            LOG_ERROR(Lib_Http, "Invalid connId={}", connId);
            return ORBIS_HTTP_ERROR_INVALID_ID;
        }
        if (!path) {
            LOG_ERROR(Lib_Http, "path is null");
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        if (ContainsCrLf(path)) {
            LOG_ERROR(Lib_Http, "path contains CR/LF (CRLF-injection rejected): {}", path);
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        int_method = map_method(method);
        if (int_method < 0) {
            if (!method) {
                LOG_ERROR(Lib_Http, "method is null");
                return ORBIS_HTTP_ERROR_INVALID_VALUE;
            }
            LOG_INFO(Lib_Http, "method '{}' not in standard table; routing via CUSTOM slot",
                     method);
            int_method = ORBIS_HTTP_METHOD_CUSTOM;
        }
        const auto& conn = it->second;
        url = conn.scheme + "://" + conn.hostname + ":" + std::to_string(conn.port);
        if (path[0] != '\0') {
            if (path[0] != '/') {
                url.push_back('/');
            }
            url.append(path);
        }
    }
    int reqId = sceHttpCreateRequestWithURL(connId, int_method, url.c_str(), contentLength);
    if (reqId > 0 && method) {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        auto it = g_state.requests.find(reqId);
        if (it != g_state.requests.end()) {
            it->second->method_str = method;
        }
    }
    return reqId;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int connId, const char* method, const char* url,
                                              u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, url={}, contentLength={}", connId,
             method ? method : "(null)", url ? url : "(null)", contentLength);
    int int_method;
    {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (!g_state.inited) {
            LOG_ERROR(Lib_Http, "Not initialized");
            return ORBIS_HTTP_ERROR_BEFORE_INIT;
        }
        if (!g_state.connections.contains(connId)) {
            LOG_ERROR(Lib_Http, "Invalid connId={}", connId);
            return ORBIS_HTTP_ERROR_INVALID_ID;
        }
        if (!method) {
            LOG_ERROR(Lib_Http, "method is null");
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        std::string s = method;
        if (s == "GET")
            int_method = ORBIS_HTTP_METHOD_GET;
        else if (s == "POST")
            int_method = ORBIS_HTTP_METHOD_POST;
        else if (s == "HEAD")
            int_method = ORBIS_HTTP_METHOD_HEAD;
        else if (s == "PUT")
            int_method = ORBIS_HTTP_METHOD_PUT;
        else if (s == "DELETE")
            int_method = ORBIS_HTTP_METHOD_DELETE;
        else if (s == "TRACE")
            int_method = ORBIS_HTTP_METHOD_TRACE;
        else {
            LOG_INFO(Lib_Http, "method '{}' not in standard table; routing via CUSTOM slot",
                     method);
            int_method = ORBIS_HTTP_METHOD_CUSTOM;
        }
    }
    int reqId = sceHttpCreateRequestWithURL(connId, int_method, url, contentLength);
    if (reqId > 0) {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        auto it = g_state.requests.find(reqId);
        if (it != g_state.requests.end()) {
            it->second->method_str = method;
        }
    }
    return reqId;
}

//***********************************
// Error Obtainment functions
//***********************************
int PS4_SYSV_ABI sceHttpGetLastErrno(int reqId, int* errNum) {
    LOG_INFO(Lib_Http, "called reqId={}, errNum={}", reqId, fmt::ptr(errNum));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!errNum) {
        LOG_ERROR(Lib_Http, "errNum output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    auto it = g_state.requests.find(reqId);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", reqId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *errNum = it->second->last_errno;
    return ORBIS_OK;
}

//***********************************
// HTTP Header Parsing functions
//***********************************
int PS4_SYSV_ABI sceHttpParseStatusLine(const char* statusLine, u64 lineLen, int32_t* httpMajorVer,
                                        int32_t* httpMinorVer, int32_t* responseCode,
                                        const char** reasonPhrase, u64* phraseLen) {
    LOG_INFO(Lib_Http,
             "called statusLine={}, lineLen={}, httpMajorVer={}, httpMinorVer={}, responseCode={}, "
             "reasonPhrase={}, phraseLen={}",
             fmt::ptr(statusLine), lineLen, fmt::ptr(httpMajorVer), fmt::ptr(httpMinorVer),
             fmt::ptr(responseCode), fmt::ptr(reasonPhrase), fmt::ptr(phraseLen));

    if (!statusLine) {
        LOG_ERROR(Lib_Http, "Invalid Response");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    if (!httpMajorVer || !httpMinorVer || !responseCode || !reasonPhrase || !phraseLen) {
        LOG_ERROR(Lib_Http, "Invalid value");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE;
    }

    *httpMajorVer = 0;
    *httpMinorVer = 0;

    if (lineLen < 8) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    if (memcmp(statusLine, "HTTP/", 5) != 0) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    auto isAsciiDigit = [](char c) -> bool {
        const unsigned char uc = static_cast<unsigned char>(c);
        return uc < 0x80 && std::isdigit(uc);
    };

    // First byte of major version
    if (!isAsciiDigit(statusLine[5])) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    // Major version digit loop
    u64 index = 7;
    {
        unsigned char ch = static_cast<unsigned char>(statusLine[5]);
        while (ch < 0x80 && std::isdigit(ch)) {
            *httpMajorVer = *httpMajorVer * 10 + (statusLine[index - 2] - '0');
            const char next = statusLine[index - 1];
            index++;
            if (static_cast<unsigned char>(next) >= 0x80) {
                return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
            }
            ch = static_cast<unsigned char>(next);
        }
    }

    // After major loop, the previously-loaded byte must be '.'.
    if (statusLine[index - 2] != '.') {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    // First byte of minor version
    if (!isAsciiDigit(statusLine[index - 1])) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    // Minor version digit loop
    {
        unsigned char ch = static_cast<unsigned char>(statusLine[index - 1]);
        while (ch < 0x80 && std::isdigit(ch)) {
            *httpMinorVer = *httpMinorVer * 10 + (statusLine[index - 1] - '0');
            const char next = statusLine[index];
            index++;
            if (static_cast<unsigned char>(next) >= 0x80) {
                return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
            }
            ch = static_cast<unsigned char>(next);
        }
    }

    // After minor loop, statusLine[index - 1] must be ' '.
    if (statusLine[index - 1] != ' ') {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    // Need >=3 bytes for the response code
    const u64 remaining = lineLen - index;
    if (remaining < 3) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }

    // Validate and parse the 3-digit response code
    for (int i = 0; i < 3; ++i) {
        if (!isAsciiDigit(statusLine[index + i])) {
            return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
        }
    }
    *responseCode = (statusLine[index] - '0') * 100 + (statusLine[index + 1] - '0') * 10 +
                    (statusLine[index + 2] - '0');

    const char* phraseStart = statusLine + index + 3;
    const u64 maxScan = remaining - 3;
    for (u64 scanLen = 0; scanLen <= maxScan; ++scanLen) {
        if (phraseStart[scanLen] != '\n') {
            continue;
        }
        *reasonPhrase = phraseStart;
        if (scanLen == 0) {
            *phraseLen = 0;
        } else {
            *phraseLen = scanLen - (phraseStart[scanLen - 1] == '\r' ? 1 : 0);
        }
        const u64 bytesConsumed = (phraseStart - statusLine) + scanLen + 1;
        LOG_INFO(Lib_Http, "parsed HTTP/{}.{} {}, phraseLen={}, bytes consumed={}", *httpMajorVer,
                 *httpMinorVer, *responseCode, *phraseLen, bytesConsumed);
        return static_cast<int>(bytesConsumed);
    }

    LOG_ERROR(Lib_Http, "no '\\n' found in status line");
    return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
}

int PS4_SYSV_ABI sceHttpParseResponseHeader(const char* header, u64 headerLen, const char* fieldStr,
                                            const char** fieldValue, u64* valueLen) {
    LOG_TRACE(Lib_Http, "called header={}, headerLen={}, fieldStr={}, fieldValue={}, valueLen={}",
              fmt::ptr(header), headerLen, fieldStr ? fieldStr : "(null)", fmt::ptr(fieldValue),
              fmt::ptr(valueLen));

    if (!header) {
        LOG_ERROR(Lib_Http, "Invalid response");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE;
    }
    if (!fieldStr || !fieldValue || !valueLen) {
        LOG_ERROR(Lib_Http, "Invalid value");
        return ORBIS_HTTP_ERROR_PARSE_HTTP_INVALID_VALUE;
    }

    const u64 fieldStrLen = strnlen(fieldStr, 0xfff);

    auto isAsciiSpace = [](unsigned char c) -> bool { return c < 0x80 && std::isspace(c) != 0; };
    auto caseInsensitiveEq = [](const char* a, const char* b, u64 n) -> bool {
        for (u64 i = 0; i < n; ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    };

    bool atLineStart = true;
    u64 valueOffset = 0;
    bool found = false;

    if (headerLen != 0) {
        u64 cur = 0;
        u64 next = 1;
        while (true) {
            if (atLineStart) {
                const unsigned char first = static_cast<unsigned char>(header[cur]);
                if (!isAsciiSpace(first)) {
                    if (fieldStrLen < headerLen - cur &&
                        caseInsensitiveEq(fieldStr, header + cur, fieldStrLen) &&
                        header[cur + fieldStrLen] == ':') {
                        // Found.
                        valueOffset = cur + fieldStrLen + 1;
                        found = true;
                        break;
                    }
                }
            }
            atLineStart = (header[cur] == '\n');
            cur = next;
            next += 1;
            if (cur >= headerLen)
                break;
        }
    }
    if (!found) {
        return ORBIS_HTTP_ERROR_PARSE_HTTP_NOT_FOUND;
    }

    while (valueOffset < headerLen) {
        const unsigned char c = static_cast<unsigned char>(header[valueOffset]);
        if (!isAsciiSpace(c))
            break;
        if (c == '\n') {
            // Past EOL with only whitespace seen. Firmware advances one more
            // and exits the loop so the value scan starts past the '\n'.
            valueOffset++;
            break;
        }
        valueOffset++;
    }

    u64 valueStart = valueOffset;
    u64 scan = valueOffset;
    u64 lineEnd = valueOffset;   // points one past the '\n' of the final line
    u64 lengthEnd = valueOffset; // where the trimmed value ends (strips trailing \r)

    if (valueOffset < headerLen) {
        bool sawCR = false;
        while (scan < headerLen) {
            // Walk to next '\n'.
            while (scan < headerLen && header[scan] != '\n') {
                scan++;
            }
            if (scan >= headerLen) {
                // No '\n' before end of buffer: value runs to headerLen.
                lengthEnd = headerLen;
                lineEnd = headerLen;
                break;
            }
            // scan points at '\n'. Note trailing '\r'.
            sawCR = (scan > valueStart && header[scan - 1] == '\r');
            const u64 afterLF = scan + 1;
            // Check for line folding: next byte is SP or HT.
            if (afterLF < headerLen && (header[afterLF] == ' ' || header[afterLF] == '\t')) {
                // Continuation - keep scanning.
                scan = afterLF;
                continue;
            }
            // End of value.
            lineEnd = afterLF;
            lengthEnd = sawCR ? (scan - 1) : scan;
            break;
        }
    } else {
        lineEnd = headerLen;
        lengthEnd = headerLen;
    }

    const u64 finalLen = (lengthEnd > valueStart) ? (lengthEnd - valueStart) : 0;
    if (finalLen == 0) {
        *fieldValue = nullptr;
        *valueLen = 0;
        return 0;
    }

    *fieldValue = header + valueStart;
    *valueLen = finalLen;
    return static_cast<int>(lineEnd);
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
        LOG_ERROR(Lib_Http, "Invalid url");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }
    if (out == nullptr && require == nullptr) {
        LOG_ERROR(Lib_Http, "Invalid value");
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

    auto schemeDefaultPort = [&]() -> uint16_t {
        if (scheme.size() > 0x20)
            return 0;
        auto prefixCaseEq = [&](const char* target) {
            const size_t tlen = std::strlen(target);
            if (scheme.size() < tlen)
                return false;
            for (size_t i = 0; i < tlen; ++i) {
                if (std::tolower(static_cast<unsigned char>(scheme[i])) !=
                    std::tolower(static_cast<unsigned char>(target[i])))
                    return false;
            }
            return true;
        };
        if (prefixCaseEq("HTTPS"))
            return 443;
        if (prefixCaseEq("HTTP"))
            return 80;
        if (prefixCaseEq("TTP"))
            return 80;
        return 0;
    };

    const bool isMailto = (scheme.size() == 6) && std::memcmp(scheme.data(), "mailto", 6) == 0;

    std::string built;
    built.reserve(256);

    // Scheme: write "<scheme>:"
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_SCHEME) && !scheme.empty()) {
        built.append(scheme);
        built.push_back(':');
    }
    if (!srcElement->opaque) {
        built.append("//");
    }

    // Userinfo (username[:password]@)
    const bool hasUser = (option & ORBIS_HTTP_URI_BUILD_WITH_USERNAME) && !username.empty();
    const bool hasPass = (option & ORBIS_HTTP_URI_BUILD_WITH_PASSWORD) && !password.empty();
    if (hasUser) {
        built.append(username);
    }
    if (hasPass) {
        built.push_back(':');
        built.append(password);
    }
    if (hasUser || hasPass) {
        built.push_back('@');
    }

    // Host
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_HOSTNAME) && !hostname.empty()) {
        built.append(hostname);
    }

    // Port: only if (a) we have one, (b) it isn't the scheme's default, and (c) scheme isn't
    // mailto.
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_PORT) && srcElement->port != 0) {
        const uint16_t def = schemeDefaultPort();
        const bool skip = (def != 0 || isMailto) && (def == srcElement->port);
        if (!skip) {
            built.push_back(':');
            built.append(std::to_string(srcElement->port));
        }
    }

    // Path / Query / Fragment
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_PATH) && !path.empty()) {
        built.append(path);
    }
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_QUERY) && !query.empty()) {
        built.append(query);
    }
    if ((option & ORBIS_HTTP_URI_BUILD_WITH_FRAGMENT) && !fragment.empty()) {
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
        return ORBIS_HTTP_ERROR_OUT_OF_MEMORY; // buffer too small
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

    if (!parsedUriElement.opaque) {
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

    parsedUriElement.path = mergedUrl + combinedLength;

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
    const bool writeOutput = (out != nullptr) && (pool != nullptr);
    if (!writeOutput && !require) {
        LOG_ERROR(Lib_Http, "Invalid value");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    if (writeOutput) {
        memset(out, 0, sizeof(OrbisHttpUriElement));
    }

    char* poolBytes = (char*)pool;

    bool hasScheme = false;
    u64 schemeLen = 0;
    {
        u64 i = 0;
        while (i < 0x20 && srcUri[i]) {
            if (srcUri[i] == ':')
                break;
            const unsigned char c = static_cast<unsigned char>(srcUri[i]);
            if (!isalnum(c) && c != '+' && c != '-' && c != '.')
                break;
            i++;
        }
        if (i > 0 && srcUri[i] == ':' && isalpha(static_cast<unsigned char>(srcUri[0]))) {
            hasScheme = true;
            schemeLen = i;
        }
    }

    u64 poolUsed;    // bytes used in pool
    u64 inputOffset; // current position in input string

    if (hasScheme) {
        if (writeOutput) {
            if (prepare < schemeLen + 1) {
                LOG_ERROR(Lib_Http, "Out of memory");
                return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
            }
            memcpy(poolBytes, srcUri, schemeLen);
            poolBytes[schemeLen] = '\0';
            out->scheme = poolBytes;
        }
        poolUsed = schemeLen + 1;
        inputOffset = schemeLen + 1;
    } else {
        if (writeOutput) {
            if (prepare < 2) {
                LOG_ERROR(Lib_Http, "Out of memory");
                return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
            }
            poolBytes[0] = '\0';
            out->scheme = poolBytes;
        }
        poolUsed = 1;
        inputOffset = 0;
    }

    int slashCount = 0;
    {
        const char* p = srcUri + inputOffset;
        while (*p == '/') {
            slashCount++;
            p++;
        }
    }
    if (slashCount >= 2) {
        inputOffset += 2;
        // opaque stays at 0 from memset
    } else {
        if (writeOutput) {
            out->opaque = true;
        }
    }

    const char* authStart = srcUri + inputOffset;
    u64 scanPos = 0;  // current byte offset within the authority area
    u64 colonPos = 0; // offset of the first ':' (only valid when seenColon)
    bool seenColon = false;
    bool seenAt = false;
    u64 atPos = 0;

    auto isUserinfoPunct = [](unsigned char c) -> bool {
        switch (c) {
        case 0x21:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x3a:
        case 0x3b:
        case 0x3d:
        case 0x5f:
        case 0x7e:
            return true;
        default:
            return false;
        }
    };

    while (true) {
        const unsigned char c = static_cast<unsigned char>(authStart[scanPos]);
        if (c == 0)
            break;
        if (c == '@') {
            seenAt = true;
            atPos = scanPos;
            break;
        }
        if (!seenColon && c == ':') {
            seenColon = true;
            colonPos = scanPos;
        } else {
            if ((signed char)c < 0)
                break;
            if (!isalnum(c) && !isUserinfoPunct(c))
                break;
        }
        scanPos++;
    }

    // Write user/password to pool.
    char* userDest = poolBytes + poolUsed;
    u64 inputAdvance = 0;

    if (seenAt) {
        u64 passOffset;
        u64 passLen;
        u64 userLen;
        if (seenColon) {
            userLen = colonPos;
            passOffset = colonPos + 1;
            passLen = atPos - passOffset;
        } else {
            userLen = atPos;
            passOffset = atPos + 1;
            passLen = 0;
        }

        if (writeOutput) {
            const u64 needed = passOffset + passLen + 1;
            if (prepare - poolUsed < needed) {
                return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
            }
            memcpy(userDest, authStart, userLen);
            userDest[userLen] = '\0';
            memcpy(userDest + passOffset, authStart + passOffset, passLen);
            userDest[passOffset + passLen] = '\0';
            out->username = userDest;
            out->password = userDest + passOffset;
        }
        poolUsed += passOffset + passLen + 1;
        inputAdvance = atPos + 1;
    } else {
        if (writeOutput) {
            if (prepare - poolUsed < 2) {
                return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
            }
            userDest[0] = '\0';
            userDest[1] = '\0';
            out->username = userDest;
            out->password = userDest + 1;
        }
        poolUsed += 2;
    }

    inputOffset += inputAdvance;

    char* hostDest = poolBytes + poolUsed;
    const char* hostStart = srcUri + inputOffset;
    const char firstHostChar = hostStart[0];
    u64 hostScanLen = 0;   // bytes scanned in input (including brackets)
    u64 storedHostLen = 0; // bytes stored to pool

    if (firstHostChar == '.') {
        hostScanLen = 0;
        storedHostLen = 0;
    } else if (firstHostChar == '[') {
        hostScanLen = 1;
        while (true) {
            if (hostScanLen == 0xff) {
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
            const unsigned char c = static_cast<unsigned char>(hostStart[hostScanLen]);
            if (c == 0)
                break;
            if ((signed char)c < 0)
                break;
            if (c == ']')
                break;
            // IPv6 mode allows ':' in addition to host chars
            if (!isalnum(c) && c != '-' && c != '.' && c != '_' && c != ':')
                break;
            hostScanLen++;
        }
        if (hostStart[hostScanLen] != ']') {
            return ORBIS_HTTP_ERROR_INVALID_URL;
        }
        storedHostLen = hostScanLen - 1;
        hostScanLen++; // consume ']' for input advance
    } else {
        // Normal host scan: alphanumeric + '-' '.' '_'
        while (true) {
            if (hostScanLen == 0xff) {
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
            const unsigned char c = static_cast<unsigned char>(hostStart[hostScanLen]);
            if (c == 0)
                break;
            if ((signed char)c < 0)
                break;
            if (!isalnum(c) && c != '-' && c != '.' && c != '_')
                break;
            hostScanLen++;
        }
        storedHostLen = hostScanLen;
    }

    if (writeOutput) {
        if (prepare - poolUsed < storedHostLen + 1) {
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }
        const char* hostCopySrc = (firstHostChar == '[') ? hostStart + 1 : hostStart;
        memcpy(hostDest, hostCopySrc, storedHostLen);
        hostDest[storedHostLen] = '\0';
        out->hostname = hostDest;
    }
    poolUsed += storedHostLen + 1;
    inputOffset += hostScanLen;

    bool hasExplicitPort = false;
    uint16_t portValue = 0;

    if (srcUri[inputOffset] == ':') {
        inputOffset++;
        const char* digits = srcUri + inputOffset;
        u64 digitsLen = 0;
        u32 port32 = 0;
        while (digitsLen < 5 && isdigit(static_cast<unsigned char>(digits[digitsLen]))) {
            port32 = port32 * 10 + (digits[digitsLen] - '0');
            digitsLen++;
        }
        if (port32 > 0x10000) {
            LOG_ERROR(Lib_Http, "Invalid URL");
            return ORBIS_HTTP_ERROR_INVALID_URL;
        }
        const char afterPort = digits[digitsLen];
        if (afterPort != '\0' && afterPort != '/') {
            return ORBIS_HTTP_ERROR_INVALID_URL;
        }
        if (digitsLen > 0) {
            hasExplicitPort = true;
            portValue = static_cast<uint16_t>(port32);
        }
        inputOffset += digitsLen;
    }

    if (writeOutput) {
        if (hasExplicitPort) {
            out->port = portValue;
        } else if (out->scheme) {
            const size_t schSize = std::strlen(out->scheme);
            if (schSize <= 0x20) {
                auto prefixCaseEq = [&](const char* target) {
                    const size_t tlen = std::strlen(target);
                    if (schSize < tlen)
                        return false;
                    for (size_t i = 0; i < tlen; ++i) {
                        if (std::tolower(static_cast<unsigned char>(out->scheme[i])) !=
                            std::tolower(static_cast<unsigned char>(target[i])))
                            return false;
                    }
                    return true;
                };
                if (prefixCaseEq("HTTPS"))
                    out->port = 443;
                else if (prefixCaseEq("HTTP"))
                    out->port = 80;
                else if (prefixCaseEq("TTP"))
                    out->port = 80;
            }
        }
    }

    char* pathDest = poolBytes + poolUsed;
    const char* pathStart = srcUri + inputOffset;
    u64 pathLen = 0;
    while (pathStart[pathLen] && pathStart[pathLen] != '?' && pathStart[pathLen] != '#') {
        if (pathLen >= 0x3fff) {
            return ORBIS_HTTP_ERROR_INVALID_URL;
        }
        pathLen++;
    }

    if (writeOutput) {
        if (prepare - poolUsed < pathLen + 1) {
            LOG_ERROR(Lib_Http, "Out of memory");
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(pathDest, pathStart, pathLen);
        pathDest[pathLen] = '\0';
        std::vector<char> tmp(pathLen + 1);
        memcpy(tmp.data(), pathStart, pathLen);
        tmp[pathLen] = '\0';
        sceHttpUriSweepPath(pathDest, tmp.data(), pathLen + 1);
        out->path = pathDest;
    }
    poolUsed += pathLen + 1;
    inputOffset += pathLen;

    char* queryDest = poolBytes + poolUsed;
    u64 queryLen = 0;
    if (srcUri[inputOffset] == '?') {
        queryLen = 1; // include leading '?'
        while (srcUri[inputOffset + queryLen] && srcUri[inputOffset + queryLen] != '#') {
            if (queryLen >= 0x3fff) {
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
            queryLen++;
        }
    }

    if (writeOutput) {
        if (prepare - poolUsed < queryLen + 1) {
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(queryDest, srcUri + inputOffset, queryLen);
        queryDest[queryLen] = '\0';
        out->query = queryDest;
    }
    poolUsed += queryLen + 1;
    inputOffset += queryLen;

    char* fragDest = poolBytes + poolUsed;
    u64 fragLen = 0;
    if (srcUri[inputOffset] == '#') {
        u64 i = 1; // include leading '#'
        while (srcUri[inputOffset + i]) {
            if (i >= 0x3fff) {
                return ORBIS_HTTP_ERROR_INVALID_URL;
            }
            i++;
        }
        fragLen = i;
    }

    if (writeOutput) {
        if (prepare - poolUsed < fragLen + 1) {
            return ORBIS_HTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(fragDest, srcUri + inputOffset, fragLen);
        fragDest[fragLen] = '\0';
        out->fragment = fragDest;
    }
    poolUsed += fragLen + 1;

    if (require) {
        *require = poolUsed;
    }

    LOG_TRACE(Lib_Http, "parsed successfully, poolUsed={}", poolUsed);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriSweepPath(char* dst, const char* src, u64 srcSize) {
    LOG_TRACE(Lib_Http, "called dst={}, src={}, srcSize={}", fmt::ptr(dst), src ? src : "(null)",
              srcSize);

    if (srcSize == 0) {
        return ORBIS_OK;
    }
    if (!dst || !src) {
        LOG_ERROR(Lib_Http, "Invalid parameters: dst={}, src={}", fmt::ptr(dst), fmt::ptr(src));
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    // Non-absolute
    if (src[0] != '/') {
        const u64 copyLen = srcSize - 1;
        memcpy(dst, src, copyLen);
        dst[copyLen] = '\0';
        return ORBIS_OK;
    }

    // Absolute path: dst[0]='/', dst[1]='\0'
    dst[0] = '/';
    dst[1] = '\0';
    if (srcSize - 1U <= 1) {
        return ORBIS_OK;
    }

    u64 srcPos = 1;
    char* segmentEnd = dst;
    while (srcPos < srcSize - 1U) {
        if (src[srcPos] == '.') {
            if (src[srcPos + 1] == '/') {
                // "./" - skip
                srcPos += 2;
                continue;
            }
            if (src[srcPos + 1] == '.' && src[srcPos + 2] == '/') {
                char* newSegmentEnd = dst;
                if (segmentEnd != dst) {
                    *segmentEnd = '\0';
                    char* prevSlash = std::strrchr(dst, '/');
                    if (prevSlash == nullptr) {
                        newSegmentEnd = nullptr;
                    } else {
                        prevSlash[1] = '\0';
                        newSegmentEnd = prevSlash;
                    }
                }
                srcPos += 3;
                segmentEnd = newSegmentEnd;
                continue;
            }
        }

        const char* segmentStart = src + srcPos;
        const char* nextSlash = std::strchr(segmentStart, '/');
        const u64 remaining = srcSize - srcPos - 1U;
        u64 copyLen;
        if (nextSlash == nullptr) {
            copyLen = remaining;
        } else {
            const u64 segLen = static_cast<u64>(nextSlash + 1 - segmentStart);
            copyLen = (segLen <= remaining) ? segLen : remaining;
        }
        memcpy(segmentEnd + 1, segmentStart, copyLen);
        segmentEnd[copyLen + 1] = '\0';
        segmentEnd += copyLen;
        srcPos += copyLen;
    }

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
