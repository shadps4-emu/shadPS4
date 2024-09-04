// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "imgui/imgui_std.h"

#include "common/assert.h"
#include "msgdialog.h"

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
    SetCursorPosY(ws.y / 2.0f - 50.0f);
    PushTextWrapPos(ws.x - 30.0f);
    SetCursorPosX((ws.x - CalcTextSize(text, nullptr, false, ws.x - 40.0f).x) / 2.0f);
    Text("%s", text);
    PopTextWrapPos();
}

void MsgDialogUi::DrawUser() {
    const auto& [button_type, msg, btn_param, _] = *param->userMsgParam;
    const auto ws = GetWindowSize();
    DrawCenteredText(msg);
    ASSERT(button_type <= ButtonType::TWO_BUTTONS);
    auto [count, text1, text2] = user_button_texts[static_cast<u32>(button_type)];
    if (count == 0xFF) { // TWO_BUTTONS -> User defined message
        count = 2;
        text1 = btn_param->msg1;
        text2 = btn_param->msg2;
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
            if (!focus_first) {
                SetItemDefaultNav();
            }
            PopID();
            SameLine();
        }
        PushID(1);
        if (Button(text1, BUTTON_SIZE)) {
            Finish(ButtonId::BUTTON1);
        }
        if (focus_first) {
            SetItemDefaultNav();
        }
        PopID();
        SameLine();
    }
    EndGroup();
}

void MsgDialogUi::DrawProgressBar() {
    const auto& [bar_type, msg, _] = *param->progBarParam;
    DrawCenteredText(msg);
    const auto ws = GetWindowSize();
    SetCursorPos({
        ws.x * ((1 - PROGRESS_BAR_WIDTH) / 2.0f),
        ws.y - 10.0f - BUTTON_SIZE.y,
    });
    bool has_cancel = bar_type == ProgressBarType::PERCENTAGE_CANCEL;
    float bar_width = PROGRESS_BAR_WIDTH * ws.x;
    if (has_cancel) {
        bar_width -= BUTTON_SIZE.x - 10.0f;
    }
    BeginGroup();
    ProgressBar((float)progress_bar_value / 100.0f, {bar_width, BUTTON_SIZE.y});
    if (has_cancel) {
        SameLine();
        if (Button("Cancel", BUTTON_SIZE)) {
            Finish(ButtonId::INVALID, Result::USER_CANCELED);
        }
    }
    EndGroup();
}

void MsgDialogUi::DrawSystemMessage() {}

MsgDialogUi::MsgDialogUi(const Param* param, Status* status, MsgDialogResult* result)
    : dialog_unique_id([] {
          static s32 last_id = 0;
          return ++last_id;
      }()),
      param(param), status(status), result(result) {
    if (status && *status == Status::RUNNING) {
        AddLayer(this);
    }
}
MsgDialogUi::~MsgDialogUi() {
    Finish(ButtonId::INVALID);
}
MsgDialogUi::MsgDialogUi(MsgDialogUi&& other) noexcept
    : Layer(other), dialog_unique_id(other.dialog_unique_id), param(other.param),
      status(other.status), result(other.result) {
    other.dialog_unique_id = 0;
    other.param = nullptr;
    other.status = nullptr;
    other.result = nullptr;
}
MsgDialogUi& MsgDialogUi::operator=(MsgDialogUi other) {
    using std::swap;
    swap(dialog_unique_id, other.dialog_unique_id);
    swap(param, other.param);
    swap(status, other.status);
    swap(result, other.result);
    if (status && *status == Status::RUNNING) {
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
    param = nullptr;
    status = nullptr;
    result = nullptr;
    RemoveLayer(this);
}

void MsgDialogUi::SetProgressBarValue(u32 value, bool increment) {
    if (increment) {
        progress_bar_value += value;
    } else {
        progress_bar_value = value;
    }
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
#define TITLE "Message Dialog##MD"
    char id[sizeof(TITLE) + sizeof(int)] = TITLE "\0\0\0\0";
    *reinterpret_cast<int*>(&id[sizeof(TITLE) - 1]) =
        dialog_unique_id |
        0x80808080; // keep the MSB set to extend the string length (null terminated)
#undef TITLE
    if (Begin(id, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        switch (param->mode) {
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
}