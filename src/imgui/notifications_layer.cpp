// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include <queue>

#include "imgui/imgui_std.h"
#include "notifications_layer.h"

CMRC_DECLARE(res);
namespace shadNotifications {

std::optional<NotificationsUI> current_notif;
std::queue<NotificationInfo> notif_queue;
std::mutex queueMtx;

const std::map<shadNotifications::stockIcons, std::string> iconMap = {
    {shadNotifications::stockIcons::shadPS4, "src/images/shadps4.png"},
    {shadNotifications::stockIcons::Settings, "src/images/big_picture/settings.png"},
    {shadNotifications::stockIcons::Profiles, "src/images/big_picture/profiles.png"},
    {shadNotifications::stockIcons::Input, "src/images/big_picture/controller.png"},
};

NotificationsUI::NotificationsUI(NotificationInfo info) {
    AddLayer(this);
    currentInfo = info;

    // Resetting the animation
    elapsed_time = 0.0f; // Resetting animation time
    fade_opacity = 0.0f; // Starts invisible
}

NotificationsUI::~NotificationsUI() {
    RemoveLayer(this);
}

void NotificationsUI::Draw() {
    auto& io = ImGui::GetIO();
    currentInfo.timer -= io.DeltaTime;

    float AdjustWidth = io.DisplaySize.x / 1920;
    float AdjustHeight = io.DisplaySize.y / 1080;
    ImVec2 padding = ImGui::GetStyle().WindowPadding;

    float wrapWidth = 300 * AdjustWidth - padding.x * 2; // 350 window size - 50 image size
    float textHeight =
        ImGui::CalcTextSize(currentInfo.message.c_str(), nullptr, false, wrapWidth).y;
    ImVec2 window_size{(350 * AdjustWidth),
                       std::max({70 * AdjustHeight, (textHeight + padding.y * 2.0f)})};

    elapsed_time += io.DeltaTime;
    float progress = std::min(elapsed_time / animation_duration, 1.0f);
    float final_pos_x, start_x;
    float height;

    if (currentInfo.pos == position::TopLeft) {
        start_x = -window_size.x;
        final_pos_x = 20 * AdjustWidth;
        height = 20 * AdjustHeight;
    } else if (currentInfo.pos == position::TopRight) {
        start_x = io.DisplaySize.x;
        final_pos_x = io.DisplaySize.x - window_size.x - 20 * AdjustWidth;
        height = 20 * AdjustHeight;
    } else if (currentInfo.pos == position::BottomLeft) {
        start_x = -window_size.x;
        final_pos_x = 20 * AdjustWidth;
        height = io.DisplaySize.y - window_size.y - 20 * AdjustHeight;
    } else if (currentInfo.pos == position::BottomRight) {
        start_x = io.DisplaySize.x;
        final_pos_x = io.DisplaySize.x - window_size.x - 20 * AdjustWidth;
        height = io.DisplaySize.y - window_size.y - 20 * AdjustHeight;
    }

    ImVec2 current_pos = ImVec2(start_x + (final_pos_x - start_x) * progress, height);
    ImGui::SetNextWindowPos(current_pos);

    // If the remaining time of the notif is less than or equal to 1 second, the fade-out begins.
    if (currentInfo.timer <= 1.0f) {
        float fade_out_time = 1.0f - (currentInfo.timer / 1.0f);
        fade_opacity = 1.0f - fade_out_time;
    } else {
        // Fade in , 0 to 1
        fade_opacity = 1.0f;
    }

    fade_opacity = std::max(0.0f, std::min(fade_opacity, 1.0f));

    ImGui::SetNextWindowSize(window_size);
    ImGui::SetNextWindowPos(current_pos);
    ImGui::SetNextWindowCollapsed(false);
    ImGui::KeepNavHighlight();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade_opacity);

    if (ImGui::Begin("Notification Window", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoInputs)) {
        ImGui::GetColorU32(ImVec4{0.7f});

        if (currentInfo.icon.GetTexture().im_id) {
            if (currentInfo.addIconBackground) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    pos, ImVec2(pos.x + 50 * AdjustWidth, pos.y + 50 * AdjustHeight),
                    IM_COL32(211, 211, 211, 255));
            }
            ImGui::Image(currentInfo.icon.GetTexture().im_id,
                         ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
        }
        ImGui::SameLine();
        ImGui::TextWrapped("%s", currentInfo.message.c_str());
    }

    ImGui::PopStyleVar();
    ImGui::End();

    if (currentInfo.timer <= 0) {
        std::lock_guard<std::mutex> lock(queueMtx);
        if (!notif_queue.empty()) {
            NotificationInfo next = notif_queue.front();
            notif_queue.pop();
            current_notif.emplace(next);
        } else {
            current_notif.reset();
        }
    }
}

void QueueNotification(std::string message, float timer, position pos, stockIcons icon) {
    auto resource = cmrc::res::get_filesystem();
    auto file = resource.open(iconMap.at(icon));
    std::vector<u8> imgdata = std::vector<u8>(file.begin(), file.end());

    bool addBackground = false;
    if (icon == shadNotifications::stockIcons::Input ||
        icon == shadNotifications::stockIcons::Settings ||
        icon == shadNotifications::stockIcons::Profiles) {
        addBackground = true;
    }

    QueueNotification(message, timer, pos, imgdata, addBackground);
}

void QueueNotification(std::string message, float timer, position pos, std::vector<u8> pngData,
                       bool addBackground) {
    std::lock_guard<std::mutex> lock(queueMtx);

    NotificationInfo info;
    info.message = message;
    info.timer = timer;
    info.pos = pos;
    info.icon = ImGui::RefCountedTexture::DecodePngTexture(pngData);
    info.addIconBackground = addBackground;

    if (!current_notif.has_value()) {
        current_notif.emplace(info);
    } else {
        notif_queue.push(info);
    }
}

} // namespace shadNotifications
