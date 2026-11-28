// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "core/libraries/ime/ime_dictionary.h"

namespace Libraries::Ime {

std::u16string ImeKeyboardKanaText(std::u16string_view input, bool katakana, bool half_width);
char16_t UpdateImeGreekDeadKey(char16_t previous, char16_t character);
std::u16string CombineImeKeyboardCharacter(char16_t previous, char16_t character,
                                           ImeKbVariant variant);
u32 ResolveImeExternalKeyboardMode(ImeKbVariant variant, u32 mode);
std::string TransformImeExternalText(std::string_view text, ImeKbVariant variant, u32 mode,
                                     bool map_native = true);

std::string ComposeImeExternalText(std::string_view text, ImeKbVariant variant, u32 mode,
                                   char16_t& dead_key);

class ImeComposition {
public:
    void Configure(ImeKbVariant variant, const std::filesystem::path& directory,
                   bool expanded_preedit = false,
                   std::shared_ptr<const ImeDictionary> application_dictionary = {},
                   bool application_priority = false);
    bool Accepts(std::string_view input) const;
    bool Insert(std::string_view document, int start, int end, std::string_view input, int limit,
                std::optional<u32> external_mode = {});
    bool Backspace();
    bool Select(int index);
    bool Cycle(int direction);
    bool Rewrite(std::string_view reading);
    std::string Confirm();
    std::string Cancel();
    void Clear();

    bool AcceptsExternal(std::string_view input, u32 mode) const;
    bool Active() const;
    ImeKbVariant Variant() const;
    std::optional<u32> ExternalMode() const;
    bool SetExternalMode(u32 mode);
    bool Enabled() const;
    bool Korean() const;
    int Start() const;
    int Length() const;
    int Caret() const;
    int Selected() const;
    const std::string& Reading() const;
    const std::vector<std::string>& Candidates() const;
    std::string Preview() const;
    const std::string& CommittedText() const;

private:
    ImeKbVariant variant = ImeKbVariant::Automatic;
    std::shared_ptr<const ImeDictionary> dictionary;
    std::shared_ptr<const ImeDictionary> application_dictionary;
    bool application_priority = false;
    std::u16string source;
    std::u16string prefix;
    std::u16string suffix;
    std::string committed;
    std::string reading;
    std::string display;
    std::vector<std::string> candidates;
    std::optional<u32> external_mode;
    int start = 0;
    int available = 0;
    int max_preedit_length = ORBIS_IME_MAX_PREEDIT_LENGTH;
    int selected = -1;
    int confirmed_caret = 0;
    bool composing = false;

    std::u16string Render(bool final) const;
    bool Rebuild();
};

} // namespace Libraries::Ime
