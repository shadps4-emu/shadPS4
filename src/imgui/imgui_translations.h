// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>

#include "common/config.h"

///////////// ImGui Translation Tables

// disable clang line limits for ease of translation
// clang-format off

const std::map<std::string, std::string> TrophyEarnedTable = {
    {"Japanese", "Trophy Earned"},
    {"English (US)", "Trophy Earned"},
    {"French", "Trophy Earned"},
    {"Spanish", "Trophy Earned"},
    {"German", "Trophy Earned"},
    {"Italian", "Trophy Earned"},
    {"Dutch", "Trophy Earned"},
    {"Portuguese (PT)", "Trophy Earned"},
    {"Russian", "Trophy Earned"},
    {"Korean", "Trophy Earned"},
    {"Chinese (Traditional)", "Trophy Earned"},
    {"Chinese (Simplified)", "Trophy Earned"},
    {"Finnish", "Trophy Earned"},
    {"Swedish", "Trophy Earned"},
    {"Danish", "Trophy Earned"},
    {"Norwegian", "Trophy Earned"},
    {"Polish", "Trophy Earned"},
    {"Portuguese (BR)", "Trophy Earned"},
    {"English (UK)", "Trophy Earned"},
    {"Turkish", "Trophy Earned"},
    {"Spanish (Latin America)", "Trophy Earned"},
    {"Arabic", "Trophy Earned"},
    {"French (Canada)", "Trophy Earned"},
    {"Czech", "Trophy Earned"},
    {"Hungarian", "Trophy Earned"},
    {"Greek", "Trophy Earned"},
    {"Romanian", "Trophy Earned"},
    {"Thai", "Trophy Earned"},
    {"Vietnamese", "Trophy Earned"},
    {"Indonesian", "Trophy Earned"},
    {"Ukrainian", "Trophy Earned"}};

// clang-format on

///////////// End ImGui Translation Tables

std::map<u32, std::string> langMap = {{0, "Japanese"},
                                      {1, "English (US)"},
                                      {2, "French"},
                                      {3, "Spanish"},
                                      {4, "German"},
                                      {5, "Italian"},
                                      {6, "Dutch"},
                                      {7, "Portuguese (PT)"},
                                      {8, "Russian"},
                                      {9, "Korean"},
                                      {10, "Chinese (Traditional)"},
                                      {11, "Chinese (Simplified)"},
                                      {12, "Finnish"},
                                      {13, "Swedish"},
                                      {14, "Danish"},
                                      {15, "Norwegian"},
                                      {16, "Polish"},
                                      {17, "Portuguese (BR)"},
                                      {18, "English (UK)"},
                                      {19, "Turkish"},
                                      {20, "Spanish (Latin America)"},
                                      {21, "Arabic"},
                                      {22, "French (Canada)"},
                                      {23, "Czech"},
                                      {24, "Hungarian"},
                                      {25, "Greek"},
                                      {26, "Romanian"},
                                      {27, "Thai"},
                                      {28, "Vietnamese"},
                                      {29, "Indonesian"},
                                      {30, "Ukrainian"}};

std::map<std::string, std::map<std::string, std::string>> tableMap = {
    {"Trophy Earned", TrophyEarnedTable},
};

namespace ImguiTranslate {

std::string tr(std::string input) {
    if (!tableMap.contains(input)) {
        return input;
    }

    std::map<std::string, std::string> translationTable = tableMap[input];
    std::string language = langMap[Config::GetLanguage()];
    if (!translationTable.contains(language)) {
        return input;
    }

    return translationTable[language];
}

} // namespace ImguiTranslate
