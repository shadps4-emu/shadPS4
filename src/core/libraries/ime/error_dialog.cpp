// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/commondialog.h"
#include "error_dialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_std.h"

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

namespace Libraries::ErrorDialog {

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

static auto g_status = Status::NONE;
static ErrorDialogUi g_dialog_ui;

struct Param {
    s32 size;
    s32 errorCode;
    OrbisUserServiceUserId userId;
    s32 _reserved;
};

Error PS4_SYSV_ABI sceErrorDialogClose() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    g_dialog_ui.Finish();
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogGetStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Error PS4_SYSV_ABI sceErrorDialogInitialize() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    g_status = Status::INITIALIZED;
    return Error::OK;
}

Error PS4_SYSV_ABI sceErrorDialogOpen(const Param* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_ErrorDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_ErrorDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    const auto err = static_cast<u32>(param->errorCode);
    LOG_DEBUG(Lib_ErrorDialog, "called param->errorCode = {:#x}", err);
    ASSERT(param->size == sizeof(Param));

    const std::string err_message = fmt::format("An error has occurred. \nCode: {:#X}", err);
    g_status = Status::RUNNING;
    g_dialog_ui = ErrorDialogUi{&g_status, err_message};
    return Error::OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenDetail() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenWithReport() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceErrorDialogTerminate() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status == Status::RUNNING) {
        sceErrorDialogClose();
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_status = Status::NONE;
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogUpdateStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ekXHb1kDBl0", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogClose);
    LIB_FUNCTION("t2FvHRXzgqk", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogGetStatus);
    LIB_FUNCTION("I88KChlynSs", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogInitialize);
    LIB_FUNCTION("M2ZF-ClLhgY", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogOpen);
    LIB_FUNCTION("jrpnVQfJYgQ", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogOpenDetail);
    LIB_FUNCTION("wktCiyWoDTI", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogOpenWithReport);
    LIB_FUNCTION("9XAxK2PMwk8", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogTerminate);
    LIB_FUNCTION("WWiGuh9XfgQ", "libSceErrorDialog", 1, "libSceErrorDialog", 1, 1,
                 sceErrorDialogUpdateStatus);
};

} // namespace Libraries::ErrorDialog