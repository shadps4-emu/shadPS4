// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>
#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"
#include "np_profile_dialog.h"

namespace Libraries::Np::NpProfileDialog {

struct NpProfileDialogState {
    std::string onlineId;
    OrbisNpAccountId accountId{};
    bool hasAccountId{false};
    OrbisNpProfileDialogMode mode{OrbisNpProfileDialogMode::ORBIS_NP_PROFILE_DIALOG_MODE_INVALID};
    int userId{};
};

class NpProfileDialogUi : public ImGui::Layer {
public:
    explicit NpProfileDialogUi(NpProfileDialogState* state = nullptr,
                               CommonDialog::Status* status = nullptr,
                               OrbisNpProfileDialogResult* result = nullptr);
    ~NpProfileDialogUi() override;

    NpProfileDialogUi(const NpProfileDialogUi& other) = delete;
    NpProfileDialogUi(NpProfileDialogUi&& other) noexcept;
    NpProfileDialogUi& operator=(NpProfileDialogUi other);

    void Finish(CommonDialog::Result user_action);

    void Draw() override;

private:
    NpProfileDialogState* state{};
    CommonDialog::Status* status{};
    OrbisNpProfileDialogResult* result{};
    bool first_render{false};
    float open_alpha{0.0f};
};

} // namespace Libraries::Np::NpProfileDialog
