// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace shadNotifications {

enum class position {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

struct NotificationInfo {
    std::string message;
    float timer;
    position position;
};

class NotificationsUI final : public ImGui::Layer {
public:
    NotificationsUI(NotificationInfo info);
    ~NotificationsUI() override;

    void Draw() override;

private:
    ImGui::RefCountedTexture icon;
    NotificationInfo currentInfo;

    // notification animation
    const float animation_duration = 0.5f; // Animation duration
    const float fade_out_duration = 0.5f;  // Final fade duration
    float fade_opacity = 0.0f;             // Initial opacity (invisible)
    float elapsed_time = 0.0f;             // Animation time
};

void QueueNotification(std::string message, float timer, position position);

}; // namespace shadNotifications
