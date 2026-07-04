// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <imgui.h>

#include "common/logging/log.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/invitation_dialog/invitation_dialog_ui.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/ssl.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_std.h"

static constexpr ImVec2 BUTTON_SIZE{130.0f, 30.0f};

namespace Libraries::InvitationDialog {

using CommonDialog::Result;
using CommonDialog::Status;

InvitationDialogUi::InvitationDialogUi(Status* status, DialogState* state)
    : status(status), state(state) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

InvitationDialogUi::~InvitationDialogUi() {
    Finish(Result::OK);
}

InvitationDialogUi::InvitationDialogUi(InvitationDialogUi&& other) noexcept
    : Layer(other), first_render(other.first_render), status(other.status), state(other.state) {
    other.status = nullptr;
    other.state = nullptr;
}

InvitationDialogUi& InvitationDialogUi::operator=(InvitationDialogUi&& other) noexcept {
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

void InvitationDialogUi::Finish(Result result) {
    if (state) {
        state->result = result;
    }
    if (status) {
        *status = Status::FINISHED;
    }
    status = nullptr;
    RemoveLayer(this);
}

void InvitationDialogUi::Draw() {
    using namespace ImGui;
    if (status == nullptr || *status != Status::RUNNING || state == nullptr) {
        return;
    }
    const auto& io = GetIO();
    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 500.0f),
        std::min(io.DisplaySize.y, 320.0f),
    };

    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    KeepNavHighlight();
    if (Begin("Invitation Dialog##InvitationDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();

        const bool send_mode = state->mode == ORBIS_INVITATION_DIALOG_MODE_SEND;
        SetWindowFontScale(1.3f);
        Text("%s", send_mode ? "Send Invitation" : "Invitations");
        SetWindowFontScale(1.0f);
        Separator();
        Spacing();

        if (send_mode) {
            if (!state->session_id.empty()) {
                TextWrapped("Session: %s", state->session_id.c_str());
            }
            if (state->user_editable) {
                TextWrapped("Select friends to invite:");
                auto& np = Libraries::Np::NpHandler::GetInstance();
                const auto friend_list = np.GetFriendList(static_cast<s32>(state->user_id));
                if (friend_list.friends.empty()) {
                    TextDisabled("No friends available. Add friends from the friends overlay.");
                } else {
                    SetNextItemWidth(-1.0f);
                    InputTextWithHint("##invitation_friend_filter", "Search friends",
                                      state->friend_filter, sizeof(state->friend_filter));
                    const auto to_lower = [](std::string v) {
                        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
                            return static_cast<char>(std::tolower(c));
                        });
                        return v;
                    };
                    const std::string filter = to_lower(state->friend_filter);
                    if (BeginChild("##invitation_friends", ImVec2(0.0f, 120.0f), true)) {
                        auto& sel = state->selected_npids;
                        for (const auto& f : friend_list.friends) {
                            if (!filter.empty() &&
                                to_lower(f.npid).find(filter) == std::string::npos) {
                                continue; // filtered out of view; selection is unaffected
                            }
                            PushID(f.npid.c_str());
                            const auto it = std::find(sel.begin(), sel.end(), f.npid);
                            bool checked = it != sel.end();
                            const std::string label =
                                f.npid + (f.online ? " (online)" : " (offline)");
                            if (Checkbox(label.c_str(), &checked)) {
                                if (checked && it == sel.end()) {
                                    sel.push_back(f.npid);
                                } else if (!checked && it != sel.end()) {
                                    sel.erase(it);
                                }
                            }
                            PopID();
                        }
                    }
                    EndChild();
                }
            } else {
                // TODO ?
            }
            if (!state->message.empty()) {
                Spacing();
                TextWrapped("Message: %s", state->message.c_str());
            }
        } else {
            // RECV: firmware passes no invitation payload (RecvInfo is reserved), so the pending
            // invitation would be fetched from shadNet here -- deferred like SEND's networking.
            TextWrapped("You have received a game invitation.");
            Spacing();
            TextDisabled("Accept to join the session, or decline to dismiss.");
        }

        const auto ws = GetWindowSize();
        const float y = ws.y - 10.0f - BUTTON_SIZE.y;

        if (send_mode) {
            const bool no_recipients = state->user_editable && state->selected_npids.empty();
            SetCursorPos({ws.x / 2.0f - BUTTON_SIZE.x - 10.0f, y});
            BeginDisabled(no_recipients);
            if (Button("Send", BUTTON_SIZE)) {
                // TODO : Implement sending invitations
                Finish(Result::OK);
            }
            EndDisabled();
            if (first_render) {
                SetItemCurrentNavFocus();
            }
            SameLine();
            SetCursorPos({ws.x / 2.0f + 10.0f, y});
            if (Button("Cancel", BUTTON_SIZE)) {
                Finish(Result::USER_CANCELED);
            }
        } else {
            SetCursorPos({ws.x / 2.0f - BUTTON_SIZE.x - 10.0f, y});
            if (Button("Accept", BUTTON_SIZE)) {
                // TODO : fetch the pending invitation and raise the
                // SCE_SYSTEM_SERVICE_EVENT_SESSION_INVITATION event so the title joins the session.
                Finish(Result::OK);
            }
            if (first_render) {
                SetItemCurrentNavFocus();
            }
            SameLine();
            SetCursorPos({ws.x / 2.0f + 10.0f, y});
            if (Button("Decline", BUTTON_SIZE)) {
                Finish(Result::USER_CANCELED);
            }
        }
    }
    End();

    first_render = false;
}

} // namespace Libraries::InvitationDialog
