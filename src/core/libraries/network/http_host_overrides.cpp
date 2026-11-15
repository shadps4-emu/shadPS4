// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/network/http_host_overrides.h"

#include <cctype>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <nlohmann/json.hpp>
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/scm_rev.h"

#if __has_include(<httplib.h>)
#include <httplib.h>
#define HOST_OVERRIDES_WITH_HTTPLIB 1
#endif

namespace Libraries::Http {

namespace {

constexpr const char* RepoHost = "https://raw.githubusercontent.com";
constexpr const char* RepoPath = "/shad-net/http_host_overrides/main/";
constexpr std::size_t MaxFileSize = 256 * 1024;
constexpr int ConnectTimeoutSec = 5;
constexpr int ReadTimeoutSec = 5;

bool ReadFile(const std::filesystem::path& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    out = buf.str();
    return true;
}

// Write to a temporary file and rename over the target so a crash or a concurrent
// reader never sees a half-written file.
bool WriteFileAtomic(const std::filesystem::path& path, std::string_view data) {
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) {
            return false;
        }
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!f) {
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

void RemoveIfExists(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace

namespace {
std::mutex g_title_id_mutex;
std::string g_title_id;
} // namespace

// Set by the emulator at boot. Kept here instead of reading Common::ElfInfo, whose
// accessors assert when no game is loaded (e.g. in the HTTP unit tests).
void SetHostOverridesTitleId(std::string_view title_id) {
    std::scoped_lock lock{g_title_id_mutex};
    g_title_id = title_id;
}

std::string GetHostOverridesTitleId() {
    std::scoped_lock lock{g_title_id_mutex};
    return g_title_id;
}

std::filesystem::path HostOverridesDir() {
    return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "host_overrides";
}

bool IsValidHostOverridesTitleId(std::string_view title_id) {
    if (title_id.size() != 9) {
        return false;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (!std::isupper(static_cast<unsigned char>(title_id[i]))) {
            return false;
        }
    }
    for (std::size_t i = 4; i < 9; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(title_id[i]))) {
            return false;
        }
    }
    return true;
}

void UpdateHostOverridesFromRepo(std::string_view title_id) {
#ifndef HOST_OVERRIDES_WITH_HTTPLIB
    LOG_WARNING(Lib_Http, "host overrides download unavailable in this build");
#else
    if (!IsValidHostOverridesTitleId(title_id)) {
        LOG_DEBUG(Lib_Http, "host overrides download skipped: '{}' is not a title id", title_id);
        return;
    }
    const std::string id(title_id);
    const auto dir = HostOverridesDir();
    const auto json_path = dir / (id + ".json");
    const auto etag_path = dir / (id + ".json.etag");
    std::error_code ec;

    // No .etag marker means the user wrote this file: leave it alone.
    const bool have_file = std::filesystem::exists(json_path, ec);
    const bool downloaded = std::filesystem::exists(etag_path, ec);
    if (have_file && !downloaded) {
        LOG_INFO(Lib_Http, "host overrides for {}: using local file, auto-download skipped", id);
        return;
    }
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        LOG_WARNING(Lib_Http, "host overrides: cannot create {}: {}", dir.string(), ec.message());
        return;
    }

    httplib::Client cli(RepoHost);
    cli.set_connection_timeout(ConnectTimeoutSec, 0);
    cli.set_read_timeout(ReadTimeoutSec, 0);
    // No automatic redirects: httplib treats any 3xx, including 304 Not Modified, as a
    // redirect and fails it for lacking a Location header.
    cli.set_follow_location(false);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    cli.enable_server_certificate_verification(true);
#endif

    httplib::Headers headers{{"User-Agent", std::string("shadPS4/") + Common::g_version}};
    std::string cached_etag;
    if (have_file && ReadFile(etag_path, cached_etag) && !cached_etag.empty()) {
        headers.emplace("If-None-Match", cached_etag);
    }

    const std::string url_path = std::string(RepoPath) + id + ".json";
    const auto res = cli.Get(url_path, headers);
    if (!res) {
        LOG_WARNING(Lib_Http, "host overrides: fetching {} failed ({}); using cached copy if any",
                    id, httplib::to_string(res.error()));
        return;
    }

    switch (res->status) {
    case 304:
        LOG_INFO(Lib_Http, "host overrides for {} are up to date", id);
        return;
    case 404:
        if (have_file) { // only reached for downloaded files (marker present)
            LOG_INFO(Lib_Http, "host overrides for {} were removed upstream; deleting local copy",
                     id);
            RemoveIfExists(json_path);
        } else {
            LOG_INFO(Lib_Http, "no published host overrides for {}", id);
        }
        RemoveIfExists(etag_path);
        return;
    case 200:
        break;
    default:
        LOG_WARNING(Lib_Http, "host overrides: unexpected HTTP {} for {}; keeping cached copy",
                    res->status, id);
        return;
    }

    if (res->body.size() > MaxFileSize) {
        LOG_WARNING(Lib_Http, "host overrides for {} too large ({} bytes); ignored", id,
                    res->body.size());
        return;
    }
    try {
        if (!nlohmann::json::parse(res->body).is_object()) {
            LOG_WARNING(Lib_Http, "host overrides for {}: root is not an object; ignored", id);
            return;
        }
    } catch (const std::exception& e) {
        LOG_WARNING(Lib_Http, "host overrides for {}: invalid JSON ({}); ignored", id, e.what());
        return;
    }

    if (!WriteFileAtomic(json_path, res->body)) {
        LOG_WARNING(Lib_Http, "host overrides: cannot write {}", json_path.string());
        return;
    }
    if (!WriteFileAtomic(etag_path, res->get_header_value("ETag"))) {
        LOG_WARNING(Lib_Http, "host overrides: cannot write {}", etag_path.string());
    }
    LOG_INFO(Lib_Http, "downloaded host overrides for {} ({} bytes)", id, res->body.size());
#endif
}

} // namespace Libraries::Http
