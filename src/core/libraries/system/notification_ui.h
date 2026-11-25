// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <SDL3/SDL_audio.h>

#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::SystemNotification {

enum class NotificationIcon : u8 {
    System,
    Error,
    Capture,
    Download,
    Upload,
    Party,
    User,
    Device,
    App,
};

enum class NotificationSound : u8 {
    System,
    Notification,
    PsButton,
    PartyEnter,
    PartyQuit,
    Capture,
};

enum class BatteryStatus : u8 {
    None,
    Level0,
    Level1,
    Level2,
    Level3,
    Charging,
    LowBlinking,
    NoCharge,
};

struct AvatarFetchState;

class SystemNotificationUI final : public ImGui::Layer {
public:
    SystemNotificationUI(const std::filesystem::path& iconPath, std::vector<u8> iconData,
                         const std::string& title, const std::string& message,
                         NotificationIcon iconType = NotificationIcon::System,
                         NotificationSound soundType = NotificationSound::System, s32 userId = -1,
                         const std::string& userName = "",
                         BatteryStatus batteryStatus = BatteryStatus::None,
                         const std::string& targetUser = "");
    ~SystemNotificationUI() override;

    void Finish();

    void Draw() override;

private:
    void PlayMp3(std::span<const u8> mp3Data);
    void PlayWav(std::span<const u8> wavData);

    std::string notif_title;
    std::string notif_message;
    std::string notif_user_name;
    ImGui::RefCountedTexture notif_icon;
    ImGui::RefCountedTexture user_avatar;
    ImGui::RefCountedTexture notification_bg_icon;
    std::array<ImGui::RefCountedTexture, 5> battery_icons;
    std::shared_ptr<AvatarFetchState> avatar_fetch;
    std::shared_ptr<AvatarFetchState> body_avatar_fetch;
    BatteryStatus battery_status = BatteryStatus::None;
    s32 notif_user_id = -1;
    u32 icon_bg_color = 0;
    bool custom_icon_bg = false;
    bool has_fetched_user_avatar = false;
    bool has_body_avatar = false;
    bool is_white_bg = false;
    bool bg_left_shadow = false;
    bool has_user_panel = false;
    SDL_AudioStream* stream = nullptr;
    SDL_AudioDeviceID audioDevice = 0;
    float elapsed_time = 0.0f;
    double notif_timer = 5.0;
    std::string side = "left";

    friend void UpdateDeviceBatteryStatus(s32 userId, BatteryStatus batteryStatus);
};

struct NotificationInfo {
    std::filesystem::path icon_path;
    std::vector<u8> icon_data;
    std::string title;
    std::string message;
    NotificationIcon icon_type = NotificationIcon::System;
    NotificationSound sound_type = NotificationSound::System;
    s32 user_id = -1;
    std::string user_name;
    BatteryStatus battery_status = BatteryStatus::None;
    std::string target_user;
};

bool IsNotificationShowing();
void ShowNextQueuedNotification();

void AddNotificationToQueue(const std::string& message, const std::string& title = "",
                            const std::filesystem::path& iconPath = {},
                            std::vector<u8> iconData = {},
                            NotificationIcon iconType = NotificationIcon::System,
                            NotificationSound soundType = NotificationSound::System,
                            s32 userId = -1, const std::string& userName = "",
                            BatteryStatus batteryStatus = BatteryStatus::None,
                            const std::string& targetUser = "");

void AddDeviceHandoverNotification(s32 userId, const std::string& userName,
                                   BatteryStatus batteryStatus);

void UpdateDeviceBatteryStatus(s32 userId, BatteryStatus batteryStatus);

} // namespace Libraries::SystemNotification
