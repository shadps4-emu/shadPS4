// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/debug_state.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_dialog_ui.h"
#include "core/libraries/ime/ime_kb_layout.h"
#include "core/libraries/pad/pad.h"
#include "core/memory.h"
#include "core/tls.h"
#include "imgui/imgui_std.h"
#include "imgui/renderer/imgui_core.h"
#include "input/controller.h"

using namespace ImGui;

namespace Libraries::ImeDialog {
namespace {
constexpr ImU32 kSelectorBorderColor = IM_COL32(248, 248, 248, 255);
constexpr ImU32 kSelectorOverlayColor = IM_COL32(255, 255, 255, 18);
constexpr float kSelectorBorderThickness = 2.0f;
constexpr float kSelectorInnerMargin = 2.0f;

ImVec4 BrightenColor(ImU32 color, float delta) {
    ImVec4 out = ImGui::ColorConvertU32ToFloat4(color);
    out.x = std::clamp(out.x + delta, 0.0f, 1.0f);
    out.y = std::clamp(out.y + delta, 0.0f, 1.0f);
    out.z = std::clamp(out.z + delta, 0.0f, 1.0f);
    return out;
}

bool IsMappedGuestBuffer(const void* ptr, size_t bytes) {
    if (!ptr || bytes == 0) {
        return false;
    }
    auto* memory = ::Core::Memory::Instance();
    if (!memory) {
        return false;
    }
    return memory->IsValidMapping(reinterpret_cast<VAddr>(ptr), bytes);
}

size_t BoundedUtf16Length(const char16_t* text, size_t max_len) {
    if (!text || max_len == 0) {
        return 0;
    }
    for (size_t i = 0; i < max_len; ++i) {
        if (text[i] == u'\0') {
            return i;
        }
    }
    return max_len;
}

int Utf8CharCount(const char* text, int byte_len) {
    if (!text || byte_len <= 0) {
        return 0;
    }
    return ImTextCountCharsFromUtf8(text, text + byte_len);
}

int Utf8ByteIndexFromCharIndex(const char* text, int char_index) {
    if (!text || char_index <= 0) {
        return 0;
    }
    const char* p = text;
    int count = 0;
    while (*p && count < char_index) {
        unsigned int c = 0;
        const int step = ImTextCharFromUtf8(&c, p, nullptr);
        if (step <= 0) {
            break;
        }
        p += step;
        ++count;
    }
    return static_cast<int>(p - text);
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

void DrawInactiveCaretOverlay(const ImRect& frame_rect, const char* text, int caret_byte,
                              int selection_start_byte, int selection_end_byte, bool multiline) {
    if (!text) {
        return;
    }
    if (selection_start_byte != selection_end_byte) {
        return;
    }

    const int text_len = static_cast<int>(std::strlen(text));
    const int clamped_byte = std::clamp(caret_byte, 0, text_len);
    const char* const caret_ptr = text + clamped_byte;

    const ImGuiStyle& style = GetStyle();
    const float left = frame_rect.Min.x + style.FramePadding.x;
    const float right = frame_rect.Max.x - style.FramePadding.x;
    if (right <= left) {
        return;
    }

    const float top = frame_rect.Min.y + style.FramePadding.y;
    const float bottom = frame_rect.Max.y - style.FramePadding.y;
    if (bottom <= top) {
        return;
    }

    float caret_x = left;
    float caret_y = top;
    float caret_bottom = bottom;
    if (multiline) {
        const char* line_begin = text;
        int line_index = 0;
        const char* p = text;
        while (p < caret_ptr) {
            if (*p == '\n') {
                line_begin = p + 1;
                ++line_index;
            }
            ++p;
        }
        caret_x = left + CalcTextSize(line_begin, caret_ptr, false, -1.0f).x;
        caret_y = top + static_cast<float>(line_index) * GetTextLineHeight();
        caret_bottom = caret_y + GetTextLineHeight();
    } else {
        caret_x = left + CalcTextSize(text, caret_ptr, false, -1.0f).x;
    }

    caret_x = std::clamp(caret_x, left, right);
    caret_y = std::clamp(caret_y, top, bottom);
    caret_bottom = std::clamp(caret_bottom, caret_y, bottom);
    if (caret_bottom <= caret_y) {
        return;
    }

    const ImU32 caret_col = GetColorU32(ImGuiCol_Text);
    GetWindowDrawList()->AddLine({caret_x, caret_y}, {caret_x, caret_bottom}, caret_col, 1.5f);
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
        Input::State pad_state{};
        bool connected = false;
        int connected_count = 0;
        (*controllers)[index]->ReadState(&pad_state, &connected, &connected_count);
        if (!connected) {
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

struct VirtualLeftStickDirections {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

constexpr float kNavAxisThreshold = 0.55f;
constexpr float kAxisDeadzone = 0.24f;
constexpr double kStickNavInitialDelaySlow = 0.26;
constexpr double kStickNavInitialDelayFast = 0.16;
constexpr double kStickNavRepeatSlow = 0.20;
constexpr double kStickNavRepeatFast = 0.11;
constexpr float kPanelMoveSpeed = 900.0f; // pixels per second at full tilt

float ToAxisUnit(const s32 axis) {
    const float centered = (static_cast<float>(axis) - 128.0f) / 127.0f;
    return std::clamp(centered, -1.0f, 1.0f);
}

float ApplyAxisDeadzone(const float v) {
    const float av = std::abs(v);
    if (av <= kAxisDeadzone) {
        return 0.0f;
    }
    const float scaled = (av - kAxisDeadzone) / (1.0f - kAxisDeadzone);
    return std::copysign(std::clamp(scaled, 0.0f, 1.0f), v);
}

struct VirtualPadSnapshot {
    u32 buttons = 0;
    VirtualLeftStickDirections left_stick_dirs{};
    ImVec2 left_stick{};
    ImVec2 panel_delta{};
};

VirtualPadSnapshot ReadVirtualPadSnapshot(Libraries::UserService::OrbisUserServiceUserId user_id,
                                          const float delta_time) {
    VirtualPadSnapshot snapshot{};

    const float imgui_rx =
        ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickRight)->AnalogValue -
                          ImGui::GetKeyData(ImGuiKey_GamepadRStickLeft)->AnalogValue);
    const float imgui_ry =
        ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickDown)->AnalogValue -
                          ImGui::GetKeyData(ImGuiKey_GamepadRStickUp)->AnalogValue);

    float virtual_rx = 0.0f;
    float virtual_ry = 0.0f;
    Input::State state{};
    if (ReadControllerState(user_id, &state)) {
        snapshot.buttons = static_cast<u32>(state.buttonsState);
        const float lx =
            ApplyAxisDeadzone(ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftX)]));
        const float ly =
            ApplyAxisDeadzone(ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftY)]));
        snapshot.left_stick_dirs.left = lx <= -kNavAxisThreshold;
        snapshot.left_stick_dirs.right = lx >= kNavAxisThreshold;
        snapshot.left_stick_dirs.up = ly <= -kNavAxisThreshold;
        snapshot.left_stick_dirs.down = ly >= kNavAxisThreshold;
        snapshot.left_stick = {lx, ly};
        virtual_rx = ApplyAxisDeadzone(
            ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightX)]));
        virtual_ry = ApplyAxisDeadzone(
            ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightY)]));
    }

    if (delta_time > 0.0f) {
        const float rx = (std::abs(virtual_rx) > std::abs(imgui_rx)) ? virtual_rx : imgui_rx;
        const float ry = (std::abs(virtual_ry) > std::abs(imgui_ry)) ? virtual_ry : imgui_ry;
        snapshot.panel_delta = {rx * kPanelMoveSpeed * delta_time,
                                ry * kPanelMoveSpeed * delta_time};
    }
    return snapshot;
}

enum class StickNavDirection : int {
    None = 0,
    Left = 1,
    Right = 2,
    Up = 3,
    Down = 4,
};

StickNavDirection ResolveStickNavDirection(const float lx, const float ly, float* strength) {
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

Libraries::Ime::ImeKbLayoutSelection ResolveInitialKbLayoutSelection(
    OrbisImeExtOption ext_option, OrbisImePanelPriority panel_priority) {
    Libraries::Ime::ImeKbLayoutSelection selection{};
    const bool set_priority = True(ext_option & OrbisImeExtOption::SET_PRIORITY);
    if (set_priority) {
        switch (panel_priority) {
        case OrbisImePanelPriority::Symbol:
            selection.family = Libraries::Ime::ImeKbLayoutFamily::Symbols;
            break;
        case OrbisImePanelPriority::Accent:
            selection.family = Libraries::Ime::ImeKbLayoutFamily::Specials;
            break;
        case OrbisImePanelPriority::Alphabet:
        case OrbisImePanelPriority::Default:
        default:
            selection.family = Libraries::Ime::ImeKbLayoutFamily::Latin;
            break;
        }
    }

    const bool shift_lock = set_priority && True(ext_option & OrbisImeExtOption::PRIORITY_SHIFT);
    if (shift_lock && selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols) {
        // SDK docs: PRIORITY_SHIFT starts the initial panel in Shift-lock state.
        selection.case_state = Libraries::Ime::ImeKbCaseState::CapsLock;
    }
    return selection;
}

} // namespace

ImeDialogState::ImeDialogState()
    : input_changed(false), user_id(-1), is_multi_line(false), is_numeric(false),
      fixed_position(false), type(OrbisImeType::Default),
      supported_languages(static_cast<OrbisImeLanguage>(0)),
      enter_label(OrbisImeEnterLabel::Default), text_filter(nullptr), keyboard_filter(nullptr),
      max_text_length(ORBIS_IME_DIALOG_MAX_TEXT_LENGTH), text_buffer(nullptr), original_text(),
      title(), placeholder(), current_text() {}

ImeDialogState::ImeDialogState(const OrbisImeDialogParam* param,
                               const OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, "param={}, text_buffer={}", static_cast<const void*>(param),
             static_cast<void*>(param ? param->input_text_buffer : nullptr));
    if (!param) {
        LOG_ERROR(Lib_ImeDialog, "param==nullptr, returning without init");
        return;
    }

    user_id = param->user_id;
    is_multi_line = True(param->option & OrbisImeOption::MULTILINE);
    fixed_position = True(param->option & OrbisImeOption::FIXED_POSITION);
    use_over2k = True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES);
    is_numeric = param->type == OrbisImeType::Number;
    type = param->type;
    supported_languages = param->supported_languages;
    enter_label = param->enter_label;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->ext_keyboard_filter : nullptr;
    ext_option = extended ? extended->option : OrbisImeExtOption::DEFAULT;
    disable_device = extended ? extended->disable_device : OrbisImeDisableDevice::DEFAULT;
    panel_priority = extended ? extended->priority : OrbisImePanelPriority::Default;
    style_config = Libraries::Ime::ResolveImeStyleConfig(extended);
    max_text_length = param->max_text_length;
    text_buffer = param->input_text_buffer;
    LOG_INFO(Lib_ImeDialog,
             "ImeDialogState: option=0x{:X} (multiline={}, password={}, ext_kbd={}, fixed_pos={}, "
             "over2k={}), enter_label={}, type={}",
             static_cast<u32>(param->option), is_multi_line,
             True(param->option & OrbisImeOption::PASSWORD),
             True(param->option & OrbisImeOption::EXT_KEYBOARD),
             True(param->option & OrbisImeOption::FIXED_POSITION),
             True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES),
             static_cast<u32>(enter_label), static_cast<u32>(type));
    LOG_DEBUG(Lib_ImeDialog,
              "ImeDialogState: user_id={}, type={}, enter_label={}, multiline={}, numeric={}, "
              "max_len={}, text_filter={}, keyboard_filter={}",
              static_cast<u32>(user_id), static_cast<u32>(type), static_cast<u32>(enter_label),
              is_multi_line, is_numeric, max_text_length, (void*)text_filter,
              (void*)keyboard_filter);

    if (param->title) {
        std::size_t title_len = std::char_traits<char16_t>::length(param->title);
        title.resize(title_len * 4 + 1);
        title[title_len * 4] = '\0';

        if (!ConvertOrbisToUTF8(param->title, title_len, &title[0], title_len * 4 + 1)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert title to utf8 encoding");
        }
    }

    if (param->placeholder) {
        std::size_t placeholder_len = std::char_traits<char16_t>::length(param->placeholder);
        placeholder.resize(placeholder_len * 4 + 1);
        placeholder[placeholder_len * 4] = '\0';

        if (!ConvertOrbisToUTF8(param->placeholder, placeholder_len, &placeholder[0],
                                placeholder_len * 4 + 1)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert placeholder to utf8 encoding");
        }
    }

    std::size_t text_len = 0;
    if (text_buffer) {
        const size_t bytes = (static_cast<size_t>(max_text_length) + 1) * sizeof(char16_t);
        if (IsMappedGuestBuffer(text_buffer, bytes)) {
            text_len = BoundedUtf16Length(text_buffer, max_text_length);
        } else {
            LOG_ERROR(Lib_ImeDialog, "ImeDialogState: input_text_buffer not mapped");
        }
    }
    if (text_len > max_text_length) {
        text_len = max_text_length;
    }
    original_text.resize(static_cast<std::size_t>(max_text_length) + 1, u'\0');
    if (text_buffer) {
        for (std::size_t i = 0; i < text_len; ++i) {
            original_text[i] = text_buffer[i];
        }
    }
    if (text_buffer) {
        if (!ConvertOrbisToUTF8(text_buffer, text_len, current_text.begin(),
                                ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4 + 1)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
        }
    }
    caret_index = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    caret_byte_index = static_cast<int>(current_text.size());
    caret_dirty = true;
    panel_layout_valid = (sceImeDialogGetPanelPositionAndForm(&panel_layout) == ORBIS_OK);
    if (extended) {
        (void)sceImeDialogGetPanelSizeExtended(param, extended, &panel_req_width,
                                               &panel_req_height);
    } else {
        (void)sceImeDialogGetPanelSize(param, &panel_req_width, &panel_req_height);
    }
    LOG_DEBUG(Lib_ImeDialog, "ImeDialogState: initial_text_len={}", current_text.size());
}

ImeDialogState::ImeDialogState(ImeDialogState&& other) noexcept
    : input_changed(other.input_changed), caret_index(other.caret_index),
      caret_byte_index(other.caret_byte_index), caret_dirty(other.caret_dirty),
      use_over2k(other.use_over2k), panel_layout(other.panel_layout),
      panel_layout_valid(other.panel_layout_valid), panel_req_width(other.panel_req_width),
      panel_req_height(other.panel_req_height), ext_option(other.ext_option),
      disable_device(other.disable_device), panel_priority(other.panel_priority),
      style_config(other.style_config), user_id(other.user_id), is_multi_line(other.is_multi_line),
      is_numeric(other.is_numeric), fixed_position(other.fixed_position), type(other.type),
      supported_languages(other.supported_languages), enter_label(other.enter_label),
      text_filter(other.text_filter), keyboard_filter(other.keyboard_filter),
      max_text_length(other.max_text_length), text_buffer(other.text_buffer),
      original_text(std::move(other.original_text)), title(std::move(other.title)),
      placeholder(std::move(other.placeholder)), current_text(other.current_text) {

    other.text_buffer = nullptr;
}

ImeDialogState& ImeDialogState::operator=(ImeDialogState&& other) {
    if (this != &other) {
        input_changed = other.input_changed;
        caret_index = other.caret_index;
        caret_byte_index = other.caret_byte_index;
        caret_dirty = other.caret_dirty;
        use_over2k = other.use_over2k;
        panel_layout = other.panel_layout;
        panel_layout_valid = other.panel_layout_valid;
        panel_req_width = other.panel_req_width;
        panel_req_height = other.panel_req_height;
        ext_option = other.ext_option;
        disable_device = other.disable_device;
        panel_priority = other.panel_priority;
        style_config = other.style_config;
        user_id = other.user_id;
        is_multi_line = other.is_multi_line;
        is_numeric = other.is_numeric;
        fixed_position = other.fixed_position;
        type = other.type;
        supported_languages = other.supported_languages;
        enter_label = other.enter_label;
        text_filter = other.text_filter;
        keyboard_filter = other.keyboard_filter;
        max_text_length = other.max_text_length;
        text_buffer = other.text_buffer;
        original_text = std::move(other.original_text);
        title = std::move(other.title);
        placeholder = std::move(other.placeholder);
        current_text = other.current_text;

        other.text_buffer = nullptr;
    }

    return *this;
}

bool ImeDialogState::CopyTextToOrbisBuffer(bool use_original) {
    if (!text_buffer) {
        LOG_DEBUG(Lib_ImeDialog, "CopyTextToOrbisBuffer: no text_buffer");
        return false;
    }

    if (use_original) {
        const std::size_t count =
            original_text.empty() ? 0 : static_cast<std::size_t>(max_text_length) + 1;
        if (count > 0) {
            std::copy(original_text.begin(), original_text.end(), text_buffer);
        }
        LOG_DEBUG(Lib_ImeDialog, "CopyTextToOrbisBuffer: restored original");
        return true;
    }

    const std::size_t utf8_len = current_text.size();
    const bool ok = ConvertUTF8ToOrbis(current_text.begin(), utf8_len, text_buffer,
                                       static_cast<std::size_t>(max_text_length) + 1);
    LOG_DEBUG(Lib_ImeDialog, "CopyTextToOrbisBuffer: {}", ok ? "ok" : "failed");
    return ok;
}

bool ImeDialogState::NormalizeNewlines() {
    if (current_text.size() == 0) {
        return false;
    }
    std::string src = current_text.to_string();
    std::string out;
    out.reserve(src.size());
    bool changed = false;
    for (size_t i = 0; i < src.size(); ++i) {
        const char ch = src[i];
        if (ch == '\r') {
            if (i + 1 < src.size() && src[i + 1] == '\n') {
                ++i;
            }
            out.push_back('\n');
            changed = true;
        } else {
            out.push_back(ch);
        }
    }
    if (changed) {
        current_text.FromString(out);
    }
    return changed;
}

bool ImeDialogState::ClampCurrentTextToMaxLen() {
    if (current_text.size() == 0 || max_text_length == 0) {
        return false;
    }
    const int utf16_len = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    if (utf16_len <= static_cast<int>(max_text_length)) {
        return false;
    }
    std::vector<char16_t> utf16(static_cast<size_t>(max_text_length) + 1, u'\0');
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(utf16.data()),
                      static_cast<int>(max_text_length) + 1, current_text.begin(),
                      current_text.begin() + current_text.size());
    size_t len = BoundedUtf16Length(utf16.data(), static_cast<size_t>(max_text_length));
    std::string out;
    out.resize(len * 4 + 1, '\0');
    ImTextStrToUtf8(out.data(), out.size(), reinterpret_cast<const ImWchar*>(utf16.data()),
                    reinterpret_cast<const ImWchar*>(utf16.data()) + len);
    out.resize(std::strlen(out.c_str()));
    current_text.FromString(out);
    return true;
}

bool ImeDialogState::CallTextFilter() {
    if (!text_filter || !input_changed) {
        return true;
    }

    input_changed = false;

    char16_t src_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 src_text_length = 0;
    char16_t out_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 out_text_length = ORBIS_IME_DIALOG_MAX_TEXT_LENGTH;

    if (!ConvertUTF8ToOrbis(current_text.begin(), current_text.size(), src_text,
                            ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to orbis encoding");
        return false;
    }
    src_text_length = static_cast<u32>(
        BoundedUtf16Length(src_text, static_cast<size_t>(ORBIS_IME_DIALOG_MAX_TEXT_LENGTH)));

    int ret = text_filter(out_text, &out_text_length, src_text, src_text_length);

    if (ret != 0) {
        return false;
    }

    if (!ConvertOrbisToUTF8(out_text, out_text_length, current_text.begin(),
                            ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
        return false;
    }

    const bool changed = NormalizeNewlines() | ClampCurrentTextToMaxLen();
    const int new_len = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    if (caret_index > new_len) {
        caret_index = new_len;
        caret_dirty = true;
    } else if (changed) {
        caret_dirty = true;
    }

    CopyTextToOrbisBuffer(false);
    return true;
}

bool ImeDialogState::CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode,
                                        u32* out_status) {
    if (!keyboard_filter) {
        return true;
    }

    int ret = keyboard_filter(src_keycode, out_keycode, out_status, nullptr);
    return ret == 0;
}

bool ImeDialogState::ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len,
                                        char* utf8_text, std::size_t utf8_text_len) {
    std::fill(utf8_text, utf8_text + utf8_text_len, '\0');
    const ImWchar* orbis_text_ptr = reinterpret_cast<const ImWchar*>(orbis_text);
    ImTextStrToUtf8(utf8_text, utf8_text_len, orbis_text_ptr, orbis_text_ptr + orbis_text_len);

    return true;
}

bool ImeDialogState::ConvertUTF8ToOrbis(const char* utf8_text, std::size_t utf8_text_len,
                                        char16_t* orbis_text, std::size_t orbis_text_len) {
    std::fill(orbis_text, orbis_text + orbis_text_len, u'\0');
    const char* utf8_end = utf8_text ? (utf8_text + utf8_text_len) : nullptr;
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(orbis_text), orbis_text_len, utf8_text, utf8_end);

    return true;
}

ImeDialogUi::ImeDialogUi(ImeDialogState* state, OrbisImeDialogStatus* status,
                         OrbisImeDialogResult* result)
    : state(state), status(status), result(result) {

    if (state && *status == OrbisImeDialogStatus::Running) {
        kb_layout_selection =
            ResolveInitialKbLayoutSelection(state->ext_option, state->panel_priority);
        kb_alpha_family =
            (kb_layout_selection.family == Libraries::Ime::ImeKbLayoutFamily::Specials)
                ? Libraries::Ime::ImeKbLayoutFamily::Specials
                : Libraries::Ime::ImeKbLayoutFamily::Latin;
        AddLayer(this);
        ImGui::Core::AcquireGamepadInputCapture();
        gamepad_input_capture_active = true;
    }
}

ImeDialogUi::~ImeDialogUi() {
    std::scoped_lock lock(draw_mutex);

    Free();
}

ImeDialogUi::ImeDialogUi(ImeDialogUi&& other) noexcept
    : state(other.state), status(other.status), result(other.result),
      first_render(other.first_render), accept_armed(other.accept_armed),
      native_input_active(other.native_input_active),
      pointer_navigation_active(other.pointer_navigation_active),
      edit_menu_popup(other.edit_menu_popup), request_input_focus(other.request_input_focus),
      request_input_select_all(other.request_input_select_all),
      text_select_mode(other.text_select_mode),
      pending_input_selection_apply(other.pending_input_selection_apply),
      prev_virtual_cross_down(other.prev_virtual_cross_down),
      prev_virtual_lstick_left_down(other.prev_virtual_lstick_left_down),
      prev_virtual_lstick_right_down(other.prev_virtual_lstick_right_down),
      prev_virtual_lstick_up_down(other.prev_virtual_lstick_up_down),
      prev_virtual_lstick_down_down(other.prev_virtual_lstick_down_down),
      left_stick_repeat_dir(other.left_stick_repeat_dir),
      left_stick_next_repeat_time(other.left_stick_next_repeat_time),
      prev_virtual_buttons(other.prev_virtual_buttons),
      prev_virtual_square_down(other.prev_virtual_square_down),
      prev_virtual_l1_down(other.prev_virtual_l1_down),
      prev_virtual_r1_down(other.prev_virtual_r1_down),
      prev_virtual_dpad_left_down(other.prev_virtual_dpad_left_down),
      prev_virtual_dpad_right_down(other.prev_virtual_dpad_right_down),
      prev_virtual_dpad_up_down(other.prev_virtual_dpad_up_down),
      prev_virtual_dpad_down_down(other.prev_virtual_dpad_down_down),
      virtual_square_next_repeat_time(other.virtual_square_next_repeat_time),
      virtual_l1_next_repeat_time(other.virtual_l1_next_repeat_time),
      virtual_r1_next_repeat_time(other.virtual_r1_next_repeat_time),
      virtual_dpad_left_next_repeat_time(other.virtual_dpad_left_next_repeat_time),
      virtual_dpad_right_next_repeat_time(other.virtual_dpad_right_next_repeat_time),
      virtual_dpad_up_next_repeat_time(other.virtual_dpad_up_next_repeat_time),
      virtual_dpad_down_next_repeat_time(other.virtual_dpad_down_next_repeat_time),
      panel_position_initialized(other.panel_position_initialized),
      panel_drag_active(other.panel_drag_active), panel_position(other.panel_position),
      input_cursor_utf16(other.input_cursor_utf16), input_cursor_byte(other.input_cursor_byte),
      input_selection_start_byte(other.input_selection_start_byte),
      input_selection_end_byte(other.input_selection_end_byte),
      text_select_anchor_utf16(other.text_select_anchor_utf16),
      text_select_focus_utf16(other.text_select_focus_utf16),
      top_virtual_col(other.top_virtual_col), panel_selection(other.panel_selection),
      pending_keyboard_row(other.pending_keyboard_row),
      pending_keyboard_col(other.pending_keyboard_col),
      last_keyboard_selected_row(other.last_keyboard_selected_row),
      last_keyboard_selected_col(other.last_keyboard_selected_col),
      edit_menu_index(other.edit_menu_index), kb_layout_selection(other.kb_layout_selection),
      kb_alpha_family(other.kb_alpha_family),
      gamepad_input_capture_active(other.gamepad_input_capture_active) {

    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
    other.prev_virtual_lstick_left_down = false;
    other.prev_virtual_lstick_right_down = false;
    other.prev_virtual_lstick_up_down = false;
    other.prev_virtual_lstick_down_down = false;
    other.left_stick_repeat_dir = 0;
    other.left_stick_next_repeat_time = 0.0;
    other.prev_virtual_buttons = 0;
    other.prev_virtual_square_down = false;
    other.prev_virtual_l1_down = false;
    other.prev_virtual_r1_down = false;
    other.prev_virtual_dpad_left_down = false;
    other.prev_virtual_dpad_right_down = false;
    other.prev_virtual_dpad_up_down = false;
    other.prev_virtual_dpad_down_down = false;
    other.virtual_square_next_repeat_time = 0.0;
    other.virtual_l1_next_repeat_time = 0.0;
    other.virtual_r1_next_repeat_time = 0.0;
    other.virtual_dpad_left_next_repeat_time = 0.0;
    other.virtual_dpad_right_next_repeat_time = 0.0;
    other.virtual_dpad_up_next_repeat_time = 0.0;
    other.virtual_dpad_down_next_repeat_time = 0.0;
    other.kb_alpha_family = Libraries::Ime::ImeKbLayoutFamily::Latin;
    other.gamepad_input_capture_active = false;

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        if (!gamepad_input_capture_active) {
            ImGui::Core::AcquireGamepadInputCapture();
            gamepad_input_capture_active = true;
        }
    }
}

ImeDialogUi& ImeDialogUi::operator=(ImeDialogUi&& other) {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    Free();

    state = other.state;
    status = other.status;
    result = other.result;
    first_render = other.first_render;
    accept_armed = other.accept_armed;
    native_input_active = other.native_input_active;
    pointer_navigation_active = other.pointer_navigation_active;
    edit_menu_popup = other.edit_menu_popup;
    request_input_focus = other.request_input_focus;
    request_input_select_all = other.request_input_select_all;
    text_select_mode = other.text_select_mode;
    pending_input_selection_apply = other.pending_input_selection_apply;
    prev_virtual_cross_down = other.prev_virtual_cross_down;
    prev_virtual_lstick_left_down = other.prev_virtual_lstick_left_down;
    prev_virtual_lstick_right_down = other.prev_virtual_lstick_right_down;
    prev_virtual_lstick_up_down = other.prev_virtual_lstick_up_down;
    prev_virtual_lstick_down_down = other.prev_virtual_lstick_down_down;
    left_stick_repeat_dir = other.left_stick_repeat_dir;
    left_stick_next_repeat_time = other.left_stick_next_repeat_time;
    prev_virtual_buttons = other.prev_virtual_buttons;
    prev_virtual_square_down = other.prev_virtual_square_down;
    prev_virtual_l1_down = other.prev_virtual_l1_down;
    prev_virtual_r1_down = other.prev_virtual_r1_down;
    prev_virtual_dpad_left_down = other.prev_virtual_dpad_left_down;
    prev_virtual_dpad_right_down = other.prev_virtual_dpad_right_down;
    prev_virtual_dpad_up_down = other.prev_virtual_dpad_up_down;
    prev_virtual_dpad_down_down = other.prev_virtual_dpad_down_down;
    virtual_square_next_repeat_time = other.virtual_square_next_repeat_time;
    virtual_l1_next_repeat_time = other.virtual_l1_next_repeat_time;
    virtual_r1_next_repeat_time = other.virtual_r1_next_repeat_time;
    virtual_dpad_left_next_repeat_time = other.virtual_dpad_left_next_repeat_time;
    virtual_dpad_right_next_repeat_time = other.virtual_dpad_right_next_repeat_time;
    virtual_dpad_up_next_repeat_time = other.virtual_dpad_up_next_repeat_time;
    virtual_dpad_down_next_repeat_time = other.virtual_dpad_down_next_repeat_time;
    panel_position_initialized = other.panel_position_initialized;
    panel_drag_active = other.panel_drag_active;
    panel_position = other.panel_position;
    input_cursor_utf16 = other.input_cursor_utf16;
    input_cursor_byte = other.input_cursor_byte;
    input_selection_start_byte = other.input_selection_start_byte;
    input_selection_end_byte = other.input_selection_end_byte;
    text_select_anchor_utf16 = other.text_select_anchor_utf16;
    text_select_focus_utf16 = other.text_select_focus_utf16;
    top_virtual_col = other.top_virtual_col;
    panel_selection = other.panel_selection;
    pending_keyboard_row = other.pending_keyboard_row;
    pending_keyboard_col = other.pending_keyboard_col;
    last_keyboard_selected_row = other.last_keyboard_selected_row;
    last_keyboard_selected_col = other.last_keyboard_selected_col;
    edit_menu_index = other.edit_menu_index;
    kb_layout_selection = other.kb_layout_selection;
    kb_alpha_family = other.kb_alpha_family;
    gamepad_input_capture_active = other.gamepad_input_capture_active;
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;
    other.prev_virtual_lstick_left_down = false;
    other.prev_virtual_lstick_right_down = false;
    other.prev_virtual_lstick_up_down = false;
    other.prev_virtual_lstick_down_down = false;
    other.left_stick_repeat_dir = 0;
    other.left_stick_next_repeat_time = 0.0;
    other.prev_virtual_buttons = 0;
    other.prev_virtual_square_down = false;
    other.prev_virtual_l1_down = false;
    other.prev_virtual_r1_down = false;
    other.prev_virtual_dpad_left_down = false;
    other.prev_virtual_dpad_right_down = false;
    other.prev_virtual_dpad_up_down = false;
    other.prev_virtual_dpad_down_down = false;
    other.virtual_square_next_repeat_time = 0.0;
    other.virtual_l1_next_repeat_time = 0.0;
    other.virtual_r1_next_repeat_time = 0.0;
    other.virtual_dpad_left_next_repeat_time = 0.0;
    other.virtual_dpad_right_next_repeat_time = 0.0;
    other.virtual_dpad_up_next_repeat_time = 0.0;
    other.virtual_dpad_down_next_repeat_time = 0.0;
    other.kb_alpha_family = Libraries::Ime::ImeKbLayoutFamily::Latin;
    other.gamepad_input_capture_active = false;

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        if (!gamepad_input_capture_active) {
            ImGui::Core::AcquireGamepadInputCapture();
            gamepad_input_capture_active = true;
        }
    }

    return *this;
}

void ImeDialogUi::Free() {
    RemoveLayer(this);
    if (gamepad_input_capture_active) {
        ImGui::Core::ReleaseGamepadInputCapture();
        gamepad_input_capture_active = false;
    }
}

void ImeDialogUi::FinishDialog(OrbisImeDialogEndStatus endstatus, bool restore_original,
                               const char* reason) {
    if (!status || !result || !state) {
        return;
    }
    state->CopyTextToOrbisBuffer(restore_original);
    *status = OrbisImeDialogStatus::Finished;
    result->endstatus = endstatus;
    LOG_INFO(Lib_ImeDialog, "ImeDialog {} -> status=Finished", reason ? reason : "Done");
    Free();
}

void ImeDialogUi::Draw() {
    std::unique_lock<std::mutex> lock{draw_mutex};

    if (!state) {
        return;
    }

    if (!status || *status != OrbisImeDialogStatus::Running) {
        Free();
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;
    const bool imgui_typing_mode_active =
        native_input_active || request_input_focus || pending_input_selection_apply;
    const bool ps4_typing_mode_active = !imgui_typing_mode_active;
    const VirtualPadSnapshot virtual_pad = ReadVirtualPadSnapshot(state->user_id, io.DeltaTime);

    OrbisImePositionAndForm layout = state->panel_layout;
    const bool has_layout = state->panel_layout_valid;
    const auto viewport = Libraries::Ime::ComputeImeViewportMetrics(state->use_over2k);
    const ImVec2 viewport_size = viewport.size;
    const ImVec2 viewport_offset = viewport.offset;
    const float scale_x = viewport.scale_x;
    const float scale_y = viewport.scale_y;
    const float ui_scale = viewport.ui_scale;

    ImVec2 window_size;
    const bool has_panel_size = (has_layout && layout.width > 0 && layout.height > 0) ||
                                (state->panel_req_width > 0 && state->panel_req_height > 0);
    if (has_layout && layout.width > 0 && layout.height > 0) {
        window_size = {layout.width * scale_x, layout.height * scale_y};
    } else if (state->panel_req_width > 0 && state->panel_req_height > 0) {
        window_size = {static_cast<float>(state->panel_req_width) * scale_x,
                       static_cast<float>(state->panel_req_height) * scale_y};
    } else {
        window_size = {std::min(std::max(0.0f, viewport_size.x - 40.0f), 640.0f),
                       std::min(std::max(0.0f, viewport_size.y - 40.0f), 420.0f)};
    }
    if (!has_panel_size) {
        window_size.x = std::max(window_size.x, 320.0f);
        window_size.y = std::max(window_size.y, 240.0f);
    }

    const float panel_w = window_size.x;
    const float panel_h = window_size.y;

    float base_x = 0.0f;
    float base_y = 0.0f;
    if (has_layout) {
        base_x = viewport_offset.x + layout.posx * scale_x;
        base_y = viewport_offset.y + layout.posy * scale_y;
        if (layout.horizontal_alignment == OrbisImeHorizontalAlignment::Center) {
            base_x -= window_size.x * 0.5f;
        } else if (layout.horizontal_alignment == OrbisImeHorizontalAlignment::Right) {
            base_x -= window_size.x;
        }
        if (layout.vertical_alignment == OrbisImeVerticalAlignment::Center) {
            base_y -= window_size.y * 0.5f;
        } else if (layout.vertical_alignment == OrbisImeVerticalAlignment::Bottom) {
            base_y -= window_size.y;
        }
    } else {
        base_x = viewport_offset.x + (viewport_size.x - window_size.x) * 0.5f;
        base_y = viewport_offset.y + (viewport_size.y - window_size.y) * 0.5f;
    }
    const float min_x = viewport_offset.x;
    const float max_x = viewport_offset.x + std::max(0.0f, viewport_size.x - window_size.x);
    const float min_y = viewport_offset.y;
    const float max_y = viewport_offset.y + std::max(0.0f, viewport_size.y - window_size.y);
    base_x = std::clamp(base_x, min_x, max_x);
    base_y = std::clamp(base_y, min_y, max_y);

    if (!panel_position_initialized) {
        panel_position = {base_x, base_y};
        panel_position_initialized = true;
    }
    if (state->fixed_position) {
        panel_position = {base_x, base_y};
        panel_drag_active = false;
    } else {
        if (!ps4_typing_mode_active) {
            panel_drag_active = false;
        } else {
            const ImVec2 mouse_pos = io.MousePos;
            const bool mouse_over_panel = mouse_pos.x >= panel_position.x &&
                                          mouse_pos.x <= (panel_position.x + window_size.x) &&
                                          mouse_pos.y >= panel_position.y &&
                                          mouse_pos.y <= (panel_position.y + window_size.y);
            if (!panel_drag_active && IsMouseClicked(ImGuiMouseButton_Left, false) &&
                mouse_over_panel) {
                panel_drag_active = true;
            }
            if (panel_drag_active) {
                if (IsMouseDown(ImGuiMouseButton_Left)) {
                    panel_position.x += io.MouseDelta.x;
                    panel_position.y += io.MouseDelta.y;
                } else {
                    panel_drag_active = false;
                }
            }
            const ImVec2 right_stick_delta = virtual_pad.panel_delta;
            panel_position.x += right_stick_delta.x;
            panel_position.y += right_stick_delta.y;
        }
    }
    panel_position.x = std::clamp(panel_position.x, min_x, max_x);
    panel_position.y = std::clamp(panel_position.y, min_y, max_y);
    SetNextWindowPos(panel_position);
    SetNextWindowSize(window_size);
    SetNextWindowDockID(0, ImGuiCond_Always);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    const auto lock_window_scroll = []() {
        SetScrollX(0.0f);
        SetScrollY(0.0f);
        ImGuiWindow* window = GetCurrentWindow();
        if (!window) {
            return;
        }
        window->Scroll = ImVec2(0.0f, 0.0f);
        const float max_f = std::numeric_limits<float>::max();
        window->ScrollTarget = ImVec2(max_f, max_f);
    };

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoNavInputs;
    if (Begin("IME Dialog##ImeDialog", nullptr, window_flags)) {
        lock_window_scroll();
        KeepNavHighlight();
        DrawPrettyBackground();
        const Libraries::Ime::ImePanelMetricsConfig metrics_cfg{
            .panel_w = panel_w,
            .panel_h = panel_h,
            .multiline = state->is_multi_line,
            .show_title = !state->title.empty(),
            .base_font_size = GetFontSize(),
            .window_pos = GetWindowPos(),
        };
        const Libraries::Ime::ImePanelMetrics metrics =
            Libraries::Ime::ComputeImePanelMetrics(metrics_cfg);

        const u32 virtual_buttons = virtual_pad.buttons;
        if (first_render) {
            prev_virtual_buttons = virtual_buttons;
        }
        const auto virtual_down = [&](Libraries::Pad::OrbisPadButtonDataOffset button) {
            const u32 mask = static_cast<u32>(button);
            return (virtual_buttons & mask) != 0;
        };
        const auto virtual_pressed = [&](Libraries::Pad::OrbisPadButtonDataOffset button) {
            const u32 mask = static_cast<u32>(button);
            return (virtual_buttons & mask) != 0 && (prev_virtual_buttons & mask) == 0;
        };
        const auto virtual_repeat_pressed = [&](Libraries::Pad::OrbisPadButtonDataOffset button,
                                                bool& prev_down_state,
                                                double& next_repeat_time) -> bool {
            const bool down = virtual_down(button);
            if (!down) {
                prev_down_state = false;
                next_repeat_time = 0.0;
                return false;
            }

            const bool pressed = virtual_pressed(button);
            const double now = ImGui::GetTime();
            if (!prev_down_state) {
                prev_down_state = true;
                next_repeat_time = now + static_cast<double>(io.KeyRepeatDelay);
                return pressed;
            }
            if (pressed) {
                next_repeat_time = now + static_cast<double>(io.KeyRepeatDelay);
                return true;
            }
            if (io.KeyRepeatRate <= 0.0f) {
                return false;
            }
            if (now >= next_repeat_time) {
                next_repeat_time = now + static_cast<double>(io.KeyRepeatRate);
                return true;
            }
            return false;
        };
        const bool imgui_cross_down = IsKeyDown(ImGuiKey_GamepadFaceDown);
        const bool virtual_cross_down =
            virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
        const bool cross_down = imgui_cross_down || virtual_cross_down;
        if (first_render) {
            prev_virtual_cross_down = cross_down;
        }

        // Use an explicit edge detector for activation to avoid stale SDL key events triggering
        // immediate unintended key presses when OSK opens.
        const bool panel_activate_pressed_raw = cross_down && !prev_virtual_cross_down;

        const bool gamepad_nav_left = IsKeyPressed(ImGuiKey_GamepadDpadLeft, true);
        const bool gamepad_nav_right = IsKeyPressed(ImGuiKey_GamepadDpadRight, true);
        const bool gamepad_nav_up = IsKeyPressed(ImGuiKey_GamepadDpadUp, true);
        const bool gamepad_nav_down = IsKeyPressed(ImGuiKey_GamepadDpadDown, true);
        const bool virtual_nav_left =
            virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Left,
                                   prev_virtual_dpad_left_down, virtual_dpad_left_next_repeat_time);
        const bool virtual_nav_right = virtual_repeat_pressed(
            Libraries::Pad::OrbisPadButtonDataOffset::Right, prev_virtual_dpad_right_down,
            virtual_dpad_right_next_repeat_time);
        const bool virtual_nav_up =
            virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Up,
                                   prev_virtual_dpad_up_down, virtual_dpad_up_next_repeat_time);
        const bool virtual_nav_down =
            virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Down,
                                   prev_virtual_dpad_down_down, virtual_dpad_down_next_repeat_time);
        const bool imgui_lstick_left_pressed = IsKeyPressed(ImGuiKey_GamepadLStickLeft, false);
        const bool imgui_lstick_right_pressed = IsKeyPressed(ImGuiKey_GamepadLStickRight, false);
        const bool imgui_lstick_up_pressed = IsKeyPressed(ImGuiKey_GamepadLStickUp, false);
        const bool imgui_lstick_down_pressed = IsKeyPressed(ImGuiKey_GamepadLStickDown, false);
        const float imgui_lx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickLeft)->AnalogValue);
        const float imgui_ly =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickUp)->AnalogValue);
        const VirtualLeftStickDirections virtual_lstick_dirs = virtual_pad.left_stick_dirs;
        if (first_render) {
            prev_virtual_lstick_left_down = virtual_lstick_dirs.left;
            prev_virtual_lstick_right_down = virtual_lstick_dirs.right;
            prev_virtual_lstick_up_down = virtual_lstick_dirs.up;
            prev_virtual_lstick_down_down = virtual_lstick_dirs.down;
        }
        const bool virtual_lstick_left_edge =
            virtual_lstick_dirs.left && !prev_virtual_lstick_left_down;
        const bool virtual_lstick_right_edge =
            virtual_lstick_dirs.right && !prev_virtual_lstick_right_down;
        const bool virtual_lstick_up_edge = virtual_lstick_dirs.up && !prev_virtual_lstick_up_down;
        const bool virtual_lstick_down_edge =
            virtual_lstick_dirs.down && !prev_virtual_lstick_down_down;

        const bool controller_shortcuts_disabled =
            True(state->disable_device & OrbisImeDisableDevice::CONTROLLER);
        const bool allow_osk_shortcuts = ps4_typing_mode_active && !controller_shortcuts_disabled;
        const float combined_lx = (std::abs(virtual_pad.left_stick.x) > std::abs(imgui_lx))
                                      ? virtual_pad.left_stick.x
                                      : imgui_lx;
        const float combined_ly = (std::abs(virtual_pad.left_stick.y) > std::abs(imgui_ly))
                                      ? virtual_pad.left_stick.y
                                      : imgui_ly;
        float stick_strength = 0.0f;
        const StickNavDirection stick_dir =
            ResolveStickNavDirection(combined_lx, combined_ly, &stick_strength);
        bool stick_nav_left = false;
        bool stick_nav_right = false;
        bool stick_nav_up = false;
        bool stick_nav_down = false;
        if (!allow_osk_shortcuts || stick_dir == StickNavDirection::None) {
            left_stick_repeat_dir = 0;
            left_stick_next_repeat_time = 0.0;
        } else {
            const double now = ImGui::GetTime();
            const float t = std::clamp(
                (stick_strength - kNavAxisThreshold) / (1.0f - kNavAxisThreshold), 0.0f, 1.0f);
            const double initial_delay = std::lerp(
                kStickNavInitialDelaySlow, kStickNavInitialDelayFast, static_cast<double>(t));
            const double repeat_interval =
                std::lerp(kStickNavRepeatSlow, kStickNavRepeatFast, static_cast<double>(t));
            bool dir_edge_pressed = false;
            switch (stick_dir) {
            case StickNavDirection::Left:
                dir_edge_pressed = imgui_lstick_left_pressed || virtual_lstick_left_edge;
                break;
            case StickNavDirection::Right:
                dir_edge_pressed = imgui_lstick_right_pressed || virtual_lstick_right_edge;
                break;
            case StickNavDirection::Up:
                dir_edge_pressed = imgui_lstick_up_pressed || virtual_lstick_up_edge;
                break;
            case StickNavDirection::Down:
                dir_edge_pressed = imgui_lstick_down_pressed || virtual_lstick_down_edge;
                break;
            case StickNavDirection::None:
            default:
                break;
            }
            bool stick_pulse = false;
            if (left_stick_repeat_dir != static_cast<int>(stick_dir) || dir_edge_pressed) {
                stick_pulse = true;
                left_stick_repeat_dir = static_cast<int>(stick_dir);
                left_stick_next_repeat_time = now + initial_delay;
            } else if (now >= left_stick_next_repeat_time) {
                stick_pulse = true;
                left_stick_next_repeat_time = now + repeat_interval;
            }
            if (stick_pulse) {
                switch (stick_dir) {
                case StickNavDirection::Left:
                    stick_nav_left = true;
                    break;
                case StickNavDirection::Right:
                    stick_nav_right = true;
                    break;
                case StickNavDirection::Up:
                    stick_nav_up = true;
                    break;
                case StickNavDirection::Down:
                    stick_nav_down = true;
                    break;
                case StickNavDirection::None:
                default:
                    break;
                }
            }
        }
        const bool nav_left = (allow_osk_shortcuts && gamepad_nav_left) ||
                              (allow_osk_shortcuts && (virtual_nav_left || stick_nav_left));
        const bool nav_right = (allow_osk_shortcuts && gamepad_nav_right) ||
                               (allow_osk_shortcuts && (virtual_nav_right || stick_nav_right));
        const bool nav_up = (allow_osk_shortcuts && gamepad_nav_up) ||
                            (allow_osk_shortcuts && (virtual_nav_up || stick_nav_up));
        const bool nav_down = (allow_osk_shortcuts && gamepad_nav_down) ||
                              (allow_osk_shortcuts && (virtual_nav_down || stick_nav_down));
        const bool cancel_shortcut_pressed =
            allow_osk_shortcuts &&
            (IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Circle));

        const bool gamepad_control_input =
            allow_osk_shortcuts &&
            (gamepad_nav_left || gamepad_nav_right || gamepad_nav_up || gamepad_nav_down ||
             stick_nav_left || stick_nav_right || stick_nav_up || stick_nav_down ||
             IsKeyPressed(ImGuiKey_GamepadFaceDown, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceUp, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceLeft, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
             IsKeyPressed(ImGuiKey_GamepadL1, false) || IsKeyPressed(ImGuiKey_GamepadR1, false) ||
             IsKeyPressed(ImGuiKey_GamepadL2, false) || IsKeyPressed(ImGuiKey_GamepadR2, false) ||
             IsKeyPressed(ImGuiKey_GamepadL3, false) || IsKeyPressed(ImGuiKey_GamepadR3, false));
        const bool virtual_control_input =
            allow_osk_shortcuts &&
            (virtual_nav_left || virtual_nav_right || virtual_nav_up || virtual_nav_down ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Cross) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Circle) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R1) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3));
        const bool osk_control_input = gamepad_control_input || virtual_control_input ||
                                       (allow_osk_shortcuts && panel_activate_pressed_raw);
        const ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        const bool pointer_input = IsMouseClicked(ImGuiMouseButton_Left, false) ||
                                   IsMouseClicked(ImGuiMouseButton_Right, false) ||
                                   IsMouseClicked(ImGuiMouseButton_Middle, false) ||
                                   (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f);
        if (pointer_input) {
            pointer_navigation_active = true;
        }
        if (osk_control_input) {
            pointer_navigation_active = false;
        }
        if (native_input_active && gamepad_control_input && !request_input_focus &&
            !pending_input_selection_apply) {
            native_input_active = false;
            ImGui::ClearActiveID();
        }
        if (!accept_armed) {
            if (!cross_down) {
                accept_armed = true;
                LOG_DEBUG(Lib_ImeDialog, "ImeDialog: accept armed");
            }
        }
        const bool panel_activate_pressed =
            allow_osk_shortcuts && accept_armed && panel_activate_pressed_raw;

        using SelectionIndex = Libraries::Ime::ImeSelectionGridIndex;
        const auto& selected_kb_layout = Libraries::Ime::GetImeKeyboardLayout(kb_layout_selection);
        const int keyboard_min_col = 0;
        const int keyboard_min_row = 0;
        const int keyboard_cols = std::max(1, static_cast<int>(selected_kb_layout.cols));
        const int keyboard_rows = std::max(1, static_cast<int>(selected_kb_layout.rows));
        const int keyboard_max_col = keyboard_cols - 1;
        const int keyboard_max_row = keyboard_rows - 1;

        const auto& top_layout_cfg =
            Libraries::Ime::GetImeTopPanelLayoutConfig(kb_layout_selection);
        const int top_col_min = 0;
        const int top_cols_cfg = std::max(1, static_cast<int>(top_layout_cfg.cols));
        const int top_col_max = top_col_min + top_cols_cfg - 1;

        const auto top_to_keyboard_col = [&](int top_col) {
            const int clamped_top = std::clamp(top_col, top_col_min, top_col_max);
            if (keyboard_cols <= 1 || top_cols_cfg <= 1) {
                return keyboard_min_col;
            }
            const int top_offset = clamped_top - top_col_min;
            return keyboard_min_col + (top_offset * (keyboard_cols - 1)) / (top_cols_cfg - 1);
        };
        const auto keyboard_to_top_col = [&](int keyboard_col) {
            const int clamped_kb = std::clamp(keyboard_col, keyboard_min_col, keyboard_max_col);
            if (top_cols_cfg <= 1 || keyboard_cols <= 1) {
                return top_col_min;
            }
            const int kb_offset = clamped_kb - keyboard_min_col;
            return top_col_min + (kb_offset * (top_cols_cfg - 1)) / (keyboard_cols - 1);
        };

        struct TopNavElement {
            PanelSelectionTarget target = PanelSelectionTarget::Prediction;
            int min_col = 0;
            int max_col = 0;
        };

        std::vector<int> top_col_to_element_index(static_cast<std::size_t>(top_cols_cfg), -1);
        std::vector<TopNavElement> top_elements;
        top_elements.reserve(static_cast<std::size_t>(top_cols_cfg));

        const auto append_top_element = [&](Libraries::Ime::ImeTopPanelElementId id, int min_col,
                                            int max_col) {
            PanelSelectionTarget target = PanelSelectionTarget::Prediction;
            switch (id) {
            case Libraries::Ime::ImeTopPanelElementId::Prediction:
                target = PanelSelectionTarget::Prediction;
                break;
            case Libraries::Ime::ImeTopPanelElementId::Close:
                target = PanelSelectionTarget::Close;
                break;
            default:
                return;
            }

            if (static_cast<int>(top_elements.size()) >= top_cols_cfg) {
                return;
            }
            if (max_col < min_col) {
                return;
            }

            const int clamped_min = std::clamp(min_col, top_col_min, top_col_max);
            const int clamped_max = std::clamp(max_col, top_col_min, top_col_max);
            if (clamped_max < clamped_min) {
                return;
            }

            top_elements.push_back({target, clamped_min, clamped_max});
            const int element_index = static_cast<int>(top_elements.size()) - 1;
            for (int col = clamped_min; col <= clamped_max; ++col) {
                top_col_to_element_index[static_cast<std::size_t>(col - top_col_min)] =
                    element_index;
            }
        };

        for (std::size_t i = 0; i < top_layout_cfg.element_count; ++i) {
            const auto& spec = top_layout_cfg.elements[i];
            const int min_col = std::clamp(static_cast<int>(spec.col), top_col_min, top_col_max);
            const int span = std::max(1, static_cast<int>(spec.col_span));
            const int max_col = std::clamp(min_col + span - 1, top_col_min, top_col_max);
            append_top_element(spec.id, min_col, max_col);
        }
        if (top_elements.empty()) {
            append_top_element(Libraries::Ime::ImeTopPanelElementId::Prediction, top_col_min,
                               top_col_max);
        }

        const auto element_index_for_col = [&](int col) {
            if (col < top_col_min || col > top_col_max) {
                return -1;
            }
            return top_col_to_element_index[static_cast<std::size_t>(col - top_col_min)];
        };
        const auto element_index_for_target = [&](PanelSelectionTarget target) {
            for (int i = 0; i < static_cast<int>(top_elements.size()); ++i) {
                if (top_elements[static_cast<std::size_t>(i)].target == target) {
                    return i;
                }
            }
            return -1;
        };
        const auto set_top_selection = [&](PanelSelectionTarget target, int preferred_col) {
            panel_selection = target;
            const int target_idx = element_index_for_target(target);
            if (target_idx < 0) {
                top_virtual_col = std::clamp(preferred_col, top_col_min, top_col_max);
                return;
            }
            const auto& element = top_elements[static_cast<std::size_t>(target_idx)];
            top_virtual_col = std::clamp(preferred_col, element.min_col, element.max_col);
        };
        const auto top_col_for_selection = [&](PanelSelectionTarget target) {
            const int target_idx = element_index_for_target(target);
            const int clamped_col = std::clamp(top_virtual_col, top_col_min, top_col_max);
            if (target_idx < 0) {
                return clamped_col;
            }
            const auto& element = top_elements[static_cast<std::size_t>(target_idx)];
            return std::clamp(clamped_col, element.min_col, element.max_col);
        };
        const bool menu_modal = (edit_menu_popup != EditMenuPopup::None);
        bool entered_top_from_keyboard = false;
        if (!menu_modal && !text_select_mode && !pointer_navigation_active &&
            panel_selection == PanelSelectionTarget::Keyboard) {
            const bool wrap_to_top = (nav_up && last_keyboard_selected_row == keyboard_min_row) ||
                                     (nav_down && last_keyboard_selected_row == keyboard_max_row);
            if (wrap_to_top) {
                const int top_col = keyboard_to_top_col(last_keyboard_selected_col);
                const int element_idx = element_index_for_col(top_col);
                if (element_idx >= 0) {
                    const auto& element = top_elements[static_cast<std::size_t>(element_idx)];
                    set_top_selection(element.target, top_col);
                } else if (!top_elements.empty()) {
                    const auto& first_element = top_elements[0];
                    set_top_selection(first_element.target, first_element.min_col);
                }
                entered_top_from_keyboard = true;
            }
        }

        SetWindowFontScale(std::max(ui_scale, metrics.input_font_scale));

        if (first_render) {
            const auto game_res = DebugState.game_resolution;
            const auto out_res = DebugState.output_resolution;
            const float req_w = has_layout ? static_cast<float>(layout.width)
                                           : static_cast<float>(state->panel_req_width);
            const float req_h = has_layout ? static_cast<float>(layout.height)
                                           : static_cast<float>(state->panel_req_height);
            LOG_INFO(Lib_ImeDialog,
                     "ImeDialog UI metrics: game_res={}x{}, out_res={}x{}, viewport_pos=({}, {}), "
                     "viewport_size=({}, {}), base={}x{}, scale=({:.4f}, {:.4f}), "
                     "panel_req=({}, {}), panel_scaled=({}, {})",
                     game_res.first, game_res.second, out_res.first, out_res.second,
                     viewport_offset.x, viewport_offset.y, viewport_size.x, viewport_size.y,
                     viewport.base_w, viewport.base_h, scale_x, scale_y, req_w, req_h,
                     window_size.x, window_size.y);
        }

        if (!state->title.empty()) {
            SetCursorPosY(0.0f);
            SetCursorPosX(metrics.padding_x);
            SetWindowFontScale(metrics.label_font_scale);
            TextUnformatted(state->title.data());
            SetWindowFontScale(std::max(ui_scale, metrics.input_font_scale));
        }

        bool input_hovered = false;
        if (state->is_multi_line) {
            input_hovered = DrawMultiLineInputText(metrics, pointer_navigation_active);
        } else {
            input_hovered = DrawInputText(metrics, pointer_navigation_active);
        }
        const bool input_selected =
            pointer_navigation_active && (input_hovered || native_input_active);
        auto draw_selector = [&](ImVec2 pos, ImVec2 size, bool selected) {
            if (!selected) {
                return;
            }
            const float inset = kSelectorInnerMargin;
            if (size.x <= inset * 2.0f || size.y <= inset * 2.0f) {
                return;
            }
            const ImVec2 sel_min{pos.x + inset, pos.y + inset};
            const ImVec2 sel_max{pos.x + size.x - inset, pos.y + size.y - inset};
            const float selector_corner_radius = std::max(0.0f, metrics.corner_radius - inset);
            auto* selector_draw = GetWindowDrawList();
            selector_draw->AddRectFilled(sel_min, sel_max, kSelectorOverlayColor,
                                         selector_corner_radius);
            selector_draw->AddRect(sel_min, sel_max, kSelectorBorderColor, selector_corner_radius,
                                   0, kSelectorBorderThickness);
        };
        draw_selector(metrics.input_pos_screen, metrics.input_size, input_selected);

        auto* draw = GetWindowDrawList();
        const ImU32 pane_bg = Libraries::Ime::ImeColorToImU32(state->style_config.color_base);
        const ImU32 pane_border = Libraries::Ime::ImeColorToImU32(state->style_config.color_line);
        draw->AddRectFilled(metrics.predict_pos,
                            {metrics.predict_pos.x + metrics.predict_size.x,
                             metrics.predict_pos.y + metrics.predict_size.y},
                            pane_bg, metrics.corner_radius);
        draw->AddRect(metrics.predict_pos,
                      {metrics.predict_pos.x + metrics.predict_size.x,
                       metrics.predict_pos.y + metrics.predict_size.y},
                      pane_border, metrics.corner_radius);
        SetCursorScreenPos(metrics.predict_pos);
        PushID("##ImeDialogPredict");
        PushItemFlag(ImGuiItemFlags_NoNav, true);
        InvisibleButton("##ImeDialogPredict", metrics.predict_size);
        PopItemFlag();
        const bool prediction_clicked =
            IsMouseClicked(ImGuiMouseButton_Left, false) && IsItemHovered();
        const int prediction_element_idx =
            element_index_for_target(PanelSelectionTarget::Prediction);
        if (pointer_navigation_active && prediction_clicked && prediction_element_idx >= 0) {
            const auto& prediction_element =
                top_elements[static_cast<std::size_t>(prediction_element_idx)];
            set_top_selection(PanelSelectionTarget::Prediction,
                              SelectionIndex::GridColumnFromX(
                                  io.MousePos.x, metrics.predict_pos.x, metrics.predict_size.x,
                                  prediction_element.min_col, prediction_element.max_col));
        }
        PopID();
        draw_selector(metrics.predict_pos, metrics.predict_size,
                      panel_selection == PanelSelectionTarget::Prediction);
        const ImU32 close_button_bg =
            Libraries::Ime::ImeColorToImU32(state->style_config.color_button_function);
        PushStyleColor(ImGuiCol_Button, BrightenColor(close_button_bg, 0.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, BrightenColor(close_button_bg, 0.08f));
        PushStyleColor(ImGuiCol_ButtonActive, BrightenColor(close_button_bg, 0.16f));
        SetCursorScreenPos(metrics.close_pos);
        PushItemFlag(ImGuiItemFlags_NoNav, true);
        bool cancel_pressed =
            Button("X##ImeDialogClose", {metrics.close_size.x, metrics.close_size.y});
        PopItemFlag();
        cancel_pressed = cancel_pressed || cancel_shortcut_pressed;
        const bool close_clicked = IsMouseClicked(ImGuiMouseButton_Left, false) && IsItemHovered();
        const int close_element_idx = element_index_for_target(PanelSelectionTarget::Close);
        if (pointer_navigation_active && close_clicked && close_element_idx >= 0) {
            const auto& close_element = top_elements[static_cast<std::size_t>(close_element_idx)];
            set_top_selection(PanelSelectionTarget::Close, close_element.min_col);
        }
        PopStyleColor(3);
        draw_selector(metrics.close_pos, metrics.close_size,
                      panel_selection == PanelSelectionTarget::Close);
        if (!cancel_pressed && panel_selection == PanelSelectionTarget::Close &&
            panel_activate_pressed) {
            cancel_pressed = true;
        }

        SetCursorScreenPos(metrics.kb_pos);

        bool entered_keyboard_from_top = false;
        const auto move_top_navigation = [&](int dir_x, int dir_y) {
            const PanelSelectionTarget current = panel_selection;
            int col = top_col_for_selection(current);
            int origin_element_idx = element_index_for_col(col);
            if (origin_element_idx < 0) {
                origin_element_idx = element_index_for_target(current);
            }

            if (dir_x != 0) {
                if (origin_element_idx >= 0) {
                    const int span = top_col_max - top_col_min + 1;
                    for (int step = 1; step <= span; ++step) {
                        const int next_col =
                            top_col_min + (col - top_col_min + dir_x * step + span) % span;
                        const int next_element_idx = element_index_for_col(next_col);
                        if (next_element_idx < 0 || next_element_idx == origin_element_idx) {
                            continue;
                        }
                        const auto& next_element =
                            top_elements[static_cast<std::size_t>(next_element_idx)];
                        set_top_selection(next_element.target, next_col);
                        return;
                    }
                }
            }

            if (origin_element_idx >= 0) {
                if (dir_y > 0) {
                    pending_keyboard_row = keyboard_min_row;
                    pending_keyboard_col = top_to_keyboard_col(col);
                    panel_selection = PanelSelectionTarget::Keyboard;
                    entered_keyboard_from_top = true;
                    return;
                }
                if (dir_y < 0) {
                    pending_keyboard_row = keyboard_max_row;
                    pending_keyboard_col = top_to_keyboard_col(col);
                    panel_selection = PanelSelectionTarget::Keyboard;
                    entered_keyboard_from_top = true;
                    return;
                }
            }
        };
        if (!menu_modal && !text_select_mode && !pointer_navigation_active &&
            !entered_top_from_keyboard && panel_selection != PanelSelectionTarget::Keyboard) {
            if (nav_left) {
                move_top_navigation(-1, 0);
            } else if (nav_right) {
                move_top_navigation(1, 0);
            } else if (nav_up) {
                move_top_navigation(0, -1);
            } else if (nav_down) {
                move_top_navigation(0, 1);
            }
        }

        bool accept_pressed = false;
        const auto text_length_utf16 = [&]() {
            const char* text = state->current_text.begin();
            const int byte_len = static_cast<int>(state->current_text.size());
            return Utf16CountFromUtf8Range(text, text ? (text + byte_len) : nullptr);
        };
        const auto sync_text_buffers = [&]() {
            state->input_changed = true;
            (void)state->CopyTextToOrbisBuffer(false);
        };
        const auto apply_selection_state = [&]() {
            const int len = text_length_utf16();
            const int caret = std::clamp(
                (text_select_focus_utf16 >= 0)
                    ? text_select_focus_utf16
                    : ((state->caret_index >= 0) ? state->caret_index : input_cursor_utf16),
                0, len);
            const int anchor =
                text_select_mode ? std::clamp(text_select_anchor_utf16, 0, len) : caret;
            const int focus =
                text_select_mode ? std::clamp(text_select_focus_utf16, 0, len) : caret;
            const char* text = state->current_text.begin();
            const int anchor_byte = Utf8ByteIndexFromUtf16Index(text ? text : "", anchor);
            const int focus_byte = Utf8ByteIndexFromUtf16Index(text ? text : "", focus);
            input_cursor_utf16 = focus;
            input_cursor_byte = focus_byte;
            input_selection_start_byte = std::min(anchor_byte, focus_byte);
            input_selection_end_byte = std::max(anchor_byte, focus_byte);
            state->caret_index = focus;
            state->caret_byte_index = focus_byte;
            state->caret_dirty = native_input_active;
            if (native_input_active) {
                pending_input_selection_apply = true;
                request_input_focus = true;
            } else {
                pending_input_selection_apply = false;
                request_input_focus = false;
            }
        };
        const auto apply_text_edit = [&](int start_utf16, int end_utf16,
                                         const char* insert_utf8) -> bool {
            const std::string current = state->current_text.to_string();
            const char* current_text = current.c_str();
            const int len_utf16 = Utf16CountFromUtf8Range(
                current_text, current_text + static_cast<int>(current.size()));
            const int start = std::clamp(start_utf16, 0, len_utf16);
            const int end = std::clamp(end_utf16, start, len_utf16);
            const int removed_utf16 = end - start;
            const int available_utf16 =
                static_cast<int>(state->max_text_length) - (len_utf16 - removed_utf16);

            std::string insert_clamped{};
            if (insert_utf8 && insert_utf8[0] != '\0' && available_utf16 > 0) {
                const char* p = insert_utf8;
                int used_utf16 = 0;
                while (*p && used_utf16 < available_utf16) {
                    unsigned int codepoint = 0;
                    const int step = ImTextCharFromUtf8(&codepoint, p, nullptr);
                    if (step <= 0) {
                        break;
                    }
                    const int units = (codepoint > 0xFFFF) ? 2 : 1;
                    if (used_utf16 + units > available_utf16) {
                        break;
                    }
                    insert_clamped.append(p, static_cast<std::size_t>(step));
                    p += step;
                    used_utf16 += units;
                }
            }

            if (removed_utf16 == 0 && insert_clamped.empty()) {
                return false;
            }

            const int start_byte = Utf8ByteIndexFromUtf16Index(current_text, start);
            const int end_byte = Utf8ByteIndexFromUtf16Index(current_text, end);
            std::string updated{};
            updated.reserve(current.size() - static_cast<std::size_t>(end_byte - start_byte) +
                            insert_clamped.size());
            updated.append(current, 0, static_cast<std::size_t>(start_byte));
            updated.append(insert_clamped);
            updated.append(current, static_cast<std::size_t>(end_byte), std::string::npos);

            state->current_text.FromString(updated);
            if (state->is_multi_line) {
                (void)state->NormalizeNewlines();
            }
            (void)state->ClampCurrentTextToMaxLen();
            sync_text_buffers();

            const int inserted_utf16 = Utf16CountFromUtf8Range(
                insert_clamped.c_str(),
                insert_clamped.c_str() + static_cast<int>(insert_clamped.size()));
            const int new_caret_utf16 = std::clamp(start + inserted_utf16, 0, text_length_utf16());
            text_select_mode = false;
            text_select_anchor_utf16 = new_caret_utf16;
            text_select_focus_utf16 = new_caret_utf16;
            apply_selection_state();
            return true;
        };
        const auto selection_utf16_range = [&]() -> std::pair<int, int> {
            const int len = text_length_utf16();
            if (!text_select_mode) {
                const int caret = std::clamp(state->caret_index, 0, len);
                return {caret, caret};
            }
            const int anchor = std::clamp(text_select_anchor_utf16, 0, len);
            const int focus = std::clamp(text_select_focus_utf16, 0, len);
            return {std::min(anchor, focus), std::max(anchor, focus)};
        };
        const auto insert_text_at_caret = [&](const char* suffix) {
            const auto [sel_start, sel_end] = selection_utf16_range();
            return apply_text_edit(sel_start, sel_end, suffix);
        };
        const auto backspace_at_caret = [&]() {
            const auto [sel_start, sel_end] = selection_utf16_range();
            if (sel_end > sel_start) {
                return apply_text_edit(sel_start, sel_end, "");
            }
            if (sel_end <= 0) {
                return false;
            }
            const std::string current = state->current_text.to_string();
            const char* text = current.c_str();
            const int caret_byte = Utf8ByteIndexFromUtf16Index(text, sel_end);
            int prev_byte = caret_byte;
            do {
                --prev_byte;
            } while (prev_byte > 0 &&
                     (static_cast<unsigned char>(text[prev_byte]) & 0xC0u) == 0x80u);
            const int prev_utf16 = Utf16CountFromUtf8Range(text, text + prev_byte);
            return apply_text_edit(prev_utf16, sel_end, "");
        };
        const auto clear_all_text = [&]() {
            const int len = text_length_utf16();
            if (len <= 0) {
                return false;
            }
            // Matches libSceIme backend all-delete behavior (sceImeBackendAllDeleteConvertString):
            // clear the whole editable string in one operation.
            return apply_text_edit(0, len, "");
        };

        Libraries::Ime::ImeKbGridLayout kb_layout{};
        kb_layout.pos = metrics.kb_pos;
        kb_layout.size = metrics.kb_size;
        kb_layout.key_gap_x = metrics.key_gap;
        kb_layout.key_gap_y = metrics.key_gap;
        kb_layout.cols = keyboard_cols;
        kb_layout.rows = keyboard_rows;
        const int layout_rows = std::max(1, kb_layout.rows);
        const float computed_key_h =
            (kb_layout.size.y - kb_layout.key_gap_y * static_cast<float>(layout_rows - 1)) /
            static_cast<float>(layout_rows);
        kb_layout.key_h = std::max(8.0f, computed_key_h);
        kb_layout.corner_radius = metrics.corner_radius;

        Libraries::Ime::ImeKbDrawParams kb_params{};
        kb_params.selection = kb_layout_selection;
        kb_params.layout_model = &selected_kb_layout;
        kb_params.supported_languages = state->supported_languages;
        kb_params.enter_label = state->enter_label;
        kb_params.show_selection_highlight = (panel_selection == PanelSelectionTarget::Keyboard);
        kb_params.allow_nav_input = allow_osk_shortcuts && !menu_modal && !text_select_mode &&
                                    (panel_selection == PanelSelectionTarget::Keyboard) &&
                                    !entered_keyboard_from_top;
        kb_params.allow_activate_input = allow_osk_shortcuts && accept_armed && !menu_modal &&
                                         !text_select_mode &&
                                         (panel_selection == PanelSelectionTarget::Keyboard);
        kb_params.external_nav_left = allow_osk_shortcuts && (virtual_nav_left || stick_nav_left);
        kb_params.external_nav_right =
            allow_osk_shortcuts && (virtual_nav_right || stick_nav_right);
        kb_params.external_nav_up = allow_osk_shortcuts && (virtual_nav_up || stick_nav_up);
        kb_params.external_nav_down = allow_osk_shortcuts && (virtual_nav_down || stick_nav_down);
        kb_params.external_activate_pressed = panel_activate_pressed;
        kb_params.requested_selected_row = pending_keyboard_row;
        kb_params.requested_selected_col = pending_keyboard_col;
        Libraries::Ime::ApplyImeStyleToKeyboardDrawParams(state->style_config, kb_params);
        pending_keyboard_row = -1;
        pending_keyboard_col = -1;

        Libraries::Ime::ImeKbDrawState kb_state{};
        SetWindowFontScale(metrics.key_font_scale);
        Libraries::Ime::DrawImeKeyboardGrid(kb_layout, kb_params, kb_state);
        SetWindowFontScale(metrics.input_font_scale);
        if (kb_state.selected_row >= 0 && kb_state.selected_col >= 0) {
            last_keyboard_selected_row = kb_state.selected_row;
            last_keyboard_selected_col = kb_state.selected_col;
        }
        if (pointer_navigation_active && kb_state.clicked) {
            panel_selection = PanelSelectionTarget::Keyboard;
        }

        const auto cycle_case_state = [&]() {
            switch (kb_layout_selection.case_state) {
            case Libraries::Ime::ImeKbCaseState::Lower:
                kb_layout_selection.case_state = Libraries::Ime::ImeKbCaseState::Upper;
                break;
            case Libraries::Ime::ImeKbCaseState::Upper:
                kb_layout_selection.case_state = Libraries::Ime::ImeKbCaseState::CapsLock;
                break;
            case Libraries::Ime::ImeKbCaseState::CapsLock:
            default:
                kb_layout_selection.case_state = Libraries::Ime::ImeKbCaseState::Lower;
                break;
            }
        };
        const auto set_family_and_reset_page = [&](Libraries::Ime::ImeKbLayoutFamily family) {
            kb_layout_selection.family = family;
            kb_layout_selection.page = 0;
            if (family == Libraries::Ime::ImeKbLayoutFamily::Latin ||
                family == Libraries::Ime::ImeKbLayoutFamily::Specials) {
                kb_alpha_family = family;
            }
        };
        const auto toggle_family_mode = [&](Libraries::Ime::ImeKbLayoutFamily target_family) {
            if (target_family == Libraries::Ime::ImeKbLayoutFamily::Symbols) {
                if (kb_layout_selection.family == Libraries::Ime::ImeKbLayoutFamily::Symbols) {
                    set_family_and_reset_page(kb_alpha_family);
                } else {
                    if (kb_layout_selection.family == Libraries::Ime::ImeKbLayoutFamily::Latin ||
                        kb_layout_selection.family == Libraries::Ime::ImeKbLayoutFamily::Specials) {
                        kb_alpha_family = kb_layout_selection.family;
                    }
                    set_family_and_reset_page(Libraries::Ime::ImeKbLayoutFamily::Symbols);
                }
                return;
            }
            if (kb_layout_selection.family == target_family) {
                set_family_and_reset_page(Libraries::Ime::ImeKbLayoutFamily::Latin);
            } else {
                set_family_and_reset_page(target_family);
            }
        };
        const auto flip_mode_page = [&](int direction) {
            if (kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols &&
                kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Specials) {
                return;
            }
            const int page = static_cast<int>(kb_layout_selection.page);
            kb_layout_selection.page = static_cast<u8>((page + direction + 2) % 2);
        };
        const auto consume_temporary_uppercase = [&](bool typed_character) {
            if (typed_character &&
                kb_layout_selection.case_state == Libraries::Ime::ImeKbCaseState::Upper) {
                kb_layout_selection.case_state = Libraries::Ime::ImeKbCaseState::Lower;
            }
        };
        const auto has_clipboard_text = [&]() {
            const char* text = ImGui::GetClipboardText();
            return text && text[0] != '\0';
        };
        const auto get_selection_byte_range = [&]() {
            const int text_len = static_cast<int>(state->current_text.size());
            const int sel_start = std::clamp(input_selection_start_byte, 0, text_len);
            const int sel_end = std::clamp(input_selection_end_byte, 0, text_len);
            return std::pair{std::min(sel_start, sel_end), std::max(sel_start, sel_end)};
        };
        const auto copy_selected_text = [&]() {
            const std::string current = state->current_text.to_string();
            const auto [sel_start, sel_end] = get_selection_byte_range();
            if (sel_end > sel_start) {
                const std::string selection =
                    current.substr(static_cast<std::size_t>(sel_start),
                                   static_cast<std::size_t>(sel_end - sel_start));
                ImGui::SetClipboardText(selection.c_str());
            } else {
                ImGui::SetClipboardText(current.c_str());
            }
        };
        const auto collapse_selection_to_caret = [&](int caret_utf16) {
            const int len = text_length_utf16();
            const int clamped_caret = std::clamp(caret_utf16, 0, len);
            text_select_mode = false;
            text_select_anchor_utf16 = clamped_caret;
            text_select_focus_utf16 = clamped_caret;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto move_text_caret = [&](int delta_utf16, bool preserve_selection) {
            const int len = text_length_utf16();
            int base = text_select_mode ? text_select_focus_utf16 : input_cursor_utf16;
            if (base < 0) {
                base = input_cursor_utf16;
            }
            base = std::clamp(base, 0, len);
            const int next = std::clamp(base + delta_utf16, 0, len);
            if (preserve_selection && text_select_mode) {
                text_select_focus_utf16 = next;
                apply_selection_state();
                panel_selection = PanelSelectionTarget::Keyboard;
                return next != base;
            }
            collapse_selection_to_caret(next);
            return next != base;
        };
        const auto begin_text_selection_from_caret = [&]() {
            text_select_mode = true;
            const int len = text_length_utf16();
            const int caret = std::clamp(input_cursor_utf16, 0, len);
            text_select_anchor_utf16 = caret;
            text_select_focus_utf16 = caret;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto select_all_text = [&]() {
            text_select_mode = true;
            const int len = text_length_utf16();
            text_select_anchor_utf16 = 0;
            text_select_focus_utf16 = len;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto open_main_menu = [&]() {
            edit_menu_popup = EditMenuPopup::Main;
            edit_menu_index = 0;
        };
        const auto open_actions_menu = [&]() {
            edit_menu_popup = EditMenuPopup::Actions;
            edit_menu_index = 0;
        };
        const auto apply_main_menu_action = [&](int action_index) {
            switch (action_index) {
            case 0: // Select
                begin_text_selection_from_caret();
                edit_menu_popup = EditMenuPopup::None;
                break;
            case 1: // Select All
                select_all_text();
                open_actions_menu();
                break;
            case 2: // Paste
                if (has_clipboard_text()) {
                    (void)insert_text_at_caret(ImGui::GetClipboardText());
                }
                edit_menu_popup = EditMenuPopup::None;
                break;
            default:
                break;
            }
        };
        const auto apply_actions_menu_action = [&](int action_index) {
            switch (action_index) {
            case 0: // Copy
                copy_selected_text();
                collapse_selection_to_caret(text_select_focus_utf16 >= 0 ? text_select_focus_utf16
                                                                         : input_cursor_utf16);
                edit_menu_popup = EditMenuPopup::None;
                break;
            case 1: // Paste
                if (has_clipboard_text()) {
                    (void)insert_text_at_caret(ImGui::GetClipboardText());
                }
                edit_menu_popup = EditMenuPopup::None;
                break;
            default:
                break;
            }
        };

        bool opened_menu_this_frame = false;
        if (!(allow_osk_shortcuts && !menu_modal)) {
            prev_virtual_square_down = false;
            prev_virtual_l1_down = false;
            prev_virtual_r1_down = false;
            virtual_square_next_repeat_time = 0.0;
            virtual_l1_next_repeat_time = 0.0;
            virtual_r1_next_repeat_time = 0.0;
        }
        if (allow_osk_shortcuts && !menu_modal &&
            kb_state.pressed_action == Libraries::Ime::ImeKbKeyAction::None) {
            const bool l1_down = IsKeyDown(ImGuiKey_GamepadL1) ||
                                 virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::L1);
            const bool l1_edge_pressed =
                IsKeyPressed(ImGuiKey_GamepadL1, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1);
            const bool l1_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadL1, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1,
                                       prev_virtual_l1_down, virtual_l1_next_repeat_time);
            const bool square_down = IsKeyDown(ImGuiKey_GamepadFaceLeft) ||
                                     virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Square);
            const bool square_edge_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceLeft, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square);
            const bool square_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceLeft, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square,
                                       prev_virtual_square_down, virtual_square_next_repeat_time);
            const bool r1_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadR1, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R1,
                                       prev_virtual_r1_down, virtual_r1_next_repeat_time);
            const bool clear_all_shortcut_pressed =
                (l1_down && square_edge_pressed) || (square_down && l1_edge_pressed);
            const bool l2_down = IsKeyDown(ImGuiKey_GamepadL2) ||
                                 virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::L2);
            const bool l2_pressed = IsKeyPressed(ImGuiKey_GamepadL2, false) ||
                                    virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2);
            const bool tri_down = IsKeyDown(ImGuiKey_GamepadFaceUp) ||
                                  virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
            const bool tri_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceUp, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
            const bool symbols_shortcut_pressed =
                (l2_down && tri_pressed) || (tri_down && l2_pressed);
            if (symbols_shortcut_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::SymbolsMode;
            } else if (tri_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Space;
            } else if (kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols &&
                       (IsKeyPressed(ImGuiKey_GamepadL3, false) ||
                        virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3))) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::SpecialsMode;
            } else if (IsKeyPressed(ImGuiKey_GamepadR2, false) ||
                       virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2)) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Done;
            } else if (clear_all_shortcut_pressed) {
                (void)clear_all_text();
            } else if (square_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Backspace;
            } else if (l1_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::ArrowLeft;
            } else if (r1_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::ArrowRight;
            } else if (IsKeyPressed(ImGuiKey_GamepadR3, false) ||
                       virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3)) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Settings;
            } else if (kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols &&
                       (IsKeyPressed(ImGuiKey_GamepadL2, false) ||
                        virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2))) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Shift;
            }
        }
        switch (kb_state.pressed_action) {
        case Libraries::Ime::ImeKbKeyAction::Character:
            consume_temporary_uppercase(insert_text_at_caret(kb_state.pressed_label));
            break;
        case Libraries::Ime::ImeKbKeyAction::Shift:
            cycle_case_state();
            break;
        case Libraries::Ime::ImeKbKeyAction::SymbolsMode:
            toggle_family_mode(Libraries::Ime::ImeKbLayoutFamily::Symbols);
            break;
        case Libraries::Ime::ImeKbKeyAction::SpecialsMode:
            toggle_family_mode(Libraries::Ime::ImeKbLayoutFamily::Specials);
            break;
        case Libraries::Ime::ImeKbKeyAction::ArrowLeft:
            (void)move_text_caret(-1, text_select_mode);
            break;
        case Libraries::Ime::ImeKbKeyAction::ArrowRight:
            (void)move_text_caret(1, text_select_mode);
            break;
        case Libraries::Ime::ImeKbKeyAction::PagePrev:
            flip_mode_page(-1);
            break;
        case Libraries::Ime::ImeKbKeyAction::PageNext:
            flip_mode_page(1);
            break;
        case Libraries::Ime::ImeKbKeyAction::Space:
            (void)insert_text_at_caret(" ");
            break;
        case Libraries::Ime::ImeKbKeyAction::Backspace:
            (void)backspace_at_caret();
            break;
        case Libraries::Ime::ImeKbKeyAction::NewLine:
            if (state->is_multi_line) {
                (void)insert_text_at_caret("\n");
            } else {
                accept_pressed = true;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Menu:
            if (edit_menu_popup == EditMenuPopup::None) {
                open_main_menu();
                opened_menu_this_frame = true;
            } else {
                edit_menu_popup = EditMenuPopup::None;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Settings:
            pointer_navigation_active = !pointer_navigation_active;
            if (!pointer_navigation_active) {
                if (native_input_active) {
                    native_input_active = false;
                    ImGui::ClearActiveID();
                }
                panel_selection = PanelSelectionTarget::Keyboard;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Done:
            accept_pressed = true;
            break;
        default:
            break;
        }
        if (kb_state.done_pressed) {
            accept_pressed = true;
        }

        if (text_select_mode && edit_menu_popup == EditMenuPopup::None &&
            !pointer_navigation_active) {
            int delta = 0;
            if (nav_left || nav_up) {
                delta = -1;
            } else if (nav_right || nav_down) {
                delta = 1;
            }
            if (delta != 0) {
                const int len = text_length_utf16();
                if (text_select_anchor_utf16 < 0 || text_select_focus_utf16 < 0) {
                    const int caret = std::clamp(input_cursor_utf16, 0, len);
                    text_select_anchor_utf16 = caret;
                    text_select_focus_utf16 = caret;
                }
                text_select_focus_utf16 = std::clamp(text_select_focus_utf16 + delta, 0, len);
                apply_selection_state();
            } else if (panel_activate_pressed) {
                open_actions_menu();
                opened_menu_this_frame = true;
            }
        }

        if (edit_menu_popup != EditMenuPopup::None && cancel_pressed) {
            cancel_pressed = false;
            edit_menu_popup = EditMenuPopup::None;
        } else if (text_select_mode && cancel_pressed) {
            cancel_pressed = false;
            text_select_mode = false;
            const int len = text_length_utf16();
            const int caret = std::clamp(text_select_focus_utf16, 0, len);
            text_select_anchor_utf16 = caret;
            text_select_focus_utf16 = caret;
            apply_selection_state();
        }

        if (edit_menu_popup != EditMenuPopup::None) {
            constexpr std::array<const char*, 3> kMainMenuItems = {"Select", "Select All", "Paste"};
            constexpr std::array<const char*, 2> kActionMenuItems = {"Copy", "Paste"};
            const bool is_main_menu = (edit_menu_popup == EditMenuPopup::Main);
            const int item_count = is_main_menu ? static_cast<int>(kMainMenuItems.size())
                                                : static_cast<int>(kActionMenuItems.size());
            const bool clipboard_ready = has_clipboard_text();
            const auto item_label = [&](int index) -> const char* {
                return is_main_menu ? kMainMenuItems[static_cast<std::size_t>(index)]
                                    : kActionMenuItems[static_cast<std::size_t>(index)];
            };
            const auto item_enabled = [&](int index) {
                if (is_main_menu && index == 2) {
                    return clipboard_ready;
                }
                if (!is_main_menu && index == 1) {
                    return clipboard_ready;
                }
                return true;
            };

            if (!pointer_navigation_active) {
                if (nav_up) {
                    edit_menu_index = (edit_menu_index + item_count - 1) % item_count;
                } else if (nav_down) {
                    edit_menu_index = (edit_menu_index + 1) % item_count;
                }
            }
            edit_menu_index = std::clamp(edit_menu_index, 0, item_count - 1);

            const float menu_w = std::min(metrics.kb_size.x * 0.42f, 280.0f);
            const float menu_inner_pad = std::max(6.0f, metrics.key_gap * 0.7f);
            const float item_gap = std::max(3.0f, metrics.key_gap * 0.35f);
            const float item_h = std::max(28.0f, metrics.key_h * 0.70f);
            const float menu_h = menu_inner_pad * 2.0f + item_h * static_cast<float>(item_count) +
                                 item_gap * static_cast<float>(item_count - 1);
            const ImVec2 menu_pos{
                metrics.kb_pos.x + (metrics.kb_size.x - menu_w) * 0.5f,
                metrics.kb_pos.y + (metrics.kb_size.y - menu_h) * 0.5f,
            };
            const ImVec2 menu_max{menu_pos.x + menu_w, menu_pos.y + menu_h};
            draw->AddRectFilled(menu_pos, menu_max, IM_COL32(16, 16, 16, 245),
                                metrics.corner_radius);
            draw->AddRect(menu_pos, menu_max, IM_COL32(100, 100, 100, 255), metrics.corner_radius);

            const bool menu_activate = !opened_menu_this_frame && panel_activate_pressed;
            bool click_activate = false;
            for (int i = 0; i < item_count; ++i) {
                const float item_y = menu_pos.y + menu_inner_pad + i * (item_h + item_gap);
                const ImVec2 item_pos{menu_pos.x + menu_inner_pad, item_y};
                const ImVec2 item_size{menu_w - menu_inner_pad * 2.0f, item_h};
                const bool selected = (i == edit_menu_index);
                const bool enabled = item_enabled(i);
                const ImU32 item_bg = !enabled   ? IM_COL32(30, 30, 30, 255)
                                      : selected ? IM_COL32(60, 96, 146, 255)
                                                 : IM_COL32(45, 45, 45, 255);
                draw->AddRectFilled(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y},
                                    item_bg, metrics.corner_radius * 0.6f);
                draw->AddRect(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y},
                              IM_COL32(90, 90, 90, 255), metrics.corner_radius * 0.6f);

                ImGui::PushID(2000 + i);
                ImGui::SetCursorScreenPos(item_pos);
                ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
                ImGui::InvisibleButton("##ImeDialogEditMenuItem", item_size);
                ImGui::PopItemFlag();
                if (ImGui::IsItemHovered()) {
                    edit_menu_index = i;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && enabled) {
                    edit_menu_index = i;
                    click_activate = true;
                }
                ImGui::PopID();

                const char* label = item_label(i);
                const ImVec2 text_size = ImGui::CalcTextSize(label);
                const ImVec2 text_pos{
                    item_pos.x + (item_size.x - text_size.x) * 0.5f,
                    item_pos.y + (item_size.y - text_size.y) * 0.5f,
                };
                const ImU32 text_col =
                    enabled ? IM_COL32(232, 232, 232, 255) : IM_COL32(128, 128, 128, 255);
                draw->AddText(text_pos, text_col, label);
            }

            if (pointer_navigation_active && ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) &&
                !ImGui::IsMouseHoveringRect(menu_pos, menu_max, false)) {
                edit_menu_popup = EditMenuPopup::None;
            } else if (menu_activate || click_activate) {
                if (item_enabled(edit_menu_index)) {
                    const EditMenuPopup previous_popup = edit_menu_popup;
                    if (previous_popup == EditMenuPopup::Main) {
                        apply_main_menu_action(edit_menu_index);
                    } else {
                        apply_actions_menu_action(edit_menu_index);
                    }
                    if (edit_menu_popup != EditMenuPopup::None &&
                        edit_menu_popup != previous_popup) {
                        opened_menu_this_frame = true;
                    }
                }
            }
        }

        Dummy({metrics.kb_size.x, metrics.kb_size.y + metrics.padding_bottom});

        if (accept_pressed) {
            LOG_INFO(Lib_ImeDialog, "ImeDialog OK text(len={}): \"{}\"", state->current_text.size(),
                     state->current_text.begin());
            FinishDialog(OrbisImeDialogEndStatus::Ok, false, "OK");
        } else if (cancel_pressed) {
            FinishDialog(OrbisImeDialogEndStatus::UserCanceled, true, "Cancel");
        }
        prev_virtual_buttons = virtual_buttons;
        prev_virtual_cross_down = cross_down;
        prev_virtual_lstick_left_down = virtual_lstick_dirs.left;
        prev_virtual_lstick_right_down = virtual_lstick_dirs.right;
        prev_virtual_lstick_up_down = virtual_lstick_dirs.up;
        prev_virtual_lstick_down_down = virtual_lstick_dirs.down;
        lock_window_scroll();
        SetWindowFontScale(1.0f);
    }
    End();

    first_render = false;
}

bool ImeDialogUi::DrawInputText(const Libraries::Ime::ImePanelMetrics& metrics,
                                bool pointer_selection_enabled) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    if (request_input_focus) {
        SetKeyboardFocusHere();
        request_input_focus = false;
    }

    if (state->caret_dirty && !text_select_mode) {
        const char* text = state->current_text.begin();
        const int len_utf16 =
            Utf16CountFromUtf8Range(text, text + static_cast<int>(state->current_text.size()));
        int caret_utf16 = state->caret_index;
        if (caret_utf16 < 0) {
            caret_utf16 = 0;
        } else if (caret_utf16 > len_utf16) {
            caret_utf16 = len_utf16;
        }
        const int caret_byte = Utf8ByteIndexFromUtf16Index(text, caret_utf16);
        state->caret_index = caret_utf16;
        state->caret_byte_index = caret_byte;
        input_cursor_utf16 = caret_utf16;
        input_cursor_byte = caret_byte;
        input_selection_start_byte = caret_byte;
        input_selection_end_byte = caret_byte;
        text_select_anchor_utf16 = caret_utf16;
        text_select_focus_utf16 = caret_utf16;
        if (native_input_active) {
            pending_input_selection_apply = true;
            request_input_focus = true;
        } else {
            pending_input_selection_apply = false;
            request_input_focus = false;
            state->caret_dirty = false;
        }
    }

    const ImVec2 rect_min = metrics.input_pos_screen;
    const ImVec2 rect_max{rect_min.x + input_size.x, rect_min.y + input_size.y};
    const bool clicked_input = IsMouseClicked(ImGuiMouseButton_Left, false) &&
                               IsMouseHoveringRect(rect_min, rect_max, false);
    if (clicked_input) {
        native_input_active = true;
        SetKeyboardFocusHere();
    }

    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways;
    if (!native_input_active) {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }

    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    PushStyleColor(ImGuiCol_FrameBg,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_text_field));
    PushStyleColor(ImGuiCol_FrameBgHovered,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_preedit));
    PushStyleColor(ImGuiCol_FrameBgActive,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_preedit));
    PushStyleColor(ImGuiCol_Text, Libraries::Ime::ImeColorToImVec4(state->style_config.color_text));
    PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length * 4 + 1, input_size, flags, InputTextCallback, this)) {
        state->input_changed = true;
        const bool changed = state->NormalizeNewlines() | state->ClampCurrentTextToMaxLen();
        if (changed) {
            const int buf_len = static_cast<int>(state->current_text.size());
            const int caret_byte = std::clamp(state->caret_byte_index, 0, buf_len);
            state->caret_index = Utf16CountFromUtf8Range(state->current_text.begin(),
                                                         state->current_text.begin() + caret_byte);
            const int new_len = Utf16CountFromUtf8Range(
                state->current_text.begin(),
                state->current_text.begin() + static_cast<int>(state->current_text.size()));
            if (state->caret_index > new_len) {
                state->caret_index = new_len;
            }
            state->caret_dirty = true;
        }
        state->CopyTextToOrbisBuffer(false);
    }
    PopItemFlag();
    PopStyleColor(4);
    const ImRect frame_rect = {GetItemRectMin(), GetItemRectMax()};
    if (!IsItemActive()) {
        DrawInactiveCaretOverlay(frame_rect, state->current_text.begin(), input_cursor_byte,
                                 input_selection_start_byte, input_selection_end_byte, false);
    }
    const bool hovered = IsItemHovered();
    if (IsMouseClicked(ImGuiMouseButton_Left, false) && !hovered && native_input_active) {
        native_input_active = false;
    }
    return pointer_selection_enabled && (hovered || IsItemActive() || clicked_input);
}

bool ImeDialogUi::DrawMultiLineInputText(const Libraries::Ime::ImePanelMetrics& metrics,
                                         bool pointer_selection_enabled) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    if (request_input_focus) {
        SetKeyboardFocusHere();
        request_input_focus = false;
    }
    if (state->caret_dirty && !text_select_mode) {
        const char* text = state->current_text.begin();
        const int len_utf16 =
            Utf16CountFromUtf8Range(text, text + static_cast<int>(state->current_text.size()));
        int caret_utf16 = state->caret_index;
        if (caret_utf16 < 0) {
            caret_utf16 = 0;
        } else if (caret_utf16 > len_utf16) {
            caret_utf16 = len_utf16;
        }
        const int caret_byte = Utf8ByteIndexFromUtf16Index(text, caret_utf16);
        state->caret_index = caret_utf16;
        state->caret_byte_index = caret_byte;
        input_cursor_utf16 = caret_utf16;
        input_cursor_byte = caret_byte;
        input_selection_start_byte = caret_byte;
        input_selection_end_byte = caret_byte;
        text_select_anchor_utf16 = caret_utf16;
        text_select_focus_utf16 = caret_utf16;
        if (native_input_active) {
            pending_input_selection_apply = true;
            request_input_focus = true;
        } else {
            pending_input_selection_apply = false;
            request_input_focus = false;
            state->caret_dirty = false;
        }
    }
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackCharFilter |
                                static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline) |
                                ImGuiInputTextFlags_CallbackAlways;

    const ImVec2 rect_min = metrics.input_pos_screen;
    const ImVec2 rect_max{rect_min.x + input_size.x, rect_min.y + input_size.y};
    const bool clicked_input = IsMouseClicked(ImGuiMouseButton_Left, false) &&
                               IsMouseHoveringRect(rect_min, rect_max, false);
    if (clicked_input) {
        native_input_active = true;
        SetKeyboardFocusHere();
    }
    if (!native_input_active) {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }

    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    PushStyleColor(ImGuiCol_FrameBg,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_text_field));
    PushStyleColor(ImGuiCol_FrameBgHovered,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_preedit));
    PushStyleColor(ImGuiCol_FrameBgActive,
                   Libraries::Ime::ImeColorToImVec4(state->style_config.color_preedit));
    PushStyleColor(ImGuiCol_Text, Libraries::Ime::ImeColorToImVec4(state->style_config.color_text));
    PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length * 4 + 1, input_size, flags, InputTextCallback, this)) {
        state->input_changed = true;
        const bool changed = state->ClampCurrentTextToMaxLen();
        if (changed) {
            const int buf_len = static_cast<int>(state->current_text.size());
            const int caret_byte = std::clamp(state->caret_byte_index, 0, buf_len);
            state->caret_index = Utf16CountFromUtf8Range(state->current_text.begin(),
                                                         state->current_text.begin() + caret_byte);
            const int new_len = Utf16CountFromUtf8Range(
                state->current_text.begin(),
                state->current_text.begin() + static_cast<int>(state->current_text.size()));
            if (state->caret_index > new_len) {
                state->caret_index = new_len;
            }
            state->caret_dirty = true;
        }
        state->CopyTextToOrbisBuffer(false);
    }
    PopItemFlag();
    PopStyleColor(4);
    const ImRect frame_rect = {GetItemRectMin(), GetItemRectMax()};
    if (!IsItemActive()) {
        DrawInactiveCaretOverlay(frame_rect, state->current_text.begin(), input_cursor_byte,
                                 input_selection_start_byte, input_selection_end_byte, true);
    }
    const bool hovered = IsItemHovered();
    if (IsMouseClicked(ImGuiMouseButton_Left, false) && !hovered && native_input_active) {
        native_input_active = false;
    }
    return pointer_selection_enabled && (hovered || IsItemActive() || clicked_input);
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);
    ASSERT(ui);
    if (!ui->state) {
        return 1;
    }

    if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
        int buf_len = std::max(0, data->BufTextLen);
        if (ui->request_input_select_all) {
            data->SelectAll();
            ui->request_input_select_all = false;
        }
        if (ui->pending_input_selection_apply) {
            const int len_chars = Utf16CountFromUtf8Range(data->Buf, data->Buf + buf_len);
            int anchor = ui->text_select_anchor_utf16;
            int focus = ui->text_select_focus_utf16;
            if (anchor < 0 || focus < 0) {
                const int caret_utf16 =
                    Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
                anchor = caret_utf16;
                focus = caret_utf16;
            }
            anchor = std::clamp(anchor, 0, len_chars);
            focus = std::clamp(focus, 0, len_chars);
            const int anchor_byte = Utf8ByteIndexFromUtf16Index(data->Buf, anchor);
            const int focus_byte = Utf8ByteIndexFromUtf16Index(data->Buf, focus);
            data->CursorPos = focus_byte;
            data->SelectionStart = std::min(anchor_byte, focus_byte);
            data->SelectionEnd = std::max(anchor_byte, focus_byte);
            ui->text_select_anchor_utf16 = anchor;
            ui->text_select_focus_utf16 = focus;
            ui->pending_input_selection_apply = false;
            ui->state->caret_dirty = false;
        }
        if (data->BufTextLen > 0) {
            const int caret_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
            LOG_DEBUG(Lib_ImeDialog, "ImeDialog caret: buf_len={}, cursor_byte={}, caret_utf16={}",
                      data->BufTextLen, data->CursorPos, caret_utf16);
        }
        if (ui->state->caret_dirty && !ui->text_select_mode) {
            const int len_chars = Utf16CountFromUtf8Range(data->Buf, data->Buf + buf_len);
            int caret = ui->state->caret_index;
            if (caret < 0) {
                caret = 0;
            } else if (caret > len_chars) {
                caret = len_chars;
            }
            const int caret_byte = Utf8ByteIndexFromUtf16Index(data->Buf, caret);
            data->CursorPos = caret_byte;
            data->SelectionStart = caret_byte;
            data->SelectionEnd = caret_byte;
            ui->state->caret_dirty = false;
        }
        if (ClampInputBufferToUtf16Limit(data, static_cast<int>(ui->state->max_text_length))) {
            buf_len = std::max(0, data->BufTextLen);
        }
        const int cursor_byte = std::clamp(data->CursorPos, 0, buf_len);
        const int selection_start_byte =
            std::clamp(std::min(data->SelectionStart, data->SelectionEnd), 0, buf_len);
        const int selection_end_byte =
            std::clamp(std::max(data->SelectionStart, data->SelectionEnd), 0, buf_len);
        const int cursor_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + cursor_byte);
        ui->state->caret_byte_index = cursor_byte;
        ui->state->caret_index = cursor_utf16;
        ui->input_cursor_byte = cursor_byte;
        ui->input_cursor_utf16 = cursor_utf16;
        ui->input_selection_start_byte = selection_start_byte;
        ui->input_selection_end_byte = selection_end_byte;
        if (!ui->text_select_mode) {
            ui->text_select_anchor_utf16 = cursor_utf16;
            ui->text_select_focus_utf16 = cursor_utf16;
        }
        return 0;
    }

    LOG_DEBUG(Lib_ImeDialog, ">> InputTextCallback: EventFlag={}, EventChar={}", data->EventFlag,
              data->EventChar);

    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter &&
        RejectInputCharByUtf16Limit(data, static_cast<int>(ui->state->max_text_length))) {
        return 1;
    }

    // Should we filter punctuation?
    if (ui->state->is_numeric && (data->EventChar < '0' || data->EventChar > '9') &&
        data->EventChar != '\b' && data->EventChar != ',' && data->EventChar != '-' &&
        data->EventChar != '.') {
        LOG_INFO(Lib_ImeDialog, "InputTextCallback: rejecting non-digit char '{}'",
                 static_cast<char>(data->EventChar));
        return 1;
    }

    if (ui->state->is_multi_line && (data->EventChar == '\n' || data->EventChar == '\r')) {
        const int caret_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
        ui->state->caret_index = caret_utf16 + 1;
        ui->state->caret_dirty = true;
    }

    if (!ui->state->keyboard_filter) {
        LOG_DEBUG(Lib_ImeDialog, "InputTextCallback: no keyboard_filter, accepting char");
        return 0;
    }
    LOG_DEBUG(Lib_ImeDialog, "InputTextCallback: skipping keyboard_filter on render thread");
    return 0;

    // ImGui encodes ImWchar32 as multi-byte UTF-8 characters
    char* event_char = reinterpret_cast<char*>(&data->EventChar);

    // Call the keyboard filter
    OrbisImeKeycode src_keycode = {
        .keycode = 0,
        .character = 0,
        .status = 1,                              // ??? 1 = key pressed, 0 = key released
        .type = OrbisImeKeyboardType::ENGLISH_US, // TODO set this to the correct value (maybe use
                                                  // the current language?)
        .user_id = ui->state->user_id,
        .resource_id = 0,
        .timestamp = {0},
    };

    char16_t tmp_char[2] = {0};
    if (!ui->state->ConvertUTF8ToOrbis(event_char, 4, tmp_char, 2)) {
        LOG_ERROR(Lib_ImeDialog, "InputTextCallback: ConvertUTF8ToOrbis failed");
        return 0;
    }
    src_keycode.character = tmp_char[0];
    LOG_DEBUG(Lib_ImeDialog, "InputTextCallback: converted to Orbis char={:#X}",
              static_cast<uint16_t>(src_keycode.character));
    src_keycode.keycode = src_keycode.character; // TODO set this to the correct value

    u16 out_keycode;
    u32 out_status;

    bool keep = ui->state->CallKeyboardFilter(&src_keycode, &out_keycode, &out_status);
    LOG_DEBUG(Lib_ImeDialog,
              "InputTextCallback: CallKeyboardFilter returned %s (keycode=0x%X, status=0x%X)",
              keep ? "true" : "false", out_keycode, out_status);
    // TODO. set the keycode

    if (!keep) {
        LOG_INFO(Lib_ImeDialog, "InputTextCallback: keyboard_filter rejected char");
        return 1;
    }
    return 0;
}

} // namespace Libraries::ImeDialog
