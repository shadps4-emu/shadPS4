// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include <magic_enum.hpp>
#include <string>
#include <cwchar>

#include "common/assert.h"
#include "core/libraries/dialogs/ime_dialog.h"
#include "core/libraries/dialogs/ime_dialog_ui.h"

#ifndef _WIN32
#define IME_NATIVE_ENCODING "WCHAR_T"
#define IME_ORBIS_ENCODING "UTF-16LE"
#endif

using namespace ImGui;

namespace Libraries::ImeDialog {

ImeDialogState::ImeDialogState(const OrbisImeDialogParam* param, const OrbisImeParamExtended* extended) {
    if (!param) return;

    userId = param->userId;
    isMultiLine = param->option & OrbisImeDialogOption::MULTILINE;
    type = param->type;
    enterLabel = param->enterLabel;
    textFilter = param->filter;
    extKeyboardFilter = extended ? extended->extKeyboardFilter : nullptr;
    maxTextLength = param->maxTextLength;
    textBuffer = param->inputTextBuffer;

#ifndef _WIN32
    orbis_to_native = iconv_open(IME_NATIVE_ENCODING, IME_ORBIS_ENCODING);
    native_to_orbis = iconv_open(IME_ORBIS_ENCODING , IME_NATIVE_ENCODING);

    ASSERT_MSG(orbis_to_native != (iconv_t)-1, "Failed to open iconv orbis_to_native");
    ASSERT_MSG(native_to_orbis != (iconv_t)-1, "Failed to open iconv native_to_orbis");

    std::size_t title_len = std::char_traits<char16_t>::length(param->title) + 1;
#elif
    std::size_t title_len = std::wcslen(reinterpret_cast<const wchar_t*>(param->title)) + 1;
#endif
    title = new wchar_t[title_len];

    if (!ConvertOrbisToNative(param->title, title_len, title, title_len)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert title to native encoding");
        return;
    }

    if (!param->placeholder) {
        return;
    }

#ifndef _WIN32
    std::size_t placeholder_len = std::char_traits<char16_t>::length(param->placeholder) + 1;
#elif
    std::size_t placeholder_len = std::wcslen(reinterpret_cast<const wchar_t*>(param->placeholder)) + 1;
#endif
    placeholder = new wchar_t[placeholder_len];

    if (!ConvertOrbisToNative(param->placeholder, placeholder_len, placeholder, placeholder_len)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert placeholder to native encoding");
    }
}

ImeDialogState::~ImeDialogState() {
#ifndef _WIN32
    if (orbis_to_native != (iconv_t)-1) {
        iconv_close(orbis_to_native);
    }
    if (native_to_orbis != (iconv_t)-1) {
        iconv_close(native_to_orbis);
    }

    if (title) {
        delete[] title;
    }

    if (placeholder) {
        delete[] placeholder;
    }
#endif
}

bool ImeDialogState::CallTextFilter() {
    if (!textFilter) {
        return true;
    }

    //TODO
    return true;
}

bool ImeDialogState::ConvertOrbisToNative(const char16_t* orbis_text, std::size_t orbis_text_len, wchar_t* native_text, std::size_t native_text_len) {
#ifndef _WIN32
    std::size_t orbis_text_len_bytes = orbis_text_len * sizeof(char16_t);
    std::size_t native_text_len_bytes = native_text_len * sizeof(wchar_t);

    char16_t* orbis_text_ptr = const_cast<char16_t*>(orbis_text);
    wchar_t* native_text_ptr = native_text;

    std::size_t result = iconv(orbis_to_native, (char**)&orbis_text_ptr, &orbis_text_len_bytes, (char**)&native_text_ptr, &native_text_len_bytes);

    return result != -1;
#elif
    std::wcsncpy(native_text, reinterpret_cast<const wchar_t*>(orbis_text), native_text_len);
    return true;
#endif
}

bool ImeDialogState::ConvertNativeToOrbis(const wchar_t* native_text, std::size_t native_text_len, char16_t* orbis_text, std::size_t orbis_text_len) {
#ifndef _WIN32
    std::size_t native_text_len_bytes = native_text_len * sizeof(wchar_t);
    std::size_t orbis_text_len_bytes = orbis_text_len * sizeof(char16_t);

    wchar_t* native_text_ptr = const_cast<wchar_t*>(native_text);
    char16_t* orbis_text_ptr = orbis_text;

    std::size_t result = iconv(native_to_orbis, (char**)&native_text_ptr, &native_text_len_bytes, (char**)&orbis_text_ptr, &orbis_text_len_bytes);

    return result != -1;
#elif
    std::wcsncpy(reinterpret_cast<wchar_t*>(orbis_text), native_text, orbis_text_len);
    return true;
#endif
}

} // namespace Libraries::ImeDialog