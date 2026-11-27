// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>
#include <string_view>
#include <vector>

#include "core/libraries/ime/ime_common.h"

namespace Libraries::Ime {

struct ImeKbLayoutSelection;

enum class ImeKbVariant : u8 {
    Automatic,
    English_US,
    English_GB,
    Polish,
    Spanish,
    German,
    SpanishLatinAmerica,
    Dutch,
    Danish,
    Russian,
    Swedish,
    Finnish,
    French_CA,
    French,
    Italian,
    Norwegian,
    Portuguese_PT,
    Portuguese_BR,
    Turkish,
    Japanese,
    Japanese_Gojuon,
    Korean,
    Chinese_CN,
    Chinese_HK,
    Chinese_TW,
    Chinese_TW_Zhuyin,
    Chinese_TW_Cangjie,
    Arabic,
    Thai,
    Czech,
    Greek,
    Hungarian,
    Indonesian,
    Romanian,
    Vietnamese,
    Ukrainian,
};

struct ImeKbLabels {
    const char* space_label;
    const char* accents_label;
    const char* letters_label;
    const char* symbols_label;
    std::array<const char*, 4> enter_labels;
    std::array<const char*, 3> main_menu_items;
    std::array<const char*, 2> action_menu_items;
    std::array<const char*, 2> width_labels;
    std::array<const char*, 3> script_action_labels;
};

struct ImeKbLanguageProfile {
    OrbisImeLanguage language = OrbisImeLanguage::ENGLISH_US;
    int console_language = 1;
    const ImeKbLabels* labels = nullptr;
    std::u16string_view preferred_characters{};
    ImeKbVariant variant = ImeKbVariant::Automatic;
    const char* name = "English (US)";

    const ImeKbLabels& GetLabels() const;
    const char* GetDisplayName() const;
};

std::span<const ImeKbLanguageProfile> GetImeKeyboardLanguageProfiles();
std::vector<ImeKbVariant> GetOrderedImeKeyboardLanguages(OrbisImeLanguage languages,
                                                         int console_language);
int GetImeKeyboardSettingId(const ImeKbLanguageProfile& profile);
const ImeKbLanguageProfile* FindImeKeyboardSetting(int id);
const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(ImeKbVariant variant);
ImeKbLayoutSelection GetImeKeyboardLanguageSelection(const ImeKbLayoutSelection& selection,
                                                     ImeKbVariant variant);

struct ImeKeyboardLanguagePicker {
    std::vector<ImeKbVariant> languages;
    int index = -1;
    bool activate_armed = false;
    float openness = 0.0f;
    float visual_index = 0.0f;
    float slide_origin = 0.0f;
    int slide_target = 0;
    double slide_started_at = 0.0;

    bool IsOpen() const {
        return index >= 0;
    }
    bool IsVisible() const {
        return IsOpen() || openness > 0.0f;
    }
    void Open(const ImeKbLayoutSelection& selection);
    void Move(int direction);
    ImeKbLayoutSelection Preview(const ImeKbLayoutSelection& selection) const;
    void Confirm(ImeKbLayoutSelection& selection);
};
const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(
    OrbisImeLanguage language, ImeKbVariant variant = ImeKbVariant::Automatic);
const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(OrbisImeLanguage language,
                                                          std::string_view preferred_layout);
OrbisImeLanguage ResolveImeInputLanguages(OrbisImeLanguage supported_languages,
                                          OrbisImeLanguage configured_languages,
                                          OrbisImeOption options);
bool HasMultipleImeKeyboardLanguages(OrbisImeLanguage supported_languages);
OrbisImeLanguage ResolveImeKeyboardLanguage(OrbisImeLanguage supported_languages,
                                            int console_language);

} // namespace Libraries::Ime
