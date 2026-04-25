// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_std.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::ErrorDialog {

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

using OrbisUserServiceUserId = s32;

struct Param;

CommonDialog::Error PS4_SYSV_ABI sceErrorDialogClose();
CommonDialog::Status PS4_SYSV_ABI sceErrorDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogOpen(const Param* param);
int PS4_SYSV_ABI sceErrorDialogOpenDetail();
int PS4_SYSV_ABI sceErrorDialogOpenWithReport();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI sceErrorDialogUpdateStatus();

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

class ErrorDialogUi final : public ImGui::Layer {
    bool first_render{false};

    Status* status{nullptr};
    std::string err_message{};

public:
    explicit ErrorDialogUi(Status* status = nullptr, std::string err_message = "")
        : status(status), err_message(std::move(err_message)) {
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
    }
    ~ErrorDialogUi() override {
        Finish();
    }
    ErrorDialogUi(const ErrorDialogUi& other) = delete;
    ErrorDialogUi(ErrorDialogUi&& other) noexcept
        : Layer(other), status(other.status), err_message(std::move(other.err_message)) {
        other.status = nullptr;
    }
    ErrorDialogUi& operator=(ErrorDialogUi other) {
        using std::swap;
        swap(status, other.status);
        swap(err_message, other.err_message);
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
        return *this;
    }

    void Finish() {
        if (status) {
            *status = Status::FINISHED;
        }
        status = nullptr;
        RemoveLayer(this);
    }

    void Draw() override {
        using namespace ImGui;
        if (status == nullptr || *status != Status::RUNNING) {
            return;
        }
        const auto& io = GetIO();

        const ImVec2 window_size{
            std::min(io.DisplaySize.x, 500.0f),
            std::min(io.DisplaySize.y, 300.0f),
        };

        CentralizeNextWindow();
        SetNextWindowSize(window_size);
        SetNextWindowCollapsed(false);
        if (first_render || !io.NavActive) {
            SetNextWindowFocus();
        }
        KeepNavHighlight();
        if (Begin("Error Dialog##ErrorDialog", nullptr,
                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
            const auto ws = GetWindowSize();

            DrawPrettyBackground();
            const char* begin = &err_message.front();
            const char* end = &err_message.back() + 1;
            SetWindowFontScale(1.3f);
            DrawCenteredText(begin, end,
                             GetContentRegionAvail() - ImVec2{0.0f, 15.0f + BUTTON_SIZE.y});
            SetWindowFontScale(1.0f);

            SetCursorPos({
                ws.x / 2.0f - BUTTON_SIZE.x / 2.0f,
                ws.y - 10.0f - BUTTON_SIZE.y,
            });
            if (Button("OK", BUTTON_SIZE)) {
                Finish();
            }
            if (first_render) {
                SetItemCurrentNavFocus();
            }
        }
        End();

        first_render = false;
    }
};

struct Library {
    Library(Core::Loader::SymbolsResolver* sym);

    CommonDialog::Status g_status = CommonDialog::Status::NONE;
    ErrorDialogUi g_dialog_ui;
};
} // namespace Libraries::ErrorDialog