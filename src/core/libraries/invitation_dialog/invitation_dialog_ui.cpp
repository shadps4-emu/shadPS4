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
    : Layer(other), first_render(other.first_render), status(other.status), state(other.state),
      pending_accept(std::move(other.pending_accept)), pending_send(std::move(other.pending_send)) {
    other.status = nullptr;
    other.state = nullptr;
}

InvitationDialogUi& InvitationDialogUi::operator=(InvitationDialogUi&& other) noexcept {
    first_render = other.first_render;
    status = other.status;
    state = other.state;
    pending_accept = std::move(other.pending_accept);
    pending_send = std::move(other.pending_send);
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
    if (pending_accept && pending_accept->done.load(std::memory_order_acquire)) {
        const bool ok = pending_accept->success.load(std::memory_order_acquire);
        const std::string finished_id = pending_accept->invitation_id;
        pending_accept.reset();
        if (status && *status == Status::RUNNING && state) {
            if (ok) {
                Finish(Result::OK);
                return;
            }
            const auto it =
                std::find_if(state->recv_invitations.begin(), state->recv_invitations.end(),
                             [&](const auto& p) { return p.invitation_id == finished_id; });
            if (it != state->recv_invitations.end()) {
                state->recv_invitations.erase(it);
            }
            state->recv_selected = state->recv_invitations.empty() ? -1 : 0;
            if (state->recv_invitations.empty()) {
                Finish(Result::USER_CANCELED);
                return;
            }
        }
    }
    if (pending_send && pending_send->done.load(std::memory_order_acquire)) {
        const bool ok = pending_send->success.load(std::memory_order_acquire);
        auto finished = std::move(pending_send);
        if (status && *status == Status::RUNNING && state) {
            if (ok) {
                state->sent_online_ids = std::move(finished->sent_npids);
                state->sent_account_ids = std::move(finished->sent_account_ids);
            }
            Finish(Result::OK);
            return;
        }
    }

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
            // RECV: list the pending invitations stashed by NpHandler; the user picks one.
            if (state->recv_invitations.empty()) {
                TextWrapped("You have no pending invitations.");
            } else {
                TextWrapped("You have %d pending invitation(s):",
                            static_cast<int>(state->recv_invitations.size()));
                Spacing();
                if (BeginChild("##pending_invitations", ImVec2(0.0f, 120.0f), true)) {
                    for (int i = 0; i < static_cast<int>(state->recv_invitations.size()); i++) {
                        PushID(i);
                        const auto& inv = state->recv_invitations[i];
                        const std::string label =
                            (inv.from_npid.empty() ? std::string("(unknown)") : inv.from_npid) +
                            " invited you to a game session.";
                        if (Selectable(label.c_str(), state->recv_selected == i)) {
                            state->recv_selected = i;
                        }
                        PopID();
                    }
                }
                EndChild();
                Spacing();
                TextDisabled("Accept to join the selected session, or decline to dismiss it.");
            }
        }

        const auto ws = GetWindowSize();
        const float y = ws.y - 10.0f - BUTTON_SIZE.y;

        if (send_mode) {
            const bool no_recipients = state->user_editable && state->selected_npids.empty();
            const bool busy = static_cast<bool>(pending_send);
            SetCursorPos({ws.x / 2.0f - BUTTON_SIZE.x - 10.0f, y});
            BeginDisabled(no_recipients || busy);
            if (Button(busy ? "Sending..." : "Send", BUTTON_SIZE)) {
                // USERENABLE: send the npids the user picked. USERDISABLE: send the app-fixed
                // list resolved at Open -- online IDs (non-A) or account IDs (A); only one is set.
                // Resolve recipients: online IDs (picker or non-A fixed list) and/or account IDs
                // (A fixed list). Only one form is populated per dialog.
                std::vector<std::string> sent_npids;
                if (state->user_editable) {
                    sent_npids = state->selected_npids;
                } else {
                    for (const auto& oid : state->online_ids) {
                        size_t n = 0;
                        while (n < sizeof(oid.data) && oid.data[n] != '\0') {
                            n++;
                        }
                        sent_npids.emplace_back(oid.data, n);
                    }
                }
                std::vector<std::string> to = sent_npids;
                for (const auto id : state->account_ids) {
                    to.push_back(std::to_string(id));
                }
                auto pending = std::make_shared<PendingSend>();
                pending->sent_npids = sent_npids;
                pending->sent_account_ids = state->account_ids;
                pending_send = pending;
                const s32 uid = static_cast<s32>(state->user_id);
                const std::string session_id = state->session_id;
                const std::string message = state->message;
                std::thread([pending, uid, session_id, to, message]() {
                    const bool ok = Libraries::Np::NpHandler::GetInstance().SendSessionInvitation(
                        uid, session_id, to, message);
                    pending->success.store(ok, std::memory_order_release);
                    pending->done.store(true, std::memory_order_release);
                }).detach();
            }
            EndDisabled();
            if (first_render) {
                SetItemCurrentNavFocus();
            }
            SameLine();
            SetCursorPos({ws.x / 2.0f + 10.0f, y});
            BeginDisabled(busy);
            if (Button("Cancel", BUTTON_SIZE)) {
                Finish(Result::USER_CANCELED);
            }
            EndDisabled();
        } else {
            const bool no_selection =
                state->recv_selected < 0 ||
                state->recv_selected >= static_cast<int>(state->recv_invitations.size());
            const bool busy = static_cast<bool>(pending_accept);
            const float total_w = BUTTON_SIZE.x * 3.0f + 20.0f;
            float x = ws.x / 2.0f - total_w / 2.0f;
            SetCursorPos({x, y});
            BeginDisabled(no_selection || busy);
            if (Button(busy ? "Accepting..." : "Accept", BUTTON_SIZE)) {
                const auto& inv = state->recv_invitations[state->recv_selected];
                auto pending = std::make_shared<PendingAccept>();
                pending->invitation_id = inv.invitation_id;
                pending_accept = pending;
                const s32 uid = static_cast<s32>(state->user_id);
                std::thread([pending, uid]() {
                    const bool ok = Libraries::Np::NpHandler::GetInstance().AcceptSessionInvitation(
                        uid, pending->invitation_id);
                    pending->success.store(ok, std::memory_order_release);
                    pending->done.store(true, std::memory_order_release);
                }).detach();
            }
            EndDisabled();
            if (first_render && !no_selection) {
                SetItemCurrentNavFocus();
            }
            SameLine();
            x += BUTTON_SIZE.x + 10.0f;
            SetCursorPos({x, y});
            BeginDisabled(no_selection || busy);
            if (Button("Decline", BUTTON_SIZE)) {
                const auto& inv = state->recv_invitations[state->recv_selected];
                Libraries::Np::NpHandler::GetInstance().DeclineSessionInvitation(
                    static_cast<s32>(state->user_id), inv.invitation_id);
                state->recv_invitations.erase(state->recv_invitations.begin() +
                                              state->recv_selected);
                state->recv_selected = state->recv_invitations.empty() ? -1 : 0;
                if (state->recv_invitations.empty()) {
                    Finish(Result::USER_CANCELED);
                }
            }
            EndDisabled();
            SameLine();
            x += BUTTON_SIZE.x + 10.0f;
            SetCursorPos({x, y});
            BeginDisabled(busy);
            if (Button("Cancel", BUTTON_SIZE)) {
                Finish(Result::USER_CANCELED);
            }
            EndDisabled();
            if (first_render && no_selection) {
                SetItemCurrentNavFocus();
            }
        }
    }
    End();

    first_render = false;
}

} // namespace Libraries::InvitationDialog
