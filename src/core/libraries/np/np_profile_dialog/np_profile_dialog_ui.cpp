// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <utility>

#include <imgui.h>
#include <imgui/imgui_std.h>
#include "np_profile_dialog_ui.h"

using namespace ImGui;
using namespace Libraries::CommonDialog;

namespace Libraries::Np::NpProfileDialog {

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

NpProfileDialogUi::NpProfileDialogUi(NpProfileDialogState* state, Status* status, int* result)
    : state(state), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}

NpProfileDialogUi::~NpProfileDialogUi() {
    Finish(0);
}

NpProfileDialogUi::NpProfileDialogUi(NpProfileDialogUi&& other) noexcept
    : Layer(other), state(other.state), status(other.status), result(other.result) {
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
}

NpProfileDialogUi& NpProfileDialogUi::operator=(NpProfileDialogUi other) {
    using std::swap;
    swap(state, other.state);
    swap(status, other.status);
    swap(result, other.result);
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
    return *this;
}

void NpProfileDialogUi::Finish(int result_code) {
    if (result) {
        *result = result_code;
    }
    if (status) {
        *status = Status::FINISHED;
    }
    state = nullptr;
    status = nullptr;
    result = nullptr;
    RemoveLayer(this);
}

void NpProfileDialogUi::Draw() {
    if (status == nullptr || *status != Status::RUNNING) {
        return;
    }

    const auto& io = GetIO();
    ImVec2 window_size{std::min(io.DisplaySize.x, 400.0f), std::min(io.DisplaySize.y, 200.0f)};

    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }
    KeepNavHighlight();

    if (Begin("NP Profile##NpProfileDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {

        Text("Player Profile");
        Separator();
        Spacing();

        Text("Online ID:");
        Text("%s", state->onlineId.c_str());

        Spacing();
        SetCursorPos({
            window_size.x / 2.0f - BUTTON_SIZE.x / 2.0f,
            window_size.y - 10.0f - BUTTON_SIZE.y,
        });

        if (Button("OK", BUTTON_SIZE)) {
            Finish(0);
        }

        if (first_render) {
            SetItemCurrentNavFocus();
        }
    }

    End();

    first_render = false;
}

} // namespace Libraries::Np::NpProfileDialog
