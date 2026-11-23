// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_ui_shared.h"

#include <cmath>
#include <cstring>
#include <numbers>
#include <string>
#include <magic_enum/magic_enum_flags.hpp>

#include "common/scope_exit.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/libraries/ime/ime_kb_language.h"
#include "imgui/notifications_layer.h"
#include "input/controller.h"

namespace Libraries::Ime {

namespace {

enum class StickNavDirection : int {
    None = 0,
    Left = 1,
    Right = 2,
    Up = 3,
    Down = 4,
};

constexpr float kNavAxisThreshold = 0.50f;
constexpr double kStickNavInitialDelaySlow = 0.26;
constexpr double kStickNavInitialDelayFast = 0.16;
constexpr double kStickNavRepeatSlow = 0.20;
constexpr double kStickNavRepeatFast = 0.11;
constexpr double kCaretBlinkCycleSec = 1.20;
constexpr double kCaretBlinkOnSec = 0.80;

struct VirtualButtonRepeatResult {
    bool pressed = false;
    bool repeat = false;
};

struct StickNavPulseState {
    int repeat_dir = 0;
    double next_repeat_time = 0.0;
};

struct StickNavPulseResult {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool left_repeat = false;
    bool right_repeat = false;
    bool up_repeat = false;
    bool down_repeat = false;
};

bool IsCaretBlinkVisible() {
    const ImGuiIO& io = ImGui::GetIO();
    if (!io.ConfigInputTextCursorBlink) {
        return true;
    }
    const double phase = std::fmod(ImGui::GetTime(), kCaretBlinkCycleSec);
    return phase <= kCaretBlinkOnSec;
}

} // namespace

namespace UiShared {

constexpr float kAxisDeadzone = 0.24f;
constexpr float kPanelMoveSpeed = 900.0f; // pixels per second at full tilt

float ToAxisUnit(const s32 axis) {
    const float centered = (static_cast<float>(axis) - 128.0f) / 127.0f;
    return std::clamp(centered, -1.0f, 1.0f);
}

bool ReadControllerState(Libraries::UserService::OrbisUserServiceUserId user_id,
                         Input::State* out_state) {
    if (!out_state) {
        return false;
    }
    auto* controllers = Common::Singleton<Input::GameControllers>::Instance();
    if (!controllers) {
        return false;
    }

    const auto read_state = [&](u8 index, bool* has_state) {
        if (has_state) {
            *has_state = false;
        }
        Input::State pad_state = (*controllers)[index]->ReadState();
        if (!pad_state.connected) {
            return false;
        }
        if (has_state) {
            *has_state = true;
        }
        *out_state = pad_state;
        return true;
    };

    const auto mapped = Input::GameControllers::GetControllerIndexFromUserID(user_id);
    if (mapped.has_value() && *mapped < 5) {
        bool has_state = false;
        if (read_state(*mapped, &has_state) && has_state) {
            return true;
        }
    }

    for (u8 index = 0; index < 5; ++index) {
        if (mapped.has_value() && index == *mapped) {
            continue;
        }
        bool has_state = false;
        if (read_state(index, &has_state) && has_state) {
            return true;
        }
    }
    return false;
}

} // namespace UiShared

ImVec4 BrightenColor(ImU32 color, float delta) {
    ImVec4 out = ImGui::ColorConvertU32ToFloat4(color);
    out.x = std::clamp(out.x + delta, 0.0f, 1.0f);
    out.y = std::clamp(out.y + delta, 0.0f, 1.0f);
    out.z = std::clamp(out.z + delta, 0.0f, 1.0f);
    return out;
}

static ImU32 ScaleColorAlpha(ImU32 color, float alpha_scale) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    c.w = std::clamp(c.w * alpha_scale, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(c);
}

void TriggerSelectorPressPulse(SelectorFadeState& state, double now) {
    state.press_pulse_started_at = now;
}

float ComputePressPulseExpand(double pulse_started_at, double now, float pulse_duration_sec,
                              float max_expand_px) {
    if (pulse_started_at < 0.0 || pulse_duration_sec <= 0.0f || max_expand_px <= 0.0f) {
        return 0.0f;
    }
    const double elapsed = now - pulse_started_at;
    if (elapsed < 0.0 || elapsed >= static_cast<double>(pulse_duration_sec)) {
        return 0.0f;
    }

    const float t = static_cast<float>(elapsed / static_cast<double>(pulse_duration_sec));
    const float envelope = std::sin(t * std::numbers::pi_v<float>);
    return std::max(0.0f, envelope) * max_expand_px;
}

void UpdateSelectorFadeState(SelectorFadeState& state, ImVec2 pos, ImVec2 size, float inset,
                             float corner_radius, bool selected, double now) {
    bool next_visible = false;
    ImVec2 next_min{};
    ImVec2 next_max{};
    if (selected && size.x > inset * 2.0f && size.y > inset * 2.0f) {
        next_visible = true;
        next_min = {pos.x + inset, pos.y + inset};
        next_max = {pos.x + size.x - inset, pos.y + size.y - inset};
    }

    const bool was_visible = state.current_visible;
    if (was_visible && !next_visible) {
        state.previous_min = state.current_min;
        state.previous_max = state.current_max;
        state.previous_corner_radius = state.current_corner_radius;
        state.previous_visible = true;
        state.previous_started_at = now;
    }

    state.current_visible = next_visible;
    if (!next_visible) {
        return;
    }
    state.current_min = next_min;
    state.current_max = next_max;
    state.current_corner_radius = std::max(0.0f, corner_radius);
}

void DrawSelectorFadeState(const SelectorFadeState& state, ImDrawList* draw_list,
                           ImU32 overlay_color, ImU32 border_color, float border_thickness,
                           float fade_duration_sec, double now, float current_expand_px) {
    if (!draw_list) {
        return;
    }

    if (state.previous_visible && fade_duration_sec > 0.0f) {
        const double elapsed = now - state.previous_started_at;
        if (elapsed >= 0.0 && elapsed < static_cast<double>(fade_duration_sec)) {
            const float alpha =
                1.0f - static_cast<float>(elapsed / static_cast<double>(fade_duration_sec));
            if (alpha > 0.0f) {
                draw_list->AddRectFilled(state.previous_min, state.previous_max,
                                         ScaleColorAlpha(overlay_color, alpha),
                                         state.previous_corner_radius);
                draw_list->AddRect(state.previous_min, state.previous_max,
                                   ScaleColorAlpha(border_color, alpha),
                                   state.previous_corner_radius, 0, border_thickness);
            }
        }
    }

    if (!state.current_visible) {
        return;
    }
    const float expand = std::max(0.0f, current_expand_px);
    const ImVec2 current_min{state.current_min.x - expand, state.current_min.y - expand};
    const ImVec2 current_max{state.current_max.x + expand, state.current_max.y + expand};
    const float corner_radius = state.current_corner_radius + expand;
    draw_list->AddRectFilled(current_min, current_max, overlay_color, corner_radius);
    draw_list->AddRect(current_min, current_max, border_color, corner_radius, 0, border_thickness);
}

ImeKbLayoutSelection ResolveInitialKbLayoutSelection(OrbisImeExtOption ext_option,
                                                     OrbisImePanelPriority panel_priority,
                                                     OrbisImeLanguage supported_languages,
                                                     OrbisImeType type) {
    ImeKbLayoutSelection selection{};
    selection.language =
        ResolveImeKeyboardLanguage(supported_languages, EmulatorSettings.GetConsoleLanguage());
    if (type == OrbisImeType::Number) {
        selection.family = ImeKbLayoutFamily::Numeric;
        return selection;
    }
    const bool set_priority = True(ext_option & OrbisImeExtOption::SET_PRIORITY);
    if (set_priority) {
        switch (panel_priority) {
        case OrbisImePanelPriority::Symbol:
            selection.family = ImeKbLayoutFamily::Symbols;
            break;
        case OrbisImePanelPriority::Accent:
            selection.family = ImeKbLayoutFamily::Specials;
            break;
        case OrbisImePanelPriority::Alphabet:
        case OrbisImePanelPriority::Default:
        default:
            selection.family = ImeKbLayoutFamily::Latin;
            break;
        }
    }

    const bool shift_lock = set_priority && True(ext_option & OrbisImeExtOption::PRIORITY_SHIFT);
    if (shift_lock && selection.family != ImeKbLayoutFamily::Symbols) {
        // SDK docs: PRIORITY_SHIFT starts the initial panel in Shift-lock state.
        selection.case_state = ImeKbCaseState::CapsLock;
    }
    return selection;
}

void NotifyImeKeyboardLayoutFallback(OrbisImeLanguage supported_languages, OrbisImeType type) {
    if (type == OrbisImeType::Number) {
        return;
    }
    const bool console_default = static_cast<u64>(supported_languages) == 0;
    const int console_language = EmulatorSettings.GetConsoleLanguage();
    const bool implemented = std::ranges::any_of(
        GetImeKeyboardLanguageProfiles(), [&](const ImeKbLanguageProfile& profile) {
            return console_default ? profile.console_language == console_language
                                   : True(supported_languages & profile.language);
        });
    if (implemented) {
        return;
    }
    const std::string requested =
        console_default ? "console language" : magic_enum::enum_flags_name(supported_languages);
    shadNotifications::QueueNotification(
        "IME keyboard layout not implemented: " + requested + ".\nUsing English (US).", 6.0f,
        shadNotifications::position::BottomRight, shadNotifications::stockIcons::Input);
}

void InitializeDefaultOskSelectionAnchor(const ImeKbLayoutSelection& layout_selection,
                                         OrbisImeExtOption ext_option, int& pending_row,
                                         int& pending_col, int& last_row, int& last_col) {
    if (layout_selection.family == ImeKbLayoutFamily::Numeric) {
        pending_row = last_row = 0;
        pending_col = last_col = 0;
        return;
    }
    constexpr int kLatinLowerDefaultRow = 2;
    constexpr int kLatinLowerDefaultCol = 4;

    const bool is_default_latin_lower = layout_selection.family == ImeKbLayoutFamily::Latin &&
                                        layout_selection.case_state == ImeKbCaseState::Lower;
    const bool game_requested_other =
        True(ext_option & OrbisImeExtOption::SET_PRIORITY) && !is_default_latin_lower;
    if (!is_default_latin_lower || game_requested_other) {
        return;
    }

    const auto& layout = GetImeKeyboardLayout(layout_selection);
    const int typing_rows =
        ImeSelectionGridIndex::ResolveTypingRows(layout.rows, layout.function_rows);
    pending_row = std::min(kLatinLowerDefaultRow, typing_rows - 1);
    pending_col = std::min(kLatinLowerDefaultCol, std::max(0, static_cast<int>(layout.cols) - 1));
    last_row = kLatinLowerDefaultRow;
    last_col = kLatinLowerDefaultCol;
}

void CycleKeyboardCaseState(ImeKbLayoutSelection& selection) {
    switch (selection.case_state) {
    case ImeKbCaseState::Lower:
        selection.case_state = ImeKbCaseState::Upper;
        break;
    case ImeKbCaseState::Upper:
        selection.case_state = ImeKbCaseState::CapsLock;
        break;
    case ImeKbCaseState::CapsLock:
    default:
        selection.case_state = ImeKbCaseState::Lower;
        break;
    }
}

void ToggleKeyboardFamilyMode(ImeKbLayoutSelection& selection, ImeKbLayoutFamily& alpha_family,
                              ImeKbLayoutFamily target_family) {
    const auto set_family_and_reset_page = [&](const ImeKbLayoutFamily family) {
        selection.family = family;
        selection.page = 0;
        if (family == ImeKbLayoutFamily::Latin || family == ImeKbLayoutFamily::Specials) {
            alpha_family = family;
        }
    };

    if (target_family == ImeKbLayoutFamily::Symbols) {
        if (selection.family == ImeKbLayoutFamily::Symbols) {
            set_family_and_reset_page(alpha_family);
        } else {
            if (selection.family == ImeKbLayoutFamily::Latin ||
                selection.family == ImeKbLayoutFamily::Specials) {
                alpha_family = selection.family;
            }
            set_family_and_reset_page(ImeKbLayoutFamily::Symbols);
        }
        return;
    }

    if (selection.family == target_family) {
        set_family_and_reset_page(ImeKbLayoutFamily::Latin);
    } else {
        set_family_and_reset_page(target_family);
    }
}

bool FocusKeyboardActionKeySelection(const ImeKbLayoutSelection& selection, ImeKbKeyAction action,
                                     int& out_row, int& out_col) {
    const auto& layout = GetImeKeyboardLayout(selection);
    if (!layout.keys || layout.key_count == 0) {
        return false;
    }

    for (std::size_t i = 0; i < layout.key_count; ++i) {
        const auto& key = layout.keys[i];
        if (key.action != action) {
            continue;
        }
        out_row = static_cast<int>(key.row);
        out_col = static_cast<int>(key.col);
        return true;
    }
    return false;
}

void FlipKeyboardModePage(ImeKbLayoutSelection& selection, int direction) {
    if (selection.family != ImeKbLayoutFamily::Symbols &&
        selection.family != ImeKbLayoutFamily::Specials) {
        return;
    }
    const int page = static_cast<int>(selection.page);
    const auto& profile = GetImeKeyboardLanguageProfile(selection.language);
    const int page_count =
        std::max(1, static_cast<int>(selection.family == ImeKbLayoutFamily::Symbols
                                         ? profile.symbol_pages
                                         : profile.special_pages));
    selection.page = static_cast<u8>(((page + direction) % page_count + page_count) % page_count);
}

int Utf16CountFromUtf8Range(const char* text, const char* end) {
    if (!text) {
        return 0;
    }
    const char* const range_end = end ? end : (text + std::strlen(text));
    const char* p = text;
    int utf16_units = 0;
    while (p < range_end && *p) {
        unsigned int c = 0;
        const int step = ImTextCharFromUtf8(&c, p, range_end);
        if (step <= 0) {
            break;
        }
        utf16_units += (c > 0xFFFF) ? 2 : 1;
        p += step;
    }
    return utf16_units;
}

int Utf8ByteIndexFromUtf16Index(const char* text, int utf16_index) {
    if (!text || utf16_index <= 0) {
        return 0;
    }
    const char* p = text;
    int count = 0;
    while (*p && count < utf16_index) {
        unsigned int c = 0;
        const int step = ImTextCharFromUtf8(&c, p, nullptr);
        if (step <= 0) {
            break;
        }
        const int utf16_units = (c > 0xFFFF) ? 2 : 1;
        if (count + utf16_units > utf16_index) {
            break;
        }
        count += utf16_units;
        p += step;
    }
    return static_cast<int>(p - text);
}

int MoveImeCaretByLine(const char* text, int caret_utf16, int direction, int& preferred_column) {
    if (!text) {
        return 0;
    }
    const int length = static_cast<int>(std::strlen(text));
    const int caret = Utf8ByteIndexFromUtf16Index(text, caret_utf16);
    int line_start = caret;
    while (line_start > 0 && text[line_start - 1] != '\n') {
        --line_start;
    }
    if (preferred_column < 0) {
        preferred_column = Utf16CountFromUtf8Range(text + line_start, text + caret);
    }
    int target_start = line_start;
    if (direction < 0) {
        if (line_start == 0) {
            return 0;
        }
        target_start = line_start - 1;
        while (target_start > 0 && text[target_start - 1] != '\n') {
            --target_start;
        }
    } else {
        while (target_start < length && text[target_start] != '\n') {
            ++target_start;
        }
        if (target_start == length) {
            return Utf16CountFromUtf8Range(text, text + length);
        }
        ++target_start;
    }
    int target_end = target_start;
    while (target_end < length && text[target_end] != '\n') {
        ++target_end;
    }
    const int column =
        std::min(preferred_column, Utf16CountFromUtf8Range(text + target_start, text + target_end));
    return Utf16CountFromUtf8Range(text, text + target_start) + column;
}

bool IsImeCharacterAllowed(unsigned int character, OrbisImeType type) {
    if (character > 0xffff || (character >= 0xd800 && character <= 0xdfff)) {
        return false;
    }
    if (type == OrbisImeType::Number) {
        return (character >= '0' && character <= '9') || character == '.' || character == ',' ||
               character == '-';
    }
    if (type == OrbisImeType::BasicLatin || type == OrbisImeType::Mail) {
        return character >= 0x20 && character < 0x7f;
    }
    return true;
}

bool DrawImeInputText(const char* label, const char* hint, char* buffer, int capacity,
                      const ImVec2& size, ImGuiInputTextFlags flags,
                      ImGuiInputTextCallback callback, void* userdata, bool password,
                      bool disable_clipboard) {
    auto& platform = ImGui::GetPlatformIO();
    const auto get_clipboard = platform.Platform_GetClipboardTextFn;
    const auto set_clipboard = platform.Platform_SetClipboardTextFn;
    if (disable_clipboard && ImGui::GetActiveID() == ImGui::GetID(label)) {
        const auto owner = ImHashStr("shadps4/ime/clipboard-disabled");
        if (ImGui::GetIO().KeyCtrl) {
            for (auto key : {ImGuiKey_C, ImGuiKey_X, ImGuiKey_V, ImGuiKey_Insert}) {
                ImGui::SetKeyOwner(key, owner, ImGuiInputFlags_LockThisFrame);
            }
        }
        if (ImGui::GetIO().KeyShift) {
            ImGui::SetKeyOwner(ImGuiKey_Insert, owner, ImGuiInputFlags_LockThisFrame);
            ImGui::SetKeyOwner(ImGuiKey_Delete, owner, ImGuiInputFlags_LockThisFrame);
        }
    }
    if (disable_clipboard) {
        platform.Platform_GetClipboardTextFn = [](ImGuiContext*) { return ""; };
        platform.Platform_SetClipboardTextFn = [](ImGuiContext*, const char*) {};
    }
    SCOPE_EXIT {
        platform.Platform_GetClipboardTextFn = get_clipboard;
        platform.Platform_SetClipboardTextFn = set_clipboard;
    };
    if (password) {
        flags |= ImGuiInputTextFlags_Password;
    }
    return ImGui::InputTextEx(label, hint, buffer, capacity, size, flags, callback, userdata);
}

std::string BoundImeInsertion(const char* text, int max_utf16, bool multiline, bool numeric,
                              OrbisImeType type) {
    std::string result;
    if (!text) {
        return result;
    }
    for (const char* p = text; *p && max_utf16 > 0;) {
        unsigned int codepoint = 0;
        const int bytes = ImTextCharFromUtf8(&codepoint, p, nullptr);
        if (bytes <= 0) {
            break;
        }
        const char* start = p;
        p += bytes;
        if (numeric && (codepoint < '0' || codepoint > '9') && codepoint != '.' &&
            codepoint != ',' && codepoint != '-') {
            continue;
        }
        if (codepoint == '\r' || codepoint == '\n') {
            if (codepoint == '\r' && *p == '\n') {
                ++p;
            }
            if (multiline) {
                result.push_back('\n');
                --max_utf16;
            }
            continue;
        }
        if (codepoint < ' ' || codepoint == 0x7f || !IsImeCharacterAllowed(codepoint, type)) {
            continue;
        }
        const int units = codepoint > 0xffff ? 2 : 1;
        if (units > max_utf16) {
            break;
        }
        result.append(start, static_cast<size_t>(bytes));
        max_utf16 -= units;
    }
    return result;
}

bool FilterImeText(OrbisImeTextFilter filter, std::u16string& text, u32 max_length, u32& caret) {
    if (!filter) {
        return false;
    }
    std::array<char16_t, ORBIS_IME_MAX_TEXT_LENGTH + 1> output{};
    u32 length = std::min(max_length, ORBIS_IME_MAX_TEXT_LENGTH);
    if (filter(output.data(), &length, text.data(), static_cast<u32>(text.size())) != 0 ||
        length > max_length || length > ORBIS_IME_MAX_TEXT_LENGTH) {
        return false;
    }
    u32 count = 0;
    while (count < length && output[count]) {
        if ((static_cast<u16>(output[count]) & 0xf800U) == 0xd800U) {
            return false;
        }
        ++count;
    }
    std::u16string filtered(output.data(), count);
    if (filtered == text) {
        return false;
    }
    caret = std::min(caret, static_cast<u32>(text.size()));
    const bool at_end = caret == text.size();
    size_t prefix = 0;
    while (prefix < text.size() && prefix < filtered.size() && text[prefix] == filtered[prefix]) {
        ++prefix;
    }
    size_t suffix = 0;
    while (suffix < text.size() - prefix && suffix < filtered.size() - prefix &&
           text[text.size() - suffix - 1] == filtered[filtered.size() - suffix - 1]) {
        ++suffix;
    }
    if (at_end) {
        caret = count;
    } else if (caret > prefix) {
        caret =
            static_cast<u32>(caret >= text.size() - suffix
                                 ? filtered.size() - std::min<size_t>(text.size() - caret, suffix)
                                 : filtered.size() - suffix);
    }
    caret = std::min(caret, count);
    text = std::move(filtered);
    return true;
}

void InsertImeInputNewline(ImGuiInputTextCallbackData* data, int max_utf16) {
    const bool selected_text = data->SelectionStart != data->SelectionEnd;
    const int start =
        selected_text ? std::min(data->SelectionStart, data->SelectionEnd) : data->CursorPos;
    const int end = selected_text ? std::max(data->SelectionStart, data->SelectionEnd) : start;
    const int length = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->BufTextLen);
    const int selected = Utf16CountFromUtf8Range(data->Buf + start, data->Buf + end);
    if (length - selected >= max_utf16) {
        return;
    }
    if (end > start) {
        data->DeleteChars(start, end - start);
    }
    data->CursorPos = start;
    data->SelectionStart = data->SelectionEnd = start;
    data->InsertChars(start, "\n");
}

bool RejectInputCharByUtf16Limit(const ImGuiInputTextCallbackData* data, int max_utf16) {
    if (!data || max_utf16 < 0 || data->EventChar == 0) {
        return false;
    }
    ImGuiContext* g = data->Ctx;
    if (!g) {
        return false;
    }
    ImGuiInputTextState* st = &g->InputTextState;
    if (!st || !st->TextSrc) {
        return false;
    }

    const int cur_units = Utf16CountFromUtf8Range(st->TextSrc, st->TextSrc + st->TextLen);
    int selected_units = 0;
    if (st->HasSelection()) {
        const int sel_begin = st->GetSelectionStart();
        const int sel_end = st->GetSelectionEnd();
        selected_units = Utf16CountFromUtf8Range(st->TextSrc + sel_begin, st->TextSrc + sel_end);
    }

    const int incoming_units = data->EventChar > 0xFFFF ? 2 : 1;
    const int remaining_units = max_utf16 - (cur_units - selected_units);
    return remaining_units < incoming_units;
}

bool ClampInputBufferToUtf16Limit(ImGuiInputTextCallbackData* data, int max_utf16) {
    if (!data || max_utf16 < 0) {
        return false;
    }
    const int utf16_len = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->BufTextLen);
    if (utf16_len <= max_utf16) {
        return false;
    }

    const int keep_bytes = Utf8ByteIndexFromUtf16Index(data->Buf, max_utf16);
    if (keep_bytes < data->BufTextLen) {
        data->DeleteChars(keep_bytes, data->BufTextLen - keep_bytes);
    }
    data->CursorPos = std::clamp(data->CursorPos, 0, data->BufTextLen);
    data->SelectionStart = std::clamp(data->SelectionStart, 0, data->BufTextLen);
    data->SelectionEnd = std::clamp(data->SelectionEnd, 0, data->BufTextLen);
    return true;
}

void DrawInactiveCaretOverlay(ImGuiID input_id, const ImRect& frame_rect, const char* text,
                              int caret_byte, int selection_start_byte, int selection_end_byte,
                              bool multiline, bool password) {
    if (!text) {
        return;
    }

    std::string masked;
    if (password) {
        const int length = static_cast<int>(std::strlen(text));
        caret_byte = Utf16CountFromUtf8Range(text, text + std::clamp(caret_byte, 0, length));
        selection_start_byte =
            Utf16CountFromUtf8Range(text, text + std::clamp(selection_start_byte, 0, length));
        selection_end_byte =
            Utf16CountFromUtf8Range(text, text + std::clamp(selection_end_byte, 0, length));
        masked.assign(Utf16CountFromUtf8Range(text), '*');
        text = masked.c_str();
    }
    const auto& style = ImGui::GetStyle();
    auto* window = ImGui::GetCurrentWindow();
    auto* draw_list = window->DrawList;
    auto origin = frame_rect.Min + style.FramePadding;
    auto clip = frame_rect;
    clip.ClipWith(window->ClipRect);
    float font_size = ImGui::GetFontSize();
    if (multiline) {
        const auto& children = window->DC.ChildWindows;
        const auto child = std::ranges::find(children, input_id, &ImGuiWindow::ChildId);
        if (child == children.end()) {
            return;
        }
        origin = (*child)->DC.CursorStartPos + style.FramePadding;
        clip.ClipWith((*child)->ClipRect);
        font_size = (*child)->FontRefSize;
        draw_list = (*child)->DrawList;
    }

    const int length = static_cast<int>(std::strlen(text));
    const char* text_end = text + length;
    const int selection_start =
        std::clamp(std::min(selection_start_byte, selection_end_byte), 0, length);
    const int selection_end =
        std::clamp(std::max(selection_start_byte, selection_end_byte), 0, length);
    const auto measure = [&](const char* start, const char* end) {
        return ImFontCalcTextSizeEx(ImGui::GetFont(), font_size, FLT_MAX, 0.0f, start, end,
                                    text_end, nullptr, nullptr, ImDrawTextFlags_WrapKeepBlanks)
            .x;
    };
    if (clip.IsInverted()) {
        return;
    }
    if (selection_start < selection_end) {
        const char* first = text + selection_start;
        const char* last = text + selection_end;
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
        float y = origin.y;
        draw_list->PushClipRect(clip.Min, clip.Max, true);
        for (const char* start = text; start < last && y < clip.Max.y; y += font_size) {
            const char* newline = multiline ? std::strchr(start, '\n') : nullptr;
            const char* end = newline ? newline : text_end;
            if (first <= end && y + font_size > clip.Min.y) {
                const float left = origin.x + measure(start, std::max(start, first));
                float right = origin.x + measure(start, std::min(end, last));
                if (newline && last > newline) {
                    right += font_size * 0.25f;
                }
                draw_list->AddRectFilled({left, y}, {right, y + font_size}, color);
            }
            if (!newline) {
                break;
            }
            start = newline + 1;
        }
        draw_list->PopClipRect();
        return;
    }
    if (!IsCaretBlinkVisible()) {
        return;
    }
    const char* caret = text + std::clamp(caret_byte, 0, length);
    const char* line_start = text;
    int line = 0;
    if (multiline) {
        for (const char* p = text; p < caret; ++p) {
            if (*p == '\n') {
                line_start = p + 1;
                ++line;
            }
        }
    }
    const float advance = measure(line_start, caret);
    const float x = ImTrunc(origin.x + advance);
    const float baseline = ImTrunc(origin.y + (line + 1) * font_size);
    const ImRect cursor{x, baseline - font_size + 0.5f, x + 1.0f, baseline - 1.5f};
    if (cursor.Overlaps(clip)) {
        draw_list->PushClipRect(clip.Min, clip.Max, true);
        draw_list->AddLineV(x, cursor.Min.y, cursor.Max.y,
                            ImGui::GetColorU32(ImGuiCol_InputTextCursor),
                            style.InputTextCursorSize);
        draw_list->PopClipRect();
    }
}

static float ApplyAxisDeadzone(float v) {
    const float av = std::abs(v);
    if (av <= UiShared::kAxisDeadzone) {
        return 0.0f;
    }
    const float scaled = (av - UiShared::kAxisDeadzone) / (1.0f - UiShared::kAxisDeadzone);
    return std::copysign(std::clamp(scaled, 0.0f, 1.0f), v);
}

VirtualPadSnapshot ReadVirtualPadSnapshot(Libraries::UserService::OrbisUserServiceUserId user_id,
                                          float delta_time, bool include_imgui_fallback) {
    VirtualPadSnapshot snapshot{};
    constexpr u32 kMaskLeft = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Left);
    constexpr u32 kMaskRight = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Right);
    constexpr u32 kMaskUp = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Up);
    constexpr u32 kMaskDown = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Down);
    constexpr u32 kMaskCross = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    constexpr u32 kMaskSquare = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    constexpr u32 kMaskCircle = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Circle);
    constexpr u32 kMaskL1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    constexpr u32 kMaskR1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R1);
    constexpr u32 kMaskL2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    constexpr u32 kMaskR2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R2);
    constexpr u32 kMaskL3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L3);
    constexpr u32 kMaskR3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R3);
    constexpr float kTriggerButtonThreshold = 0.25f;

    float lx = 0.0f;
    float ly = 0.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    float l2_analog = 0.0f;
    float r2_analog = 0.0f;
    u32 buttons = 0;
    Input::State state{};
    if (UiShared::ReadControllerState(user_id, &state)) {
        buttons = static_cast<u32>(state.buttonsState);
        const bool cross_down = (buttons & kMaskCross) != 0;
        const bool circle_down = (buttons & kMaskCircle) != 0;
        if (EmulatorSettings.IsCircleEnter() && cross_down != circle_down) {
            buttons ^= kMaskCross | kMaskCircle;
        }
        lx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftX)]));
        ly = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftY)]));
        rx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightX)]));
        ry = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightY)]));
        l2_analog = std::clamp(
            static_cast<float>(state.axes[static_cast<std::size_t>(Input::Axis::TriggerLeft)]) /
                255.0f,
            0.0f, 1.0f);
        r2_analog = std::clamp(
            static_cast<float>(state.axes[static_cast<std::size_t>(Input::Axis::TriggerRight)]) /
                255.0f,
            0.0f, 1.0f);
    }

    if (include_imgui_fallback) {
        const float imgui_lx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickLeft)->AnalogValue);
        const float imgui_ly =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickUp)->AnalogValue);
        const float imgui_rx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadRStickLeft)->AnalogValue);
        const float imgui_ry =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadRStickUp)->AnalogValue);
        const float imgui_l2 =
            std::clamp(ImGui::GetKeyData(ImGuiKey_GamepadL2)->AnalogValue, 0.0f, 1.0f);
        const float imgui_r2 =
            std::clamp(ImGui::GetKeyData(ImGuiKey_GamepadR2)->AnalogValue, 0.0f, 1.0f);

        if (std::abs(imgui_lx) > std::abs(lx)) {
            lx = imgui_lx;
        }
        if (std::abs(imgui_ly) > std::abs(ly)) {
            ly = imgui_ly;
        }
        if (std::abs(imgui_rx) > std::abs(rx)) {
            rx = imgui_rx;
        }
        if (std::abs(imgui_ry) > std::abs(ry)) {
            ry = imgui_ry;
        }
        l2_analog = std::max(l2_analog, imgui_l2);
        r2_analog = std::max(r2_analog, imgui_r2);

        const auto merge_imgui_button = [&](const ImGuiKey key, const u32 mask) {
            if (ImGui::IsKeyDown(key)) {
                buttons |= mask;
            }
        };
        merge_imgui_button(ImGuiKey_GamepadDpadLeft, kMaskLeft);
        merge_imgui_button(ImGuiKey_GamepadDpadRight, kMaskRight);
        merge_imgui_button(ImGuiKey_GamepadDpadUp, kMaskUp);
        merge_imgui_button(ImGuiKey_GamepadDpadDown, kMaskDown);
        merge_imgui_button(ImGuiKey_GamepadFaceDown, kMaskCross);
        merge_imgui_button(ImGuiKey_GamepadFaceUp, kMaskTriangle);
        merge_imgui_button(ImGuiKey_GamepadFaceLeft, kMaskSquare);
        merge_imgui_button(ImGuiKey_GamepadFaceRight, kMaskCircle);
        merge_imgui_button(ImGuiKey_GamepadL1, kMaskL1);
        merge_imgui_button(ImGuiKey_GamepadR1, kMaskR1);
        merge_imgui_button(ImGuiKey_GamepadL3, kMaskL3);
        merge_imgui_button(ImGuiKey_GamepadR3, kMaskR3);
        if (l2_analog >= kTriggerButtonThreshold) {
            buttons |= kMaskL2;
        }
        if (r2_analog >= kTriggerButtonThreshold) {
            buttons |= kMaskR2;
        }
    }

    snapshot.buttons = buttons;
    snapshot.left_stick = {lx, ly};
    snapshot.l2_analog = l2_analog;

    if (delta_time > 0.0f) {
        snapshot.panel_delta = {rx * UiShared::kPanelMoveSpeed * delta_time,
                                ry * UiShared::kPanelMoveSpeed * delta_time};
    }
    return snapshot;
}

static StickNavDirection ResolveStickNavDirection(float lx, float ly, float* strength) {
    const float abs_x = std::abs(lx);
    const float abs_y = std::abs(ly);
    if (abs_x < kNavAxisThreshold && abs_y < kNavAxisThreshold) {
        if (strength) {
            *strength = 0.0f;
        }
        return StickNavDirection::None;
    }

    if (abs_x >= abs_y) {
        if (strength) {
            *strength = abs_x;
        }
        return (lx < 0.0f) ? StickNavDirection::Left : StickNavDirection::Right;
    }

    if (strength) {
        *strength = abs_y;
    }
    return (ly < 0.0f) ? StickNavDirection::Up : StickNavDirection::Down;
}

static VirtualButtonRepeatResult ProcessRepeatButton(bool down, bool edge_pressed, double now,
                                                     double repeat_delay, double repeat_rate,
                                                     bool& prev_down_state,
                                                     double& next_repeat_time) {
    VirtualButtonRepeatResult result{};
    if (!down) {
        prev_down_state = false;
        next_repeat_time = 0.0;
        return result;
    }

    if (!prev_down_state) {
        prev_down_state = true;
        next_repeat_time = now + repeat_delay;
        result.pressed = edge_pressed;
        return result;
    }

    if (edge_pressed) {
        next_repeat_time = now + repeat_delay;
        result.pressed = true;
        return result;
    }

    if (repeat_rate <= 0.0) {
        return result;
    }

    if (now >= next_repeat_time) {
        next_repeat_time = now + repeat_rate;
        result.pressed = true;
        result.repeat = true;
    }
    return result;
}

static bool IsStickDirectionEdgePressed(StickNavDirection direction, bool left_edge,
                                        bool right_edge, bool up_edge, bool down_edge) {
    switch (direction) {
    case StickNavDirection::Left:
        return left_edge;
    case StickNavDirection::Right:
        return right_edge;
    case StickNavDirection::Up:
        return up_edge;
    case StickNavDirection::Down:
        return down_edge;
    case StickNavDirection::None:
    default:
        return false;
    }
}

static StickNavPulseResult ProcessStickNavPulse(bool enabled, StickNavDirection direction,
                                                float strength, bool direction_edge_pressed,
                                                StickNavPulseState& state, double now) {
    StickNavPulseResult result{};
    if (!enabled || direction == StickNavDirection::None) {
        state.repeat_dir = 0;
        state.next_repeat_time = 0.0;
        return result;
    }

    const float t =
        std::clamp((strength - kNavAxisThreshold) / (1.0f - kNavAxisThreshold), 0.0f, 1.0f);
    const double initial_delay =
        std::lerp(kStickNavInitialDelaySlow, kStickNavInitialDelayFast, static_cast<double>(t));
    const double repeat_interval =
        std::lerp(kStickNavRepeatSlow, kStickNavRepeatFast, static_cast<double>(t));

    const int dir_id = static_cast<int>(direction);
    const int prev_dir = state.repeat_dir;
    bool pulse = false;
    bool pulse_repeat = false;

    if (prev_dir != dir_id) {
        // Treat direction changes before the pending repeat tick as fast-rotate input.
        const bool fast_rotate_change = (prev_dir != 0) && (now < state.next_repeat_time);
        state.repeat_dir = dir_id;
        state.next_repeat_time = now + initial_delay;
        pulse = (prev_dir == 0) || direction_edge_pressed || fast_rotate_change;
    } else if (direction_edge_pressed) {
        pulse = true;
        state.next_repeat_time = now + initial_delay;
    } else if (now >= state.next_repeat_time) {
        pulse = true;
        pulse_repeat = true;
        state.next_repeat_time = now + repeat_interval;
    }

    if (!pulse) {
        return result;
    }

    switch (direction) {
    case StickNavDirection::Left:
        result.left = true;
        result.left_repeat = pulse_repeat;
        break;
    case StickNavDirection::Right:
        result.right = true;
        result.right_repeat = pulse_repeat;
        break;
    case StickNavDirection::Up:
        result.up = true;
        result.up_repeat = pulse_repeat;
        break;
    case StickNavDirection::Down:
        result.down = true;
        result.down_repeat = pulse_repeat;
        break;
    case StickNavDirection::None:
    default:
        break;
    }
    return result;
}

OskVirtualPadInputView::OskVirtualPadInputView(const OskPadInputFrame& input_frame,
                                               const ImGuiIO& io)
    : frame(input_frame), repeat_delay(static_cast<double>(io.KeyRepeatDelay)),
      repeat_rate(static_cast<double>(io.KeyRepeatRate)) {}

bool OskVirtualPadInputView::Down(Libraries::Pad::OrbisPadButtonDataOffset button) const {
    return (frame.virtual_buttons & static_cast<u32>(button)) != 0;
}

bool OskVirtualPadInputView::Pressed(Libraries::Pad::OrbisPadButtonDataOffset button) const {
    const u32 mask = static_cast<u32>(button);
    return (frame.virtual_buttons & mask) != 0 && (frame.prev_virtual_buttons & mask) == 0;
}

bool OskVirtualPadInputView::RepeatPressed(Libraries::Pad::OrbisPadButtonDataOffset button,
                                           bool& prev_down_state, double& next_repeat_time,
                                           bool* out_repeat) const {
    const VirtualButtonRepeatResult result =
        ProcessRepeatButton(Down(button), Pressed(button), ImGui::GetTime(), repeat_delay,
                            repeat_rate, prev_down_state, next_repeat_time);
    if (out_repeat) {
        *out_repeat = result.repeat;
    }
    return result.pressed;
}

static void ResetOskShortcutRepeatState(OskShortcutRepeatState& state) {
    state.prev_square_down = false;
    state.prev_l1_down = false;
    state.prev_r1_down = false;
    state.l2_shortcut_armed = true;
    state.square_next_repeat_time = 0.0;
    state.l1_next_repeat_time = 0.0;
    state.r1_next_repeat_time = 0.0;
    state.triangle_next_repeat_time = 0.0;
}

static bool ConsumeTriggerShortcutPress(bool trigger_down, bool trigger_edge_pressed,
                                        float trigger_analog_value, bool& shortcut_armed) {
    // Require clear release before accepting another trigger shortcut press.
    constexpr float kTriggerReleaseThreshold = 0.20f;
    if (!trigger_down && trigger_analog_value <= kTriggerReleaseThreshold) {
        shortcut_armed = true;
    }
    if (!shortcut_armed || !trigger_edge_pressed) {
        return false;
    }
    shortcut_armed = false;
    return true;
}

OskShortcutActionResult EvaluateOskShortcutAction(bool allow_osk_shortcuts, bool menu_modal,
                                                  bool evaluate_action,
                                                  const OskPadInputFrame& panel_input,
                                                  const OskVirtualPadInputView& virtual_pad_input,
                                                  u32 prev_virtual_buttons,
                                                  ImeKbLayoutFamily layout_family,
                                                  OskShortcutRepeatState& repeat_state) {
    OskShortcutActionResult result{};
    if (!(allow_osk_shortcuts && !menu_modal)) {
        ResetOskShortcutRepeatState(repeat_state);
        return result;
    }
    if (!evaluate_action) {
        return result;
    }

    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);

    const bool l1_down = virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    const bool l1_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    const bool l1_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::L1, repeat_state.prev_l1_down,
        repeat_state.l1_next_repeat_time);

    const bool square_down =
        virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    const bool square_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    const bool square_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::Square, repeat_state.prev_square_down,
        repeat_state.square_next_repeat_time);

    const bool r1_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::R1, repeat_state.prev_r1_down,
        repeat_state.r1_next_repeat_time);

    const bool clear_all_shortcut_pressed =
        (l1_down && square_edge_pressed) || (square_down && l1_edge_pressed);

    const bool l2_down = virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    const bool l2_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    const float l2_analog = std::max(panel_input.l2_analog, l2_down ? 1.0f : 0.0f);
    const bool l2_pressed = ConsumeTriggerShortcutPress(l2_down, l2_edge_pressed, l2_analog,
                                                        repeat_state.l2_shortcut_armed);

    const bool tri_down =
        virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    const bool tri_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    bool prev_virtual_triangle_down = (prev_virtual_buttons & kMaskTriangle) != 0;
    const bool tri_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::Triangle, prev_virtual_triangle_down,
        repeat_state.triangle_next_repeat_time);
    const bool tri_space_pressed = tri_pressed && !l2_down;

    const bool numeric = layout_family == ImeKbLayoutFamily::Numeric;
    const bool symbols_shortcut_pressed =
        !numeric && ((l2_down && tri_edge_pressed) || (tri_down && l2_pressed));
    if (symbols_shortcut_pressed) {
        result.action = ImeKbKeyAction::SymbolsMode;
    } else if (!numeric && tri_space_pressed) {
        result.action = ImeKbKeyAction::Space;
    } else if (!numeric && layout_family != ImeKbLayoutFamily::Symbols &&
               virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3)) {
        result.action = ImeKbKeyAction::SpecialsMode;
    } else if (virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2)) {
        result.action = ImeKbKeyAction::Done;
    } else if (clear_all_shortcut_pressed) {
        result.clear_all = true;
    } else if (square_repeat_pressed) {
        result.action = ImeKbKeyAction::Backspace;
    } else if (l1_repeat_pressed) {
        result.action = ImeKbKeyAction::ArrowLeft;
    } else if (r1_repeat_pressed) {
        result.action = ImeKbKeyAction::ArrowRight;
    } else if (!numeric &&
               virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3)) {
        result.action = ImeKbKeyAction::Settings;
    } else if (!numeric && layout_family != ImeKbLayoutFamily::Symbols && l2_pressed) {
        result.action = ImeKbKeyAction::Shift;
    }
    return result;
}

namespace {

bool IsVirtualPadButtonDown(const u32 buttons, const u32 button_mask) {
    return (buttons & button_mask) != 0;
}

bool IsVirtualPadButtonPressed(const u32 buttons, const u32 prev_buttons, const u32 button_mask) {
    return IsVirtualPadButtonDown(buttons, button_mask) &&
           !IsVirtualPadButtonDown(prev_buttons, button_mask);
}

VirtualButtonRepeatResult ProcessVirtualPadButtonRepeat(const u32 buttons, const u32 prev_buttons,
                                                        const u32 button_mask, const double now,
                                                        const double repeat_delay,
                                                        const double repeat_rate,
                                                        bool& prev_down_state,
                                                        double& next_repeat_time) {
    return ProcessRepeatButton(IsVirtualPadButtonDown(buttons, button_mask),
                               IsVirtualPadButtonPressed(buttons, prev_buttons, button_mask), now,
                               repeat_delay, repeat_rate, prev_down_state, next_repeat_time);
}

} // namespace

OskPadInputFrame ComputeOskPadInputFrame(const VirtualPadSnapshot& virtual_pad,
                                         bool allow_osk_shortcuts, bool first_render,
                                         OskPadInputState& state) {
    constexpr u32 kMaskLeft = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Left);
    constexpr u32 kMaskRight = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Right);
    constexpr u32 kMaskUp = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Up);
    constexpr u32 kMaskDown = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Down);
    constexpr u32 kMaskCross = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    constexpr u32 kMaskSquare = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    constexpr u32 kMaskCircle = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Circle);
    constexpr u32 kMaskL1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    constexpr u32 kMaskR1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R1);
    constexpr u32 kMaskL2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    constexpr u32 kMaskR2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R2);
    constexpr u32 kMaskL3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L3);
    constexpr u32 kMaskR3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R3);

    OskPadInputFrame frame{};
    frame.virtual_buttons = virtual_pad.buttons;
    frame.prev_virtual_buttons = first_render ? frame.virtual_buttons : state.prev_virtual_buttons;
    frame.l2_analog = virtual_pad.l2_analog;

    const auto virtual_down = [&](const u32 mask) {
        return IsVirtualPadButtonDown(frame.virtual_buttons, mask);
    };
    const auto virtual_pressed = [&](const u32 mask) {
        return IsVirtualPadButtonPressed(frame.virtual_buttons, frame.prev_virtual_buttons, mask);
    };

    const ImGuiIO& io = ImGui::GetIO();
    const double now = ImGui::GetTime();
    const double repeat_delay = static_cast<double>(io.KeyRepeatDelay);
    const double repeat_rate = static_cast<double>(io.KeyRepeatRate);

    frame.cross_down = virtual_down(kMaskCross);

    bool prev_virtual_cross_hold_down =
        IsVirtualPadButtonDown(frame.prev_virtual_buttons, kMaskCross);
    const VirtualButtonRepeatResult virtual_cross_repeat = ProcessRepeatButton(
        virtual_down(kMaskCross), virtual_pressed(kMaskCross), now, repeat_delay, repeat_rate,
        prev_virtual_cross_hold_down, state.virtual_cross_next_repeat_time);

    const bool prev_cross_down = first_render ? frame.cross_down : state.prev_virtual_cross_down;
    frame.panel_activate_pressed_raw = frame.cross_down && !prev_cross_down;
    frame.panel_activate_repeat_raw = virtual_cross_repeat.repeat;

    const VirtualButtonRepeatResult virtual_nav_left = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskLeft, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_left_down, state.virtual_dpad_left_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_right = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskRight, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_right_down, state.virtual_dpad_right_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_up = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskUp, now, repeat_delay, repeat_rate,
        state.prev_virtual_dpad_up_down, state.virtual_dpad_up_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_down = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskDown, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_down_down, state.virtual_dpad_down_next_repeat_time);

    frame.virtual_lstick_dirs.left = virtual_pad.left_stick.x <= -kNavAxisThreshold;
    frame.virtual_lstick_dirs.right = virtual_pad.left_stick.x >= kNavAxisThreshold;
    frame.virtual_lstick_dirs.up = virtual_pad.left_stick.y <= -kNavAxisThreshold;
    frame.virtual_lstick_dirs.down = virtual_pad.left_stick.y >= kNavAxisThreshold;
    const bool prev_virtual_lstick_left_down =
        first_render ? frame.virtual_lstick_dirs.left : state.prev_virtual_lstick_left_down;
    const bool prev_virtual_lstick_right_down =
        first_render ? frame.virtual_lstick_dirs.right : state.prev_virtual_lstick_right_down;
    const bool prev_virtual_lstick_up_down =
        first_render ? frame.virtual_lstick_dirs.up : state.prev_virtual_lstick_up_down;
    const bool prev_virtual_lstick_down_down =
        first_render ? frame.virtual_lstick_dirs.down : state.prev_virtual_lstick_down_down;

    const bool virtual_lstick_left_edge =
        frame.virtual_lstick_dirs.left && !prev_virtual_lstick_left_down;
    const bool virtual_lstick_right_edge =
        frame.virtual_lstick_dirs.right && !prev_virtual_lstick_right_down;
    const bool virtual_lstick_up_edge =
        frame.virtual_lstick_dirs.up && !prev_virtual_lstick_up_down;
    const bool virtual_lstick_down_edge =
        frame.virtual_lstick_dirs.down && !prev_virtual_lstick_down_down;

    const bool raw_virtual_control_input =
        virtual_pressed(kMaskLeft) || virtual_pressed(kMaskRight) || virtual_pressed(kMaskUp) ||
        virtual_pressed(kMaskDown) || virtual_lstick_left_edge || virtual_lstick_right_edge ||
        virtual_lstick_up_edge || virtual_lstick_down_edge || virtual_pressed(kMaskCross) ||
        virtual_pressed(kMaskTriangle) || virtual_pressed(kMaskSquare) ||
        virtual_pressed(kMaskCircle) || virtual_pressed(kMaskL1) || virtual_pressed(kMaskR1) ||
        virtual_pressed(kMaskL2) || virtual_pressed(kMaskR2) || virtual_pressed(kMaskL3) ||
        virtual_pressed(kMaskR3);
    frame.raw_osk_control_input = raw_virtual_control_input;

    float stick_strength = 0.0f;
    const StickNavDirection stick_dir = ResolveStickNavDirection(
        virtual_pad.left_stick.x, virtual_pad.left_stick.y, &stick_strength);
    const bool stick_dir_edge_pressed =
        IsStickDirectionEdgePressed(stick_dir, virtual_lstick_left_edge, virtual_lstick_right_edge,
                                    virtual_lstick_up_edge, virtual_lstick_down_edge);

    StickNavPulseState stick_nav_state{state.left_stick_repeat_dir,
                                       state.left_stick_next_repeat_time};
    const StickNavPulseResult stick_nav_pulse =
        ProcessStickNavPulse(allow_osk_shortcuts, stick_dir, stick_strength, stick_dir_edge_pressed,
                             stick_nav_state, now);
    state.left_stick_repeat_dir = stick_nav_state.repeat_dir;
    state.left_stick_next_repeat_time = stick_nav_state.next_repeat_time;

    frame.virtual_nav_left = virtual_nav_left.pressed;
    frame.virtual_nav_right = virtual_nav_right.pressed;
    frame.virtual_nav_up = virtual_nav_up.pressed;
    frame.virtual_nav_down = virtual_nav_down.pressed;
    frame.virtual_nav_left_repeat = virtual_nav_left.repeat;
    frame.virtual_nav_right_repeat = virtual_nav_right.repeat;
    frame.virtual_nav_up_repeat = virtual_nav_up.repeat;
    frame.virtual_nav_down_repeat = virtual_nav_down.repeat;
    frame.stick_nav_left = stick_nav_pulse.left;
    frame.stick_nav_right = stick_nav_pulse.right;
    frame.stick_nav_up = stick_nav_pulse.up;
    frame.stick_nav_down = stick_nav_pulse.down;
    frame.stick_nav_left_repeat = stick_nav_pulse.left_repeat;
    frame.stick_nav_right_repeat = stick_nav_pulse.right_repeat;
    frame.stick_nav_up_repeat = stick_nav_pulse.up_repeat;
    frame.stick_nav_down_repeat = stick_nav_pulse.down_repeat;

    frame.nav_left = allow_osk_shortcuts && (frame.virtual_nav_left || frame.stick_nav_left);
    frame.nav_right = allow_osk_shortcuts && (frame.virtual_nav_right || frame.stick_nav_right);
    frame.nav_up = allow_osk_shortcuts && (frame.virtual_nav_up || frame.stick_nav_up);
    frame.nav_down = allow_osk_shortcuts && (frame.virtual_nav_down || frame.stick_nav_down);
    frame.nav_left_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_left_repeat || frame.stick_nav_left_repeat);
    frame.nav_right_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_right_repeat || frame.stick_nav_right_repeat);
    frame.nav_up_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_up_repeat || frame.stick_nav_up_repeat);
    frame.nav_down_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_down_repeat || frame.stick_nav_down_repeat);

    frame.virtual_control_input =
        allow_osk_shortcuts &&
        (virtual_nav_left.pressed || virtual_nav_right.pressed || virtual_nav_up.pressed ||
         virtual_nav_down.pressed || stick_nav_pulse.left || stick_nav_pulse.right ||
         stick_nav_pulse.up || stick_nav_pulse.down || virtual_pressed(kMaskCross) ||
         virtual_pressed(kMaskTriangle) || virtual_pressed(kMaskSquare) ||
         virtual_pressed(kMaskCircle) || virtual_pressed(kMaskL1) || virtual_pressed(kMaskR1) ||
         virtual_pressed(kMaskL2) || virtual_pressed(kMaskR2) || virtual_pressed(kMaskL3) ||
         virtual_pressed(kMaskR3));

    frame.osk_control_input =
        frame.virtual_control_input || (allow_osk_shortcuts && (frame.panel_activate_pressed_raw ||
                                                                frame.panel_activate_repeat_raw));
    return frame;
}

void CommitOskPadInputFrame(const OskPadInputFrame& frame, OskPadInputState& state) {
    state.prev_virtual_buttons = frame.virtual_buttons;
    state.prev_virtual_cross_down = frame.cross_down;
    state.prev_virtual_lstick_left_down = frame.virtual_lstick_dirs.left;
    state.prev_virtual_lstick_right_down = frame.virtual_lstick_dirs.right;
    state.prev_virtual_lstick_up_down = frame.virtual_lstick_dirs.up;
    state.prev_virtual_lstick_down_down = frame.virtual_lstick_dirs.down;
}

void DisarmMenuActivate(bool& menu_activate_armed) {
    menu_activate_armed = false;
}

void RearmMenuActivateOnRelease(bool activate_down, bool& menu_activate_armed) {
    if (!menu_activate_armed && !activate_down) {
        menu_activate_armed = true;
    }
}

bool ConsumeMenuActivatePress(bool panel_activate_pressed, bool opened_menu_this_frame,
                              bool& menu_activate_armed) {
    if (!menu_activate_armed || opened_menu_this_frame || !panel_activate_pressed) {
        return false;
    }
    menu_activate_armed = false;
    return true;
}

} // namespace Libraries::Ime
