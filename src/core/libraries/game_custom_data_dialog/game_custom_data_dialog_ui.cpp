// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <string>
#include <fmt/format.h>
#include <imgui.h>

#include "common/logging/log.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog_ui.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "imgui/imgui_std.h"

namespace Libraries::GameCustomDataDialog {

using CommonDialog::Result;
using CommonDialog::Status;

namespace {

constexpr ImVec2 kButtonSize{130.0f, 30.0f};
constexpr ImVec2 kThumbnailMax{192.0f, 108.0f};

std::string ToLower(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return v;
}

std::string DescribeExpiry(s32 minutes) {
    if (minutes <= 0) {
        return "system default";
    }
    if (minutes % (24 * 60) == 0) {
        const s32 days = minutes / (24 * 60);
        return fmt::format("{} day{}", days, days == 1 ? "" : "s");
    }
    if (minutes % 60 == 0) {
        const s32 hours = minutes / 60;
        return fmt::format("{} hour{}", hours, hours == 1 ? "" : "s");
    }
    return fmt::format("{} minute{}", minutes, minutes == 1 ? "" : "s");
}

std::string Join(const std::vector<std::string>& items) {
    std::string out;
    for (const auto& item : items) {
        out += (out.empty() ? "" : ", ") + item;
    }
    return out;
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
    : Layer(other), first_render(other.first_render), status(other.status), state(other.state) {
    other.status = nullptr;
    other.state = nullptr;
}

GameCustomDataDialogUi& GameCustomDataDialogUi::operator=(GameCustomDataDialogUi&& other) noexcept {
    first_render = other.first_render;
    status = other.status;
    state = other.state;
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

void GameCustomDataDialogUi::Send() {
    if (state->user_editable) {
        state->sent_online_ids = state->selected_npids;
    } else {
        state->sent_online_ids = state->fixed_online_ids;
        state->sent_account_ids = state->fixed_account_ids;
    }
    std::vector<std::string> to = state->sent_online_ids;
    for (const auto id : state->sent_account_ids) {
        to.push_back(fmt::format("account {}", id));
    }
    LOG_INFO(Lib_GameCustomDataDialog, "player pressed Send: '{}' to {}", state->data_name,
             Join(to));
    LOG_INFO(Lib_GameCustomDataDialog, "  final message: {:?}", std::string(state->message.data()));
    LOG_WARNING(Lib_GameCustomDataDialog,
                "  not delivered: shadNet has no game custom data service yet");
    Finish(Result::OK);
}

void GameCustomDataDialogUi::DrawSend() {
    using namespace ImGui;
    const auto ws = GetWindowSize();
    const float content_height = ws.y - GetCursorPosY() - kButtonSize.y - 20.0f;

    if (BeginChild("##gcd_content", ImVec2(0.0f, content_height), false)) {
        // Thumbnail on the left, name and details on the right.
        if (state->has_thumbnail) {
            ImVec2 size = kThumbnailMax;
            if (state->thumbnail) {
                const auto image = state->thumbnail.GetTexture();
                if (image.width != 0 && image.height != 0) {
                    const float scale =
                        std::min(kThumbnailMax.x / image.width, kThumbnailMax.y / image.height);
                    size = ImVec2(image.width * scale, image.height * scale);
                }
                Image(image.im_id, size);
            } else {
                Dummy(size); // still decoding
            }
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
        InputTextExLimited("##gcd_message", nullptr, state->message.data(),
                           static_cast<int>(state->message.size()), ImVec2(-1.0f, 70.0f),
                           ImGuiInputTextFlags_Multiline,
                           ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE);

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
            const auto friends = Libraries::Np::NpHandler::GetInstance()
                                     .GetFriendList(static_cast<s32>(state->user_id))
                                     .friends;
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
                            continue;
                        }
                        PushID(f.npid.c_str());
                        const auto it = std::find(sel.begin(), sel.end(), f.npid);
                        bool checked = it != sel.end();
                        BeginDisabled(full && !checked);
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
        Spacing();
        TextDisabled("Game data can't be delivered yet: shadNet doesn't support it.");
    }
    EndChild();

    const float y = ws.y - 10.0f - kButtonSize.y;
    const bool no_recipients = state->user_editable && state->selected_npids.empty();
    SetCursorPos({ws.x / 2.0f - kButtonSize.x - 10.0f, y});
    BeginDisabled(no_recipients);
    const bool send = Button("Send", kButtonSize);
    EndDisabled();
    if (first_render && !no_recipients) {
        SetItemCurrentNavFocus();
    }
    SameLine();
    SetCursorPos({ws.x / 2.0f + 10.0f, y});
    if (Button("Cancel", kButtonSize)) {
        LOG_INFO(Lib_GameCustomDataDialog, "player cancelled sending '{}'", state->data_name);
        Finish(Result::USER_CANCELED);
        return;
    }
    if (first_render && no_recipients) {
        SetItemCurrentNavFocus();
    }
    if (send) {
        Send();
    }
}

void GameCustomDataDialogUi::DrawReceive() {
    using namespace ImGui;
    const auto ws = GetWindowSize();
    TextWrapped("There is no game data for you.");
    Spacing();
    TextDisabled("Receiving game data isn't supported by shadNet yet.");

    SetCursorPos({ws.x / 2.0f - kButtonSize.x / 2.0f, ws.y - 10.0f - kButtonSize.y});
    if (Button("Close", kButtonSize)) {
        Finish(Result::USER_CANCELED);
        return;
    }
    if (first_render) {
        SetItemCurrentNavFocus();
    }
}

void GameCustomDataDialogUi::Draw() {
    using namespace ImGui;
    if (status == nullptr || *status != Status::RUNNING || state == nullptr) {
        return;
    }
    const bool receive = state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV;
    const auto& io = GetIO();
    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 620.0f),
        std::min(io.DisplaySize.y, receive ? 200.0f : 460.0f),
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
