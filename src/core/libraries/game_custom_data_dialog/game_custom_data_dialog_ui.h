// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::GameCustomDataDialog {

struct DialogState {
    s32 mode = ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_INVALID;
    Libraries::UserService::OrbisUserServiceUserId user_id =
        Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    void* callback_arg = nullptr;
    CommonDialog::Result result = CommonDialog::Result::OK;

    std::string data_name;
    std::string data_detail;
    std::string data_url; // SEND_URL
    bool with_item_id = false;
    u32 data_size = 0; // SEND
    s32 expire_minutes = 0;
    std::vector<std::string> platforms;
    ImGui::RefCountedTexture thumbnail;
    bool has_thumbnail = false;
    std::array<char, 2049> message{};
    bool user_editable = false;
    u32 user_max_count = 0;
    std::vector<std::string> fixed_online_ids;
    std::vector<Libraries::Np::OrbisNpAccountId> fixed_account_ids;
    std::vector<std::string> selected_npids;
    char friend_filter[64] = {};
    std::vector<std::string> sent_online_ids;
    std::vector<Libraries::Np::OrbisNpAccountId> sent_account_ids;
};

class GameCustomDataDialogUi final : public ImGui::Layer {
    bool first_render{false};
    CommonDialog::Status* status{nullptr};
    DialogState* state{nullptr};

    void DrawSend();
    void DrawReceive();
    void Send();

public:
    explicit GameCustomDataDialogUi(CommonDialog::Status* status = nullptr,
                                    DialogState* state = nullptr);
    ~GameCustomDataDialogUi() override;
    GameCustomDataDialogUi(const GameCustomDataDialogUi&) = delete;
    GameCustomDataDialogUi& operator=(const GameCustomDataDialogUi&) = delete;
    GameCustomDataDialogUi(GameCustomDataDialogUi&& other) noexcept;
    GameCustomDataDialogUi& operator=(GameCustomDataDialogUi&& other) noexcept;
    void Finish(CommonDialog::Result result);
    void Draw() override;
};

} // namespace Libraries::GameCustomDataDialog
