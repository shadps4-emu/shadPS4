// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <httplib.h>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "http_error.h"

namespace Libraries::Http {

static bool g_isHttpInitialized = true;

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

namespace {

constexpr int SCE_HTTP_METHOD_GET = 0;
constexpr int SCE_HTTP_METHOD_POST = 1;
constexpr int SCE_HTTP_METHOD_HEAD = 2;
constexpr int SCE_HTTP_METHOD_OPTIONS = 3;
constexpr int SCE_HTTP_METHOD_PUT = 4;
constexpr int SCE_HTTP_METHOD_DELETE = 5;
constexpr int SCE_HTTP_METHOD_TRACE = 6;
constexpr int SCE_HTTP_METHOD_CONNECT = 7;
constexpr int SCE_HTTP_METHOD_CUSTOM = 8;

// Bitmask of int-method values that are accepted by sceHttpCreateRequestWithURL.
// Bits set: 0,1,2,4,5,6,8 (GET, POST, HEAD, PUT, DELETE, TRACE, CUSTOM).
constexpr u32 SCE_HTTP_VALID_METHOD_MASK = 0x177u;

constexpr s32 SCE_HTTP_HEADER_OVERWRITE = 0;
constexpr s32 SCE_HTTP_HEADER_ADD = 1;
constexpr int SCE_HTTP_VERSION_1_0 = 1;
constexpr int SCE_HTTP_VERSION_1_1 = 2;

constexpr u32 SCE_HTTP_NB_EVENT_SOCK_ERR = 0x01;
constexpr u32 SCE_HTTP_NB_EVENT_RESOLVED = 0x02;
constexpr u32 SCE_HTTP_NB_EVENT_IN = 0x04;  // Response data ready to read.
constexpr u32 SCE_HTTP_NB_EVENT_OUT = 0x08; // Ready to send (request can write).
constexpr u32 SCE_HTTP_NB_EVENT_ICM = 0x10;
constexpr u32 SCE_HTTP_NB_EVENT_HUP = 0x20;

// Used as the initial value of HttpSettings::ssl_flags.
constexpr u32 ORBIS_HTTPS_FLAG_SDK_DEFAULT = ORBIS_HTTPS_FLAG_SERVER_VERIFY |
                                             ORBIS_HTTPS_FLAG_CN_CHECK |
                                             ORBIS_HTTPS_FLAG_KNOWN_CA_CHECK | ORBIS_HTTPS_FLAG_SNI;

// Validation masks consumed by sceHttpsEnableOption / sceHttpsDisableOption
constexpr u32 ORBIS_HTTPS_FLAG_PUBLIC_VALID = 0x000020ff;
constexpr u32 ORBIS_HTTPS_FLAG_PRIVATE_VALID = 0x00002dff;

struct HttpSettings {
    u32 connect_timeout_us = 0;
    u32 send_timeout_us = 0;
    u32 recv_timeout_us = 0;
    bool auto_redirect = true;
    bool inflate_gzip = true;
    bool accept_encoding_gzip = true;
    // SSL flag mask. Bitmask of OrbisHttpsFlags.
    u32 ssl_flags = ORBIS_HTTPS_FLAG_SDK_DEFAULT;
    OrbisHttpsCallback ssl_callback = nullptr;
    void* ssl_callback_user_arg = nullptr;
};

using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

namespace HeaderOps {
// Case-insensitive comparison.
inline bool IEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Remove every entry whose name case-insensitively matches `name`.
// Returns the number of entries removed.
inline size_t EraseAll(HttpHeaders& headers, std::string_view name) {
    auto new_end = std::remove_if(headers.begin(), headers.end(),
                                  [name](const auto& kv) { return IEquals(kv.first, name); });
    const auto removed = static_cast<size_t>(headers.end() - new_end);
    headers.erase(new_end, headers.end());
    return removed;
}

// Append a (name, value) pair unconditionally
inline void Append(HttpHeaders& headers, std::string name, std::string value) {
    headers.emplace_back(std::move(name), std::move(value));
}

// erase existing entries with this name, then append.
inline void Replace(HttpHeaders& headers, std::string name, std::string value) {
    EraseAll(headers, name);
    headers.emplace_back(std::move(name), std::move(value));
}

// Find the first entry matching `name` (case-insensitively); return
// nullptr if not found. Caller can read or modify .second through the
// returned pointer.
inline std::pair<std::string, std::string>* FindFirst(HttpHeaders& headers, std::string_view name) {
    for (auto& kv : headers) {
        if (IEquals(kv.first, name))
            return &kv;
    }
    return nullptr;
}

inline const std::pair<std::string, std::string>* FindFirst(const HttpHeaders& headers,
                                                            std::string_view name) {
    for (const auto& kv : headers) {
        if (IEquals(kv.first, name))
            return &kv;
    }
    return nullptr;
}
} // namespace HeaderOps

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

struct HttpTemplate {
    std::string user_agent;
    int http_version;
    int auto_proxy_conf;
    HttpHeaders headers;
    bool nonblock = false;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
    HttpSettings settings;
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
    HttpHeaders headers;
    bool nonblock = false;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
    HttpSettings settings;
};

enum class HttpRequestState {
    Created,
    Sending,
    Sent,
    Aborted,
};

struct HttpResponse {
    int status_code = 0;           // 200, 404, etc.
    std::string reason_phrase;     // "OK", "Not Found", etc.
    HttpHeaders headers;           // Response headers as delivered (allows duplicates;
                                   // a server can legitimately send multiple Set-Cookie,
                                   // multiple Cache-Control directives, etc.).
    std::string all_headers_blob;  // Pre-formatted "Name: Value\r\n..." string. This
                                   // is what sceHttpGetAllResponseHeaders returns by
                                   // const char* pointer.
    u64 content_length = 0;        // Body length in bytes.
    int content_length_result = 0; // Result code for sceHttpGetResponseContentLength.
                                   // 0 = value valid; non-zero indicates conditions
                                   // like chunked encoding or absent Content-Length.
    std::vector<u8> body;          // Body bytes (may be empty). ReadData copies from
                                   // here using read_cursor.
    u64 read_cursor = 0;           // Next byte ReadData will copy. Reaches body.size()
                                   // ReadData returns 0 = EOF.
};

struct HttpRequest {
    int conn_id;            // Owning connection (validated at create time).
    int method;             // ORBIS_HTTP_METHOD_*. -1 if a string method was used.
    std::string method_str; // Set instead of `method` for the *2 variants
    std::string url;        // The full request URL (may differ from connection URL).
    u64 content_length = 0; // Game-declared body size for POST/PUT.
    HttpHeaders headers;
    HttpRequestState state = HttpRequestState::Created;
    HttpResponse res;
    std::condition_variable cv;
    bool nonblock = false;
    bool deleted = false;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
    HttpSettings settings;
    s32 last_errno = 0; // Last cpp-httplib Error code (POSIX-style negative)
    s32 last_ssl_error = 0;
    u32 last_ssl_detail = 0;
};

struct Epoll {
    int ctx_id = 0; // Owning libhttpCtxId ,validated at Destroy.
    std::deque<OrbisHttpNBEvent> events;
    std::condition_variable cv;
    bool destroyed = false;
    bool abort_requested = false;
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
    bool ssl_status_logged = false; // flag to check if ssl is enabled
    bool process_default_accept_encoding_gzip = true;
};

static HttpState g_state;

constexpr auto kMockLatency = std::chrono::milliseconds(50);
constexpr bool kRealNetworkEnabled = true;

// placeholder for webapi TODO fix me
constexpr std::string_view kMockPsnHostSuffix = ".mock-psn.shadps4.invalid";

static bool IsMockPsnHost(const std::string& host) {
    if (host.size() < kMockPsnHostSuffix.size()) {
        return false;
    }
    return std::string_view(host).substr(host.size() - kMockPsnHostSuffix.size()) ==
           kMockPsnHostSuffix;
}

struct RealRequestPlan {
    std::string scheme;     // "http" or "https"
    std::string host;       // hostname only (no port)
    u16 port = 0;           // numeric port (80 / 443 if scheme-default)
    std::string path;       // path + query string (the part cpp-httplib wants)
    int method = 0;         // SCE_HTTP_METHOD_*
    std::string method_str; // populated for the *2 variants if `method` is invalid
    HttpHeaders headers;
    std::vector<u8> body;
    HttpSettings settings;
    bool nonblock = false;
};

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

// Parse the request's URL into scheme/host/port/path.
static bool ParseRequestUrl(const std::string& url, std::string& scheme, std::string& host,
                            u16& port, std::string& path_with_query) {
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return false;
    }
    scheme = url.substr(0, scheme_end);
    auto authority_start = scheme_end + 3;
    auto authority_end = url.find('/', authority_start);
    std::string authority;
    if (authority_end == std::string::npos) {
        authority = url.substr(authority_start);
        path_with_query = "/";
    } else {
        authority = url.substr(authority_start, authority_end - authority_start);
        path_with_query = url.substr(authority_end);
    }
    auto colon = authority.find(':');
    if (colon == std::string::npos) {
        host = authority;
        port = (scheme == "https") ? 443 : 80;
    } else {
        host = authority.substr(0, colon);
        try {
            port = static_cast<u16>(std::stoi(authority.substr(colon + 1)));
        } catch (...) {
            return false;
        }
    }
    return !host.empty();
}

// Convert our integer SCE_HTTP_METHOD_* into a verb string usable for httplib
static const char* HttpMethodName(int method) {
    switch (method) {
    case SCE_HTTP_METHOD_GET:
        return "GET";
    case SCE_HTTP_METHOD_POST:
        return "POST";
    case SCE_HTTP_METHOD_HEAD:
        return "HEAD";
    case SCE_HTTP_METHOD_OPTIONS:
        return "OPTIONS";
    case SCE_HTTP_METHOD_PUT:
        return "PUT";
    case SCE_HTTP_METHOD_DELETE:
        return "DELETE";
    case SCE_HTTP_METHOD_TRACE:
        return "TRACE";
    case SCE_HTTP_METHOD_CONNECT:
        return "CONNECT";
    case SCE_HTTP_METHOD_CUSTOM:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

// Convert our headers list into cpp-httplib's Headers (which is itself a
// multimap and natively allows duplicates).
static httplib::Headers BuildHttplibHeaders(const HttpHeaders& src) {
    httplib::Headers out;
    for (const auto& [k, v] : src) {
        out.emplace(k, v);
    }
    return out;
}

// Convert cpp-httplib's response back into HttpResponse fields
static void PopulateRealResponse(HttpResponse& res, const httplib::Result& result) {
    res.status_code = result->status;
    res.reason_phrase = result->reason.empty() ? "OK" : result->reason;
    res.headers.clear();
    for (const auto& [k, v] : result->headers) {
        res.headers.emplace_back(k, v);
    }
    res.all_headers_blob.clear();
    for (const auto& [k, v] : res.headers) {
        res.all_headers_blob += k + ": " + v + "\r\n";
    }
    res.all_headers_blob += "\r\n";
    res.body.assign(result->body.begin(), result->body.end());
    res.content_length = res.body.size();
    res.content_length_result = 0;
    res.read_cursor = 0;
}

static bool ExecuteRealRequest(const RealRequestPlan& plan, HttpResponse& out_res, s32* out_errno,
                               s32* out_ssl_error, u32* out_ssl_detail) {
    // Each output gets reset to "no error" and is filled in when something
    // actually fails.
    if (out_errno)
        *out_errno = 0;
    if (out_ssl_error)
        *out_ssl_error = 0;
    if (out_ssl_detail)
        *out_ssl_detail = 0;
    try {
        std::string base_url = plan.scheme + "://" + plan.host;
        if ((plan.scheme == "https" && plan.port != 443) ||
            (plan.scheme == "http" && plan.port != 80)) {
            base_url += ":" + std::to_string(plan.port);
        }
        httplib::Client cli(base_url);

        if (!plan.nonblock) {
            auto pick_timeout_seconds = [](u32 us, u32 default_s) -> std::chrono::seconds {
                if (us == 0) {
                    return std::chrono::seconds(default_s);
                }
                // Round up to the next whole second.
                u64 secs = (static_cast<u64>(us) + 999'999ull) / 1'000'000ull;
                if (secs == 0)
                    secs = 1;
                return std::chrono::seconds(secs);
            };
            cli.set_connection_timeout(pick_timeout_seconds(plan.settings.connect_timeout_us, 30));
            cli.set_read_timeout(pick_timeout_seconds(plan.settings.recv_timeout_us, 120));
            cli.set_write_timeout(pick_timeout_seconds(plan.settings.send_timeout_us, 120));
        }
        cli.set_follow_location(plan.settings.auto_redirect);

        auto headers = BuildHttplibHeaders(plan.headers);
        if (plan.settings.accept_encoding_gzip) {
            constexpr std::string_view kAcceptEncoding = "Accept-Encoding";
            auto iequals = [](std::string_view a, std::string_view b) {
                if (a.size() != b.size())
                    return false;
                for (size_t i = 0; i < a.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(a[i])) !=
                        std::tolower(static_cast<unsigned char>(b[i]))) {
                        return false;
                    }
                }
                return true;
            };
            bool already_has_accept_encoding = false;
            for (const auto& kv : headers) {
                if (iequals(kv.first, kAcceptEncoding)) {
                    already_has_accept_encoding = true;
                    break;
                }
            }
            if (!already_has_accept_encoding) {
                headers.emplace("Accept-Encoding", "gzip");
            }
        }
#ifdef CPPHTTPLIB_ZLIB_SUPPORT
        cli.set_decompress(plan.settings.inflate_gzip);
#endif
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (plan.scheme == "https") {
            // TODO invloke callback during httplib handshake
            const bool flag_says_verify =
                (plan.settings.ssl_flags & ORBIS_HTTPS_FLAG_SERVER_VERIFY) != 0;
            const bool callback_overrides = (plan.settings.ssl_callback != nullptr);
            const bool verify_server = flag_says_verify && !callback_overrides;
            cli.enable_server_certificate_verification(verify_server);
            if (!verify_server) {
                LOG_INFO(Lib_Http,
                         "real request to {}://{}: server cert verification disabled "
                         "(ssl_flags=0x{:x}, ssl_callback={})",
                         plan.scheme, plan.host, plan.settings.ssl_flags,
                         callback_overrides ? "registered" : "none");
            }
        }
#endif
        std::string content_type;
        if (auto* hp = HeaderOps::FindFirst(plan.headers, "Content-Type"); hp) {
            content_type = hp->second;
        }
        const char* body_ptr =
            plan.body.empty() ? "" : reinterpret_cast<const char*>(plan.body.data());
        size_t body_size = plan.body.size();

        auto result = [&]() {
            switch (plan.method) {
            case SCE_HTTP_METHOD_GET:
                return cli.Get(plan.path, headers);
            case SCE_HTTP_METHOD_POST:
                return cli.Post(plan.path, headers, body_ptr, body_size, content_type);
            case SCE_HTTP_METHOD_HEAD:
                return cli.Head(plan.path, headers);
            case SCE_HTTP_METHOD_OPTIONS:
                return cli.Options(plan.path, headers);
            case SCE_HTTP_METHOD_PUT:
                return cli.Put(plan.path, headers, body_ptr, body_size, content_type);
            case SCE_HTTP_METHOD_DELETE:
                return cli.Delete(plan.path, headers, body_ptr, body_size, content_type);
            case SCE_HTTP_METHOD_CUSTOM: {
                if (plan.method_str.empty()) {
                    LOG_ERROR(Lib_Http,
                              "method=CUSTOM but method_str is empty; falling back to mock");
                    return cli.Get("");
                }
                httplib::Request creq;
                creq.method = plan.method_str;
                creq.path = plan.path;
                creq.headers = headers; // Content-Type, if any, is already in here.
                if (!plan.body.empty()) {
                    creq.body.assign(body_ptr, body_size);
                }
                LOG_INFO(Lib_Http, "real request: custom method '{}' to {}://{}{}", plan.method_str,
                         plan.scheme, plan.host, plan.path);
                return cli.send(creq);
            }
            default:
                LOG_ERROR(Lib_Http, "Unsupported method {} ({}); falling back to mock", plan.method,
                          HttpMethodName(plan.method));
                return cli.Get("");
            }
        }();
        if (!result) {
            const auto err_val = static_cast<int>(result.error());
            LOG_ERROR(Lib_Http, "cpp-httplib request failed (host={}, path={}): error={}",
                      plan.host, plan.path, err_val);

            // Generic transport errno
            if (out_errno)
                *out_errno = -err_val;

            // httlp-lib has a limitation on distinguishing SSL errors from non-SSL errors
            if (out_ssl_error && plan.scheme == "https") {
                switch (err_val) {
                case 10: // SSLServerVerification
                    *out_ssl_error = ORBIS_HTTPS_ERROR_CERT;
                    if (out_ssl_detail) {
                        *out_ssl_detail = ORBIS_HTTPS_ERR_INTERNAL;
                    }
                    break;
                case 8:  // SSLConnection (handshake failed for non-cert reasons)
                case 9:  // SSLLoadingCerts (couldn't load local trust store)
                case 12: // Compression (TLS compression issue, rare)
                    *out_ssl_error = ORBIS_HTTPS_ERROR_INTERNAL;
                    // detail stays 0 for INTERNAL category.
                    break;
                case 14: // ProxyConnection (HTTP proxy returned an error pre-TLS)
                    *out_ssl_error = ORBIS_HTTPS_ERROR_PROXY;
                    // detail stays 0 for PROXY category.
                    break;
                default:
                    // Non-SSL-class error on an HTTPS request (e.g. plain
                    // Connection refused before TLS even started). Leave
                    // ssl_error at 0; sceHttpGetLastErrno still reports it.
                    break;
                }
            }
            return false;
        }
        PopulateRealResponse(out_res, result);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_Http,
                  "cpp-httplib threw exception for {}://{}{}: {} (probably no SSL "
                  "support; falling back to mock)",
                  plan.scheme, plan.host, plan.path, e.what());
        return false;
    } catch (...) {
        LOG_ERROR(Lib_Http,
                  "cpp-httplib threw unknown exception for {}://{}{}; falling back "
                  "to mock",
                  plan.scheme, plan.host, plan.path);
        return false;
    }
}

static OrbisHttpEpollHandle EncodeEpollHandle(int id) {
    return reinterpret_cast<OrbisHttpEpollHandle>(static_cast<intptr_t>(id));
}

static int DecodeEpollHandle(OrbisHttpEpollHandle eh) {
    return static_cast<int>(reinterpret_cast<intptr_t>(eh));
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
    // state == Sending. Two paths:
    if (req.nonblock) {
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

// added fake response in case http-lib throws an exception
static void SynthesizeMockResponse(HttpResponse& res) {
    res.status_code = 200;
    res.reason_phrase = "OK";
    res.headers.clear();
    res.headers.emplace_back("Content-Length", "0");
    res.headers.emplace_back("Content-Type", "application/octet-stream");
    res.headers.emplace_back("Server", "shadPS4-mock-libSceHttp");
    res.all_headers_blob.clear();
    for (const auto& [k, v] : res.headers) {
        res.all_headers_blob += k + ": " + v + "\r\n";
    }
    res.all_headers_blob += "\r\n";
    res.content_length = 0;
    res.content_length_result = 0; // 0 = value is valid (Content-Length header
                                   //     present and parsed successfully).
    res.body.clear();
    res.read_cursor = 0;
}

} // namespace

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
    req.state = HttpRequestState::Aborted;
    req.cv.notify_all();
    LOG_INFO(Lib_Http, "reqId={} marked Aborted", reqId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAbortRequestForce(int reqId) {
    LOG_INFO(Lib_Http, "called reqId={}", reqId);
    return sceHttpAbortRequest(reqId);
}

int PS4_SYSV_ABI sceHttpAbortWaitRequest(OrbisHttpEpollHandle eh) {
    LOG_INFO(Lib_Http, "called eh={}", fmt::ptr(eh));
    int epoll_id = DecodeEpollHandle(eh);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.epolls.find(epoll_id);
    if (it == g_state.epolls.end()) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    it->second->abort_requested = true;
    it->second->cv.notify_all();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpAddRequestHeader(int id, const char* name, const char* value, s32 mode) {
    LOG_INFO(Lib_Http, "called id={}, name={}, value={}, mode={}", id, name ? name : "(null)",
             value ? value : "(null)", mode);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }

    if (mode != SCE_HTTP_HEADER_OVERWRITE && mode != SCE_HTTP_HEADER_ADD) {
        LOG_ERROR(Lib_Http, "Invalid mode={}", mode);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    if (!name || !value) {
        LOG_ERROR(Lib_Http, "name or value is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    HttpHeaders* target = nullptr;
    const char* level = "";
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target = &it->second.headers;
        level = "template";
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target = &it->second.headers;
        level = "connection";
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target = &it->second->headers;
        level = "request";
    }
    if (!target) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    if (mode == SCE_HTTP_HEADER_OVERWRITE) {
        HeaderOps::Replace(*target, name, value);
    } else { // SCE_HTTP_HEADER_ADD
        HeaderOps::Append(*target, name, value);
    }
    LOG_INFO(Lib_Http, "stored header at {} level (id={}): {}={} (mode={})", level, id, name, value,
             mode == SCE_HTTP_HEADER_OVERWRITE ? "OVERWRITE" : "ADD");
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

    bool is_secure = false;
    if (int sc = CheckScheme(scheme, is_secure); sc < 0) {
        LOG_ERROR(Lib_Http, "scheme rejected: '{}' -> {:#x}", scheme ? scheme : "(null)", sc);
        return sc;
    }
    if (!serverName) {
        LOG_ERROR(Lib_Http, "serverName is null");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }
    if (port == 0) {
        port = is_secure ? 443 : 80;
    }

    const std::string scheme_str = is_secure ? "https" : "http";
    int conn_id = ++g_state.next_obj_id;
    HttpConnection conn;
    conn.tmpl_id = tmplId;
    conn.scheme = scheme_str;
    conn.hostname = serverName;
    conn.port = port;
    conn.keep_alive = (isEnableKeepalive != 0);
    conn.is_secure = is_secure;
    conn.url = scheme_str + "://" + serverName + ":" + std::to_string(port);
    if (auto tmpl_it = g_state.templates.find(tmplId); tmpl_it != g_state.templates.end()) {
        conn.nonblock = tmpl_it->second.nonblock;
        conn.epoll_id = tmpl_it->second.epoll_id;
        conn.epoll_user_arg = tmpl_it->second.epoll_user_arg;
        conn.settings = tmpl_it->second.settings;
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
    u16 port = parsed.port != 0 ? parsed.port : (is_secure ? 443 : 80);

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
        conn.nonblock = tmpl_it->second.nonblock;
        conn.epoll_id = tmpl_it->second.epoll_id;
        conn.epoll_user_arg = tmpl_it->second.epoll_user_arg;
        conn.settings = tmpl_it->second.settings;
    }
    g_state.connections.emplace(conn_id, std::move(conn));
    LOG_INFO(Lib_Http, "created connection connId={} host={} port={} scheme={}", conn_id,
             g_state.connections[conn_id].hostname, port, scheme_str);
    return conn_id;
}

int PS4_SYSV_ABI sceHttpCreateEpoll(int libhttpCtxId, OrbisHttpEpollHandle* eh) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    if (!eh) {
        LOG_ERROR(Lib_Http, "eh output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    int epoll_id = ++g_state.next_obj_id;
    auto epoll = std::make_shared<Epoll>();
    epoll->ctx_id = libhttpCtxId;
    g_state.epolls.emplace(epoll_id, std::move(epoll));
    *eh = EncodeEpollHandle(epoll_id);
    LOG_INFO(Lib_Http, "created epoll id={} (handle={})", epoll_id, fmt::ptr(*eh));
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceHttpCreateRequest2(int connId, const char* method, const char* path,
                                       u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, path={}, contentLength={}", connId,
             method ? method : "(null)", path ? path : "(null)", contentLength);
    auto map_method = [](const char* m) -> int {
        if (!m)
            return -1;
        std::string s = m;
        if (s == "GET")
            return SCE_HTTP_METHOD_GET;
        if (s == "POST")
            return SCE_HTTP_METHOD_POST;
        if (s == "HEAD")
            return SCE_HTTP_METHOD_HEAD;
        if (s == "PUT")
            return SCE_HTTP_METHOD_PUT;
        if (s == "DELETE")
            return SCE_HTTP_METHOD_DELETE;
        if (s == "TRACE")
            return SCE_HTTP_METHOD_TRACE;
        return -1;
    };
    if (!path) {
        LOG_ERROR(Lib_Http, "path is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    if (ContainsCrLf(path)) {
        LOG_ERROR(Lib_Http, "path contains CR/LF (CRLF-injection rejected): {}", path);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int int_method = map_method(method);
    if (int_method < 0) {
        if (!method) {
            LOG_ERROR(Lib_Http, "method is null");
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        LOG_INFO(Lib_Http, "method '{}' not in standard table; routing via CUSTOM slot", method);
        int_method = SCE_HTTP_METHOD_CUSTOM;
    }
    // Resolve the connection's URL under the lock, then delegate.
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

    const bool method_in_range = method >= 0 && method <= 8;
    const bool method_in_mask = method_in_range && ((SCE_HTTP_VALID_METHOD_MASK >> method) & 1u);
    if (!method_in_mask) {
        LOG_ERROR(Lib_Http, "Unknown method={} (bitmask 0x{:x} rejects it)", method,
                  SCE_HTTP_VALID_METHOD_MASK);
        return ORBIS_HTTP_ERROR_UNKNOWN_METHOD;
    }
    if (!url) {
        LOG_ERROR(Lib_Http, "url is null");
        return ORBIS_HTTP_ERROR_INVALID_URL;
    }
    if (ContainsCrLf(url)) {
        LOG_ERROR(Lib_Http, "url contains CR/LF (CRLF-injection rejected): {}", url);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    int req_id = ++g_state.next_obj_id;
    auto req = std::make_shared<HttpRequest>();
    req->conn_id = connId;
    req->method = method;
    req->url = url;
    req->content_length = contentLength;
    req->nonblock = conn_it->second.nonblock;
    req->epoll_id = conn_it->second.epoll_id;
    req->epoll_user_arg = conn_it->second.epoll_user_arg;
    req->settings = conn_it->second.settings;

    auto tmpl_it = g_state.templates.find(conn_it->second.tmpl_id);
    if (tmpl_it != g_state.templates.end()) {
        // TODO PS4 emits User-Agent (the system suffix "libhttp/<fw>
        // (PlayStation 4)") even when the game-supplied prefix is null.
        if (!tmpl_it->second.user_agent.empty()) {
            HeaderOps::Replace(req->headers, "User-Agent", tmpl_it->second.user_agent);
        }
    }
    HeaderOps::Replace(req->headers, "Host", conn_it->second.hostname);
    if (tmpl_it != g_state.templates.end()) {
        for (const auto& [k, v] : tmpl_it->second.headers) {
            HeaderOps::Append(req->headers, k, v);
        }
    }
    for (const auto& [k, v] : conn_it->second.headers) {
        HeaderOps::Append(req->headers, k, v);
    }
    auto inherited_headers_count = req->headers.size();
    g_state.requests.emplace(req_id, std::move(req));
    LOG_INFO(Lib_Http, "created request reqId={} (inherited {} headers)", req_id,
             inherited_headers_count);
    return req_id;
}

int PS4_SYSV_ABI sceHttpCreateRequestWithURL2(int connId, const char* method, const char* url,
                                              u64 contentLength) {
    LOG_INFO(Lib_Http, "called connId={}, method={}, url={}, contentLength={}", connId,
             method ? method : "(null)", url ? url : "(null)", contentLength);
    int int_method = -1;
    if (method) {
        std::string s = method;
        if (s == "GET")
            int_method = SCE_HTTP_METHOD_GET;
        else if (s == "POST")
            int_method = SCE_HTTP_METHOD_POST;
        else if (s == "HEAD")
            int_method = SCE_HTTP_METHOD_HEAD;
        else if (s == "PUT")
            int_method = SCE_HTTP_METHOD_PUT;
        else if (s == "DELETE")
            int_method = SCE_HTTP_METHOD_DELETE;
        else if (s == "TRACE")
            int_method = SCE_HTTP_METHOD_TRACE;
    }
    if (int_method < 0) {
        if (!method) {
            LOG_ERROR(Lib_Http, "method is null");
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        LOG_INFO(Lib_Http, "method '{}' not in standard table; routing via CUSTOM slot", method);
        int_method = SCE_HTTP_METHOD_CUSTOM;
    }
    int reqId = sceHttpCreateRequestWithURL(connId, int_method, url, contentLength);
    if (reqId > 0 && method) {
        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        auto it = g_state.requests.find(reqId);
        if (it != g_state.requests.end()) {
            it->second->method_str = method;
        }
    }
    return reqId;
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
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={} (not in active contexts)", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (httpVer != SCE_HTTP_VERSION_1_0 && httpVer != SCE_HTTP_VERSION_1_1) {
        LOG_ERROR(Lib_Http, "Invalid httpVer={}", httpVer);
        return ORBIS_HTTP_ERROR_INVALID_VERSION;
    }

    int tmpl_id = ++g_state.next_obj_id;
    HttpTemplate tmpl;
    // TODO (libhttp/PS4-suffix): real PS4 ALWAYS appends
    //    " libhttp/<firmware-version> (PlayStation 4)"
    tmpl.user_agent = userAgent ? userAgent : "";
    tmpl.http_version = httpVer;
    tmpl.auto_proxy_conf = isAutoProxyConf;
    tmpl.settings.accept_encoding_gzip = g_state.process_default_accept_encoding_gzip;
    g_state.templates.emplace(tmpl_id, std::move(tmpl));
    LOG_INFO(Lib_Http, "created template tmplId={}", tmpl_id);
    return tmpl_id;
}

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
    auto req_ptr = it->second; // shared_ptr copy keeps it alive past erase.
    req_ptr->deleted = true;
    if (req_ptr->state == HttpRequestState::Created) {
        req_ptr->state = HttpRequestState::Aborted;
    } else {
        req_ptr->state = HttpRequestState::Aborted;
        req_ptr->cv.notify_all();
    }
    g_state.requests.erase(it);
    return ORBIS_OK;
}

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

int PS4_SYSV_ABI sceHttpDestroyEpoll(int libhttpCtxId, OrbisHttpEpollHandle eh) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, eh={}", libhttpCtxId, fmt::ptr(eh));
    int epoll_id = DecodeEpollHandle(eh);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
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

int PS4_SYSV_ABI sceHttpGetAllResponseHeaders(int reqId, char** header, u64* headerSize) {
    LOG_INFO(Lib_Http, "called reqId={}, header={}, headerSize={}", reqId, fmt::ptr(header),
             fmt::ptr(headerSize));
    if (!header || !headerSize) {
        LOG_ERROR(Lib_Http, "header or headerSize output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
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
    int wait_result = WaitForResponseReady(req, lock);
    if (wait_result != ORBIS_OK) {
        if (wait_result == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, wait_result);
        }
        return wait_result;
    }
    *header = const_cast<char*>(req.res.all_headers_blob.c_str());
    *headerSize = req.res.all_headers_blob.size();
    LOG_INFO(Lib_Http, "reqId={} headers blob size={}", reqId, *headerSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetEpoll(int id, OrbisHttpEpollHandle* eh, void** userArg) {
    LOG_INFO(Lib_Http, "called id={}, eh={}, userArg={}", id, fmt::ptr(eh), fmt::ptr(userArg));
    if (!eh || !userArg) {
        LOG_ERROR(Lib_Http, "eh or userArg output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const int* src_epoll_id = nullptr;
    void* const* src_user_arg = nullptr;
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        src_epoll_id = &it->second.epoll_id;
        src_user_arg = &it->second.epoll_user_arg;
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        src_epoll_id = &it->second.epoll_id;
        src_user_arg = &it->second.epoll_user_arg;
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        src_epoll_id = &it->second->epoll_id;
        src_user_arg = &it->second->epoll_user_arg;
    }
    if (!src_epoll_id) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *eh = EncodeEpollHandle(*src_epoll_id);
    *userArg = *src_user_arg;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetNonblock(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, fmt::ptr(isEnable));
    if (!isEnable) {
        LOG_ERROR(Lib_Http, "isEnable output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const bool* target = nullptr;
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target = &it->second.nonblock;
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target = &it->second.nonblock;
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target = &it->second->nonblock;
    }
    if (!target) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *isEnable = *target ? 1 : 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetResponseContentLength(int reqId, int* result, u64* contentLength) {
    LOG_INFO(Lib_Http, "called reqId={}, result={}, contentLength={}", reqId, fmt::ptr(result),
             fmt::ptr(contentLength));
    if (!result || !contentLength) {
        LOG_ERROR(Lib_Http, "result or contentLength output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
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
    int wait_result = WaitForResponseReady(req, lock);
    if (wait_result != ORBIS_OK) {
        if (wait_result == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, wait_result);
        }
        return wait_result;
    }
    *result = req.res.content_length_result;
    *contentLength = req.res.content_length;
    LOG_INFO(Lib_Http, "reqId={} contentLength={} result={}", reqId, req.res.content_length,
             req.res.content_length_result);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetStatusCode(int reqId, int* statusCode) {
    LOG_INFO(Lib_Http, "called reqId={}, statusCode={}", reqId, fmt::ptr(statusCode));
    if (!statusCode) {
        LOG_ERROR(Lib_Http, "statusCode output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
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
    int wait_result = WaitForResponseReady(req, lock);
    if (wait_result != ORBIS_OK) {
        if (wait_result == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, wait_result);
        }
        return wait_result;
    }
    *statusCode = req.res.status_code;
    LOG_INFO(Lib_Http, "reqId={} status={}", reqId, req.res.status_code);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpInit(int libnetMemId, int libsslCtxId, u64 poolSize) {
    LOG_INFO(Lib_Http, "called libnetMemId={}, libsslCtxId={}, poolSize={}", libnetMemId,
             libsslCtxId, poolSize);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (poolSize == 0) {
        LOG_ERROR(Lib_Http, "poolSize=0 (returning ORBIS_KERNEL_ERROR_EINVAL "
                            "from underlying sceKernelMapNamedFlexibleMemory)");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (!g_state.ssl_status_logged) {
        g_state.ssl_status_logged = true;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        LOG_INFO(Lib_Http, "cpp-httplib was built with OpenSSL,https:// real-network "
                           "requests will be attempted");
#else
        LOG_INFO(Lib_Http, "cpp-httplib was built WITHOUT OpenSSL,every https:// real-"
                           "network request will throw and be caught and fall back to mock.");
#endif
#ifdef CPPHTTPLIB_ZLIB_SUPPORT
        LOG_INFO(Lib_Http, "cpp-httplib was built with zlib, gzip-compressed responses "
                           "will be transparently decompressed");
#else
        LOG_INFO(Lib_Http, "cpp-httplib was built WITHOUT zlib, gzip-compressed responses "
                           "will be returned to games as raw gzip bytes");
#endif
    }
    int ctx_id = ++g_state.next_ctx_id;
    g_state.active_contexts.insert(ctx_id);
    g_state.inited = true; // True as long as ANY context is alive.
    g_state.shutting_down.store(false);
    LOG_INFO(Lib_Http, "initialized -> ctxId={} (active contexts: {})", ctx_id,
             g_state.active_contexts.size());
    return ctx_id;
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
    LOG_INFO(Lib_Http, "called reqId={}, data={}, size={}", reqId, fmt::ptr(data), size);
    if (!data) {
        LOG_ERROR(Lib_Http, "data output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
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
    int wait_result = WaitForResponseReady(req, lock);
    if (wait_result != ORBIS_OK) {
        if (wait_result == ORBIS_HTTP_ERROR_EAGAIN) {
            LOG_DEBUG(Lib_Http, "reqId={}: EAGAIN (response not yet ready)", reqId);
        } else {
            LOG_ERROR(Lib_Http, "Wait failed for reqId={}: {:#x}", reqId, wait_result);
        }
        return wait_result;
    }
    u64 remaining = req.res.body.size() - req.res.read_cursor;
    u64 to_copy = std::min(size, remaining);
    if (to_copy > 0) {
        std::memcpy(data, req.res.body.data() + req.res.read_cursor, to_copy);
        req.res.read_cursor += to_copy;
    }
    LOG_INFO(Lib_Http, "reqId={} copied {} bytes (cursor now at {} / {})", reqId, to_copy,
             req.res.read_cursor, req.res.body.size());
    return static_cast<int>(to_copy);
}

int PS4_SYSV_ABI sceHttpRemoveRequestHeader(int id, const char* name) {
    LOG_INFO(Lib_Http, "called id={}, name={}", id, name ? name : "(null)");
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!name) {
        LOG_ERROR(Lib_Http, "name is null");
        return ORBIS_HTTP_ERROR_NOT_FOUND;
    }
    HttpHeaders* target = nullptr;
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target = &it->second.headers;
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target = &it->second.headers;
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target = &it->second->headers;
    }
    if (!target) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    // Remove EVERY matching entry, not just the first.
    const auto removed = HeaderOps::EraseAll(*target, name);
    LOG_INFO(Lib_Http, "removed {} entries for header '{}' (id={})", removed, name, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size) {
    LOG_INFO(Lib_Http, "called reqId={}, postData={}, size={}", reqId, fmt::ptr(postData), size);
    std::vector<u8> body_copy;
    if (postData && size > 0) {
        body_copy.assign(static_cast<const u8*>(postData), static_cast<const u8*>(postData) + size);
    }

    std::shared_ptr<HttpRequest> req_ptr;
    RealRequestPlan plan;
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
        // Send must happen exactly once per request
        if (req.state == HttpRequestState::Sending || req.state == HttpRequestState::Sent) {
            LOG_ERROR(Lib_Http, "Request already sent (reqId={})", reqId);
            return ORBIS_HTTP_ERROR_AFTER_SEND;
        }
        if (req.state == HttpRequestState::Aborted) {
            LOG_ERROR(Lib_Http, "Request was aborted (reqId={})", reqId);
            return ORBIS_HTTP_ERROR_ABORTED;
        }
        // Transition to Sending. Game's getters will see this state and
        // either block on cv (blocking mode) or return EAGAIN (non-blocking).
        req.state = HttpRequestState::Sending;
        req_ptr = it->second;
        plan.method = req.method;
        plan.method_str = req.method_str;
        plan.headers = req.headers;
        plan.body = std::move(body_copy);
        plan.settings = req.settings;
        plan.nonblock = req.nonblock;
        ParseRequestUrl(req.url, plan.scheme, plan.host, plan.port, plan.path);
    }

    bool will_try_real = kRealNetworkEnabled && !plan.host.empty() && !IsMockPsnHost(plan.host) &&
                         (plan.scheme == "http" || plan.scheme == "https");
    if (will_try_real) {
        LOG_INFO(Lib_Http, "reqId={} dispatched to async worker [REAL net: {} {}://{}:{}{}]", reqId,
                 HttpMethodName(plan.method), plan.scheme, plan.host, plan.port, plan.path);
    } else {
        LOG_INFO(Lib_Http,
                 "reqId={} dispatched to async worker [MOCK: latency ~{} ms, host={}, scheme={}]",
                 reqId, kMockLatency.count(), plan.host, plan.scheme);
    }

    std::thread([req_ptr, reqId, plan = std::move(plan)]() {
        bool try_real = kRealNetworkEnabled && !plan.host.empty() && !IsMockPsnHost(plan.host) &&
                        (plan.scheme == "http" || plan.scheme == "https");
        HttpResponse local_res;
        bool got_real_response = false;
        s32 real_errno = 0;
        s32 real_ssl_error = 0;
        u32 real_ssl_detail = 0;

        if (try_real) {
            got_real_response =
                ExecuteRealRequest(plan, local_res, &real_errno, &real_ssl_error, &real_ssl_detail);
        }

        if (!got_real_response) {
            std::this_thread::sleep_for(kMockLatency);
            SynthesizeMockResponse(local_res);
        }

        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (g_state.shutting_down.load() || req_ptr->deleted ||
            req_ptr->state == HttpRequestState::Aborted) {
            req_ptr->cv.notify_all();
            return;
        }
        // Move the response into the request and advance state.
        req_ptr->res = std::move(local_res);
        req_ptr->state = HttpRequestState::Sent;
        req_ptr->last_errno = real_errno;
        req_ptr->last_ssl_error = real_ssl_error;
        req_ptr->last_ssl_detail = real_ssl_detail;
        const char* path_label = got_real_response ? "(REAL)" : "(MOCK ASYNC)";
        LOG_INFO(Lib_Http, "{} reqId={} -> {} {} (body {} bytes)", path_label, reqId,
                 req_ptr->res.status_code, req_ptr->res.reason_phrase, req_ptr->res.body.size());
        // Wake any getters blocked waiting for state to advance.
        req_ptr->cv.notify_all();
        if (req_ptr->epoll_id != 0) {
            auto epoll_it = g_state.epolls.find(req_ptr->epoll_id);
            if (epoll_it != g_state.epolls.end() && !epoll_it->second->destroyed) {
                OrbisHttpNBEvent ev{};
                constexpr u32 kSuccessEventBits =
                    SCE_HTTP_NB_EVENT_RESOLVED | SCE_HTTP_NB_EVENT_IN | SCE_HTTP_NB_EVENT_OUT;
                ev.events = kSuccessEventBits;
                ev.eventDetail = kSuccessEventBits;
                ev.id = reqId;
                ev.userArg = req_ptr->epoll_user_arg;
                epoll_it->second->events.push_back(ev);
                epoll_it->second->cv.notify_all();
                LOG_INFO(Lib_Http, "{} pushed epoll event for reqId={} on epoll={}", path_label,
                         reqId, req_ptr->epoll_id);
            }
        }
    }).detach();

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetEpoll(int id, OrbisHttpEpollHandle eh, void* userArg) {
    LOG_INFO(Lib_Http, "called id={}, eh={}, userArg={}", id, fmt::ptr(eh), fmt::ptr(userArg));
    int epoll_id = DecodeEpollHandle(eh);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.epolls.contains(epoll_id)) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    int* target_epoll_id = nullptr;
    void** target_user_arg = nullptr;
    const char* level = "";
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target_epoll_id = &it->second.epoll_id;
        target_user_arg = &it->second.epoll_user_arg;
        level = "template";
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target_epoll_id = &it->second.epoll_id;
        target_user_arg = &it->second.epoll_user_arg;
        level = "connection";
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target_epoll_id = &it->second->epoll_id;
        target_user_arg = &it->second->epoll_user_arg;
        level = "request";
    }
    if (!target_epoll_id) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *target_epoll_id = epoll_id;
    *target_user_arg = userArg;
    LOG_INFO(Lib_Http, "set epoll={} userArg={} at {} level (id={})", epoll_id, fmt::ptr(userArg),
             level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetNonblock(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    bool* target = nullptr;
    const char* level = "";
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target = &it->second.nonblock;
        level = "template";
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target = &it->second.nonblock;
        level = "connection";
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target = &it->second->nonblock;
        level = "request";
    }
    if (!target) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *target = (isEnable != 0);
    LOG_INFO(Lib_Http, "set nonblock={} at {} level (id={})", *target, level, id);
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
        LOG_INFO(Lib_Http, "last context terminated, clearing state");
        g_state.shutting_down.store(true);
        for (auto& [id, req_ptr] : g_state.requests) {
            req_ptr->deleted = true;
            req_ptr->state = HttpRequestState::Aborted;
            req_ptr->cv.notify_all();
        }
        for (auto& [id, epoll_ptr] : g_state.epolls) {
            epoll_ptr->destroyed = true;
            epoll_ptr->cv.notify_all();
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

int PS4_SYSV_ABI sceHttpTryGetNonblock(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, fmt::ptr(isEnable));
    return sceHttpGetNonblock(id, isEnable);
}

int PS4_SYSV_ABI sceHttpTrySetNonblock(int id, int isEnable) {
    LOG_INFO(Lib_Http, "called id={}, isEnable={}", id, isEnable);
    return sceHttpSetNonblock(id, isEnable);
}

int PS4_SYSV_ABI sceHttpUnsetEpoll(int id) {
    LOG_INFO(Lib_Http, "called id={}", id);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    int* target_epoll_id = nullptr;
    void** target_user_arg = nullptr;
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        target_epoll_id = &it->second.epoll_id;
        target_user_arg = &it->second.epoll_user_arg;
    } else if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        target_epoll_id = &it->second.epoll_id;
        target_user_arg = &it->second.epoll_user_arg;
    } else if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        target_epoll_id = &it->second->epoll_id;
        target_user_arg = &it->second->epoll_user_arg;
    }
    if (!target_epoll_id) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    *target_epoll_id = 0;
    *target_user_arg = nullptr;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpWaitRequest(OrbisHttpEpollHandle eh, OrbisHttpNBEvent* nbev, int maxevents,
                                    int timeout) {
    LOG_INFO(Lib_Http, "called eh={}, nbev={}, maxevents={}, timeout={}", fmt::ptr(eh),
             fmt::ptr(nbev), maxevents, timeout);
    if (!nbev || maxevents <= 0) {
        LOG_ERROR(Lib_Http, "nbev null or maxevents<=0 (maxevents={})", maxevents);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    int epoll_id = DecodeEpollHandle(eh);
    std::unique_lock<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    auto it = g_state.epolls.find(epoll_id);
    if (it == g_state.epolls.end()) {
        LOG_ERROR(Lib_Http, "Invalid epoll handle (id={})", epoll_id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    auto epoll_ptr = it->second;

    auto drain_into_output = [&]() -> int {
        int count = 0;
        while (count < maxevents && !epoll_ptr->events.empty()) {
            nbev[count] = epoll_ptr->events.front();
            epoll_ptr->events.pop_front();
            ++count;
        }
        return count;
    };

    // events already queued. Drain and return immediately.
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
    if (epoll_ptr->abort_requested || epoll_ptr->destroyed || g_state.shutting_down.load()) {
        // Don't wait if we already know we'd be woken right away.
        return 0;
    }

    // Wait.
    auto predicate = [&]() {
        return !epoll_ptr->events.empty() || epoll_ptr->destroyed || epoll_ptr->abort_requested ||
               g_state.shutting_down.load();
    };
    if (timeout < 0) {
        epoll_ptr->cv.wait(lock, predicate);
    } else {
        epoll_ptr->cv.wait_for(lock, std::chrono::microseconds(timeout), predicate);
    }

    // After waking drain whatever we have.
    int count = drain_into_output();
    if (epoll_ptr->destroyed) {
        LOG_INFO(Lib_Http, "epoll id={} woken because destroyed; returning {} events", epoll_id,
                 count);
    } else if (epoll_ptr->abort_requested) {
        LOG_INFO(Lib_Http, "epoll id={} woken by abort; returning {} events", epoll_id, count);
    } else {
        LOG_INFO(Lib_Http, "epoll id={} returned {} events after wait", epoll_id, count);
    }
    return count;
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

    LOG_INFO(Lib_Http, "parsed successfully, requiredSize={}", requiredSize);
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

//***********************************
// Enable/Disable functions
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
// Get functions
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

int PS4_SYSV_ABI sceHttpsGetSslError(int id, int* errNum, u32* detail) {
    LOG_INFO(Lib_Http, "called id={}, errNum={}, detail={}", id, fmt::ptr(errNum),
             fmt::ptr(detail));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!errNum || !detail) {
        LOG_ERROR(Lib_Http, "errNum and detail output pointers must not be null (id={})", id);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }

    auto it = g_state.requests.find(id);
    if (it == g_state.requests.end()) {
        LOG_ERROR(Lib_Http, "Invalid reqId={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }

    *errNum = it->second->last_ssl_error;
    *detail = it->second->last_ssl_detail;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int id, int* isEnable) {
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
    *isEnable = s->accept_encoding_gzip ? 1 : 0;
    return ORBIS_OK;
}

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

//***********************************
// Set functions
//***********************************
int PS4_SYSV_ABI sceHttpsSetSslCallback(int id, OrbisHttpsCallback cbfunc, void* userArg) {
    LOG_INFO(Lib_Http, "called id={}, cbfunc={}, userArg={}", id,
             fmt::ptr(reinterpret_cast<void*>(cbfunc)), fmt::ptr(userArg));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->ssl_callback = cbfunc;
    s->ssl_callback_user_arg = userArg;
    LOG_INFO(Lib_Http, "ssl_callback {} at {} level (id={})", cbfunc ? "registered" : "cleared",
             level, id);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetRequestContentLength(int id, u64 contentLength) {
    LOG_INFO(Lib_Http, "called id={}, contentLength={}", id, contentLength);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited)
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
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

int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int isEnable) {
    LOG_INFO(Lib_Http, "called isEnable={}", isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    g_state.process_default_accept_encoding_gzip = (isEnable != 0);
    LOG_INFO(Lib_Http, "process-global default_accept_encoding_gzip={}", isEnable != 0);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetAcceptEncodingGZIPEnabled(int id, int isEnable) {
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
    s->accept_encoding_gzip = (isEnable != 0);
    LOG_INFO(Lib_Http, "accept_encoding_gzip={} at {} level (id={})", s->accept_encoding_gzip,
             level, id);
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
    // TODO
    // PS4 only follows 300,301,303,307
    // Http-lib also follows 302 and 308 but we can't skip that
    return ORBIS_OK;
}

//***********************************
// Stubbed functions
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

int PS4_SYSV_ABI sceHttpParseResponseHeader(const char* header, u64 headerLen, const char* fieldStr,
                                            const char** fieldValue, u64* valueLen) {
    LOG_ERROR(Lib_Http,
              "(STUBBED) called header={}, headerLen={}, fieldStr={}, fieldValue={}, valueLen={}",
              fmt::ptr(header), headerLen, fieldStr ? fieldStr : "(null)", fmt::ptr(fieldValue),
              fmt::ptr(valueLen));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRedirectCacheFlush(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpRequestGetAllHeaders() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
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
    LOG_INFO(Lib_Http, "called httpCtxId={}, list={} (DUMMY) returning empty CA list", httpCtxId,
             fmt::ptr(list));
    list->certsNum = 0;
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

int PS4_SYSV_ABI sceHttpsSetSslVersion(int id, int version) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, version={}", id, version);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsUnloadCert(int libhttpCtxId) {
    LOG_ERROR(Lib_Http, "(STUBBED) called libhttpCtxId={}", libhttpCtxId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpUriCopy() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
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
