// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::MsgDialog {

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

enum class OrbisMsgDialogProgressBarTarget : u32 {
    DEFAULT = 0,
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
    CommonDialog::Result result{CommonDialog::Result::OK};
    ButtonId buttonId{ButtonId::INVALID};
    std::array<char, 32> reserved{};
};

class MsgDialogUi final : public ImGui::Layer {
    s32 dialog_unique_id;
    const Param* param{};
    CommonDialog::Status* status{};
    MsgDialogResult* result{};
    u32 progress_bar_value{};

    void DrawUser();
    void DrawProgressBar();
    void DrawSystemMessage();

public:
    explicit MsgDialogUi(const Param* param = nullptr, CommonDialog::Status* status = nullptr,
                         MsgDialogResult* result = nullptr);
    ~MsgDialogUi() override;
    MsgDialogUi(const MsgDialogUi& other) = delete;
    MsgDialogUi(MsgDialogUi&& other) noexcept;
    MsgDialogUi& operator=(MsgDialogUi other);

    void Finish(ButtonId buttonId, CommonDialog::Result r = CommonDialog::Result::OK);

    void SetProgressBarValue(u32 value, bool increment);

    void Draw() override;

    bool ShouldGrabGamepad() override {
        return true;
    }
};

CommonDialog::Error PS4_SYSV_ABI sceMsgDialogClose();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogGetResult(MsgDialogResult* result);
CommonDialog::Status PS4_SYSV_ABI sceMsgDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogOpen(const Param* param);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarInc(OrbisMsgDialogProgressBarTarget,
                                                            u32 delta);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg(OrbisMsgDialogProgressBarTarget,
                                                               const char* msg);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarSetValue(OrbisMsgDialogProgressBarTarget,
                                                                 u32 value);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI sceMsgDialogUpdateStatus();

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::MsgDialog
