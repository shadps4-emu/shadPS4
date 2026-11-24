// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_language.h"

#include <algorithm>
#include <bit>

#include "core/libraries/ime/ime_kb_layout.h"

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
    .width_labels = {"Full", "Half"},
    .script_action_labels = {"Voicing", "Small", "Punctuation"},
};

constexpr ImeKbLabels kPolishLabels = {
    .space_label = "Spacja",
    .accents_label = "ą",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Gotowe", "Wyślij", "Wyszukaj", "Przejdź"},
    .main_menu_items = {"Zaznacz", "Zaznacz wszystko", "Wklej"},
    .action_menu_items = {"Kopiuj", "Wklej"},
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kSpanishLabels = {
    .space_label = "Espacio",
    .accents_label = "á",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Terminado", "Enviar", "Buscar", "Ir"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kGermanLabels = {
    .space_label = "Leerzeichen",
    .accents_label = "ä",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Fertig", "Senden", "Suchen", "Los"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kSpanishLatinAmericaLabels = {
    .space_label = "Espacio",
    .accents_label = "á",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Listo", "Enviar", "Buscar", "Ir"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kDutchLabels = {
    .space_label = "Spatie",
    .accents_label = "é",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Gereed", "Verzenden", "Zoeken", "Ga"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kDanishLabels = {
    .space_label = "Mellemrum",
    .accents_label = "å",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Færdig", "Send", "Søg", "Start"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kRussianLabels = {
    .space_label = "Пробел",
    .accents_label = "ABC",
    .letters_label = "АБВ",
    .symbols_label = "@#:",
    .enter_labels = {"Готово", "Отправить", "Поиск", "Перейти"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kSwedishLabels = {
    .space_label = "Blanksteg",
    .accents_label = "ä",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Klar", "Skicka", "Sök", "OK"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kFinnishLabels = {
    .space_label = "Välilyönti",
    .accents_label = "ä",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Valmis", "Lähetä", "Haku", "Mene"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kFrench_CALabels = {
    .space_label = "Espace",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Terminé", "Envoyer", "Rechercher", "OK"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kFrenchLabels = {
    .space_label = "Espace",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Terminé", "Envoyer", "Rechercher", "OK"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kItalianLabels = {
    .space_label = "Spazio",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Fine", "Invia", "Cerca", "Vai"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kNorwegianLabels = {
    .space_label = "Mellomrom",
    .accents_label = "å",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Utført", "Send", "Søk", "Gå"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kPortuguese_PTLabels = {
    .space_label = "Espaço",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Concluir", "Enviar", "Pesquisar", "Ir"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kPortuguese_BRLabels = {
    .space_label = "Espaço",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Concluído", "Enviar", "Pesquisar", "Ir"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kTurkishLabels = {
    .space_label = "Boşluk",
    .accents_label = "ç",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Bitti", "Gönder", "Arama", "Git"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kJapaneseLabels = {
    .space_label = "スペース",
    .accents_label = "ABC",
    .letters_label = "あいう",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "送信", "検索", "実行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = {"゛゜", "小", "、。?!"},
};

constexpr ImeKbLabels kJapanese_GojuonLabels = {
    .space_label = "スペース",
    .accents_label = "ABC",
    .letters_label = "あいう",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "送信", "検索", "実行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = {"゛゜", "小", "、。?!"},
};

constexpr ImeKbLabels kKoreanLabels = {
    .space_label = "스페이스",
    .accents_label = "ABC",
    .letters_label = "한글",
    .symbols_label = "@#:",
    .enter_labels = {"확정", "보내기", "검색", "확정"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kChinese_CNLabels = {
    .space_label = "空格",
    .accents_label = "ABC",
    .letters_label = "拼音",
    .symbols_label = "@#:",
    .enter_labels = {"确定", "发送", "搜索", "执行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kChinese_HKLabels = {
    .space_label = "空格",
    .accents_label = "ABC",
    .letters_label = "拼音",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "傳送", "搜尋", "執行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kChinese_TWLabels = {
    .space_label = "空格",
    .accents_label = "ABC",
    .letters_label = "拼音",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "傳送", "搜尋", "執行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kChinese_TW_ZhuyinLabels = {
    .space_label = "空格",
    .accents_label = "ABC",
    .letters_label = "注音",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "傳送", "搜尋", "執行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kChinese_TW_CangjieLabels = {
    .space_label = "空格",
    .accents_label = "ABC",
    .letters_label = "倉頡",
    .symbols_label = "@#:",
    .enter_labels = {"確定", "傳送", "搜尋", "執行"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = {"全", "半"},
    .script_action_labels = kEnglishLabels.script_action_labels,
};

// Visual-order labels for the unshaped UI renderer.
constexpr ImeKbLabels kArabicLabels = {
    .space_label = "ﺔﻓﺎﺴﻣ",
    .accents_label = "ABC",
    .letters_label = "ﺞﺑﺃ",
    .symbols_label = "@#:",
    .enter_labels = {"ﻢﺗ", "ﻝﺎﺳﺭﺇ", "ﺚﺤﺑ", "ﺀﺪﺑ"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kThaiLabels = {
    .space_label = "เว้นวรรค",
    .accents_label = "ABC",
    .letters_label = "กขค",
    .symbols_label = "@#:",
    .enter_labels = {"เสร็จสิ้น", "ส่ง", "ค้นหา", "ไป"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kCzechLabels = {
    .space_label = "Mezera",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Hotovo", "Odeslat", "Hledat", "Jít"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kGreekLabels = {
    .space_label = "Διάστημα",
    .accents_label = "ABC",
    .letters_label = "ΑΒΓ",
    .symbols_label = "@#:",
    .enter_labels = {"Ολοκληρώθηκε", "Αποστολή", "Αναζήτηση", "Μετάβαση"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kHungarianLabels = {
    .space_label = "Szóköz",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Kész", "Küldés", "Keresés", "Ugrás"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kIndonesianLabels = {
    .space_label = "Spasi",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Selesai", "Kirim", "Cari", "Buka"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kRomanianLabels = {
    .space_label = "Spațiu",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Terminat", "Trimitere", "Căutare", "Accesare"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kVietnameseLabels = {
    .space_label = "Phím cách",
    .accents_label = "à",
    .letters_label = "ABC",
    .symbols_label = "@#:",
    .enter_labels = {"Xong", "Gửi", "Tìm kiếm", "Chuyển"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr ImeKbLabels kUkrainianLabels = {
    .space_label = "Пробіл",
    .accents_label = "ABC",
    .letters_label = "АБВ",
    .symbols_label = "@#:",
    .enter_labels = {"Виконано", "Надіслати", "Пошук", "Перейти"},
    .main_menu_items = kEnglishLabels.main_menu_items,
    .action_menu_items = kEnglishLabels.action_menu_items,
    .width_labels = kEnglishLabels.width_labels,
    .script_action_labels = kEnglishLabels.script_action_labels,
};

constexpr std::array<ImeKbLanguageProfile, 35> kLanguageProfiles = {{
    {
        .language = OrbisImeLanguage::ENGLISH_US,
        .console_language = 1,
        .labels = &kEnglishLabels,
        .variant = ImeKbVariant::English_US,
        .name = "English (US)",
    },
    {
        .language = OrbisImeLanguage::ENGLISH_GB,
        .console_language = 18,
        .labels = &kEnglishLabels,
        .variant = ImeKbVariant::English_GB,
        .name = "English (UK)",
    },
    {
        .language = OrbisImeLanguage::POLISH,
        .console_language = 16,
        .labels = &kPolishLabels,
        .preferred_characters = u"ąćęłńóśźżĄĆĘŁŃÓŚŹŻ",
        .variant = ImeKbVariant::Polish,
        .name = "Polish",
    },
    {
        .language = OrbisImeLanguage::SPANISH,
        .console_language = 3,
        .labels = &kSpanishLabels,
        .variant = ImeKbVariant::Spanish,
        .name = "Spanish",
    },
    {
        .language = OrbisImeLanguage::GERMAN,
        .console_language = 4,
        .labels = &kGermanLabels,
        .variant = ImeKbVariant::German,
        .name = "German",
    },
    {
        .language = OrbisImeLanguage::SPANISH_LA,
        .console_language = 20,
        .labels = &kSpanishLatinAmericaLabels,
        .variant = ImeKbVariant::SpanishLatinAmerica,
        .name = "Spanish (Latin America)",
    },
    {
        .language = OrbisImeLanguage::DUTCH,
        .console_language = 6,
        .labels = &kDutchLabels,
        .variant = ImeKbVariant::Dutch,
        .name = "Dutch",
    },
    {
        .language = OrbisImeLanguage::DANISH,
        .console_language = 14,
        .labels = &kDanishLabels,
        .variant = ImeKbVariant::Danish,
        .name = "Danish",
    },
    {
        .language = OrbisImeLanguage::RUSSIAN,
        .console_language = 8,
        .labels = &kRussianLabels,
        .variant = ImeKbVariant::Russian,
        .name = "Russian",
    },
    {
        .language = OrbisImeLanguage::SWEDISH,
        .console_language = 13,
        .labels = &kSwedishLabels,
        .variant = ImeKbVariant::Swedish,
        .name = "Swedish",
    },
    {
        .language = OrbisImeLanguage::FINNISH,
        .console_language = 12,
        .labels = &kFinnishLabels,
        .variant = ImeKbVariant::Finnish,
        .name = "Finnish",
    },
    {
        .language = OrbisImeLanguage::FRENCH_CA,
        .console_language = 22,
        .labels = &kFrench_CALabels,
        .variant = ImeKbVariant::French_CA,
        .name = "French (Canada)",
    },
    {
        .language = OrbisImeLanguage::FRENCH,
        .console_language = 2,
        .labels = &kFrenchLabels,
        .variant = ImeKbVariant::French,
        .name = "French",
    },
    {
        .language = OrbisImeLanguage::ITALIAN,
        .console_language = 5,
        .labels = &kItalianLabels,
        .variant = ImeKbVariant::Italian,
        .name = "Italian",
    },
    {
        .language = OrbisImeLanguage::NORWEGIAN,
        .console_language = 15,
        .labels = &kNorwegianLabels,
        .variant = ImeKbVariant::Norwegian,
        .name = "Norwegian",
    },
    {
        .language = OrbisImeLanguage::PORTUGUESE_PT,
        .console_language = 7,
        .labels = &kPortuguese_PTLabels,
        .variant = ImeKbVariant::Portuguese_PT,
        .name = "Portuguese (Portugal)",
    },
    {
        .language = OrbisImeLanguage::PORTUGUESE_BR,
        .console_language = 17,
        .labels = &kPortuguese_BRLabels,
        .variant = ImeKbVariant::Portuguese_BR,
        .name = "Portuguese (Brazil)",
    },
    {
        .language = OrbisImeLanguage::TURKISH,
        .console_language = 19,
        .labels = &kTurkishLabels,
        .variant = ImeKbVariant::Turkish,
        .name = "Turkish",
    },
    {
        .language = OrbisImeLanguage::JAPANESE,
        .console_language = 0,
        .labels = &kJapaneseLabels,
        .variant = ImeKbVariant::Japanese,
        .name = "Japanese",
    },
    {
        .language = OrbisImeLanguage::JAPANESE,
        .console_language = -1,
        .labels = &kJapanese_GojuonLabels,
        .variant = ImeKbVariant::Japanese_Gojuon,
        .name = "Japanese (Gojuon)",
    },
    {
        .language = OrbisImeLanguage::KOREAN,
        .console_language = 9,
        .labels = &kKoreanLabels,
        .variant = ImeKbVariant::Korean,
        .name = "Korean",
    },
    {
        .language = OrbisImeLanguage::SIMPLIFIED_CHINESE,
        .console_language = 11,
        .labels = &kChinese_CNLabels,
        .variant = ImeKbVariant::Chinese_CN,
        .name = "Chinese (Simplified)",
    },
    {
        .language = OrbisImeLanguage::TRADITIONAL_CHINESE,
        .console_language = -1,
        .labels = &kChinese_HKLabels,
        .variant = ImeKbVariant::Chinese_HK,
        .name = "Chinese (Hong Kong)",
    },
    {
        .language = OrbisImeLanguage::TRADITIONAL_CHINESE,
        .console_language = 10,
        .labels = &kChinese_TWLabels,
        .variant = ImeKbVariant::Chinese_TW,
        .name = "Chinese (Taiwan)",
    },
    {
        .language = OrbisImeLanguage::TRADITIONAL_CHINESE,
        .console_language = -1,
        .labels = &kChinese_TW_ZhuyinLabels,
        .variant = ImeKbVariant::Chinese_TW_Zhuyin,
        .name = "Chinese (Zhuyin)",
    },
    {
        .language = OrbisImeLanguage::TRADITIONAL_CHINESE,
        .console_language = -1,
        .labels = &kChinese_TW_CangjieLabels,
        .variant = ImeKbVariant::Chinese_TW_Cangjie,
        .name = "Chinese (Cangjie)",
    },
    {
        .language = OrbisImeLanguage::ARABIC,
        .console_language = 21,
        .labels = &kArabicLabels,
        .variant = ImeKbVariant::Arabic,
        .name = "Arabic",
    },
    {
        .language = OrbisImeLanguage::THAI,
        .console_language = 27,
        .labels = &kThaiLabels,
        .variant = ImeKbVariant::Thai,
        .name = "Thai",
    },
    {
        .language = OrbisImeLanguage::CZECH,
        .console_language = 23,
        .labels = &kCzechLabels,
        .variant = ImeKbVariant::Czech,
        .name = "Czech",
    },
    {
        .language = OrbisImeLanguage::GREEK,
        .console_language = 25,
        .labels = &kGreekLabels,
        .variant = ImeKbVariant::Greek,
        .name = "Greek",
    },
    {
        .language = OrbisImeLanguage::HUNGARIAN,
        .console_language = 24,
        .labels = &kHungarianLabels,
        .variant = ImeKbVariant::Hungarian,
        .name = "Hungarian",
    },
    {
        .language = OrbisImeLanguage::INDONESIAN,
        .console_language = 29,
        .labels = &kIndonesianLabels,
        .variant = ImeKbVariant::Indonesian,
        .name = "Indonesian",
    },
    {
        .language = OrbisImeLanguage::ROMANIAN,
        .console_language = 26,
        .labels = &kRomanianLabels,
        .variant = ImeKbVariant::Romanian,
        .name = "Romanian",
    },
    {
        .language = OrbisImeLanguage::VIETNAMESE,
        .console_language = 28,
        .labels = &kVietnameseLabels,
        .variant = ImeKbVariant::Vietnamese,
        .name = "Vietnamese",
    },
    {
        .language = OrbisImeLanguage::UKRAINIAN,
        .console_language = 30,
        .labels = &kUkrainianLabels,
        .variant = ImeKbVariant::Ukrainian,
        .name = "Ukrainian",
    },
}};

} // namespace

const ImeKbLabels& ImeKbLanguageProfile::GetLabels() const {
    return labels ? *labels : kEnglishLabels;
}

std::span<const ImeKbLanguageProfile> GetImeKeyboardLanguageProfiles() {
    return kLanguageProfiles;
}

const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(OrbisImeLanguage language,
                                                          ImeKbVariant variant) {
    const auto profile = std::ranges::find_if(kLanguageProfiles, [&](const auto& profile) {
        return profile.language == language &&
               (variant == ImeKbVariant::Automatic ? profile.console_language >= 0
                                                   : profile.variant == variant);
    });
    return profile != kLanguageProfiles.end() ? *profile : kLanguageProfiles.front();
}

const ImeKbLanguageProfile& GetImeKeyboardLanguageProfile(OrbisImeLanguage language,
                                                          std::string_view preferred_layout) {
    const auto profile = std::ranges::find_if(kLanguageProfiles, [&](const auto& profile) {
        return profile.language == language && profile.name == preferred_layout;
    });
    return profile != kLanguageProfiles.end() ? *profile : GetImeKeyboardLanguageProfile(language);
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

bool HasMultipleImeKeyboardLanguages(OrbisImeLanguage supported_languages) {
    return std::popcount(static_cast<u64>(supported_languages) & kValidOrbisImeLanguageMask) > 1;
}

void CycleImeKeyboardLanguage(ImeKbLayoutSelection& selection,
                              OrbisImeLanguage supported_languages) {
    if (!HasMultipleImeKeyboardLanguages(supported_languages)) {
        return;
    }
    const auto& current = GetImeKeyboardLanguageProfile(selection.language, selection.variant);
    const auto index = static_cast<std::size_t>(&current - kLanguageProfiles.data());
    for (std::size_t offset = 1; offset <= kLanguageProfiles.size(); ++offset) {
        const auto& next = kLanguageProfiles[(index + offset) % kLanguageProfiles.size()];
        if (True(next.language & supported_languages)) {
            selection.language = next.language;
            selection.variant = next.variant;
            selection.family = selection.input_type == OrbisImeType::Number
                                   ? ImeKbLayoutFamily::Numeric
                                   : ImeKbLayoutFamily::Alphabet;
            selection.page = 0;
            selection.case_state = ImeKbCaseState::Lower;
            selection.full_width = false;
            return;
        }
    }
}

} // namespace Libraries::Ime
