// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

namespace Libraries::InvitationDialog {

// Data captured at Open(),OpenA, displayed by the UI layer, and read back by
// GetResult()/GetResultA().
struct DialogState {
    s32 mode = ORBIS_INVITATION_DIALOG_MODE_INVALID;
    CommonDialog::Result result = CommonDialog::Result::OK;
    bool user_editable = false; // USERENABLE recipient list (user picks in UI)
    std::vector<std::string> selected_npids;
    char friend_filter[64] = {};
    std::string message;
    Libraries::UserService::OrbisUserServiceUserId user_id =
        Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    std::string session_id;
};

// ImGui layer that renders the invitation dialog.
class InvitationDialogUi final : public ImGui::Layer {
    bool first_render{false};
    CommonDialog::Status* status{nullptr};
    DialogState* state{nullptr};

public:
    explicit InvitationDialogUi(CommonDialog::Status* status = nullptr,
                                DialogState* state = nullptr);
    ~InvitationDialogUi() override;
    InvitationDialogUi(const InvitationDialogUi&) = delete;
    InvitationDialogUi& operator=(const InvitationDialogUi&) = delete;
    InvitationDialogUi(InvitationDialogUi&& other) noexcept;
    InvitationDialogUi& operator=(InvitationDialogUi&& other) noexcept;

    // Records the call result and transitions the dialog to FINISHED.
    void Finish(CommonDialog::Result result);

    void Draw() override;
};

} // namespace Libraries::InvitationDialog
