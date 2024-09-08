// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

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

struct OrbisParam {
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

struct DialogResult {
    FixedValue<u32, 0> mode{};
    CommonDialog::Result result{CommonDialog::Result::OK};
    ButtonId buttonId{ButtonId::INVALID};
    std::array<char, 32> reserved{};
};

// State is used to copy all the data from the param struct
class MsgDialogState {
public:
    struct UserState {
        ButtonType type{};
        std::string msg{};
        std::string btn_param1{};
        std::string btn_param2{};
    };
    struct ProgressState {
        ProgressBarType type{};
        std::string msg{};
        u32 progress{};
    };
    struct SystemState {
        SystemMessageType type{};
    };

private:
    OrbisUserServiceUserId user_id{};
    MsgDialogMode mode{};
    std::variant<UserState, ProgressState, SystemState, std::monostate> state{std::monostate{}};

public:
    explicit MsgDialogState(const OrbisParam& param);
    MsgDialogState() = default;

    [[nodiscard]] OrbisUserServiceUserId GetUserId() const {
        return user_id;
    }

    [[nodiscard]] MsgDialogMode GetMode() const {
        return mode;
    }

    template <typename T>
    [[nodiscard]] T& GetState() {
        return std::get<T>(state);
    }
};

class MsgDialogUi final : public ImGui::Layer {
    bool first_render{false};
    MsgDialogState* state{};
    CommonDialog::Status* status{};
    DialogResult* result{};

    void DrawUser();
    void DrawProgressBar();
    void DrawSystemMessage();

public:
    explicit MsgDialogUi(MsgDialogState* state = nullptr, CommonDialog::Status* status = nullptr,
                         DialogResult* result = nullptr);
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

}; // namespace Libraries::MsgDialog