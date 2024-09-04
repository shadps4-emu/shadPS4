// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/msgdialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_std.h"
#include "imgui_internal.h"

namespace Libraries::MsgDialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

using OrbisUserServiceUserId = s32;

enum class MsgDialogMode : u32 {
    USER_MSG = 1,
    PROGRESS_BAR = 2,
    SYSTEM_MSG = 3,
};

enum class ButtonId : u32 {
    INVALID = 0,
    OK = 1,
    YES = 1,
    NO = 2,
    BUTTON1 = 1,
    BUTTON2 = 2,
};

enum class ButtonType : u32 {
    OK = 0,
    YESNO = 1,
    NONE = 2,
    OK_CANCEL = 3,
    WAIT = 5,
    WAIT_CANCEL = 6,
    YESNO_FOCUS_NO = 7,
    OK_CANCEL_FOCUS_CANCEL = 8,
    TWO_BUTTONS = 9,
};

enum class ProgressBarType : u32 {
    PERCENTAGE = 0,
    PERCENTAGE_CANCEL = 1,
};

enum class SystemMessageType : u32 {
    TRC_EMPTY_STORE = 0,
    TRC_PSN_CHAT_RESTRICTION = 1,
    TRC_PSN_UGC_RESTRICTION = 2,
    CAMERA_NOT_CONNECTED = 4,
    WARNING_PROFILE_PICTURE_AND_NAME_NOT_SHARED = 5,
};

struct ButtonsParam {
    const char* msg1{};
    const char* msg2{};
    std::array<char, 32> reserved{};
};

struct UserMessageParam {
    ButtonType buttonType{};
    s32 : 32;
    const char* msg{};
    ButtonsParam* buttonsParam{};
    std::array<char, 24> reserved{};
};

struct ProgressBarParam {
    ProgressBarType barType{};
    s32 : 32;
    const char* msg{};
    std::array<char, 64> reserved{};
};

struct SystemMessageParam {
    SystemMessageType sysMsgType{};
    std::array<char, 32> reserved{};
};

struct Param {
    CommonDialog::BaseParam baseParam;
    std::size_t size;
    MsgDialogMode mode;
    s32 : 32;
    UserMessageParam* userMsgParam;
    ProgressBarParam* progBarParam;
    SystemMessageParam* sysMsgParam;
    OrbisUserServiceUserId userId;
    std::array<char, 40> reserved;
    s32 : 32;
};

struct MsgDialogResult {
    FixedValue<u32, 0> mode{};
    Result result{Result::OK};
    ButtonId buttonId{ButtonId::INVALID};
    std::array<char, 32> reserved{};
};

static auto g_status = Status::NONE;
static MsgDialogResult g_result{};

struct {
    int count = 0;
    const char* text1;
    const char* text2;
} static constexpr button_texts[] = {
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
static_assert(std::size(button_texts) == static_cast<int>(ButtonType::TWO_BUTTONS) + 1);

static void Finish(ButtonId buttonId);

namespace {
using namespace ImGui;
class MsgDialogGui final : public Layer {
    const Param* param{};
    s32 dialog_unique_id{};

    void DrawUser(const UserMessageParam& user_message_param) {
        constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

        const auto ws = GetWindowSize();
        SetCursorPosY(ws.y / 2.0f - 50.0f);
        PushTextWrapPos(ws.x - 30.0f);
        SetCursorPosX(
            (ws.x - CalcTextSize(user_message_param.msg, nullptr, false, ws.x - 40.0f).x) / 2.0f);
        Text("%s", user_message_param.msg);
        PopTextWrapPos();
        const auto button_type = user_message_param.buttonType;
        ASSERT(user_message_param.buttonType <= ButtonType::TWO_BUTTONS);
        auto [count, text1, text2] = button_texts[static_cast<u32>(button_type)];
        if (count == 0xFF) { // TWO_BUTTONS -> User defined message
            count = 2;
            text1 = user_message_param.buttonsParam->msg1;
            text2 = user_message_param.buttonsParam->msg2;
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
                    Finish(ButtonId::BUTTON2);
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

    void DrawProgressBar(const ProgressBarParam& progress_bar_param) {}

    void DrawSystemMessage(const SystemMessageParam& system_message_param) {}

public:
    void Draw() override {
        if (g_status != Status::RUNNING) {
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
                DrawUser(*param->userMsgParam);
                break;
            case MsgDialogMode::PROGRESS_BAR:
                DrawProgressBar(*param->progBarParam);
                break;
            case MsgDialogMode::SYSTEM_MSG:
                DrawSystemMessage(*param->sysMsgParam);
                break;
            }
            End();
        }
    }

    bool ShouldGrabGamepad() override {
        return true;
    }

    explicit MsgDialogGui(const Param* param = nullptr)
        : param(param), dialog_unique_id([] {
              static s32 last_id = 0;
              return last_id++;
          }()) {}
} g_msg_dialog_gui;
} // namespace

static void Finish(ButtonId buttonId) {
    g_result.buttonId = buttonId;
    g_status = Status::FINISHED;
    Layer::RemoveLayer(&g_msg_dialog_gui);
}

Error PS4_SYSV_ABI sceMsgDialogClose() {
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    Finish(ButtonId::INVALID);
    return Error::OK;
}

Error PS4_SYSV_ABI sceMsgDialogGetResult(MsgDialogResult* result) {
    if (g_status != Status::FINISHED) {
        return Error::NOT_FINISHED;
    }
    if (result == nullptr) {
        return Error::ARG_NULL;
    }
    for (const auto v : result->reserved) {
        if (v != 0) {
            return Error::PARAM_INVALID;
        }
    }
    *result = g_result;
    return Error::OK;
}

Status PS4_SYSV_ABI sceMsgDialogGetStatus() {
    return g_status;
}

Error PS4_SYSV_ABI sceMsgDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        return Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Error::BUSY;
    }
    g_status = Status::INITIALIZED;
    CommonDialog::g_isUsed = true;

    return Error::OK;
}

Error PS4_SYSV_ABI sceMsgDialogOpen(const Param* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        return Error::INVALID_STATE;
    }
    ASSERT(param->size == sizeof(Param));
    ASSERT(param->baseParam.size == sizeof(CommonDialog::BaseParam));
    g_status = Status::RUNNING;
    g_result = {};
    g_msg_dialog_gui = MsgDialogGui(param);
    Layer::AddLayer(&g_msg_dialog_gui);
    return Error::OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarInc() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarSetValue() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceMsgDialogTerminate() {
    if (g_status == Status::RUNNING) {
        sceMsgDialogClose();
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_status = Status::NONE;
    CommonDialog::g_isUsed = false;
    return Error::OK;
}

Status PS4_SYSV_ABI sceMsgDialogUpdateStatus() {
    return g_status;
}

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("HTrcDKlFKuM", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1, sceMsgDialogClose);
    LIB_FUNCTION("Lr8ovHH9l6A", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogGetResult);
    LIB_FUNCTION("CWVW78Qc3fI", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogGetStatus);
    LIB_FUNCTION("lDqxaY1UbEo", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogInitialize);
    LIB_FUNCTION("b06Hh0DPEaE", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1, sceMsgDialogOpen);
    LIB_FUNCTION("Gc5k1qcK4fs", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarInc);
    LIB_FUNCTION("6H-71OdrpXM", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarSetMsg);
    LIB_FUNCTION("wTpfglkmv34", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarSetValue);
    LIB_FUNCTION("ePw-kqZmelo", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogTerminate);
    LIB_FUNCTION("6fIC3XKt2k0", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogUpdateStatus);
};

} // namespace Libraries::MsgDialog
