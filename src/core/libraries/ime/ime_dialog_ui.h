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
#include "core/libraries/ime/ime_keyboard_ui.h"
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
    const OrbisImeParamExtended* extended_param_ = nullptr;
    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4> current_text;

    // Optional custom keyboard style (from extended params)
    bool has_custom_style = false;
    KeyboardStyle custom_kb_style{};
    int caret_index = 0;

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

    const OrbisImeParamExtended* GetExtendedParam() const {
        return extended_param_;
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

    void InsertUtf8AtCaret(const char* utf8, std::size_t len) {
        if (!utf8 || len == 0)
            return;

        std::size_t old_len = std::strlen(current_text.begin());
        if (old_len + len >= current_text.capacity())
            return; // full, silently ignore

        // Move the text after caret forward
        char* text_begin = current_text.begin();
        std::memmove(text_begin + caret_index + len, text_begin + caret_index,
                     old_len - caret_index + 1); // +1 for null-terminator

        // Copy the inserted text at caret position
        std::memcpy(text_begin + caret_index, utf8, len);

        caret_index += (int)len; // Move caret after inserted text
        input_changed = true;
    }

    // Remove one UTF‑8 code‑point from the end (safe backspace)
    void BackspaceUtf8() {
        Utf8SafeBackspace(current_text.begin());
        input_changed = true;
    }

    void BackspaceUtf8AtCaret() {
        char* buf = current_text.begin();
        size_t len = std::strlen(buf);

        if (caret_index == 0 || len == 0)
            return;

        // Find byte index just before caret (start of previous codepoint)
        int remove_start = caret_index - 1;
        while (remove_start > 0 &&
               (static_cast<unsigned char>(buf[remove_start]) & 0b11000000) == 0b10000000)
            --remove_start;

        int remove_len = caret_index - remove_start;

        // Shift everything after caret to the left
        std::memmove(buf + remove_start, buf + caret_index,
                     len - caret_index + 1); // +1 to move null terminator
        caret_index = remove_start;

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

    OrbisImeParamExtended ext_;

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

    void DrawTitle();

    void DrawOkAndCancelButtons();

    /*── keyboard section ─*/
    KeyboardMode kb_mode = KeyboardMode::Letters;
    ShiftState shift_state = ShiftState::None;
    u64 kb_language = 0;
    KeyboardStyle kb_style;

    void DrawVirtualKeyboardSection();
    void DrawPredictionBarAnCancelButton();
};

} // namespace Libraries::ImeDialog
