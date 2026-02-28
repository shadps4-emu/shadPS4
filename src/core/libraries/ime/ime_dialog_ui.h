// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include <imgui.h>
#include "common/cstring.h"
#include "common/types.h"
#include "core/libraries/ime/ime_dialog.h"
#include "imgui/imgui_layer.h"

namespace Libraries::ImeDialog {

class ImeDialogUi;
} // namespace Libraries::ImeDialog

namespace Libraries::Ime {
struct ImePanelMetrics;
}

namespace Libraries::ImeDialog {

class ImeDialogState final {
    friend ImeDialogUi;

    bool input_changed = false;
    int caret_index = 0;
    int caret_byte_index = 0;
    bool caret_dirty = false;
    bool use_over2k = false;
    OrbisImePositionAndForm panel_layout{};
    bool panel_layout_valid = false;
    u32 panel_req_width = 0;
    u32 panel_req_height = 0;

    s32 user_id{};
    bool is_multi_line{};
    bool is_numeric{};
    OrbisImeType type{};
    OrbisImeEnterLabel enter_label{};
    OrbisImeTextFilter text_filter{};
    OrbisImeExtKeyboardFilter keyboard_filter{};
    u32 max_text_length{};
    char16_t* text_buffer{};
    std::vector<char16_t> original_text;
    std::vector<char> title;
    std::vector<char> placeholder;

    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4 + 1> current_text;

public:
    /*
     * Use default constructor ImeDialogState() to initialize default values instead of
     * ImeDialogState(const OrbisImeDialogParam* param = nullptr,const OrbisImeParamExtended*
     * extended = nullptr) to avoid validation errors in log
     */
    ImeDialogState();
    ImeDialogState(const OrbisImeDialogParam* param /*= nullptr*/,
                   const OrbisImeParamExtended* extended /*= nullptr*/);
    ImeDialogState(const ImeDialogState& other) = delete;
    ImeDialogState(ImeDialogState&& other) noexcept;
    ImeDialogState& operator=(ImeDialogState&& other);

    bool CopyTextToOrbisBuffer(bool use_original);
    bool CallTextFilter();
    bool NormalizeNewlines();
    bool ClampCurrentTextToMaxLen();

private:
    bool CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode, u32* out_status);

    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t native_text_len);
    bool ConvertUTF8ToOrbis(const char* native_text, std::size_t utf8_text_len,
                            char16_t* orbis_text, std::size_t orbis_text_len);
};

class ImeDialogUi final : public ImGui::Layer {
    ImeDialogState* state{};
    OrbisImeDialogStatus* status{};
    OrbisImeDialogResult* result{};

    bool first_render = true;
    bool accept_armed = false;
    std::mutex draw_mutex;

public:
    explicit ImeDialogUi(ImeDialogState* state = nullptr, OrbisImeDialogStatus* status = nullptr,
                         OrbisImeDialogResult* result = nullptr);
    ~ImeDialogUi() override;
    ImeDialogUi(const ImeDialogUi& other) = delete;
    ImeDialogUi(ImeDialogUi&& other) noexcept;
    ImeDialogUi& operator=(ImeDialogUi&& other);

    void Draw() override;

private:
    void FinishDialog(OrbisImeDialogEndStatus endstatus, bool restore_original, const char* reason);
    void Free();

    void DrawInputText(const Libraries::Ime::ImePanelMetrics& metrics);
    void DrawMultiLineInputText(const Libraries::Ime::ImePanelMetrics& metrics);

    static int InputTextCallback(ImGuiInputTextCallbackData* data);
};

} // namespace Libraries::ImeDialog
