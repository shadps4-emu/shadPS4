// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <ctime>
#include <string>
#include <thread>
#include <SDL3/SDL_misc.h>
#include <fmt/format.h>
#include <imgui.h>

#include "common/logging/log.h"
#include "common/stb.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog_ui.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "imgui/imgui_std.h"

namespace Libraries::GameCustomDataDialog {

using CommonDialog::Result;
using CommonDialog::Status;
using Libraries::Np::NpHandler;

namespace {

constexpr ImVec2 kButtonSize{130.0f, 30.0f};
constexpr ImVec2 kThumbnailMax{192.0f, 108.0f};
constexpr ImVec2 kListThumbnail{64.0f, 36.0f};

std::string ToLower(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return v;
}

std::string DescribeExpiry(s32 minutes) {
    if (minutes <= 0) {
        return "never";
    }
    if (minutes % (24 * 60) == 0) {
        const s32 days = minutes / (24 * 60);
        return fmt::format("in {} day{}", days, days == 1 ? "" : "s");
    }
    if (minutes % 60 == 0) {
        const s32 hours = minutes / 60;
        return fmt::format("in {} hour{}", hours, hours == 1 ? "" : "s");
    }
    return fmt::format("in {} minute{}", minutes, minutes == 1 ? "" : "s");
}

std::string Join(const std::vector<std::string>& items) {
    std::string out;
    for (const auto& item : items) {
        out += (out.empty() ? "" : ", ") + item;
    }
    return out;
}

std::string NowRfc3339() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buf;
}

// "2026-10-31T09:38:00Z" -> "2026-10-31 09:38 UTC".
std::string ShortDate(const std::string& rfc3339) {
    if (rfc3339.size() < 16) {
        return rfc3339;
    }
    return rfc3339.substr(0, 10) + " " + rfc3339.substr(11, 5) + " UTC";
}

bool DecodeThumbnail(const std::vector<u8>& bytes, ImGui::RefCountedTexture& out) {
    int w, h, comp;
    if (bytes.empty() ||
        !stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp)) {
        return false;
    }
    out = ImGui::RefCountedTexture::DecodePngTexture(bytes);
    return true;
}

void DrawThumbnail(const ImGui::RefCountedTexture& texture, bool has, ImVec2 max) {
    if (!has) {
        ImGui::Dummy(max);
        return;
    }
    ImVec2 size = max;
    if (texture) {
        const auto image = texture.GetTexture();
        if (image.width != 0 && image.height != 0) {
            const float scale = std::min(max.x / image.width, max.y / image.height);
            size = ImVec2(image.width * scale, image.height * scale);
        }
        ImGui::Image(image.im_id, size);
    } else {
        ImGui::Dummy(size); // still decoding
    }
}

} // namespace

GameCustomDataDialogUi::GameCustomDataDialogUi(Status* status, DialogState* state)
    : status(status), state(state) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

GameCustomDataDialogUi::~GameCustomDataDialogUi() {
    Finish(Result::USER_CANCELED);
}

GameCustomDataDialogUi::GameCustomDataDialogUi(GameCustomDataDialogUi&& other) noexcept
    : Layer(other), first_render(other.first_render), status(other.status), state(other.state),
      pending_send(std::move(other.pending_send)), pending_list(std::move(other.pending_list)),
      list_loaded(other.list_loaded), error(std::move(other.error)) {
    other.status = nullptr;
    other.state = nullptr;
}

GameCustomDataDialogUi& GameCustomDataDialogUi::operator=(GameCustomDataDialogUi&& other) noexcept {
    first_render = other.first_render;
    status = other.status;
    state = other.state;
    pending_send = std::move(other.pending_send);
    pending_list = std::move(other.pending_list);
    list_loaded = other.list_loaded;
    error = std::move(other.error);
    other.status = nullptr;
    other.state = nullptr;
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
    return *this;
}

void GameCustomDataDialogUi::Finish(Result result) {
    if (status && *status == Status::RUNNING) {
        if (state) {
            state->result = result;
        }
        LOG_INFO(Lib_GameCustomDataDialog, "finished: {}",
                 result == Result::OK ? "OK" : "USER_CANCELED");
        *status = Status::FINISHED;
    }
    status = nullptr;
    RemoveLayer(this);
}

// send

void GameCustomDataDialogUi::StartSend() {
    NpHandler::GameCustomDataToSend item;
    if (state->user_editable) {
        item.to = state->selected_npids;
    } else {
        item.to = state->fixed_online_ids;
        for (const auto id : state->fixed_account_ids) {
            item.to.push_back(std::to_string(id));
        }
    }
    item.message = state->message.data();
    item.data_name = state->data_name;
    item.data_description = state->data_detail;
    item.localized_names = state->localized_names;
    item.localized_descriptions = state->localized_details;
    item.expire_minutes = std::max(state->expire_minutes, 0);
    item.platforms = state->platforms;
    item.is_url = state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL;
    item.url = state->data_url;
    item.with_item_id = state->with_item_id;
    item.game_data = state->game_data;
    item.thumbnail = state->thumbnail_bytes;

    LOG_INFO(Lib_GameCustomDataDialog, "player pressed Send: '{}' to {}", item.data_name,
             Join(item.to));
    LOG_INFO(Lib_GameCustomDataDialog, "  final message: {:?}", item.message);

    error.clear();
    auto pending = std::make_shared<PendingSend>();
    pending_send = pending;
    const s32 user_id = state->user_id;
    std::thread([pending, user_id, item = std::move(item)]() {
        pending->result = NpHandler::GetInstance().SendGameCustomData(user_id, item);
        pending->done.store(true, std::memory_order_release);
    }).detach();
}

void GameCustomDataDialogUi::FinishSend() {
    const auto pending = std::move(pending_send);
    if (!pending->result.ok) {
        error = "Couldn't send: " + pending->result.message;
        return; // stay open so the player can retry or cancel
    }
    if (state->user_editable) {
        state->sent_online_ids = state->selected_npids;
    } else {
        state->sent_online_ids = state->fixed_online_ids;
        state->sent_account_ids = state->fixed_account_ids;
    }
    Finish(Result::OK);
}

void GameCustomDataDialogUi::DrawSend() {
    using namespace ImGui;
    const auto ws = GetWindowSize();
    const float content_height = ws.y - GetCursorPosY() - kButtonSize.y - 20.0f;
    const bool busy = static_cast<bool>(pending_send);

    if (BeginChild("##gcd_content", ImVec2(0.0f, content_height), false)) {
        if (state->has_thumbnail) {
            DrawThumbnail(state->thumbnail, true, kThumbnailMax);
            SameLine();
        }
        BeginGroup();
        SetWindowFontScale(1.15f);
        TextWrapped("%s", state->data_name.c_str());
        SetWindowFontScale(1.0f);
        TextWrapped("%s", state->data_detail.c_str());
        Spacing();
        if (state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL) {
            TextDisabled("Link: %s%s", state->data_url.c_str(),
                         state->with_item_id ? " (+ item ID)" : "");
        } else {
            TextDisabled("Attached game data: %u bytes", state->data_size);
        }
        TextDisabled("Expires: %s", DescribeExpiry(state->expire_minutes).c_str());
        if (!state->platforms.empty()) {
            TextDisabled("Platforms: %s", Join(state->platforms).c_str());
        }
        EndGroup();

        Spacing();
        Text("Message");
        BeginDisabled(busy);
        InputTextExLimited("##gcd_message", nullptr, state->message.data(),
                           static_cast<int>(state->message.size()), ImVec2(-1.0f, 70.0f),
                           ImGuiInputTextFlags_Multiline,
                           ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE);
        EndDisabled();

        Spacing();
        if (!state->user_editable) {
            std::vector<std::string> to = state->fixed_online_ids;
            for (const auto id : state->fixed_account_ids) {
                to.push_back(fmt::format("account {}", id));
            }
            TextWrapped("To: %s", Join(to).c_str());
        } else {
            auto& sel = state->selected_npids;
            Text("To: choose up to %u friend%s (%zu selected)", state->user_max_count,
                 state->user_max_count == 1 ? "" : "s", sel.size());
            const auto friends =
                NpHandler::GetInstance().GetFriendList(static_cast<s32>(state->user_id)).friends;
            if (friends.empty()) {
                TextDisabled("No friends available. Sign in to shadNet and add friends from the "
                             "friends overlay.");
            } else {
                SetNextItemWidth(-1.0f);
                InputTextWithHint("##gcd_friend_filter", "Search friends", state->friend_filter,
                                  sizeof(state->friend_filter));
                const std::string filter = ToLower(state->friend_filter);
                const bool full = sel.size() >= state->user_max_count;
                if (BeginChild("##gcd_friends", ImVec2(0.0f, 100.0f), true)) {
                    for (const auto& f : friends) {
                        if (!filter.empty() && ToLower(f.npid).find(filter) == std::string::npos) {
                            continue; // hidden by the filter; selection is unaffected
                        }
                        PushID(f.npid.c_str());
                        const auto it = std::find(sel.begin(), sel.end(), f.npid);
                        bool checked = it != sel.end();
                        BeginDisabled(busy || (full && !checked));
                        const std::string label = f.npid + (f.online ? " (online)" : "");
                        if (Checkbox(label.c_str(), &checked)) {
                            if (checked && it == sel.end()) {
                                sel.push_back(f.npid);
                            } else if (!checked && it != sel.end()) {
                                sel.erase(it);
                            }
                        }
                        EndDisabled();
                        PopID();
                    }
                }
                EndChild();
            }
        }
        if (!error.empty()) {
            Spacing();
            PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
            TextWrapped("%s", error.c_str());
            PopStyleColor();
        }
    }
    EndChild();

    const float y = ws.y - 10.0f - kButtonSize.y;
    const bool no_recipients = state->user_editable && state->selected_npids.empty();
    SetCursorPos({ws.x / 2.0f - kButtonSize.x - 10.0f, y});
    BeginDisabled(no_recipients || busy);
    const bool send = Button(busy ? "Sending..." : "Send", kButtonSize);
    EndDisabled();
    if (first_render && !no_recipients) {
        SetItemCurrentNavFocus();
    }
    SameLine();
    SetCursorPos({ws.x / 2.0f + 10.0f, y});
    BeginDisabled(busy);
    const bool cancel = Button("Cancel", kButtonSize);
    EndDisabled();
    if (first_render && no_recipients) {
        SetItemCurrentNavFocus();
    }
    if (cancel) {
        LOG_INFO(Lib_GameCustomDataDialog, "player cancelled sending '{}'", state->data_name);
        Finish(Result::USER_CANCELED);
    } else if (send) {
        StartSend();
    }
}

// receive

void GameCustomDataDialogUi::StartList() {
    error.clear();
    auto pending = std::make_shared<PendingList>();
    pending_list = pending;
    const s32 user_id = state->user_id;
    std::thread([pending, user_id]() {
        pending->result =
            NpHandler::GetInstance().ListReceivedGameCustomData(user_id, {}, pending->items);
        pending->done.store(true, std::memory_order_release);
    }).detach();
}

void GameCustomDataDialogUi::FinishList() {
    const auto pending = std::move(pending_list);
    list_loaded = true;
    state->received.clear();
    state->selected = -1;
    if (!pending->result.ok) {
        error = "Couldn't get game data: " + pending->result.message;
        return;
    }
    const std::string now = NowRfc3339();
    for (auto& data : pending->items) {
        DialogState::ReceivedItem item;
        item.expired = !data.expired_date.empty() && data.expired_date <= now;
        item.has_thumbnail = DecodeThumbnail(data.thumbnail, item.thumbnail);
        item.data = std::move(data);
        state->received.push_back(std::move(item));
    }
    if (!state->received.empty()) {
        state->selected = 0;
    }
}

void GameCustomDataDialogUi::DrawReceive() {
    using namespace ImGui;
    const auto ws = GetWindowSize();
    const float content_height = ws.y - GetCursorPosY() - kButtonSize.y - 20.0f;

    if (BeginChild("##gcd_received", ImVec2(0.0f, content_height), false)) {
        if (!list_loaded) {
            TextDisabled("Checking for game data...");
        } else if (!error.empty()) {
            TextWrapped("%s", error.c_str());
        } else if (state->received.empty()) {
            TextWrapped("There is no game data for you.");
        } else {
            if (BeginChild("##gcd_list", ImVec2(0.0f, 150.0f), true)) {
                for (int i = 0; i < static_cast<int>(state->received.size()); i++) {
                    const auto& item = state->received[i];
                    PushID(i);
                    const ImVec2 row = GetCursorPos();
                    if (Selectable("##row", state->selected == i, 0,
                                   ImVec2(0.0f, kListThumbnail.y))) {
                        state->selected = i;
                    }
                    SetCursorPos(row);
                    DrawThumbnail(item.thumbnail, item.has_thumbnail, kListThumbnail);
                    SameLine();
                    BeginGroup();
                    Text("%s", item.data.data_name.c_str());
                    const char* note = item.data.is_url ? ""
                                       : item.data.used ? " - used"
                                       : item.expired   ? " - expired"
                                                        : "";
                    TextDisabled("from %s, %s%s", item.data.from_online_id.c_str(),
                                 ShortDate(item.data.received_date).c_str(), note);
                    EndGroup();
                    PopID();
                }
            }
            EndChild();

            if (state->selected >= 0) {
                const auto& sel = state->received[state->selected].data;
                Spacing();
                if (!sel.message.empty()) {
                    TextWrapped("\"%s\"", sel.message.c_str());
                }
                TextWrapped("%s", sel.data_description.c_str());
                if (sel.is_url) {
                    TextDisabled("Link: %s",
                                 sel.url.empty() ? "(no longer available)" : sel.url.c_str());
                }
                if (!sel.expired_date.empty()) {
                    TextDisabled("Expires: %s", ShortDate(sel.expired_date).c_str());
                }
            }
        }
    }
    EndChild();

    const DialogState::ReceivedItem* sel =
        state->selected >= 0 ? &state->received[state->selected] : nullptr;
    const bool is_url = sel && sel->data.is_url;
    const bool usable = sel && (is_url ? !sel->data.url.empty() : !sel->data.used && !sel->expired);

    const float y = ws.y - 10.0f - kButtonSize.y;
    SetCursorPos({ws.x / 2.0f - kButtonSize.x - 10.0f, y});
    BeginDisabled(!usable);
    const bool use = Button(is_url ? "Open Link" : "Use", kButtonSize);
    EndDisabled();
    if (first_render && usable) {
        SetItemCurrentNavFocus();
    }
    SameLine();
    SetCursorPos({ws.x / 2.0f + 10.0f, y});
    const bool close = Button("Close", kButtonSize);
    if (first_render && !usable) {
        SetItemCurrentNavFocus();
    }

    if (close) {
        Finish(Result::USER_CANCELED);
    } else if (use && sel) {
        if (is_url) {
            LOG_INFO(Lib_GameCustomDataDialog, "player opened link of item {}: {}",
                     sel->data.item_id, sel->data.url);
            SDL_OpenURL(sel->data.url.c_str());
        } else {
            LOG_INFO(Lib_GameCustomDataDialog, "player chose item {} '{}' from {}",
                     sel->data.item_id, sel->data.data_name, sel->data.from_online_id);
            NpHandler::GetInstance().PostGameCustomDataEvent(state->user_id, sel->data.item_id);
        }
        Finish(Result::OK);
    }
}

void GameCustomDataDialogUi::Draw() {
    using namespace ImGui;
    if (status == nullptr || *status != Status::RUNNING || state == nullptr) {
        return;
    }
    const bool receive = state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV;
    if (pending_send && pending_send->done.load(std::memory_order_acquire)) {
        FinishSend();
        if (status == nullptr) {
            return;
        }
    }
    if (receive) {
        if (!list_loaded && !pending_list) {
            StartList();
        } else if (pending_list && pending_list->done.load(std::memory_order_acquire)) {
            FinishList();
        }
    }

    const auto& io = GetIO();
    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 620.0f),
        std::min(io.DisplaySize.y, 460.0f),
    };
    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    KeepNavHighlight();
    if (Begin("Game Custom Data Dialog##GameCustomDataDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();
        SetWindowFontScale(1.3f);
        Text("%s", receive ? "Received Game Data" : "Send Game Data");
        SetWindowFontScale(1.0f);
        Separator();
        Spacing();
        if (receive) {
            DrawReceive();
        } else {
            DrawSend();
        }
    }
    End();

    first_render = false;
}

} // namespace Libraries::GameCustomDataDialog
