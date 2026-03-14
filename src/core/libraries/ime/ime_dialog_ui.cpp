// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>
#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/debug_state.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_dialog_ui.h"
#include "core/libraries/ime/ime_kb_layout.h"
#include "core/memory.h"
#include "core/tls.h"
#include "imgui/imgui_std.h"

using namespace ImGui;

namespace Libraries::ImeDialog {
namespace {
bool IsMappedGuestBuffer(const void* ptr, size_t bytes) {
    if (!ptr || bytes == 0) {
        return false;
    }
    auto* memory = Core::Memory::Instance();
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
    const char* range_end = end ? end : (text + std::strlen(text));
    std::array<char16_t, ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1> tmp{};
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(tmp.data()), static_cast<int>(tmp.size()), text,
                      range_end);
    return static_cast<int>(BoundedUtf16Length(tmp.data(), tmp.size() - 1));
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

} // namespace

ImeDialogState::ImeDialogState()
    : input_changed(false), user_id(-1), is_multi_line(false), is_numeric(false),
      type(OrbisImeType::Default), enter_label(OrbisImeEnterLabel::Default), text_filter(nullptr),
      keyboard_filter(nullptr), max_text_length(ORBIS_IME_DIALOG_MAX_TEXT_LENGTH),
      text_buffer(nullptr), original_text(), title(), placeholder(), current_text() {}

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
    use_over2k = True(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES);
    is_numeric = param->type == OrbisImeType::Number;
    type = param->type;
    enter_label = param->enter_label;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->ext_keyboard_filter : nullptr;
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
      panel_req_height(other.panel_req_height), user_id(other.user_id),
      is_multi_line(other.is_multi_line), is_numeric(other.is_numeric), type(other.type),
      enter_label(other.enter_label), text_filter(other.text_filter),
      keyboard_filter(other.keyboard_filter), max_text_length(other.max_text_length),
      text_buffer(other.text_buffer), original_text(std::move(other.original_text)),
      title(std::move(other.title)), placeholder(std::move(other.placeholder)),
      current_text(other.current_text) {

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
        user_id = other.user_id;
        is_multi_line = other.is_multi_line;
        is_numeric = other.is_numeric;
        type = other.type;
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
        AddLayer(this);
    }
}

ImeDialogUi::~ImeDialogUi() {
    std::scoped_lock lock(draw_mutex);

    Free();
}

ImeDialogUi::ImeDialogUi(ImeDialogUi&& other) noexcept
    : state(other.state), status(other.status), result(other.result),
      first_render(other.first_render) {

    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
    }
}

ImeDialogUi& ImeDialogUi::operator=(ImeDialogUi&& other) {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    Free();

    state = other.state;
    status = other.status;
    result = other.result;
    first_render = other.first_render;
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
    }

    return *this;
}

void ImeDialogUi::Free() {
    RemoveLayer(this);
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

    constexpr int key_cols = 10;
    constexpr int key_rows = 6;
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

    if (has_layout) {
        float x = viewport_offset.x + layout.posx * scale_x;
        float y = viewport_offset.y + layout.posy * scale_y;
        if (layout.horizontal_alignment == OrbisImeHorizontalAlignment::Center) {
            x -= window_size.x * 0.5f;
        } else if (layout.horizontal_alignment == OrbisImeHorizontalAlignment::Right) {
            x -= window_size.x;
        }
        if (layout.vertical_alignment == OrbisImeVerticalAlignment::Center) {
            y -= window_size.y * 0.5f;
        } else if (layout.vertical_alignment == OrbisImeVerticalAlignment::Bottom) {
            y -= window_size.y;
        }
        x = std::clamp(x, viewport_offset.x,
                       viewport_offset.x + std::max(0.0f, viewport_size.x - window_size.x));
        y = std::clamp(y, viewport_offset.y,
                       viewport_offset.y + std::max(0.0f, viewport_size.y - window_size.y));
        SetNextWindowPos({x, y});
    } else {
        SetNextWindowPos({viewport_offset.x + viewport_size.x * 0.5f,
                          viewport_offset.y + viewport_size.y * 0.5f},
                         ImGuiCond_Always, {0.5f, 0.5f});
    }
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    if (Begin("IME Dialog##ImeDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
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

        if (state->is_multi_line) {
            DrawMultiLineInputText(metrics);
        } else {
            DrawInputText(metrics);
        }

        auto* draw = GetWindowDrawList();
        const ImU32 pane_bg = IM_COL32(18, 18, 18, 255);
        const ImU32 pane_border = IM_COL32(70, 70, 70, 255);
        draw->AddRectFilled(metrics.predict_pos,
                            {metrics.predict_pos.x + metrics.predict_size.x,
                             metrics.predict_pos.y + metrics.predict_size.y},
                            pane_bg, metrics.corner_radius);
        draw->AddRect(metrics.predict_pos,
                      {metrics.predict_pos.x + metrics.predict_size.x,
                       metrics.predict_pos.y + metrics.predict_size.y},
                      pane_border, metrics.corner_radius);
        PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        SetCursorScreenPos(metrics.close_pos);
        const bool cancel_pressed =
            Button("X##ImeDialogClose", {metrics.close_size.x, metrics.close_size.y});
        PopStyleColor(3);

        SetCursorScreenPos(metrics.kb_pos);

        if (!accept_armed) {
            if (!IsKeyDown(ImGuiKey_Enter) && !IsKeyDown(ImGuiKey_GamepadFaceDown)) {
                accept_armed = true;
                LOG_DEBUG(Lib_ImeDialog, "ImeDialog: accept armed");
            }
        }
        const bool allow_key_accept = accept_armed;

        bool accept_pressed =
            (allow_key_accept && !state->is_multi_line && IsKeyPressed(ImGuiKey_Enter)) ||
            (allow_key_accept && IsKeyPressed(ImGuiKey_GamepadFaceDown));

        Libraries::Ime::ImeKbGridLayout kb_layout{};
        kb_layout.pos = metrics.kb_pos;
        kb_layout.size = metrics.kb_size;
        kb_layout.key_gap_x = metrics.key_gap;
        kb_layout.key_gap_y = metrics.key_gap;
        kb_layout.key_h = metrics.key_h;
        kb_layout.cols = key_cols;
        kb_layout.rows = key_rows;
        kb_layout.corner_radius = metrics.corner_radius;

        Libraries::Ime::ImeKbDrawParams kb_params{};
        kb_params.enter_label = state->enter_label;
        kb_params.key_bg_alt = IM_COL32(45, 45, 45, 255);

        Libraries::Ime::ImeKbDrawState kb_state{};
        SetWindowFontScale(metrics.key_font_scale);
        Libraries::Ime::DrawImeKeyboardGrid(kb_layout, kb_params, kb_state);
        SetWindowFontScale(metrics.input_font_scale);
        if (kb_state.done_pressed) {
            accept_pressed = true;
        }

        Dummy({metrics.kb_size.x, metrics.kb_size.y + metrics.padding_bottom});

        if (accept_pressed) {
            LOG_INFO(Lib_ImeDialog, "ImeDialog OK text(len={}): \"{}\"", state->current_text.size(),
                     state->current_text.begin());
            FinishDialog(OrbisImeDialogEndStatus::Ok, false, "OK");
        } else if (cancel_pressed) {
            FinishDialog(OrbisImeDialogEndStatus::UserCanceled, true, "Cancel");
        }
        SetWindowFontScale(1.0f);
    }
    End();

    first_render = false;
}

void ImeDialogUi::DrawInputText(const Libraries::Ime::ImePanelMetrics& metrics) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length * 4 + 1, input_size,
                    ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways,
                    InputTextCallback, this)) {
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
}

void ImeDialogUi::DrawMultiLineInputText(const Libraries::Ime::ImePanelMetrics& metrics) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackCharFilter |
                                static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length * 4 + 1, input_size,
                    flags | ImGuiInputTextFlags_CallbackAlways, InputTextCallback, this)) {
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
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);
    ASSERT(ui);

    if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
        if (data->BufTextLen > 0) {
            const int caret_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
            LOG_DEBUG(Lib_ImeDialog, "ImeDialog caret: buf_len={}, cursor_byte={}, caret_utf16={}",
                      data->BufTextLen, data->CursorPos, caret_utf16);
        }
        if (ui->state->caret_dirty) {
            const int len_chars = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->BufTextLen);
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
        ui->state->caret_byte_index = data->CursorPos;
        ui->state->caret_index = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
        return 0;
    }

    LOG_DEBUG(Lib_ImeDialog, ">> InputTextCallback: EventFlag={}, EventChar={}", data->EventFlag,
              data->EventChar);

    // Should we filter punctuation?
    if (ui->state->is_numeric && (data->EventChar < '0' || data->EventChar > '9') &&
        data->EventChar != '\b' && data->EventChar != ',' && data->EventChar != '.') {
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
