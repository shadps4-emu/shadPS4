// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_language.h"

#include <algorithm>

namespace Libraries::Ime {
namespace {

constexpr ImeKbLabels kEnglishLabels = {
    .space_label = "Space",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Done", "Send", "Search", "Go"},
    .main_menu_items = {"Select", "Select All", "Paste"},
    .action_menu_items = {"Copy", "Paste"},
};

constexpr ImeKbLabels kPolishLabels = {
    .space_label = "Spacja",
    .accents_label = "ą",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Gotowe", "Wyślij", "Wyszukaj", "Przejdź"},
    .main_menu_items = {"Zaznacz", "Zaznacz wszystko", "Wklej"},
    .action_menu_items = {"Kopiuj", "Wklej"},
};

constexpr std::array<ImeKbLanguageProfile, 3> kLanguageProfiles = {{
    {
        .language = OrbisImeLanguage::ENGLISH_US,
        .console_language = 1,
        .labels = &kEnglishLabels,
    },
    {
        .language = OrbisImeLanguage::ENGLISH_GB,
        .console_language = 18,
        .labels = &kEnglishLabels,
        .currency = ImeKbCurrencyProfile::Pound,
    },
    {
        .language = OrbisImeLanguage::POLISH,
        .console_language = 16,
        .labels = &kPolishLabels,
        .preferred_characters = u"ąćęłńóśźżĄĆĘŁŃÓŚŹŻ",
        .currency = ImeKbCurrencyProfile::Euro,
        .special_pages = 1,
    },
}};

} // namespace

const ImeKbLabels& ImeKbLanguageProfile::GetLabels() const {
    return labels ? *labels : kEnglishLabels;
}

std::span<const ImeKbLanguageProfile> GetImeKeyboardLanguageProfiles() {
    return kLanguageProfiles;
}

const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(OrbisImeLanguage language) {
    const auto profile =
        std::ranges::find(kLanguageProfiles, language, &ImeKbLanguageProfile::language);
    return profile != kLanguageProfiles.end() ? *profile : kLanguageProfiles.front();
}

OrbisImeLanguage ResolveImeKeyboardLanguage(OrbisImeLanguage supported_languages,
                                            int console_language) {
    const auto allowed = [&](const ImeKbLanguageProfile& profile) {
        return static_cast<u64>(supported_languages) == 0 ||
               True(supported_languages & profile.language);
    };
    const auto configured = std::ranges::find(kLanguageProfiles, console_language,
                                              &ImeKbLanguageProfile::console_language);
    if (configured != kLanguageProfiles.end() && allowed(*configured)) {
        return configured->language;
    }
    const auto supported = std::ranges::find_if(kLanguageProfiles, allowed);
    return supported != kLanguageProfiles.end() ? supported->language
                                                : kLanguageProfiles.front().language;
}

} // namespace Libraries::Ime
