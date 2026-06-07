// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "http_error.h"

#if __has_include(<httplib.h>)
#include <httplib.h>
#define ORBIS_HTTP_WITH_HTTPLIB 1
#endif

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
    u32 resolve_timeout_us = 0;  // DNS resolution timeout (sceHttpSetResolveTimeOut)
    s32 resolve_retry = 0;       // DNS retry count (sceHttpSetResolveRetry)
    u64 recv_block_size = 0;     // Hint to streaming receiver; 0 = default
    u64 response_header_max = 0; // Max response-header bytes accepted; 0 = default
    bool auto_redirect = true;
    bool inflate_gzip = true;                     // Auto-decompress response body if gzipped
    bool accept_encoding_gzip = true;             // Send "Accept-Encoding: gzip" request header
    u32 ssl_flags = ORBIS_HTTPS_FLAG_SDK_DEFAULT; // SSL flag mask. Bitmask of OrbisHttpsFlags.
    bool nonblock = false; // false = blocking (default), true = nonblock (EAGAIN)
    // (0 = no proxy, 1 = manual host:port, 2 = automatic/PAC).
    std::string proxy_host;
    u16 proxy_port = 0;
    int proxy_http_conf = 0;
    int proxy_wlan_conf = 0;
};

struct Epoll {
    int ctx_id = 0;
    std::deque<OrbisHttpNBEvent> events;
    std::condition_variable cv;
    bool destroyed = false;
    bool abort_requested = false;
};

struct HttpTemplate {
    int ctx_id = 0; // Owning libhttp context
    std::string user_agent;
    int http_version;
    int auto_proxy_conf;
    HttpSettings settings;
    int epoll_id = 0;
    void* epoll_user_arg = nullptr;
    std::vector<std::pair<std::string, std::string>> headers;
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
    std::vector<std::pair<std::string, std::string>> headers;
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
    std::vector<std::pair<std::string, std::string>> headers;
};

struct HttpState {
    std::mutex m_mutex;
    bool inited = false;
    int next_ctx_id = 0;
    int next_obj_id = 0;
    bool default_accept_encoding_gzip = true; // Library-wide default for new templates
    std::unordered_set<int> active_contexts;
    // Contexts where sceHttpsLoadCert was called. We can't actually parse the
    // PS4 cert blobs, but we use this as a signal that
    // the game expects custom CA validation. Connections under these contexts
    // bypass cpp-httplib's default verification so endpoints aren't blocked
    std::unordered_set<int> contexts_with_loaded_certs;
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

static bool HeaderNameMatches(std::string_view a, std::string_view b);
static std::string HttpStatusLabel(int sc);

// JSON shape: flat object mapping endpoint -> replacement URL. Example:
//   {
//     "api.something.dev":                    "http://localhost:8080",
//     "discovery.something.com:5300":         "http://localhost:8080",
//     "https://api.example.com:443":          "http://localhost:8081",
//     "*":                                    "http://localhost:8080"
//   }
//
// Keys are tried in order of specificity, most-specific first:
//   1. "scheme://host:port" - matches that exact endpoint
//   2. "host:port"          - matches host+port on any scheme
//   3. "host"               - matches host on any scheme/port (most common)
//   4. "*"                  - catch-all fallback
//
// Replacement value is a URL with scheme + host + optional port. When port is
// omitted the default for the scheme is used (80 for http, 443 for https).
// When scheme is omitted the connection's original scheme is preserved.
struct HostOverrideTarget {
    std::string scheme; // "http", "https", or "" to mean "preserve original"
    std::string host;
    u16 port = 0; // 0 means "preserve original (or default-for-scheme if scheme changed)"
};

// Parse a JSON string into a hostname to target map
std::unordered_map<std::string, HostOverrideTarget> ParseHostOverridesJson(
    const std::string& json_text);

bool ApplyHostOverride(std::string& scheme, std::string& host, u16& port, bool& is_secure);

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

// Parse a single replacement value into a HostOverrideTarget. Accepts forms:
//   "host"                   preserve scheme, preserve port
//   "host:port"              preserve scheme, set port
//   "http://host"            set scheme to http, port defaults to 80
//   "https://host"           set scheme to https, port defaults to 443
//   "http://host:port"       set scheme + port explicitly
//   "https://host:port"      set scheme + port explicitly
// Bad ports fall back to port=0
static HostOverrideTarget ParseHostOverrideTarget(std::string_view value) {
    HostOverrideTarget out;

    // Pull off scheme prefix if present.
    if (const auto scheme_end = value.find("://"); scheme_end != std::string_view::npos) {
        std::string scheme(value.substr(0, scheme_end));
        std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (scheme == "http" || scheme == "https") {
            out.scheme = std::move(scheme);
        }
        // Unknown scheme,leave out.scheme empty
        value = value.substr(scheme_end + 3);
    }

    // Now value is "host" or "host:port".
    if (const auto colon = value.find(':'); colon != std::string_view::npos) {
        out.host.assign(value.substr(0, colon));
        try {
            const unsigned long p = std::stoul(std::string(value.substr(colon + 1)));
            if (p > 0 && p <= 65535) {
                out.port = static_cast<u16>(p);
            }
        } catch (...) {
            // ignore - port stays 0
        }
    } else {
        out.host.assign(value);
    }
    return out;
}

std::unordered_map<std::string, HostOverrideTarget> ParseHostOverridesJson(
    const std::string& json_text) {
    std::unordered_map<std::string, HostOverrideTarget> out;
    if (json_text.empty()) {
        return out;
    }
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(json_text);
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_Http, "host overrides JSON parse failed: {}", e.what());
        return out;
    }
    if (!root.is_object()) {
        LOG_ERROR(Lib_Http, "host overrides JSON root must be an object");
        return out;
    }
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.key().empty() && it.key().front() == '_') {
            continue;
        }
        if (!it.value().is_string()) {
            LOG_ERROR(Lib_Http, "host overrides JSON: value for '{}' is not a string; skipped",
                      it.key());
            continue;
        }
        const std::string value = it.value().get<std::string>();
        if (value.empty()) {
            LOG_ERROR(Lib_Http, "host overrides JSON: value for '{}' is empty; skipped", it.key());
            continue;
        }
        out.emplace(it.key(), ParseHostOverrideTarget(value));
    }
    return out;
}

struct HostOverrideState {
    bool loaded = false;
    std::unordered_map<std::string, HostOverrideTarget> entries;
};

static HostOverrideState LoadHostOverrideState() {
    HostOverrideState s;
    s.loaded = true;
    std::filesystem::path path;
    if (const char* path_env = std::getenv("SHADPS4_HTTP_HOST_OVERRIDES_JSON");
        path_env && path_env[0]) {
        // Explicit override path - useful for dev / testing.
        path = path_env;
    } else {
        path = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "host_overrides.json";
    }
    std::ifstream f(path);
    if (!f.is_open()) {
        return s;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    s.entries = ParseHostOverridesJson(buf.str());
    LOG_INFO(Lib_Http, "loaded {} host override entries from {}", s.entries.size(), path.string());
    return s;
}

static const HostOverrideState& GetHostOverrideState() {
    static const HostOverrideState s = LoadHostOverrideState();
    return s;
}

bool ApplyHostOverride(std::string& scheme, std::string& host, u16& port, bool& is_secure) {
    const auto& state = GetHostOverrideState();
    if (state.entries.empty()) {
        return false;
    }
    // Look up most-specific match first. Keys can be:
    //   "scheme://host:port"  - matches that exact endpoint
    //   "host:port"           - matches host+port on any scheme
    //   "host"                - matches host on any scheme/port
    //   "*"                   - catch-all fallback
    const std::string full_key = scheme + "://" + host + ":" + std::to_string(port);
    const std::string host_port_key = host + ":" + std::to_string(port);

    auto it = state.entries.find(full_key);
    if (it == state.entries.end()) {
        it = state.entries.find(host_port_key);
    }
    if (it == state.entries.end()) {
        it = state.entries.find(host);
    }
    if (it == state.entries.end()) {
        it = state.entries.find("*");
    }
    if (it == state.entries.end()) {
        return false;
    }

    const std::string orig_scheme = scheme;
    const std::string orig_host = host;
    const u16 orig_port = port;
    const HostOverrideTarget& target = it->second;

    host = target.host;

    // Scheme handling: explicit scheme in JSON wins; otherwise preserve.
    if (!target.scheme.empty()) {
        scheme = target.scheme;
        is_secure = (target.scheme == "https");
    }

    if (target.port != 0) {
        port = target.port;
    } else if (!target.scheme.empty() && target.scheme != orig_scheme) {
        if (orig_scheme == "https" && port == 443) {
            port = 80;
        } else if (orig_scheme == "http" && port == 80) {
            port = 443;
        }
    }

    LOG_INFO(Lib_Http, "host override active: {}://{}:{} -> {}://{}:{} (matched key '{}')",
             orig_scheme, orig_host, orig_port, scheme, host, port, it->first);
    return true;
}

struct SendRequestPlan {
    std::string scheme;
    std::string host;
    u16 port = 0;
    std::string path; // Path + query, e.g. "/utility/time" or "/?x=1"
    s32 method = 0;
    std::string method_str; // Only used when method == ORBIS_HTTP_METHOD_CUSTOM
    std::string content_type;
    std::vector<u8> body;
    std::vector<std::pair<std::string, std::string>> headers; // Merged tmpl + conn + req
    HttpSettings settings;
    // True if the request's owning ctx had sceHttpsLoadCert called. In that
    // case we bypass TLS verification because we can't load the game's CAs.
    bool ctx_has_loaded_certs = false;
};

// Extract the path-and-query portion from a full URL
static std::string ExtractPathFromUrl(const std::string& url) {
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return "/";
    }
    auto authority_start = scheme_end + 3;
    auto path_start = url.find('/', authority_start);
    if (path_start == std::string::npos) {
        return "/";
    }
    return url.substr(path_start);
}

// Map common HTTP method codes to their name. Used for logging only.
static const char* HttpMethodName(s32 method) {
    switch (method) {
    case ORBIS_HTTP_METHOD_GET:
        return "GET";
    case ORBIS_HTTP_METHOD_POST:
        return "POST";
    case ORBIS_HTTP_METHOD_HEAD:
        return "HEAD";
    case ORBIS_HTTP_METHOD_OPTIONS:
        return "OPTIONS";
    case ORBIS_HTTP_METHOD_PUT:
        return "PUT";
    case ORBIS_HTTP_METHOD_DELETE:
        return "DELETE";
    case ORBIS_HTTP_METHOD_TRACE:
        return "TRACE";
    case ORBIS_HTTP_METHOD_CONNECT:
        return "CONNECT";
    case ORBIS_HTTP_METHOD_CUSTOM:
        return "CUSTOM";
    default:
        return "?";
    }
}

// Dump every effective setting and the merged header list when a request is
// about to be sent.
static void LogSendRequestSettings(const HttpRequest& req, int reqId, u64 body_size) {
    const HttpSettings& s = req.settings;
    LOG_INFO(Lib_Http, "--- SendRequest dump reqId={} ---", reqId);
    const char* method_name = "(unset)";
    if (!req.method_str.empty()) {
        method_name = req.method_str.c_str();
    } else {
        switch (req.method) {
        case ORBIS_HTTP_METHOD_GET:
            method_name = "GET";
            break;
        case ORBIS_HTTP_METHOD_POST:
            method_name = "POST";
            break;
        case ORBIS_HTTP_METHOD_HEAD:
            method_name = "HEAD";
            break;
        case ORBIS_HTTP_METHOD_OPTIONS:
            method_name = "OPTIONS";
            break;
        case ORBIS_HTTP_METHOD_PUT:
            method_name = "PUT";
            break;
        case ORBIS_HTTP_METHOD_DELETE:
            method_name = "DELETE";
            break;
        case ORBIS_HTTP_METHOD_TRACE:
            method_name = "TRACE";
            break;
        case ORBIS_HTTP_METHOD_CONNECT:
            method_name = "CONNECT";
            break;
        case ORBIS_HTTP_METHOD_CUSTOM:
            method_name = "CUSTOM";
            break;
        default:
            break;
        }
    }
    LOG_INFO(Lib_Http, "  method={} url={} body_size={}", method_name,
             req.url.empty() ? "(unset)" : req.url.c_str(), body_size);

    // Resolve the owning connection + template (for full URL context and
    // header inheritance dump).
    const HttpConnection* conn = nullptr;
    const HttpTemplate* tmpl = nullptr;
    if (auto it = g_state.connections.find(req.conn_id); it != g_state.connections.end()) {
        conn = &it->second;
        if (auto tit = g_state.templates.find(conn->tmpl_id); tit != g_state.templates.end()) {
            tmpl = &tit->second;
        }
    }
    if (conn) {
        LOG_INFO(Lib_Http, "  connection: {}://{}:{} keep_alive={} secure={}", conn->scheme,
                 conn->hostname, conn->port, conn->keep_alive, conn->is_secure);
    }
    if (tmpl) {
        LOG_INFO(Lib_Http, "  template: ua=\"{}\" http_ver={} auto_proxy_conf={}", tmpl->user_agent,
                 tmpl->http_version, tmpl->auto_proxy_conf);
    }

    // Timeouts and basic flags
    LOG_INFO(Lib_Http, "  timeouts: connect={}us send={}us recv={}us resolve={}us resolve_retry={}",
             s.connect_timeout_us, s.send_timeout_us, s.recv_timeout_us, s.resolve_timeout_us,
             s.resolve_retry);
    LOG_INFO(Lib_Http,
             "  flags: auto_redirect={} inflate_gzip={} accept_encoding_gzip={} nonblock={}",
             s.auto_redirect, s.inflate_gzip, s.accept_encoding_gzip, s.nonblock);
    LOG_INFO(Lib_Http, "  buffers: recv_block_size={} response_header_max={}", s.recv_block_size,
             s.response_header_max);
    LOG_INFO(Lib_Http, "  ssl_flags={:#x}", s.ssl_flags);
    if (!s.proxy_host.empty() || s.proxy_http_conf != 0 || s.proxy_wlan_conf != 0) {
        LOG_INFO(Lib_Http, "  proxy: {}:{} http_conf={} wlan_conf={}",
                 s.proxy_host.empty() ? "(empty)" : s.proxy_host.c_str(), s.proxy_port,
                 s.proxy_http_conf, s.proxy_wlan_conf);
    }
    if (req.epoll_id != 0) {
        LOG_INFO(Lib_Http, "  epoll: bound to epoll_id={} user_arg={}", req.epoll_id,
                 req.epoll_user_arg);
    }

    // Merged header view: dump tmpl + conn + req lists
    auto dump_headers = [&](const char* origin,
                            const std::vector<std::pair<std::string, std::string>>& h) {
        if (h.empty()) {
            return;
        }
        for (const auto& [name, value] : h) {
            LOG_INFO(Lib_Http, "  header[{}] {}: {}", origin, name, value);
        }
    };
    if (tmpl) {
        dump_headers("template", tmpl->headers);
    }
    if (conn) {
        dump_headers("connection", conn->headers);
    }
    dump_headers("request", req.headers);
    LOG_INFO(Lib_Http, "--- end dump reqId={} ---", reqId);
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#define ORBIS_HTTP_HAS_HTTPS 1
#else
#define ORBIS_HTTP_HAS_HTTPS 0
#endif

#ifdef ORBIS_HTTP_WITH_HTTPLIB
static s32 TranslateHttplibError(httplib::Error err) {
    using E = httplib::Error;
    switch (err) {
    case E::Success:
        return ORBIS_OK;
    // TCP couldn't be established (DNS resolved but the host refused / no
    // route / unreachable). Closest firmware code is ResolverEnohost.
    case E::Connection:
        return ORBIS_HTTP_ERROR_RESOLVER_ENOHOST;
    // Timed out at connect time or during the request.
    case E::ConnectionTimeout:
    case E::Timeout:
        return ORBIS_HTTP_ERROR_TIMEOUT;
    // TCP set up but the stream broke mid-flight, or peer closed early.
    case E::Read:
    case E::Write:
    case E::ConnectionClosed:
        return ORBIS_HTTP_ERROR_BROKEN;
    // Anything TLS-related.
    case E::SSLConnection:
    case E::SSLLoadingCerts:
    case E::SSLServerVerification:
    case E::SSLServerHostnameVerification:
        return ORBIS_HTTP_ERROR_SSL;
    // CONNECT-method proxy handshake failed.
    case E::ProxyConnection:
        return ORBIS_HTTP_ERROR_PROXY;
    // User aborted or library cancelled.
    case E::Canceled:
        return ORBIS_HTTP_ERROR_ABORTED;
    // Exceeded our redirect-loop bound
    case E::ExceedRedirectCount:
        return ORBIS_HTTP_ERROR_NETWORK;
    // Internal-bug paths that exist across all cpp-httplib versions.
    case E::Unknown:
    case E::BindIPAddress:
    case E::UnsupportedMultipartBoundaryChars:
    case E::Compression:
        return ORBIS_HTTP_ERROR_UNKNOWN;
    }
    return ORBIS_HTTP_ERROR_UNKNOWN;
}
#endif // ORBIS_HTTP_WITH_HTTPLIB

constexpr int MaxRedirects = 5;

bool IsFollowableRedirect(int status, s32 method) {
    const bool status_in_set = (status >= 300 && status <= 303) || (status == 307);
    if (!status_in_set) {
        return false;
    }
    if (method == ORBIS_HTTP_METHOD_POST && status != 303) {
        return false;
    }
    return true;
}

// 303 changes the new method to GET unless original was HEAD
// Every other followable status preserves the original method.
s32 MethodAfterRedirect(int status, s32 original_method) {
    if (status == 303 && original_method != ORBIS_HTTP_METHOD_HEAD) {
        return ORBIS_HTTP_METHOD_GET;
    }
    return original_method;
}

struct ResolvedRedirect {
    std::string scheme;
    std::string host;
    u16 port;
    std::string path;
};

// Resolve a Location value relative to the current request's authority.
// Handles absolute URLs and absolute-path-relative forms
std::optional<ResolvedRedirect> ResolveRedirectLocation(const std::string& current_scheme,
                                                        const std::string& current_host,
                                                        u16 current_port,
                                                        std::string_view location) {
    if (location.empty()) {
        return std::nullopt;
    }
    if (const auto scheme_end = location.find("://"); scheme_end != std::string_view::npos) {
        ResolvedRedirect out;
        out.scheme.assign(location.substr(0, scheme_end));
        std::transform(out.scheme.begin(), out.scheme.end(), out.scheme.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (out.scheme != "http" && out.scheme != "https") {
            return std::nullopt;
        }
        const auto authority_start = scheme_end + 3;
        if (authority_start >= location.size()) {
            return std::nullopt;
        }
        const auto path_start = location.find('/', authority_start);
        const std::string_view authority =
            (path_start == std::string_view::npos)
                ? location.substr(authority_start)
                : location.substr(authority_start, path_start - authority_start);
        if (authority.empty()) {
            return std::nullopt;
        }
        if (const auto colon = authority.find(':'); colon != std::string_view::npos) {
            out.host.assign(authority.substr(0, colon));
            try {
                const unsigned long p = std::stoul(std::string(authority.substr(colon + 1)));
                if (p == 0 || p > 65535) {
                    return std::nullopt;
                }
                out.port = static_cast<u16>(p);
            } catch (...) {
                return std::nullopt;
            }
        } else {
            out.host.assign(authority);
            out.port = (out.scheme == "https") ? 443 : 80;
        }
        out.path =
            (path_start == std::string_view::npos) ? "/" : std::string(location.substr(path_start));
        return out;
    }
    if (location[0] == '/') {
        ResolvedRedirect out;
        out.scheme = current_scheme;
        out.host = current_host;
        out.port = current_port;
        out.path.assign(location);
        return out;
    }
    return std::nullopt;
}

static s32 RunRealHttpRequest(const SendRequestPlan& plan_in, HttpResponse& out_res,
                              u32& out_event_bits) {
    out_event_bits = 0;
#ifndef ORBIS_HTTP_WITH_HTTPLIB
    // Test or no-httplib build: behave like a transport failure.
    SynthesizeTransportFailureResponse(out_res);
    LOG_INFO(Lib_Http,
             "real I/O path requested but httplib not available; "
             "falling back to transport failure for {}://{}{}",
             plan_in.scheme, plan_in.host, plan_in.path);
    return ORBIS_HTTP_ERROR_RESOLVER_ENODNS;
#else
    // Mutable copy: PS4-faithful redirect loop rewrites scheme/host/port/path/method.
    SendRequestPlan plan = plan_in;

    auto pick_timeout_seconds = [](u32 us, u32 default_s) -> std::chrono::seconds {
        if (us == 0) {
            return std::chrono::seconds(default_s);
        }
        u64 secs = (static_cast<u64>(us) + 999999ull) / 1000000ull;
        if (secs == 0) {
            secs = 1;
        }
        return std::chrono::seconds(secs);
    };

    for (int depth = 0; depth <= MaxRedirects; ++depth) {
        if (plan.scheme == "https" && !ORBIS_HTTP_HAS_HTTPS) {
            LOG_ERROR(Lib_Http, "HTTPS request but cpp-httplib lacks OpenSSL support");
            SynthesizeTransportFailureResponse(out_res);
            return ORBIS_HTTP_ERROR_SSL;
        }

        std::string base_url = plan.scheme + "://" + plan.host;
        if ((plan.scheme == "https" && plan.port != 443) ||
            (plan.scheme == "http" && plan.port != 80)) {
            base_url += ":" + std::to_string(plan.port);
        }
        httplib::Client cli(base_url);
        cli.set_connection_timeout(pick_timeout_seconds(plan.settings.connect_timeout_us, 30));
        cli.set_read_timeout(pick_timeout_seconds(plan.settings.recv_timeout_us, 120));
        cli.set_write_timeout(pick_timeout_seconds(plan.settings.send_timeout_us, 120));

        // We always handle redirects manually per PS4 rules
        cli.set_follow_location(false);

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
        cli.set_decompress(plan.settings.inflate_gzip);
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (plan.scheme == "https") {
            const bool game_disabled_verify =
                (plan.settings.ssl_flags & ORBIS_HTTPS_FLAG_SERVER_VERIFY) == 0;
            bool verify_server = !game_disabled_verify;
            if (verify_server && plan.ctx_has_loaded_certs) {
                verify_server = false;
                LOG_INFO(Lib_Http,
                         "{}://{}: ctx loaded custom CAs (libSceSsl integration absent) -> "
                         "bypassing cert verification",
                         plan.scheme, plan.host);
            }
            cli.enable_server_certificate_verification(verify_server);
            if (game_disabled_verify) {
                LOG_INFO(Lib_Http, "{}://{}: server cert verification disabled (ssl_flags={:#x})",
                         plan.scheme, plan.host, plan.settings.ssl_flags);
            }
        }
#endif

        httplib::Headers headers;
        for (const auto& [k, v] : plan.headers) {
            headers.emplace(k, v);
        }
        if (plan.settings.accept_encoding_gzip) {
            bool already_set = false;
            for (const auto& [k, v] : plan.headers) {
                if (HeaderNameMatches(k, "Accept-Encoding")) {
                    already_set = true;
                    break;
                }
            }
            if (!already_set) {
                headers.emplace("Accept-Encoding", "gzip");
            }
        }

        const char* body_ptr =
            plan.body.empty() ? "" : reinterpret_cast<const char*>(plan.body.data());
        const size_t body_size = plan.body.size();

        auto result = [&]() {
            switch (plan.method) {
            case ORBIS_HTTP_METHOD_GET:
                return cli.Get(plan.path, headers);
            case ORBIS_HTTP_METHOD_POST:
                return cli.Post(plan.path, headers, body_ptr, body_size, plan.content_type);
            case ORBIS_HTTP_METHOD_HEAD:
                return cli.Head(plan.path, headers);
            case ORBIS_HTTP_METHOD_OPTIONS:
                return cli.Options(plan.path, headers);
            case ORBIS_HTTP_METHOD_PUT:
                return cli.Put(plan.path, headers, body_ptr, body_size, plan.content_type);
            case ORBIS_HTTP_METHOD_DELETE:
                return cli.Delete(plan.path, headers, body_ptr, body_size, plan.content_type);
            case ORBIS_HTTP_METHOD_CUSTOM: {
                httplib::Request creq;
                creq.method = plan.method_str.empty() ? "GET" : plan.method_str;
                creq.path = plan.path;
                creq.headers = headers;
                if (body_size > 0) {
                    creq.body.assign(body_ptr, body_size);
                }
                return cli.send(creq);
            }
            default:
                LOG_ERROR(Lib_Http, "Unsupported method {}; using GET", plan.method);
                return cli.Get(plan.path, headers);
            }
        }();

        if (!result) {
            const int err_val = static_cast<int>(result.error());
            LOG_ERROR(Lib_Http, "cpp-httplib failed for {} {}{}: error={} ({})",
                      HttpMethodName(plan.method), base_url, plan.path, err_val,
                      httplib::to_string(static_cast<httplib::Error>(err_val)));
            SynthesizeTransportFailureResponse(out_res);
            return TranslateHttplibError(result.error());
        }

        // Populate response (overwrites any prior-iteration 3xx).
        out_res.status_code = result->status;
        out_res.body.assign(result->body.begin(), result->body.end());
        out_res.content_length = result->body.size();
        out_res.content_length_result = 0;
        out_res.read_cursor = 0;
        std::string reason;
        {
            const std::string label = HttpStatusLabel(result->status);
            const auto space = label.find(' ');
            reason = (space == std::string::npos) ? label : label.substr(space + 1);
        }
        out_res.all_headers_blob =
            "HTTP/1.1 " + std::to_string(result->status) + " " + reason + "\r\n";
        for (const auto& [k, v] : result->headers) {
            out_res.all_headers_blob += k + ": " + v + "\r\n";
        }
        out_res.all_headers_blob += "\r\n";

        // -- PS4-faithful redirect decision --
        if (!plan.settings.auto_redirect) {
            break;
        }
        if (depth >= MaxRedirects) {
            LOG_INFO(Lib_Http,
                     "redirect depth limit ({}) reached; returning final response status={}",
                     MaxRedirects, out_res.status_code);
            break;
        }
        if (!IsFollowableRedirect(out_res.status_code, plan.method)) {
            break;
        }

        std::string location;
        for (const auto& [k, v] : result->headers) {
            if (HeaderNameMatches(k, "Location")) {
                location = v;
                break;
            }
        }
        if (location.empty()) {
            LOG_INFO(Lib_Http, "{} response without Location header; returning as-is",
                     out_res.status_code);
            break;
        }
        auto resolved = ResolveRedirectLocation(plan.scheme, plan.host, plan.port, location);
        if (!resolved) {
            LOG_INFO(Lib_Http,
                     "{} response Location='{}' unresolvable (relative or malformed); "
                     "returning as-is",
                     out_res.status_code, location);
            break;
        }

        const int prev_status = out_res.status_code;
        const s32 prev_method = plan.method;
        const s32 next_method = MethodAfterRedirect(prev_status, prev_method);
        const bool host_changed = (resolved->host != plan.host) || (resolved->port != plan.port);

        plan.scheme = std::move(resolved->scheme);
        plan.host = std::move(resolved->host);
        plan.port = resolved->port;
        plan.path = std::move(resolved->path);
        plan.method = next_method;

        // 303 + non-HEAD downgrades the method to GET; drop the request body
        // and the headers that describe it.
        if (prev_status == 303 && prev_method != ORBIS_HTTP_METHOD_HEAD) {
            plan.body.clear();
            plan.content_type.clear();
            plan.headers.erase(
                std::remove_if(plan.headers.begin(), plan.headers.end(),
                               [](const auto& kv) {
                                   return HeaderNameMatches(kv.first, "Content-Type") ||
                                          HeaderNameMatches(kv.first, "Content-Length");
                               }),
                plan.headers.end());
        }

        // Cross-host: rewrite any game-supplied Host header
        // If the game didn't add one, cpp-httplib auto-generates the correct
        // value from the new base_url on the next iteration.
        if (host_changed) {
            std::string host_value = plan.host;
            const bool default_port = (plan.scheme == "https" && plan.port == 443) ||
                                      (plan.scheme == "http" && plan.port == 80);
            if (!default_port) {
                host_value += ":" + std::to_string(plan.port);
            }
            for (auto& [k, v] : plan.headers) {
                if (HeaderNameMatches(k, "Host")) {
                    v = host_value;
                }
            }
        }

        LOG_INFO(Lib_Http, "redirect: depth={} status={} {} -> {} {}://{}:{}{}", depth, prev_status,
                 HttpMethodName(prev_method), HttpMethodName(next_method), plan.scheme, plan.host,
                 plan.port, plan.path);
    }

    out_event_bits =
        ORBIS_HTTP_NB_EVENT_IN | ORBIS_HTTP_NB_EVENT_OUT | ORBIS_HTTP_NB_EVENT_RESOLVED;
    return 0;
#endif // ORBIS_HTTP_WITH_HTTPLIB
}

// Case-insensitive ASCII comparison of two HTTP header names.
static bool HeaderNameMatches(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Resolve the headers vector for a template/connection/request id. Returns
// nullptr if id is invalid
static std::vector<std::pair<std::string, std::string>>* ResolveHeaders(int id,
                                                                        const char*& level) {
    if (auto it = g_state.templates.find(id); it != g_state.templates.end()) {
        level = "template";
        return &it->second.headers;
    }
    if (auto it = g_state.connections.find(id); it != g_state.connections.end()) {
        level = "connection";
        return &it->second.headers;
    }
    if (auto it = g_state.requests.find(id); it != g_state.requests.end()) {
        level = "request";
        return &it->second->headers;
    }
    return nullptr;
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

    std::string scheme_str = is_secure ? "https" : "http";
    std::string host_str = serverName;
    u16 effective_port = port;
    ApplyHostOverride(scheme_str, host_str, effective_port, is_secure);
    const int conn_id = ++g_state.next_obj_id;
    HttpConnection conn;
    conn.tmpl_id = tmplId;
    conn.scheme = scheme_str;
    conn.hostname = host_str;
    conn.port = effective_port;
    conn.keep_alive = (isEnableKeepalive != 0);
    conn.is_secure = is_secure;
    conn.url = scheme_str + "://" + host_str + ":" + std::to_string(effective_port);
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
    std::string host_str = parsed.hostname;
    ApplyHostOverride(scheme_str, host_str, port, is_secure);

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
    conn.hostname = host_str;
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
    tmpl.ctx_id = libhttpCtxId;
    tmpl.user_agent = userAgent ? userAgent : "";
    tmpl.http_version = httpVer;
    tmpl.auto_proxy_conf = isAutoProxyConf;
    tmpl.settings.accept_encoding_gzip = g_state.default_accept_encoding_gzip;
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

int PS4_SYSV_ABI sceHttpRequestGetAllHeaders() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSendRequest(int reqId, const void* postData, u64 size) {
    LOG_INFO(Lib_Http, "called reqId={}, postData={}, size={}", reqId, fmt::ptr(postData), size);
    std::shared_ptr<HttpRequest> req_ptr;
    SendRequestPlan plan;
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
        LogSendRequestSettings(req, reqId, size);

        plan.method = req.method;
        plan.method_str = req.method_str;
        plan.path = ExtractPathFromUrl(req.url);
        plan.settings = req.settings;
        if (auto conn_it = g_state.connections.find(req.conn_id);
            conn_it != g_state.connections.end()) {
            plan.scheme = conn_it->second.scheme;
            plan.host = conn_it->second.hostname;
            plan.port = static_cast<u16>(conn_it->second.port);
            // Inherit headers in tmpl to conn to req order
            if (auto tmpl_it = g_state.templates.find(conn_it->second.tmpl_id);
                tmpl_it != g_state.templates.end()) {
                plan.headers = tmpl_it->second.headers;
                // Check if the owning context loaded custom CAs - if so the
                // worker will bypass TLS verification.
                plan.ctx_has_loaded_certs =
                    g_state.contexts_with_loaded_certs.contains(tmpl_it->second.ctx_id);
            }
            for (const auto& h : conn_it->second.headers) {
                plan.headers.push_back(h);
            }
        }
        for (const auto& h : req.headers) {
            plan.headers.push_back(h);
        }
        // Pull Content-Type out of headers
        for (const auto& [k, v] : plan.headers) {
            if (HeaderNameMatches(k, "Content-Type")) {
                plan.content_type = v;
                break;
            }
        }
        if (postData && size > 0) {
            plan.body.assign(static_cast<const u8*>(postData),
                             static_cast<const u8*>(postData) + size);
        }
    }

    const bool online = EmulatorSettings.IsConnectedToNetwork();
    LOG_INFO(Lib_Http, "reqId={} dispatched to async worker [{} {} {}://{}:{}{}]", reqId,
             online ? "ONLINE" : "OFFLINE", HttpMethodName(plan.method), plan.scheme, plan.host,
             plan.port, plan.path);

    std::thread([req_ptr, reqId, plan = std::move(plan), online]() {
        HttpResponse local_res;
        s32 worker_errno = 0;
        u32 success_event_bits = 0; // 0 = no event (offline path uses failure bits)

        if (!online) {
            SynthesizeTransportFailureResponse(local_res);
            worker_errno = ORBIS_HTTP_ERROR_RESOLVER_ENODNS;
        } else {
            worker_errno = RunRealHttpRequest(plan, local_res, success_event_bits);
        }

        std::lock_guard<std::mutex> lock(g_state.m_mutex);
        if (g_state.shutting_down.load() || req_ptr->deleted ||
            req_ptr->state == HttpRequestState::Aborted) {
            const char* reason = g_state.shutting_down.load() ? "shutdown"
                                 : req_ptr->deleted           ? "deleted"
                                                              : "aborted";
            LOG_INFO(Lib_Http,
                     "reqId={} worker finished but request was {} before completion; "
                     "dropping result (would have been status={}, errno={:#x})",
                     reqId, reason, local_res.status_code, static_cast<u32>(worker_errno));
            req_ptr->cv.notify_all();
            return;
        }
        req_ptr->res = std::move(local_res);
        req_ptr->state = HttpRequestState::Sent;
        req_ptr->last_errno = worker_errno;
        if (worker_errno == 0) {
            LOG_INFO(Lib_Http, "(SUCCESS) reqId={} status={} body={} bytes", reqId,
                     req_ptr->res.status_code, req_ptr->res.body.size());
        } else {
            LOG_INFO(Lib_Http, "(TRANSPORT FAIL) reqId={} -> {} (body {} bytes, errno={:#x})",
                     reqId, req_ptr->res.status_code, req_ptr->res.body.size(),
                     static_cast<u32>(worker_errno));
        }
        req_ptr->cv.notify_all();

        // Push an epoll event so sceHttpWaitRequest callers see completion.
        if (req_ptr->epoll_id != 0) {
            auto epoll_it = g_state.epolls.find(req_ptr->epoll_id);
            if (epoll_it != g_state.epolls.end() && !epoll_it->second->destroyed) {
                u32 event_bits;
                if (worker_errno == 0) {
                    event_bits = success_event_bits;
                } else {
                    event_bits = ORBIS_HTTP_NB_EVENT_RESOLVER_ERR | ORBIS_HTTP_NB_EVENT_HUP;
                }
                OrbisHttpNBEvent ev{};
                ev.events = event_bits;
                ev.eventDetail = event_bits;
                ev.id = reqId;
                ev.userArg = req_ptr->epoll_user_arg;
                epoll_it->second->events.push_back(ev);
                epoll_it->second->cv.notify_all();
                LOG_DEBUG(Lib_Http, "pushed epoll event for reqId={} on epoll={} bits={:#x}", reqId,
                          req_ptr->epoll_id, event_bits);
            }
        }
    }).detach();

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

int PS4_SYSV_ABI sceHttpSetSocketCreationCallback() {
    LOG_ERROR(Lib_Http, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsFreeCaList(int libhttpCtxId, OrbisHttpsCaList* caList) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, caList={}", libhttpCtxId, fmt::ptr(caList));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (!caList) {
        LOG_ERROR(Lib_Http, "caList is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    caList->certsNum = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetCaList(int httpCtxId, OrbisHttpsCaList* list) {
    LOG_INFO(Lib_Http, "called httpCtxId={}, list={}", httpCtxId, fmt::ptr(list));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(httpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid httpCtxId={}", httpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (!list) {
        LOG_ERROR(Lib_Http, "list output pointer is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    list->certsNum = 0;
    LOG_ERROR(Lib_Http, "returning empty CA list (libSceSsl integration not implemented)");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsGetSslError(int id, int* errNum, u32* detail) {
    LOG_ERROR(Lib_Http, "(STUBBED) called id={}, errNum={}, detail={}", id, fmt::ptr(errNum),
              fmt::ptr(detail));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpsLoadCert(int libhttpCtxId, int caCertNum, const void** caList,
                                  const void* cert, const void* privKey) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, caCertNum={}, caList={}, cert={}, privKey={}",
             libhttpCtxId, caCertNum, fmt::ptr(caList), fmt::ptr(cert), fmt::ptr(privKey));
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    // Firmware would hand caList/cert/privKey to libSceSsl, which would parse
    // them into an X509_STORE used by the TLS layer. We don't implement that
    // pipeline. Instead, we record that this context expects custom CA-based
    // verification, and at TLS-handshake time we bypass cpp-httplib's default
    // CA verification so the game's private-CA-signed endpoints aren't blocked
    // by our system CA store not knowing about them.
    g_state.contexts_with_loaded_certs.insert(libhttpCtxId);
    LOG_ERROR(Lib_Http,
              "ctxId={} marked as using custom CAs; subsequent HTTPS requests on this "
              "context will bypass cert verification (libSceSsl integration not implemented)",
              libhttpCtxId);
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
    LOG_INFO(Lib_Http, "called libhttpCtxId={}", libhttpCtxId);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!g_state.active_contexts.contains(libhttpCtxId)) {
        LOG_ERROR(Lib_Http, "Invalid libhttpCtxId={}", libhttpCtxId);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    g_state.contexts_with_loaded_certs.erase(libhttpCtxId);
    LOG_INFO(Lib_Http, "ctxId={} cleared custom-CA marker", libhttpCtxId);
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
// Init/Terminate functions
//***********************************
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
    g_state.contexts_with_loaded_certs.erase(libhttpCtxId);
    if (g_state.active_contexts.empty()) {
        // Last context torn down - wipe all dependent objects.
        LOG_INFO(Lib_Http, "last context terminated, clearing state");
        g_state.shutting_down.store(true);
        size_t in_flight = 0;
        for (auto& [id, req_ptr] : g_state.requests) {
            if (req_ptr->state == HttpRequestState::Sending) {
                ++in_flight;
            }
        }
        if (in_flight > 0) {
            LOG_INFO(Lib_Http, "Term: {} request(s) still in flight,results will be abandoned",
                     in_flight);
        }
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
        g_state.contexts_with_loaded_certs.clear();
        g_state.inited = false;
    } else {
        LOG_INFO(Lib_Http, "ctxId={} terminated, {} contexts still active", libhttpCtxId,
                 g_state.active_contexts.size());
    }
    return ORBIS_OK;
}

//***********************************
// Misc functions
//***********************************
int PS4_SYSV_ABI sceHttpSetRecvBlockSize(int id, u32 blockSize) {
    LOG_INFO(Lib_Http, "called id={}, blockSize={}", id, blockSize);
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
    s->recv_block_size = blockSize;
    LOG_INFO(Lib_Http, "set {} id={} recv_block_size={}", level, id, blockSize);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetProxy(int id, int httpProxyConf, int wlanProxyConf, const char* host,
                                 u16 port) {
    LOG_INFO(Lib_Http, "called id={}, httpProxyConf={}, wlanProxyConf={}, host={}, port={}", id,
             httpProxyConf, wlanProxyConf, host ? host : "(null)", port);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (!host) {
        LOG_ERROR(Lib_Http, "host is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->proxy_http_conf = httpProxyConf;
    s->proxy_wlan_conf = wlanProxyConf;
    s->proxy_host = host;
    s->proxy_port = port;
    LOG_INFO(Lib_Http, "set {} id={} proxy={}:{} (httpConf={}, wlanConf={})", level, id, host, port,
             httpProxyConf, wlanProxyConf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetAcceptEncodingGZIPEnabled(int id, int* isEnable) {
    LOG_INFO(Lib_Http, "called id={}", id);
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
    *isEnable = s->accept_encoding_gzip ? 1 : 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int libhttpCtxId, int isEnable) {
    LOG_INFO(Lib_Http, "called libhttpCtxId={}, isEnable={}", libhttpCtxId, isEnable);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    g_state.default_accept_encoding_gzip = (isEnable != 0);
    LOG_INFO(Lib_Http, "set library default accept_encoding_gzip={}",
             g_state.default_accept_encoding_gzip);
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
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->accept_encoding_gzip = (isEnable != 0);
    LOG_INFO(Lib_Http, "set {} id={} accept_encoding_gzip={}", level, id, s->accept_encoding_gzip);
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
        LOG_DEBUG(Lib_Http, "Invalid reqId={}", reqId);
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
        LOG_ERROR(Lib_Http, "sslFlags={:#x} contains unknown bits {:#x}", sslFlags,
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
    LOG_INFO(Lib_Http, "ssl_flags now {:#x} at {} level (id={})", s->ssl_flags, level, id);
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
        LOG_ERROR(Lib_Http, "sslFlags={:#x} contains unknown bits {:#x}", sslFlags,
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
    LOG_INFO(Lib_Http, "ssl_flags now {:#x} at {} level (id={})", s->ssl_flags, level, id);
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
        LOG_ERROR(Lib_Http, "sslFlags={:#x} contains unknown bits {:#x}", sslFlags,
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
    LOG_INFO(Lib_Http, "ssl_flags now {:#x} at {} level (id={})", s->ssl_flags, level, id);
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
        LOG_ERROR(Lib_Http, "sslFlags={:#x} contains unknown bits {:#x}", sslFlags,
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
    LOG_INFO(Lib_Http, "ssl_flags now {:#x} at {} level (id={})", s->ssl_flags, level, id);
    return ORBIS_OK;
}

//***********************************
// Response Information functions
//***********************************
int PS4_SYSV_ABI sceHttpSetResolveTimeOut(int id, u32 usec) {
    LOG_INFO(Lib_Http, "called id={}, usec={}", id, usec);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    s32 sdk_ver = Common::ElfInfo::FW_100;
    ::Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_ver);
    if (sdk_ver >= Common::ElfInfo::FW_170 && usec <= 999999u) {
        LOG_ERROR(Lib_Http, "Invalid usec={} (sdk_ver={:#x})", usec, sdk_ver);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->resolve_timeout_us = usec;
    LOG_INFO(Lib_Http, "set {} id={} resolve_timeout_us={}", level, id, usec);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpSetResponseHeaderMaxSize(int id, u64 headerSize) {
    LOG_INFO(Lib_Http, "called id={}, headerSize={}", id, headerSize);
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
    s->response_header_max = headerSize;
    LOG_INFO(Lib_Http, "set {} id={} response_header_max={}", level, id, headerSize);
    return ORBIS_OK;
}

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
        LOG_DEBUG(Lib_Http, "Invalid reqId={}", reqId);
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
        LOG_INFO(Lib_Http, "reqId={} returning empty headers blob", reqId);
    } else {
        *header = const_cast<char*>(req.res.all_headers_blob.c_str());
        *headerSize = req.res.all_headers_blob.size();
        LOG_INFO(Lib_Http, "reqId={} returning {} bytes of headers", reqId, *headerSize);
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
        LOG_DEBUG(Lib_Http, "Invalid reqId={}", reqId);
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
    LOG_INFO(Lib_Http, "reqId={} result={:#x} contentLength={}", reqId, static_cast<u32>(*result),
             *contentLength);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceHttpGetStatusCode(int reqId, int* statusCode) {
    LOG_DEBUG(Lib_Http, "called reqId={}", reqId);
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
        LOG_DEBUG(Lib_Http, "Invalid reqId={}", reqId);
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
int PS4_SYSV_ABI sceHttpAddRequestHeader(int id, const char* name, const char* value, s32 mode) {
    LOG_INFO(Lib_Http, "called id={}, name={}, value={}, mode={}", id, name ? name : "(null)",
             value ? value : "(null)", mode);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (mode != ORBIS_HTTP_HEADER_OVERWRITE && mode != ORBIS_HTTP_HEADER_ADD) {
        LOG_ERROR(Lib_Http, "Invalid mode={} (must be OVERWRITE=0 or ADD=1)", mode);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    if (!name || !value) {
        LOG_ERROR(Lib_Http, "name or value is null");
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    auto* headers = ResolveHeaders(id, level);
    if (!headers) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    if (mode == ORBIS_HTTP_HEADER_OVERWRITE) {
        headers->erase(
            std::remove_if(headers->begin(), headers->end(),
                           [&](const auto& kv) { return HeaderNameMatches(kv.first, name); }),
            headers->end());
    }
    headers->emplace_back(name, value);
    LOG_INFO(Lib_Http, "added header at {} id={}: {}: {} (mode={}, total now {})", level, id, name,
             value, mode, headers->size());
    return ORBIS_OK;
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
    const char* level = "";
    auto* headers = ResolveHeaders(id, level);
    if (!headers) {
        LOG_ERROR(Lib_Http, "Invalid id={} (not a template, connection, or request)", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    // Remove ALL entries with this name (case-insensitive)
    auto new_end = std::remove_if(headers->begin(), headers->end(), [&](const auto& kv) {
        return HeaderNameMatches(kv.first, name);
    });
    if (new_end == headers->end()) {
        LOG_INFO(Lib_Http, "no header '{}' found at {} id={}", name, level, id);
        return ORBIS_HTTP_ERROR_NOT_FOUND;
    }
    size_t removed = std::distance(new_end, headers->end());
    headers->erase(new_end, headers->end());
    LOG_INFO(Lib_Http, "removed {} occurrence(s) of '{}' from {} id={} (total now {})", removed,
             name, level, id, headers->size());
    return ORBIS_OK;
}

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
int PS4_SYSV_ABI sceHttpSetResolveRetry(int id, int retry) {
    LOG_INFO(Lib_Http, "called id={}, retry={}", id, retry);
    std::lock_guard<std::mutex> lock(g_state.m_mutex);
    if (!g_state.inited) {
        LOG_ERROR(Lib_Http, "Not initialized");
        return ORBIS_HTTP_ERROR_BEFORE_INIT;
    }
    if (retry < 0) {
        LOG_ERROR(Lib_Http, "Invalid retry={} (must be >= 0)", retry);
        return ORBIS_HTTP_ERROR_INVALID_VALUE;
    }
    const char* level = "";
    HttpSettings* s = ResolveSettings(id, level);
    if (!s) {
        LOG_ERROR(Lib_Http, "Invalid id={}", id);
        return ORBIS_HTTP_ERROR_INVALID_ID;
    }
    s->resolve_retry = retry;
    LOG_INFO(Lib_Http, "set {} id={} resolve_retry={}", level, id, retry);
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
    if (req_ptr->state == HttpRequestState::Created) {
        LOG_INFO(Lib_Http,
                 "reqId={} abandoned before sceHttpSendRequest (state=Created); "
                 "{} headers, content_length={} were prepared but never transmitted",
                 reqId, req_ptr->headers.size(), req_ptr->content_length);
    }
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
        LOG_DEBUG(Lib_Http, "Invalid reqId={}", reqId);
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
    LOG_INFO(Lib_Http, "called out={}, require={}, prepare={}, srcElement={}, option={:#x}",
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
