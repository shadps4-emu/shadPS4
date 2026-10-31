// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
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
    std::vector<std::pair<std::string, std::string>> localized_names;
    std::vector<std::pair<std::string, std::string>> localized_details;
    std::string data_url;
    bool with_item_id = false;
    u32 data_size = 0;
    std::vector<u8> game_data;
    s32 expire_minutes = 0;
    std::vector<std::string> platforms;
    std::vector<u8> thumbnail_bytes;
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
    struct ReceivedItem {
        Libraries::Np::NpHandler::ReceivedGameCustomData data;
        ImGui::RefCountedTexture thumbnail;
        bool has_thumbnail = false;
        bool expired = false;
    };
    std::vector<ReceivedItem> received;
    int selected = -1;
};

class GameCustomDataDialogUi final : public ImGui::Layer {
    struct PendingSend {
        std::atomic<bool> done{false};
        Libraries::Np::NpHandler::GameCustomDataResult result;
    };
    struct PendingList {
        std::atomic<bool> done{false};
        Libraries::Np::NpHandler::GameCustomDataResult result;
        std::vector<Libraries::Np::NpHandler::ReceivedGameCustomData> items;
    };

    bool first_render{false};
    CommonDialog::Status* status{nullptr};
    DialogState* state{nullptr};
    std::shared_ptr<PendingSend> pending_send;
    std::shared_ptr<PendingList> pending_list;
    bool list_loaded{false};
    std::string error;

    void DrawSend();
    void DrawReceive();
    void StartSend();
    void FinishSend();
    void StartList();
    void FinishList();

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
