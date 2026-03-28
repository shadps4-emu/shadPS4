// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>

#include "core/emulator_settings.h"

///////////// ImGui Translation Tables

// disable clang line limits for ease of translation
// clang-format off

const std::map<std::string, std::string> JapaneseMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> FrenchMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> FrenchCanadaMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> SpanishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> SpanishLatinAmericanMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> GermanMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> ItalianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> DutchMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> PortugesePtMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> PortugeseBrMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> RussianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> KoreanMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> ChineseTraditionalMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> ChineseSimplifiedMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> FinnishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> SwedishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> DanishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> NorwegianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> PolishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> TurkishMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> ArabicMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> CzechMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> HungarianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> GreekMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> RomanianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> ThaiMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> VietnameseMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> IndonesianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

const std::map<std::string, std::string> UkranianMap = {
    {"Trophy Earned", "Trophy Earned"},
};

// clang-format on

///////////// End ImGui Translation Tables

const std::map<u32, std::map<std::string, std::string>> langMap = {
    {0, JapaneseMap},
    // {1, EnglishUsMap}, - not used
    {2, FrenchMap},
    {3, SpanishMap},
    {4, GermanMap},
    {5, ItalianMap},
    {6, DutchMap},
    {7, PortugesePtMap},
    {8, RussianMap},
    {9, KoreanMap},
    {10, ChineseTraditionalMap},
    {11, ChineseSimplifiedMap},
    {12, FinnishMap},
    {13, SwedishMap},
    {14, DanishMap},
    {15, NorwegianMap},
    {16, PolishMap},
    {17, PortugeseBrMap},
    // {18, "English (UK)"}, - not used
    {19, TurkishMap},
    {20, SpanishLatinAmericanMap},
    {21, ArabicMap},
    {22, FrenchCanadaMap},
    {23, CzechMap},
    {24, HungarianMap},
    {25, GreekMap},
    {26, RomanianMap},
    {27, ThaiMap},
    {28, VietnameseMap},
    {29, IndonesianMap},
    {30, UkranianMap},
};

namespace ImguiTranslate {

std::string tr(std::string input) {
    // since we're coding in English
    if (EmulatorSettings.GetConsoleLanguage() == 1 || EmulatorSettings.GetConsoleLanguage() == 18)
        return input;

    const std::map<std::string, std::string> translationTable =
        langMap.at(EmulatorSettings.GetConsoleLanguage());

    if (!translationTable.contains(input)) {
        return input;
    }

    return translationTable.at(input);
}

} // namespace ImguiTranslate
