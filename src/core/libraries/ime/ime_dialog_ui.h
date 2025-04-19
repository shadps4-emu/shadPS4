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

#include <unordered_set>

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

    // One UTF‑8 code‑point may take up to 4 bytes
    Common::CString<ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4> current_text;

    // Optional custom keyboard style (from extended params)
    bool has_custom_kb_style = false;
    KeyboardStyle custom_kb_style{};

public:
    /*──────────────── constructors / rule‑of‑five ────────────────*/
    ImeDialogState(const OrbisImeDialogParam* param = nullptr,
                   const OrbisImeParamExtended* ext = nullptr);
    ImeDialogState(const ImeDialogState&) = delete;
    ImeDialogState(ImeDialogState&&) noexcept;
    ImeDialogState& operator=(ImeDialogState&&);

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

    /*──────────────────────── IME support ───────────────────────*/
    bool CopyTextToOrbisBuffer();
    bool CallTextFilter();

private:
    bool CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode, u32* out_status);

    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t utf8_text_len);
    bool ConvertUTF8ToOrbis(const char* utf8_text, std::size_t utf8_text_len, char16_t* orbis_text,
                            std::size_t orbis_text_len);
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
    ImeDialogUi(const ImeDialogUi&) = delete;
    ImeDialogUi(ImeDialogUi&&) noexcept;
    ImeDialogUi& operator=(ImeDialogUi&&);

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

    void DrawVirtualKeyboardSection();
    void DrawPredictionBarAnCancelButton();
};

} // namespace Libraries::ImeDialog
