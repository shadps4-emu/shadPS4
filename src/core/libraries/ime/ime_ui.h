// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <imgui.h>
#include <queue>
#include "imgui/imgui_layer.h"

#include "common/cstring.h"
#include "common/types.h"

#include "ime.h"

namespace Libraries::Ime {

class ImeHandler;
class ImeUi;

class ImeState {
    friend class ImeHandler;
    friend class ImeUi;

    bool input_changed = false;

    void* work_buffer{};

    char16_t* text_buffer{};

    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_MAX_TEXT_LENGTH * 4> current_text;

    std::queue<OrbisImeEvent> event_queue;
    std::mutex queue_mutex;

public:
    ImeState(const OrbisImeParam* param = nullptr);
    ImeState(ImeState&& other) noexcept;
    ImeState& operator=(ImeState&& other) noexcept;

    void SendEvent(OrbisImeEvent* event);
    void SendEnterEvent();
    void SendCloseEvent();

private:
    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t native_text_len);
    bool ConvertUTF8ToOrbis(const char* native_text, std::size_t utf8_text_len,
                            char16_t* orbis_text, std::size_t orbis_text_len);
};

class ImeUi : public ImGui::Layer {
    ImeState* state{};
    const OrbisImeParam* ime_param{};

    bool first_render = true;
    std::mutex draw_mutex;

public:
    explicit ImeUi(ImeState* state = nullptr, const OrbisImeParam* param = nullptr);
    ~ImeUi() override;
    ImeUi(const ImeUi& other) = delete;
    ImeUi& operator=(ImeUi&& other);

    void Draw() override;

private:
    void Free();

    void DrawInputText();

    static int InputTextCallback(ImGuiInputTextCallbackData* data);
};

}; // namespace Libraries::Ime