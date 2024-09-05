// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "common/assert.h"
#include "imgui/imgui_std.h"
#include "msgdialog_ui.h"

using namespace ImGui;
using namespace Libraries::CommonDialog;
using namespace Libraries::MsgDialog;

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};
static constexpr float PROGRESS_BAR_WIDTH{0.8f};

struct {
    int count = 0;
    const char* text1;
    const char* text2;
} static constexpr user_button_texts[] = {
    {1, "OK"},             // 0 OK
    {2, "Yes", "No"},      // 1 YESNO
    {0},                   // 2 NONE
    {2, "OK", "Cancel"},   // 3 OK_CANCEL
    {},                    // 4 !!NOP
    {1, "Wait"},           // 5 WAIT
    {2, "Wait", "Cancel"}, // 6 WAIT_CANCEL
    {2, "Yes", "No"},      // 7 YESNO_FOCUS_NO
    {2, "OK", "Cancel"},   // 8 OK_CANCEL_FOCUS_CANCEL
    {0xFF},                // 9 TWO_BUTTONS
};
static_assert(std::size(user_button_texts) == static_cast<int>(ButtonType::TWO_BUTTONS) + 1);

static void DrawCenteredText(const char* text) {
    const auto ws = GetWindowSize();
    const auto text_size = CalcTextSize(text, nullptr, false, ws.x - 40.0f);
    PushTextWrapPos(ws.x - 30.0f);
    SetCursorPos({
        (ws.x - text_size.x) / 2.0f,
        (ws.y - text_size.y) / 2.0f - 50.0f,
    });
    Text("%s", text);
    PopTextWrapPos();
}

MsgDialogState::MsgDialogState(const OrbisParam& param) {
    this->mode = param.mode;
    switch (mode) {
    case MsgDialogMode::USER_MSG: {
        ASSERT(param.userMsgParam);
        const auto& v = *param.userMsgParam;
        auto state = UserState{
            .type = v.buttonType,
            .msg = std::string(v.msg),
        };
        if (v.buttonType == ButtonType::TWO_BUTTONS) {
            ASSERT(v.buttonsParam);
            state.btn_param1 = std::string(v.buttonsParam->msg1);
            state.btn_param2 = std::string(v.buttonsParam->msg2);
        }
        this->state = state;
    } break;
    case MsgDialogMode::PROGRESS_BAR: {
        ASSERT(param.progBarParam);
        const auto& v = *param.progBarParam;
        this->state = ProgressState{
            .type = v.barType,
            .msg = std::string(v.msg),
            .progress = 0,
        };
    } break;
    case MsgDialogMode::SYSTEM_MSG: {
        ASSERT(param.sysMsgParam);
        const auto& v = *param.sysMsgParam;
        this->state = SystemState{
            .type = v.sysMsgType,
        };
    } break;
    default:
        UNREACHABLE_MSG("Unknown dialog mode");
    }
}

void MsgDialogUi::DrawUser() {
    const auto& [button_type, msg, btn_param1, btn_param2] =
        state->GetState<MsgDialogState::UserState>();
    const auto ws = GetWindowSize();
    DrawCenteredText(msg.c_str());
    ASSERT(button_type <= ButtonType::TWO_BUTTONS);
    auto [count, text1, text2] = user_button_texts[static_cast<u32>(button_type)];
    if (count == 0xFF) { // TWO_BUTTONS -> User defined message
        count = 2;
        text1 = btn_param1.c_str();
        text2 = btn_param2.c_str();
    }
    const bool focus_first = button_type < ButtonType::YESNO_FOCUS_NO;
    SetCursorPos({
        ws.x / 2.0f - BUTTON_SIZE.x / 2.0f * static_cast<float>(count),
        ws.y - 10.0f - BUTTON_SIZE.y,
    });
    BeginGroup();
    if (count > 0) {
        // First button at the right, so we render the second button first
        if (count == 2) {
            PushID(2);
            if (Button(text2, BUTTON_SIZE)) {
                switch (button_type) {
                case ButtonType::OK_CANCEL:
                case ButtonType::WAIT_CANCEL:
                case ButtonType::OK_CANCEL_FOCUS_CANCEL:
                    Finish(ButtonId::INVALID, Result::USER_CANCELED);
                    break;
                default:
                    Finish(ButtonId::BUTTON2);
                    break;
                }
            }
            if (first_render && !focus_first) {
                SetItemCurrentNavFocus();
            }
            PopID();
            SameLine();
        }
        PushID(1);
        if (Button(text1, BUTTON_SIZE)) {
            Finish(ButtonId::BUTTON1);
        }
        if (first_render && focus_first) {
            SetItemCurrentNavFocus();
        }
        PopID();
        SameLine();
    }
    EndGroup();
}

void MsgDialogUi::DrawProgressBar() {
    const auto& [bar_type, msg, progress_bar_value] =
        state->GetState<MsgDialogState::ProgressState>();
    DrawCenteredText(msg.c_str());
    const auto ws = GetWindowSize();
    SetCursorPos({
        ws.x * ((1 - PROGRESS_BAR_WIDTH) / 2.0f),
        ws.y - 10.0f - BUTTON_SIZE.y,
    });
    const bool has_cancel = bar_type == ProgressBarType::PERCENTAGE_CANCEL;
    float bar_width = PROGRESS_BAR_WIDTH * ws.x;
    if (has_cancel) {
        bar_width -= BUTTON_SIZE.x - 10.0f;
    }
    BeginGroup();
    ProgressBar(static_cast<float>(progress_bar_value) / 100.0f, {bar_width, BUTTON_SIZE.y});
    if (has_cancel) {
        SameLine();
        if (Button("Cancel", BUTTON_SIZE)) {
            Finish(ButtonId::INVALID, Result::USER_CANCELED);
        }
        if (first_render) {
            SetItemCurrentNavFocus();
        }
    }
    EndGroup();
}

struct {
    const char* text;
} static constexpr system_message_texts[] = {
    "No product available in the store.",            // TRC_EMPTY_STORE
    "PSN chat restriction.",                         // TRC_PSN_CHAT_RESTRICTION
    "User-generated Media restriction",              // TRC_PSN_UGC_RESTRICTION
    nullptr,                                         // !!NOP
    "Camera not connected.",                         // CAMERA_NOT_CONNECTED
    "Warning: profile picture and name are not set", // WARNING_PROFILE_PICTURE_AND_NAME_NOT_SHARED
};
static_assert(std::size(system_message_texts) ==
              static_cast<int>(SystemMessageType::WARNING_PROFILE_PICTURE_AND_NAME_NOT_SHARED) + 1);

void MsgDialogUi::DrawSystemMessage() {
    // TODO: Implement go to settings & user profile
    const auto& [msg_type] = state->GetState<MsgDialogState::SystemState>();
    ASSERT(msg_type <= SystemMessageType::WARNING_PROFILE_PICTURE_AND_NAME_NOT_SHARED);
    auto [msg] = system_message_texts[static_cast<u32>(msg_type)];
    DrawCenteredText(msg);
    const auto ws = GetWindowSize();
    SetCursorPos({
        ws.x / 2.0f - BUTTON_SIZE.x / 2.0f,
        ws.y - 10.0f - BUTTON_SIZE.y,
    });
    if (Button("OK", BUTTON_SIZE)) {
        Finish(ButtonId::OK);
    }
    if (first_render) {
        SetItemCurrentNavFocus();
    }
}

MsgDialogUi::MsgDialogUi(MsgDialogState* state, Status* status, DialogResult* result)
    : state(state), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        first_render = true;
        AddLayer(this);
    }
}
MsgDialogUi::~MsgDialogUi() {
    Finish(ButtonId::INVALID);
}
MsgDialogUi::MsgDialogUi(MsgDialogUi&& other) noexcept
    : Layer(other), state(other.state), status(other.status), result(other.result) {
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
}
MsgDialogUi& MsgDialogUi::operator=(MsgDialogUi other) {
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

void MsgDialogUi::Finish(ButtonId buttonId, Result r) {
    if (result) {
        result->result = r;
        result->buttonId = buttonId;
    }
    if (status) {
        *status = Status::FINISHED;
    }
    state = nullptr;
    status = nullptr;
    result = nullptr;
    RemoveLayer(this);
}

void MsgDialogUi::Draw() {
    if (status == nullptr || *status != Status::RUNNING) {
        return;
    }
    const auto& io = GetIO();

    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 500.0f),
        std::min(io.DisplaySize.y, 300.0f),
    };

    CentralizeWindow();
    SetNextWindowSize(window_size);
    SetNextWindowFocus();
    SetNextWindowCollapsed(false);
    KeepNavHighlight();
    // Hack to allow every dialog to have a unique window
    if (Begin("Message Dialog##MessageDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        switch (state->GetMode()) {
        case MsgDialogMode::USER_MSG:
            DrawUser();
            break;
        case MsgDialogMode::PROGRESS_BAR:
            DrawProgressBar();
            break;
        case MsgDialogMode::SYSTEM_MSG:
            DrawSystemMessage();
            break;
        }
        End();
    }

    first_render = false;
}