// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

namespace Libraries::Np::NpProfileDialog {

struct NpProfileDialogState {
    std::string onlineId;
    int userId{};
};

class NpProfileDialogUi : public ImGui::Layer {
public:
    explicit NpProfileDialogUi(NpProfileDialogState* state = nullptr,
                               CommonDialog::Status* status = nullptr, int* result = nullptr);
    ~NpProfileDialogUi() override;

    NpProfileDialogUi(const NpProfileDialogUi& other) = delete;
    NpProfileDialogUi(NpProfileDialogUi&& other) noexcept;
    NpProfileDialogUi& operator=(NpProfileDialogUi other);

    void Finish(int result_code);

    void Draw() override;

private:
    NpProfileDialogState* state{};
    CommonDialog::Status* status{};
    int* result{};
    bool first_render{false};
};

} // namespace Libraries::Np::NpProfileDialog
