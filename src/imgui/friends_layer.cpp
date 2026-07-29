// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>

#include "common/types.h"
#include "core/libraries/np/np_handler.h"
#include "imgui/friends_layer.h"
#include "imgui/imgui_layer.h"
#include "imgui/shadnet_notifications_layer.h"

namespace ImGui::Friends {

bool g_open = false;
int g_user_index = 0;
char g_add_buf[64] = {};

class FriendsLayer final : public ImGui::Layer {
public:
    void Draw() override;
};

FriendsLayer g_layer;

void DrawFriendsTab() {
    auto& np = Libraries::Np::NpHandler::GetInstance();
    const std::vector<s32> users = np.GetConnectedUsers();

    if (users.empty()) {
        ImGui::TextUnformatted("No user is signed in to shadNet.");
        return;
    }

    if (g_user_index >= static_cast<int>(users.size())) {
        g_user_index = 0;
    }
    if (users.size() > 1) {
        const std::string preview = "user " + std::to_string(users[g_user_index]);
        if (ImGui::BeginCombo("User", preview.c_str())) {
            for (int i = 0; i < static_cast<int>(users.size()); ++i) {
                const std::string label = "user " + std::to_string(users[i]);
                if (ImGui::Selectable(label.c_str(), i == g_user_index)) {
                    g_user_index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
    const s32 user_id = users[g_user_index];
    const Libraries::Np::NpHandler::FriendListSnapshot fl = np.GetFriendList(user_id);

    bool appear_offline = np.IsAppearOffline();
    if (ImGui::Checkbox("Appear offline", &appear_offline)) {
        np.SetAppearOffline(appear_offline);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(friends see you as offline)");

    ImGui::SeparatorText("Add friend");
    ImGui::InputTextWithHint("##addnpid", "Online ID", g_add_buf, sizeof(g_add_buf));
    ImGui::SameLine();
    if (ImGui::Button("Send request") && g_add_buf[0] != '\0') {
        np.SendFriendRequest(user_id, g_add_buf);
        g_add_buf[0] = '\0';
    }

    if (!fl.requests_received.empty()) {
        ImGui::SeparatorText("Requests received");
        for (const auto& npid : fl.requests_received) {
            ImGui::PushID(npid.c_str());
            ImGui::TextUnformatted(npid.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Accept")) {
                np.SendFriendRequest(user_id, npid);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Decline")) {
                np.RemoveFriend(user_id, npid);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Block")) {
                np.BlockUser(user_id, npid);
            }
            ImGui::PopID();
        }
    }

    if (!fl.requests_sent.empty()) {
        ImGui::SeparatorText("Requests sent");
        for (const auto& npid : fl.requests_sent) {
            ImGui::PushID(npid.c_str());
            ImGui::TextUnformatted(npid.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel")) {
                np.RemoveFriend(user_id, npid);
            }
            ImGui::PopID();
        }
    }

    ImGui::SeparatorText(("Friends (" + std::to_string(fl.friends.size()) + ")").c_str());
    for (const auto& f : fl.friends) {
        ImGui::PushID(f.npid.c_str());
        const ImVec4 col =
            f.online ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f) : ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
        ImGui::TextColored(col, "%s", f.online ? "[on] " : "[off]");
        ImGui::SameLine();
        ImGui::TextUnformatted(f.npid.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            np.RemoveFriend(user_id, f.npid);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Block")) {
            np.BlockUser(user_id, f.npid);
        }
        ImGui::PopID();
    }

    if (!fl.blocked.empty()) {
        ImGui::SeparatorText("Blocked");
        for (const auto& npid : fl.blocked) {
            ImGui::PushID(npid.c_str());
            ImGui::TextUnformatted(npid.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Unblock")) {
                np.UnblockUser(user_id, npid);
            }
            ImGui::PopID();
        }
    }
}

void DrawNotificationsTab() {
    if (ImGui::SmallButton("Clear")) {
        ImGui::ShadNetNotify::ClearHistory();
    }
    const std::vector<ImGui::ShadNetNotify::HistoryEntry> history =
        ImGui::ShadNetNotify::GetHistory();
    ImGui::SameLine();
    ImGui::TextDisabled("%zu event(s)", history.size());
    ImGui::Separator();

    ImGui::BeginChild("##notif_history", ImVec2(0.0f, 0.0f));
    if (history.empty()) {
        ImGui::TextDisabled("No shadNet notifications yet.");
    }
    // Newest first.
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        const ImGui::ShadNetNotify::KindDisplay d = ImGui::ShadNetNotify::DisplayOf(it->kind);
        const ImVec4 col(d.color[0], d.color[1], d.color[2], d.color[3]);
        if (!it->time.empty()) {
            ImGui::TextDisabled("%s", it->time.c_str());
            ImGui::SameLine();
        }
        ImGui::TextColored(col, "[%s]", d.tag);
        ImGui::SameLine();
        ImGui::TextWrapped("%s", it->text.c_str());
    }
    ImGui::EndChild();
}

void Register() {
    ImGui::Layer::AddLayer(&g_layer);
}
void Unregister() {
    ImGui::Layer::RemoveLayer(&g_layer);
}
void Toggle() {
    g_open = !g_open;
}
bool IsOpen() {
    return g_open;
}

void FriendsLayer::Draw() {
    if (!g_open) {
        return;
    }

    const std::size_t unread = ImGui::ShadNetNotify::UnreadCount();
    const std::string title =
        unread > 0 ? "shadNet Friends (" + std::to_string(unread) + ")###shadnet_friends"
                   : "shadNet Friends###shadnet_friends";

    ImGui::SetNextWindowSize(ImVec2(440.0f, 560.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title.c_str(), &g_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##shadnet_tabs")) {
        if (ImGui::BeginTabItem("Friends")) {
            DrawFriendsTab();
            ImGui::EndTabItem();
        }
        const std::string notif_label =
            unread > 0 ? "Notifications (" + std::to_string(unread) + ")###notif"
                       : "Notifications###notif";
        if (ImGui::BeginTabItem(notif_label.c_str())) {
            ImGui::ShadNetNotify::MarkRead();
            DrawNotificationsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace ImGui::Friends
