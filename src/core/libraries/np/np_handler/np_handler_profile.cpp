// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cctype>
#include <cstring>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/np_handler/np_handler.h"

namespace Libraries::Np {

namespace {

using json = nlohmann::json;

// The largest avatar allows is 200 KiB (l). Anything well past that is
// not an avatar, so stop reading instead of buffering it.
constexpr size_t MaxAvatarBytes = 1024 * 1024;

bool SplitUrl(const std::string& url, std::string& origin, std::string& path) {
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return false;
    }
    const std::string scheme = url.substr(0, scheme_end);
    if (scheme != "http" && scheme != "https") {
        return false;
    }
    const auto path_start = url.find('/', scheme_end + 3);
    origin = url.substr(0, path_start);
    path = path_start == std::string::npos ? "/" : url.substr(path_start);
    return origin.size() > scheme_end + 3;
}

bool IsValidTarget(const std::string& target) {
    if (target.empty() || target.size() > 20) {
        return false;
    }
    for (const char c : target) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

std::string StringField(const json& obj, const char* key) {
    return obj.is_object() && obj.contains(key) && obj[key].is_string()
               ? obj[key].get<std::string>()
               : std::string();
}

bool IsCustomPicture(const std::string& url) {
    const auto slash = url.rfind('/');
    const std::string file = slash == std::string::npos ? url : url.substr(slash + 1);
    return !url.empty() && file.rfind("default_", 0) != 0;
}

std::vector<u8> DownloadPng(const std::string& url) {
    std::string origin;
    std::string path;
    if (!SplitUrl(url, origin, path)) {
        LOG_WARNING(NpHandler, "profile: unusable avatar URL '{}'", url);
        return {};
    }
    httplib::Client img(origin);
    img.set_connection_timeout(5);
    img.set_read_timeout(15);
    img.set_follow_location(true);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    // Same rule as sceHttp: verify the server unless the user turned that off.
    img.enable_server_certificate_verification(!EmulatorSettings.IsForcedHttpsDisabled());
#endif
    std::vector<u8> png;
    bool too_large = false;
    const auto got = img.Get(path, [&](const char* data, size_t len) {
        if (png.size() + len > MaxAvatarBytes) {
            too_large = true;
            return false;
        }
        png.insert(png.end(), data, data + len);
        return true;
    });
    if (too_large) {
        LOG_WARNING(NpHandler, "profile: avatar '{}' is larger than {} bytes", url, MaxAvatarBytes);
        return {};
    }
    if (!got || got->status != 200) {
        LOG_WARNING(NpHandler, "profile: avatar download of '{}' failed (HTTP {})", url,
                    got ? got->status : 0);
        return {};
    }
    if (png.size() < 8 || std::memcmp(png.data(), "\x89PNG\r\n\x1a\n", 8) != 0) {
        LOG_WARNING(NpHandler, "profile: avatar '{}' is not a PNG", url);
        return {};
    }
    return png;
}

} // namespace

NpHandler::ProfileCard NpHandler::FetchProfileCard(s32 user_id, const std::string& target) const {
    ProfileCard card;
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        LOG_DEBUG(NpHandler, "not signed in to shadNet (user_id={})", user_id);
        return card;
    }
    if (!IsValidTarget(target)) {
        LOG_WARNING(NpHandler, "'{}' is not an online ID or account ID", target);
        return card;
    }
    httplib::Client api(base_url);
    api.set_connection_timeout(5);
    api.set_read_timeout(10);
    const std::string path =
        "/v1/users/" + target + "/profile?fields=user,avatarUrl,relation&avatarSize=m";
    const auto res = api.Get(path, {{"Authorization", "Bearer " + token}});
    if (!res || res->status != 200) {
        LOG_WARNING(NpHandler, "lookup of '{}' failed (HTTP {})", target, res ? res->status : 0);
        return card;
    }
    const json body = json::parse(res->body, nullptr, false);
    const json user = body.is_object() && body.contains("user") ? body["user"] : json();
    card.online_id = StringField(user, "onlineId");
    card.account_id = StringField(user, "accountId");
    card.relation = StringField(body, "relation");
    if (card.online_id.empty()) {
        LOG_WARNING(NpHandler, "reply for '{}' has no online ID", target);
        return card;
    }
    card.ok = true;

    const std::string avatar_url = StringField(body, "avatarUrl");
    card.custom_picture = IsCustomPicture(avatar_url);
    if (!avatar_url.empty()) {
        card.avatar_png = DownloadPng(avatar_url);
    }
    LOG_INFO(NpHandler, "profile: loaded '{}' (relation '{}', avatar {} bytes)", card.online_id,
             card.relation, card.avatar_png.size());
    return card;
}

bool NpHandler::SendGriefReport(s32 user_id, const std::string& account_id,
                                const std::vector<std::string>& items) const {
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty() || !IsValidTarget(account_id) || items.empty()) {
        LOG_WARNING(NpHandler, "grief report: not sent (signed in: {}, target '{}', {} items)",
                    !token.empty(), account_id, items.size());
        return false;
    }
    httplib::Client api(base_url);
    api.set_connection_timeout(5);
    api.set_read_timeout(10);
    const std::string body = json{{"items", items}}.dump();
    const auto res = api.Post("/v1/users/" + account_id + "/griefReport",
                              {{"Authorization", "Bearer " + token}}, body, "application/json");
    if (!res || res->status != 204) {
        LOG_WARNING(NpHandler, "grief report about '{}' failed (HTTP {})", account_id,
                    res ? res->status : 0);
        return false;
    }
    LOG_INFO(NpHandler, "grief report about '{}' sent", account_id);
    return true;
}

} // namespace Libraries::Np
