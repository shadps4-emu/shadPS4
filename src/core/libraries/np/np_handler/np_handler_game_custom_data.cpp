// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <random>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/system/systemservice.h"
#include "core/user_manager.h"
#include "core/user_settings.h"

namespace Libraries::Np {

namespace {

using json = nlohmann::json;

std::string Dump(const json& j) {
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

void ReadError(const httplib::Result& res, NpHandler::GameCustomDataResult& out) {
    out.http_status = res ? res->status : 0;
    if (!res) {
        out.message = "no response from shadNet";
        return;
    }
    const json body = json::parse(res->body, nullptr, false);
    if (body.is_object() && body.contains("error") && body["error"].is_object()) {
        out.error_code = body["error"].value("code", static_cast<s64>(0));
        out.message = body["error"].value("message", std::string());
    }
    if (out.message.empty()) {
        out.message = "HTTP " + std::to_string(res->status);
    }
}

std::string Boundary() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    char buf[40];
    std::snprintf(buf, sizeof(buf), "shadps4gcd%016llx%016llx",
                  static_cast<unsigned long long>(gen()), static_cast<unsigned long long>(gen()));
    return buf;
}

std::string UserLanguage(s32 user_id) {
    const User* u = UserManagement.GetUserByID(user_id);
    const std::string lang = u ? u->np_language : std::string();
    return lang.size() == 2 ? lang : "en";
}

} // namespace

NpHandler::GameCustomDataResult NpHandler::SendGameCustomData(s32 user_id,
                                                              const GameCustomDataToSend& item) {
    GameCustomDataResult result;
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        result.message = "not signed in to shadNet";
        LOG_ERROR(NpHandler, "game custom data: {} (user_id={})", result.message, user_id);
        return result;
    }

    // The message part: GameCustomDataToSend.
    json detail = {{"dataName", item.data_name},
                   {"dataDescription", item.data_description},
                   {"expireMinutes", item.expire_minutes},
                   {"availablePlatforms", item.platforms}};
    std::string languages;
    const auto localized = [&](const char* member, const char* key, const auto& list) {
        if (list.empty()) {
            return;
        }
        json arr = json::array();
        for (const auto& [lang, text] : list) {
            arr.push_back({{"npLanguage", lang}, {key, text}});
            if (languages.find(lang) == std::string::npos) {
                languages += (languages.empty() ? "" : ",") + lang;
            }
        }
        detail[member] = std::move(arr);
    };
    localized("localizedDataNames", "dataName", item.localized_names);
    localized("localizedDataDescriptions", "dataDescription", item.localized_descriptions);
    json body_item = {{"message", item.message}, {"gameDataDetail", std::move(detail)}};
    if (item.is_url) {
        body_item["attachmentDetail"] = {
            {"type", "url"}, {"value", item.url}, {"withItemId", item.with_item_id}};
    }
    const std::string message = Dump({{"toAccountIds", item.to}, {"item", std::move(body_item)}});

    const std::string boundary = Boundary();
    std::string body;
    const auto part = [&](const char* type, const char* description, const void* data, size_t size,
                          bool attachment) {
        body += "--" + boundary + "\r\n";
        body += std::string("Content-Type:") + type + "\r\n";
        body += std::string("Content-Description:") + description + "\r\n";
        if (attachment) {
            body += "Content-Disposition:attachment\r\n";
        }
        body += "Content-Length:" + std::to_string(size) + "\r\n\r\n";
        body.append(static_cast<const char*>(data), size);
        body += "\r\n";
    };
    part("application/json; charset=utf-8", "message", message.data(), message.size(), false);
    if (!item.is_url) {
        part("application/octet-stream", "game-data-binary-0", item.game_data.data(),
             item.game_data.size(), true);
    }
    const bool png =
        item.thumbnail.size() >= 4 && std::memcmp(item.thumbnail.data(), "\x89PNG", 4) == 0;
    part(png ? "image/png" : "image/jpeg", "game-data-thumbnail-0", item.thumbnail.data(),
         item.thumbnail.size(), true);
    body += "--" + boundary + "--\r\n";

    httplib::Client cli(base_url);
    cli.set_connection_timeout(5);
    cli.set_read_timeout(20);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    if (!languages.empty()) {
        headers.emplace("Accept-Language", languages);
    }
    const std::string content_type = "multipart/mixed; boundary=" + boundary;
    const auto res = cli.Post("/v1/gameCustomData/items", headers, body, content_type.c_str());
    if (!res || (res->status != 200 && res->status != 204)) {
        ReadError(res, result);
        LOG_ERROR(NpHandler, "game custom data: send failed: HTTP {} code {} '{}'",
                  result.http_status, result.error_code, result.message);
        return result;
    }
    result.ok = true;
    result.http_status = res->status;
    LOG_INFO(NpHandler, "game custom data: sent '{}' to {} recipient(s)", item.data_name,
             item.to.size());
    return result;
}

NpHandler::GameCustomDataResult NpHandler::ListReceivedGameCustomData(
    s32 user_id, const std::string& np_language, std::vector<ReceivedGameCustomData>& out) {
    GameCustomDataResult result;
    out.clear();
    const std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        result.message = "not signed in to shadNet";
        return result;
    }
    const std::string lang = np_language.empty() ? UserLanguage(user_id) : np_language;

    httplib::Client cli(base_url);
    cli.set_connection_timeout(5);
    cli.set_read_timeout(10);
    const httplib::Headers headers = {{"Authorization", "Bearer " + token},
                                      {"Accept-Language", lang}};
    const auto list = cli.Get("/v1/users/me/gameCustomData/items?fields=@default,fromUser,"
                              "attachmentDetail,gameDataDetail&limit=100",
                              headers);
    if (!list || list->status != 200) {
        ReadError(list, result);
        LOG_ERROR(NpHandler, "game custom data: list failed: HTTP {} '{}'", result.http_status,
                  result.message);
        return result;
    }
    const json items = json::parse(list->body, nullptr, false).value("items", json::array());
    for (const json& j : items) {
        ReceivedGameCustomData r;
        r.item_id = j.value("itemId", static_cast<s64>(0));
        if (r.item_id == 0) {
            continue;
        }
        r.received_date = j.value("receivedDate", std::string());
        r.used = j.value("dataUsedFlag", false);
        if (j.contains("fromUser")) {
            r.from_online_id = j["fromUser"].value("onlineId", std::string());
            r.from_account_id = std::strtoull(
                j["fromUser"].value("accountId", std::string("0")).c_str(), nullptr, 10);
        }
        if (j.contains("attachmentDetail")) {
            r.is_url = j["attachmentDetail"].value("type", std::string()) == "url";
            r.url = j["attachmentDetail"].value("value", std::string());
        }
        if (j.contains("gameDataDetail")) {
            r.expired_date = j["gameDataDetail"].value("expiredDate", std::string());
            r.platforms =
                j["gameDataDetail"].value("availablePlatforms", std::vector<std::string>());
        }
        out.push_back(std::move(r));
    }

    for (size_t start = 0; start < out.size(); start += 20) {
        std::string ids;
        for (size_t i = start; i < std::min(out.size(), start + 20); i++) {
            ids += (ids.empty() ? "" : ",") + std::to_string(out[i].item_id);
        }
        const auto msg = cli.Get("/v1/users/me/gameCustomData/items/@items?itemIds=" + ids +
                                     "&fields=@default,message,gameDataDetail&npLanguage=" + lang,
                                 headers);
        if (!msg || msg->status != 200) {
            LOG_WARNING(NpHandler, "game custom data: message lookup failed (HTTP {})",
                        msg ? msg->status : 0);
            continue;
        }
        for (const json& j : json::parse(msg->body, nullptr, false).value("items", json::array())) {
            const s64 id = j.value("itemId", static_cast<s64>(0));
            for (auto& r : out) {
                if (r.item_id != id) {
                    continue;
                }
                r.message = j.value("message", std::string());
                if (j.contains("gameDataDetail")) {
                    r.data_name = j["gameDataDetail"].value("dataName", std::string());
                    r.data_description =
                        j["gameDataDetail"].value("dataDescription", std::string());
                }
            }
        }
    }

    // Thumbnails (a shadNet extension: on a console the system UI fetches these itself).
    for (auto& r : out) {
        const auto img =
            cli.Get("/v1/users/me/gameCustomData/items/" + std::to_string(r.item_id) + "/thumbnail",
                    headers);
        if (img && img->status == 200) {
            r.thumbnail.assign(img->body.begin(), img->body.end());
        }
    }

    result.ok = true;
    result.http_status = 200;
    LOG_INFO(NpHandler, "game custom data: {} received item(s) for user_id={}", out.size(),
             user_id);
    return result;
}

void NpHandler::PostGameCustomDataEvent(s32 user_id, s64 item_id) {
    using Libraries::GameCustomDataDialog::OrbisNpGameCustomDataEventParam;

    Libraries::SystemService::OrbisSystemServiceEvent event{};
    event.event_type = Libraries::SystemService::OrbisSystemServiceEventType::GameCustomData;
    auto* param = reinterpret_cast<OrbisNpGameCustomDataEventParam*>(event.param);
    std::memset(param, 0, sizeof(*param));
    param->itemId = static_cast<Libraries::GameCustomDataDialog::OrbisNpGameCustomDataId>(item_id);
    param->onlineId = GetOnlineId(user_id);
    param->userId = user_id;
    Libraries::SystemService::PushSystemServiceEvent(event);
    LOG_INFO(NpHandler, "Posted GAME_CUSTOM_DATA user_id={} itemId={}", user_id, item_id);
}

} // namespace Libraries::Np
