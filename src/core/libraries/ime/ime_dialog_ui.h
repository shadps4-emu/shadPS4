// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring> // for strncpy / memcpy
#include <mutex>
#include <vector>
#include <imgui.h>
#include "common/cstring.h"
#include "common/types.h"
#include "core/libraries/ime/ime_dialog.h"
#include "ime_keyboard_ui.h"
#include "imgui/imgui_layer.h"

namespace Libraries::ImeDialog {

// Forward declaration so we can befriend it
class ImeDialogUi;

//---------------------------------------------------------------------
//  ImeDialogState — holds the text and options for the IME dialog
//---------------------------------------------------------------------
class ImeDialogState final {
    friend class ImeDialogUi; // full access for the dialog‑UI layer

    /*────────────────────────── private data ─────────────────────────*/
    bool input_changed = false;

    s32 user_id{};
    bool is_multi_line{};
    bool is_numeric{};
    OrbisImeType type{};
    OrbisImeEnterLabel enter_label{};
    OrbisImeTextFilter text_filter{};
    OrbisImeExtKeyboardFilter keyboard_filter{};
    u32 max_text_length{};
    char16_t* text_buffer{};
    std::vector<char> title;
    std::vector<char> placeholder;

    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4> current_text;

    // Optional custom keyboard style (from extended params)
    bool has_custom_kb_style = false;
    KeyboardStyle custom_kb_style{};

public:
    /*──────────────── constructors / rule‑of‑five ────────────────*/
    ImeDialogState(const OrbisImeDialogParam* param = nullptr,
                   const OrbisImeParamExtended* extended = nullptr);
    ImeDialogState(const ImeDialogState& other) = delete;
    ImeDialogState(ImeDialogState&& other) noexcept;
    ImeDialogState& operator=(ImeDialogState&& other);

    bool CopyTextToOrbisBuffer();
    bool CallTextFilter();
    /*──────────────────── public read helpers ───────────────────*/
    bool IsMultiLine() const {
        return is_multi_line;
    }
    bool IsNumeric() const {
        return is_numeric;
    }
    u32 MaxTextLength() const {
        return max_text_length;
    }

    const char* TitleUtf8() const {
        return title.empty() ? nullptr : title.data();
    }
    const char* PlaceholderUtf8() const {
        return placeholder.empty() ? nullptr : placeholder.data();
    }
    const char* CurrentTextUtf8() const {
        return current_text.begin();
    }

    /*─────────────────── public write helpers ───────────────────*/
    // Replace the whole text buffer
    void SetTextUtf8(const char* utf8) {
        if (!utf8)
            return;
        std::strncpy(current_text.begin(), utf8, current_text.capacity() - 1);
        current_text[current_text.capacity() - 1] = '\0';
        input_changed = true;
    }

    // Append raw UTF‑8 sequence of length 'len'
    void AppendUtf8(const char* utf8, std::size_t len) {
        if (!utf8 || len == 0)
            return;
        std::size_t old = std::strlen(current_text.begin());
        if (old + len >= current_text.capacity())
            return; // full: silently ignore
        std::memcpy(current_text.begin() + old, utf8, len);
        current_text[old + len] = '\0';
        input_changed = true;
    }

    // Remove one UTF‑8 code‑point from the end (safe backspace)
    void BackspaceUtf8() {
        Utf8SafeBackspace(current_text.begin());
        input_changed = true;
    }



private:
    bool CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode, u32* out_status);

    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t native_text_len);
    bool ConvertUTF8ToOrbis(const char* native_text, std::size_t utf8_text_len,
                            char16_t* orbis_text, std::size_t orbis_text_len);
};

//---------------------------------------------------------------------
//  ImeDialogUi — draws the IME dialog & on‑screen keyboard
//---------------------------------------------------------------------
class ImeDialogUi final : public ImGui::Layer {
    /*────────── private data ─────────*/
    ImeDialogState* state{};
    OrbisImeDialogStatus* status{};
    OrbisImeDialogResult* result{};

    bool first_render = true;
    std::mutex draw_mutex;

public:
    // Global pointer to the active dialog‑UI (used by the callback bridge)
    static ImeDialogUi* g_activeImeDialogUi;

    /*───────── ctors / dtor ─────────*/
    explicit ImeDialogUi(ImeDialogState* state = nullptr, OrbisImeDialogStatus* status = nullptr,
                         OrbisImeDialogResult* result = nullptr);
    ~ImeDialogUi() override;
    ImeDialogUi(const ImeDialogUi& other) = delete;
    ImeDialogUi(ImeDialogUi&& other) noexcept;
    ImeDialogUi& operator=(ImeDialogUi&& other);

    /*────────── main draw ───────────*/
    void Draw() override;

    /*────────── keyboard events ─────*/
    void OnVirtualKeyEvent(const VirtualKeyEvent* evt);

private:
    /*── helpers ─*/
    void Free();

    void DrawInputText();
    void DrawMultiLineInputText();

    static int InputTextCallback(ImGuiInputTextCallbackData* data);

    /*── keyboard section ─*/
    KeyboardMode kb_mode = KeyboardMode::Letters;
    ShiftState shift_state = ShiftState::None;
    u64 kb_language = 0;
    KeyboardStyle kb_style;
    /* KeyboardStyle kb_style{
        .layout_width  = 500.0f,
        .layout_height = 250.0f,
        .key_spacing   = 5.0f,
        .color_text             = IM_COL32(225,225,225,255),
        .color_line             = IM_COL32( 88, 88, 88,255),
        .color_button_default   = IM_COL32( 35, 35, 35,255),
        .color_button_function  = IM_COL32( 50, 50, 50,255),
        .color_special          = IM_COL32(  0,140,200,255),
        .use_button_symbol_color= false,
        .color_button_symbol    = IM_COL32( 60, 60, 60,255),
    };*/

    void DrawVirtualKeyboardSection();
    void DrawPredictionBarAnCancelButton();
};

} // namespace Libraries::ImeDialog
