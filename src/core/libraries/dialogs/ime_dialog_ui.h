// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifndef _WIN32
#include <iconv.h>
#endif

#include "core/libraries/dialogs/ime_dialog.h"
#include "common/types.h"
#include "imgui/imgui_layer.h"

namespace Libraries::ImeDialog {

class ImeDialogUi;

class ImeDialogState final {
    friend ImeDialogUi;

    OrbisImeDialogStatus status = OrbisImeDialogStatus::NONE;

    s32 userId{};
    bool isMultiLine{};
    OrbisImeType type{};
    OrbisImeEnterLabel enterLabel{};
    OrbisImeTextFilter textFilter{};
    OrbisImeExtKeyboardFilter extKeyboardFilter{};
    u32 maxTextLength{};
    char16_t* textBuffer{};
    wchar_t* title = nullptr;
    wchar_t* placeholder = nullptr;
    wchar_t currentText[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH] = {0};
#ifndef _WIN32
    iconv_t orbis_to_native = (iconv_t)-1;
    iconv_t native_to_orbis = (iconv_t)-1;
#endif
public:
    ImeDialogState(const OrbisImeDialogParam* param = nullptr, const OrbisImeParamExtended* extended = nullptr);
    ~ImeDialogState();

    ImeDialogState() = default;
    
    bool CallTextFilter();

private:
    bool ConvertOrbisToNative(const char16_t* orbis_text, std::size_t orbis_text_len, wchar_t* native_text, std::size_t native_text_len);
    bool ConvertNativeToOrbis(const wchar_t* native_text, std::size_t native_text_len, char16_t* orbis_text, std::size_t orbis_text_len);
};

class ImeDialogUi final : public ImGui::Layer {
    
public:
    explicit ImeDialogUi();

    ~ImeDialogUi() override;
    ImeDialogUi(const ImeDialogUi& other) = delete;
    ImeDialogUi(ImeDialogUi&& other) noexcept;
    ImeDialogUi& operator=(ImeDialogUi other);

    void Draw() override;
};

} // namespace Libraries::ImeDialog
