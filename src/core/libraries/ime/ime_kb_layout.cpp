// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_layout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>

#include "core/debug_state.h"

namespace Libraries::Ime {
namespace {

#define KEY(r, c, label, action)                                                                   \
    ImeKbKeySpec {                                                                                 \
        r, c, 1, 1, label, nullptr, ImeKbKeyAction::action, ImeKbKeyGlyph::None                    \
    }
#define KEYHOT(r, c, label, hotkey, action)                                                        \
    ImeKbKeySpec {                                                                                 \
        r, c, 1, 1, label, hotkey, ImeKbKeyAction::action, ImeKbKeyGlyph::None                     \
    }
#define KEYSPAN(r, c, cs, rs, label, action)                                                       \
    ImeKbKeySpec {                                                                                 \
        r, c, cs, rs, label, nullptr, ImeKbKeyAction::action, ImeKbKeyGlyph::None                  \
    }
#define KEYSPANHOT(r, c, cs, rs, label, hotkey, action)                                            \
    ImeKbKeySpec {                                                                                 \
        r, c, cs, rs, label, hotkey, ImeKbKeyAction::action, ImeKbKeyGlyph::None                   \
    }
#define KEYGLYPH(r, c, glyph, action)                                                              \
    ImeKbKeySpec {                                                                                 \
        r, c, 1, 1, nullptr, nullptr, ImeKbKeyAction::action, ImeKbKeyGlyph::glyph                 \
    }
#define KEYHOTGLYPH(r, c, hotkey, glyph, action)                                                   \
    ImeKbKeySpec {                                                                                 \
        r, c, 1, 1, nullptr, hotkey, ImeKbKeyAction::action, ImeKbKeyGlyph::glyph                  \
    }
#define KEYSPANGLYPH(r, c, cs, rs, glyph, action)                                                  \
    ImeKbKeySpec {                                                                                 \
        r, c, cs, rs, nullptr, nullptr, ImeKbKeyAction::action, ImeKbKeyGlyph::glyph               \
    }
#define KEYSPANHOTGLYPH(r, c, cs, rs, hotkey, glyph, action)                                       \
    ImeKbKeySpec {                                                                                 \
        r, c, cs, rs, nullptr, hotkey, ImeKbKeyAction::action, ImeKbKeyGlyph::glyph                \
    }

constexpr float kPanelBaseW = 793.0f;
constexpr float kPanelBaseHSingle = 528.0f;
constexpr float kPanelBaseHMulti = 628.0f;
constexpr float kLabelH = 57.0f;
constexpr float kInputHSingle = 50.0f;
constexpr float kInputHMulti = 151.0f;
constexpr float kPredictH = 53.0f;
constexpr float kPredictW = 740.0f;
constexpr float kCloseW = 53.0f;
constexpr float kKeysH = 316.0f;
constexpr float kKeyGap = 9.0f;
constexpr float kPadX = 26.0f;
constexpr float kPadBottomSingle = 26.0f;
constexpr float kPadBottomMulti = 26.0f;
constexpr float kKeyFontRatio = 0.042f;
constexpr float kTypingKeyLabelScale = 1.60f;
constexpr float kCornerRatio = 0.004f;
constexpr float kSingleLineTextFill = 0.85f;
constexpr float kMultiLineTextFill = 0.85f;
constexpr int kMultiLineVisibleLines = 4;
constexpr int kKeyRows = 6;
constexpr int kSpecialsKeyRows = 7;
constexpr int kKeyCols = 10;
constexpr char kCurrencyDollar[] = "$";
constexpr char kCurrencyEuro[] = "\xE2\x82\xAC";
constexpr char kCurrencyPound[] = "\xC2\xA3";
constexpr char kCurrencyYen[] = "\xC2\xA5";
constexpr char kCurrencyWon[] = "\xE2\x82\xA9";
// SCE_IME keycodes follow USB HID usage IDs.
constexpr u16 kCodeSym1 = 0x003A;       // SCE_IME_KEYCODE_F1
constexpr u16 kCodeAccents = 0x003B;    // SCE_IME_KEYCODE_F2
constexpr u16 kCodeLetters = 0x003C;    // SCE_IME_KEYCODE_F3
constexpr u16 kCodeKeyboard = 0x003D;   // SCE_IME_KEYCODE_F4
constexpr u16 kCodeSettings = 0x003E;   // SCE_IME_KEYCODE_F5
constexpr u16 kCodeOptions = 0x0076;    // SCE_IME_KEYCODE_MENU
constexpr u16 kCodeArrowUp = 0x0052;    // SCE_IME_KEYCODE_UPARROW
constexpr u16 kCodeArrowDown = 0x0051;  // SCE_IME_KEYCODE_DOWNARROW
constexpr u16 kCodeArrowLeft = 0x0050;  // SCE_IME_KEYCODE_LEFTARROW
constexpr u16 kCodeArrowRight = 0x004F; // SCE_IME_KEYCODE_RIGHTARROW
constexpr u16 kCodeSpace = 0x002C;      // SCE_IME_KEYCODE_SPACEBAR
constexpr u16 kCodeBackspace = 0x002A;  // SCE_IME_KEYCODE_BACKSPACE
constexpr u16 kCodeReturn = 0x0028;     // SCE_IME_KEYCODE_RETURN
constexpr u16 kCodeLeftShift = 0x00E1;  // SCE_IME_KEYCODE_LEFTSHIFT
constexpr u64 kMaskDollarPreferred = static_cast<u64>(OrbisImeLanguage::ENGLISH_US) |
                                     static_cast<u64>(OrbisImeLanguage::SPANISH_LA) |
                                     static_cast<u64>(OrbisImeLanguage::PORTUGUESE_BR);
constexpr u64 kMaskPoundPreferred = static_cast<u64>(OrbisImeLanguage::ENGLISH_GB);
constexpr u64 kMaskYenPreferred = static_cast<u64>(OrbisImeLanguage::JAPANESE);
constexpr u64 kMaskWonPreferred = static_cast<u64>(OrbisImeLanguage::KOREAN);

enum class ImeKbCurrencyProfile : u8 {
    Euro = 0,
    Dollar = 1,
    Pound = 2,
    Yen = 3,
    Won = 4,
};

u16 Utf8FirstCodeUnit(const char* text) {
    if (!text || text[0] == '\0') {
        return 0;
    }
    const unsigned char c0 = static_cast<unsigned char>(text[0]);
    if ((c0 & 0x80u) == 0) {
        return static_cast<u16>(c0);
    }
    if ((c0 & 0xE0u) == 0xC0u && text[1] != '\0') {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        if ((c1 & 0xC0u) == 0x80u) {
            return static_cast<u16>(((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu));
        }
        return 0;
    }
    if ((c0 & 0xF0u) == 0xE0u && text[1] != '\0' && text[2] != '\0') {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        const unsigned char c2 = static_cast<unsigned char>(text[2]);
        if ((c1 & 0xC0u) == 0x80u && (c2 & 0xC0u) == 0x80u) {
            return static_cast<u16>(((c0 & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu));
        }
        return 0;
    }
    return 0;
}

u16 ResolveAsciiHidKeycode(u16 code_unit) {
    if (code_unit >= 'a' && code_unit <= 'z') {
        return static_cast<u16>(0x0004 + (code_unit - 'a')); // SCE_IME_KEYCODE_A...
    }
    if (code_unit >= 'A' && code_unit <= 'Z') {
        return static_cast<u16>(0x0004 + (code_unit - 'A')); // SCE_IME_KEYCODE_A...
    }
    if (code_unit >= '1' && code_unit <= '9') {
        return static_cast<u16>(0x001E + (code_unit - '1')); // SCE_IME_KEYCODE_1...
    }
    if (code_unit == '0') {
        return 0x0027; // SCE_IME_KEYCODE_0
    }

    switch (code_unit) {
    case ' ':
        return kCodeSpace;
    case '\b':
        return kCodeBackspace;
    case '\r':
    case '\n':
        return kCodeReturn;
    case '-':
    case '_':
        return 0x002D; // SCE_IME_KEYCODE_MINUS
    case '=':
    case '+':
        return 0x002E; // SCE_IME_KEYCODE_EQUAL
    case '[':
    case '{':
        return 0x002F; // SCE_IME_KEYCODE_LEFTBRACKET
    case ']':
    case '}':
        return 0x0030; // SCE_IME_KEYCODE_RIGHTBRACKET
    case '\\':
    case '|':
        return 0x0031; // SCE_IME_KEYCODE_BACKSLASH
    case ';':
    case ':':
        return 0x0033; // SCE_IME_KEYCODE_SEMICOLON
    case '\'':
    case '"':
        return 0x0034; // SCE_IME_KEYCODE_SINGLEQUOTE
    case '`':
    case '~':
        return 0x0035; // SCE_IME_KEYCODE_BACKQUOTE
    case ',':
    case '<':
        return 0x0036; // SCE_IME_KEYCODE_COMMA
    case '.':
    case '>':
        return 0x0037; // SCE_IME_KEYCODE_PERIOD
    case '/':
    case '?':
        return 0x0038; // SCE_IME_KEYCODE_SLASH
    case '!':
        return 0x001E; // Shift+1
    case '@':
        return 0x001F; // Shift+2
    case '#':
        return 0x0020; // Shift+3
    case '$':
        return 0x0021; // Shift+4
    case '%':
        return 0x0022; // Shift+5
    case '^':
        return 0x0023; // Shift+6
    case '&':
        return 0x0024; // Shift+7
    case '*':
        return 0x0025; // Shift+8
    case '(':
        return 0x0026; // Shift+9
    case ')':
        return 0x0027; // Shift+0
    default:
        return 0;
    }
}

u16 ResolveImeKeycode(const ImeKbKeySpec& key, const char* label_override = nullptr) {
    switch (key.action) {
    case ImeKbKeyAction::Character: {
        const u16 code_unit = Utf8FirstCodeUnit(label_override ? label_override : key.label);
        return ResolveAsciiHidKeycode(code_unit);
    }
    case ImeKbKeyAction::Space:
        return kCodeSpace;
    case ImeKbKeyAction::Backspace:
        return kCodeBackspace;
    case ImeKbKeyAction::NewLine:
    case ImeKbKeyAction::Done:
        return kCodeReturn;
    case ImeKbKeyAction::ArrowUp:
        return kCodeArrowUp;
    case ImeKbKeyAction::ArrowDown:
        return kCodeArrowDown;
    case ImeKbKeyAction::ArrowLeft:
    case ImeKbKeyAction::PagePrev:
        return kCodeArrowLeft;
    case ImeKbKeyAction::ArrowRight:
    case ImeKbKeyAction::PageNext:
        return kCodeArrowRight;
    case ImeKbKeyAction::SymbolsMode: {
        const char* mode_label = label_override ? label_override : key.label;
        if (mode_label && std::strcmp(mode_label, "ABC") == 0) {
            return kCodeLetters;
        }
        return kCodeSym1;
    }
    case ImeKbKeyAction::SpecialsMode: {
        const char* mode_label = label_override ? label_override : key.label;
        if (mode_label && std::strcmp(mode_label, "ABC") == 0) {
            return kCodeLetters;
        }
        return kCodeAccents;
    }
    case ImeKbKeyAction::Keyboard:
        return kCodeKeyboard;
    case ImeKbKeyAction::Menu:
        return kCodeOptions;
    case ImeKbKeyAction::Settings:
        return kCodeSettings;
    case ImeKbKeyAction::Shift:
        return kCodeLeftShift;
    case ImeKbKeyAction::None:
    default:
        return 0;
    }
}

char16_t ResolveImeCharacter(const ImeKbKeySpec& key, const char* label_override = nullptr) {
    switch (key.action) {
    case ImeKbKeyAction::Character:
        return static_cast<char16_t>(
            Utf8FirstCodeUnit(label_override ? label_override : key.label));
    case ImeKbKeyAction::Space:
        return u' ';
    case ImeKbKeyAction::NewLine:
    case ImeKbKeyAction::Done:
        return u'\r';
    default:
        return u'\0';
    }
}

ImeKbCurrencyProfile ResolveCurrencyProfile(OrbisImeLanguage supported_languages) {
    const u64 mask = static_cast<u64>(supported_languages);
    if ((mask & kMaskWonPreferred) != 0) {
        return ImeKbCurrencyProfile::Won;
    }
    if ((mask & kMaskYenPreferred) != 0) {
        return ImeKbCurrencyProfile::Yen;
    }
    if ((mask & kMaskPoundPreferred) != 0) {
        return ImeKbCurrencyProfile::Pound;
    }
    if ((mask & kMaskDollarPreferred) != 0) {
        return ImeKbCurrencyProfile::Dollar;
    }
    return ImeKbCurrencyProfile::Euro;
}

const char* GetCurrencySlotLabel(ImeKbCurrencyProfile profile, int slot) {
    switch (profile) {
    case ImeKbCurrencyProfile::Dollar: {
        static constexpr std::array<const char*, 4> kLabels = {
            kCurrencyDollar,
            kCurrencyEuro,
            kCurrencyPound,
            kCurrencyYen,
        };
        return kLabels[slot];
    }
    case ImeKbCurrencyProfile::Pound: {
        static constexpr std::array<const char*, 4> kLabels = {
            kCurrencyPound,
            kCurrencyEuro,
            kCurrencyDollar,
            kCurrencyYen,
        };
        return kLabels[slot];
    }
    case ImeKbCurrencyProfile::Yen: {
        static constexpr std::array<const char*, 4> kLabels = {
            kCurrencyYen,
            kCurrencyEuro,
            kCurrencyDollar,
            kCurrencyPound,
        };
        return kLabels[slot];
    }
    case ImeKbCurrencyProfile::Won: {
        static constexpr std::array<const char*, 4> kLabels = {
            kCurrencyWon,
            kCurrencyEuro,
            kCurrencyDollar,
            kCurrencyYen,
        };
        return kLabels[slot];
    }
    case ImeKbCurrencyProfile::Euro:
    default: {
        static constexpr std::array<const char*, 4> kLabels = {
            kCurrencyEuro,
            kCurrencyPound,
            kCurrencyYen,
            kCurrencyWon,
        };
        return kLabels[slot];
    }
    }
}

const char* ResolveSymbolOverrideLabel(const ImeKbLayoutSelection& selection,
                                       OrbisImeLanguage supported_languages,
                                       const ImeKbKeySpec& key, const char* fallback_label) {
    if (selection.family != ImeKbLayoutFamily::Symbols || key.action != ImeKbKeyAction::Character ||
        key.row != 3) {
        return fallback_label;
    }

    const bool is_page1 = (selection.page % 2) == 0;
    const ImeKbCurrencyProfile profile = ResolveCurrencyProfile(supported_languages);

    // Keep one locale-aware currency slot on symbols page 1 and preserve the
    // rest of the page for PS4 punctuation/quote marks.
    if (is_page1 && key.col == 5) {
        return GetCurrencySlotLabel(profile, 0);
    }
    return fallback_label;
}

const char* ResolveShiftOverrideLabel(const ImeKbLayoutSelection& selection,
                                      const ImeKbKeySpec& key, const char* fallback_label) {
    if (key.action != ImeKbKeyAction::Shift) {
        return fallback_label;
    }

    switch (selection.case_state) {
    case ImeKbCaseState::Upper:
    case ImeKbCaseState::CapsLock:
        return "SHIFT";
    case ImeKbCaseState::Lower:
    default:
        return "shift";
    }
}

template <std::size_t N>
constexpr ImeKbLayoutModel MakeLayoutModel(
    const std::array<ImeKbKeySpec, N>& keys, const u8 rows = static_cast<u8>(kKeyRows),
    const u8 function_rows = static_cast<u8>(ImeSelectionGridIndex::DefaultFunctionRows)) {
    return ImeKbLayoutModel{keys.data(), N, static_cast<u8>(kKeyCols), rows, function_rows};
}

const char* GetEnterLabel(OrbisImeEnterLabel label) {
    switch (label) {
    case OrbisImeEnterLabel::Go:
        return "Go";
    case OrbisImeEnterLabel::Search:
        return "Search";
    case OrbisImeEnterLabel::Send:
        return "Send";
    case OrbisImeEnterLabel::Default:
    default:
        return "Done";
    }
}

constexpr std::array<ImeKbKeySpec, 55> kLatinLowerKeys = {{
    KEY(0, 0, "1", Character),
    KEY(0, 1, "2", Character),
    KEY(0, 2, "3", Character),
    KEY(0, 3, "4", Character),
    KEY(0, 4, "5", Character),
    KEY(0, 5, "6", Character),
    KEY(0, 6, "7", Character),
    KEY(0, 7, "8", Character),
    KEY(0, 8, "9", Character),
    KEY(0, 9, "0", Character),

    KEY(1, 0, "q", Character),
    KEY(1, 1, "w", Character),
    KEY(1, 2, "e", Character),
    KEY(1, 3, "r", Character),
    KEY(1, 4, "t", Character),
    KEY(1, 5, "y", Character),
    KEY(1, 6, "u", Character),
    KEY(1, 7, "i", Character),
    KEY(1, 8, "o", Character),
    KEY(1, 9, "p", Character),

    KEY(2, 0, "a", Character),
    KEY(2, 1, "s", Character),
    KEY(2, 2, "d", Character),
    KEY(2, 3, "f", Character),
    KEY(2, 4, "g", Character),
    KEY(2, 5, "h", Character),
    KEY(2, 6, "j", Character),
    KEY(2, 7, "k", Character),
    KEY(2, 8, "l", Character),
    KEY(2, 9, "'", Character),

    KEY(3, 0, "z", Character),
    KEY(3, 1, "x", Character),
    KEY(3, 2, "c", Character),
    KEY(3, 3, "v", Character),
    KEY(3, 4, "b", Character),
    KEY(3, 5, "n", Character),
    KEY(3, 6, "m", Character),
    KEY(3, 7, ",", Character),
    KEY(3, 8, ".", Character),
    KEY(3, 9, "?", Character),

    KEYHOT(4, 0, "Shift", "L2", Shift),
    KEYHOT(4, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(4, 2, "\xC3\xA0", "L3", SpecialsMode),
    KEYSPANHOT(4, 3, 4, 1, "Space", "Tri", Space),
    KEY(4, 7, nullptr, None),
    KEYSPANHOT(4, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(5, 0, ArrowDown, ArrowDown),
    KEYGLYPH(5, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(5, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(5, 3, "R1", ArrowRight, ArrowRight),
    KEY(5, 4, nullptr, None),
    KEY(5, 5, "...", Menu),
    KEYHOT(5, 6, "Gyro", "R3", Settings),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 55> kLatinUpperKeys = {{
    KEY(0, 0, "1", Character),
    KEY(0, 1, "2", Character),
    KEY(0, 2, "3", Character),
    KEY(0, 3, "4", Character),
    KEY(0, 4, "5", Character),
    KEY(0, 5, "6", Character),
    KEY(0, 6, "7", Character),
    KEY(0, 7, "8", Character),
    KEY(0, 8, "9", Character),
    KEY(0, 9, "0", Character),

    KEY(1, 0, "Q", Character),
    KEY(1, 1, "W", Character),
    KEY(1, 2, "E", Character),
    KEY(1, 3, "R", Character),
    KEY(1, 4, "T", Character),
    KEY(1, 5, "Y", Character),
    KEY(1, 6, "U", Character),
    KEY(1, 7, "I", Character),
    KEY(1, 8, "O", Character),
    KEY(1, 9, "P", Character),

    KEY(2, 0, "A", Character),
    KEY(2, 1, "S", Character),
    KEY(2, 2, "D", Character),
    KEY(2, 3, "F", Character),
    KEY(2, 4, "G", Character),
    KEY(2, 5, "H", Character),
    KEY(2, 6, "J", Character),
    KEY(2, 7, "K", Character),
    KEY(2, 8, "L", Character),
    KEY(2, 9, "\"", Character),

    KEY(3, 0, "Z", Character),
    KEY(3, 1, "X", Character),
    KEY(3, 2, "C", Character),
    KEY(3, 3, "V", Character),
    KEY(3, 4, "B", Character),
    KEY(3, 5, "N", Character),
    KEY(3, 6, "M", Character),
    KEY(3, 7, "-", Character),
    KEY(3, 8, "_", Character),
    KEY(3, 9, "/", Character),

    KEYHOT(4, 0, "Shift", "L2", Shift),
    KEYHOT(4, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(4, 2, "\xC3\xA0", "L3", SpecialsMode),
    KEYSPANHOT(4, 3, 4, 1, "Space", "Tri", Space),
    KEY(4, 7, nullptr, None),
    KEYSPANHOT(4, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(5, 0, ArrowDown, ArrowDown),
    KEYGLYPH(5, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(5, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(5, 3, "R1", ArrowRight, ArrowRight),
    KEY(5, 4, nullptr, None),
    KEY(5, 5, "...", Menu),
    KEYHOT(5, 6, "Gyro", "R3", Settings),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 51> kSymbolsPage1Keys = {{
    KEY(0, 0, "!", Character),
    KEY(0, 1, "?", Character),
    KEY(0, 2, "\"", Character),
    KEY(0, 3, "'", Character),
    KEY(0, 4, "#", Character),
    KEY(0, 5, "%", Character),
    KEY(0, 6, "(", Character),
    KEY(0, 7, ")", Character),
    KEY(0, 8, "()", Character),
    KEY(0, 9, "/", Character),

    KEY(1, 0, "-", Character),
    KEY(1, 1, "_", Character),
    KEY(1, 2, ",", Character),
    KEY(1, 3, ".", Character),
    KEY(1, 4, ":", Character),
    KEY(1, 5, ";", Character),
    KEY(1, 6, "*", Character),
    KEY(1, 7, "+", Character),
    KEY(1, 8, "=", Character),
    KEY(1, 9, "&", Character),

    KEY(2, 0, "<", Character),
    KEY(2, 1, ">", Character),
    KEY(2, 2, "@", Character),
    KEY(2, 3, "[", Character),
    KEY(2, 4, "]", Character),
    KEY(2, 5, "[]", Character),
    KEY(2, 6, "{", Character),
    KEY(2, 7, "}", Character),
    KEY(2, 8, "{}", Character),
    KEYSPAN(2, 9, 1, 2, ">", PageNext),

    KEY(3, 0, "\\", Character),
    KEY(3, 1, "|", Character),
    KEY(3, 2, "^", Character),
    KEY(3, 3, "`", Character),
    KEY(3, 4, "$", Character),
    KEY(3, 5, "\xE2\x82\xAC", Character),
    KEY(3, 6, "\xC2\xB4", Character),
    KEY(3, 7, "\xE2\x80\x98", Character),
    KEY(3, 8, "\xE2\x80\x99", Character),

    KEYHOT(4, 1, "ABC", "L2+Tri", SymbolsMode),
    KEYSPANHOT(4, 3, 4, 1, "Space", "Tri", Space),
    KEYSPANHOT(4, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(5, 0, ArrowDown, ArrowDown),
    KEYGLYPH(5, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(5, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(5, 3, "R1", ArrowRight, ArrowRight),
    KEY(5, 4, nullptr, None),
    KEY(5, 5, "...", Menu),
    KEYHOT(5, 6, "Gyro", "R3", Settings),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 51> kSymbolsPage2Keys = {{
    KEY(0, 0, "\xE2\x80\x9A", Character),
    KEY(0, 1, "\xE2\x80\x9C", Character),
    KEY(0, 2, "\xE2\x80\x9D", Character),
    KEY(0, 3, "\xE2\x80\x9E", Character),
    KEY(0, 4, "~", Character),
    KEY(0, 5, "\xC2\xA1", Character),
    KEY(0, 6, "\xC2\xA1!", Character),
    KEY(0, 7, "\xC2\xBF", Character),
    KEY(0, 8, "\xC2\xBF?", Character),
    KEY(0, 9, "\xE2\x80\xB9", Character),

    KEY(1, 0, "\xE2\x80\xBA", Character),
    KEY(1, 1, "\xC2\xAB", Character),
    KEY(1, 2, "\xC2\xBB", Character),
    KEY(1, 3, "\xC2\xB0", Character),
    KEY(1, 4, "\xC2\xAA", Character),
    KEY(1, 5, "\xC2\xBA", Character),
    KEY(1, 6, "\xC3\x97", Character),
    KEY(1, 7, "\xC3\xB7", Character),
    KEY(1, 8, "\xC2\xA4", Character),
    KEY(1, 9, "\xC2\xA2", Character),

    KEY(2, 0, "\xC2\xA5", Character),
    KEY(2, 1, "\xC2\xA3", Character),
    KEY(2, 2, "\xE2\x82\xA9", Character),
    KEY(2, 3, "\xC2\xA7", Character),
    KEY(2, 4, "\xC2\xA6", Character),
    KEY(2, 5, "\xC2\xB5", Character),
    KEY(2, 6, "\xC2\xAC", Character),
    KEY(2, 7, "\xC2\xB9", Character),
    KEY(2, 8, "\xC2\xB2", Character),
    KEYSPAN(2, 9, 1, 2, "<", PagePrev),

    KEY(3, 0, "\xC2\xB3", Character),
    KEY(3, 1, "\xC2\xBC", Character),
    KEY(3, 2, "\xC2\xBD", Character),
    KEY(3, 3, "\xC2\xBE", Character),
    KEY(3, 4, "\xE2\x84\x96", Character),
    KEY(3, 5, "\xC2\xB7", Character),
    KEY(3, 6, nullptr, None),
    KEY(3, 7, nullptr, None),
    KEY(3, 8, nullptr, None),

    KEYHOT(4, 1, "ABC", "L2+Tri", SymbolsMode),
    KEYSPANHOT(4, 3, 4, 1, "Space", "Tri", Space),
    KEYSPANHOT(4, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(5, 0, ArrowDown, ArrowDown),
    KEYGLYPH(5, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(5, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(5, 3, "R1", ArrowRight, ArrowRight),
    KEY(5, 4, nullptr, None),
    KEY(5, 5, "...", Menu),
    KEYHOT(5, 6, "Gyro", "R3", Settings),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 65> kSpecialsPage1Keys = {{
    KEY(0, 0, "à", Character),
    KEY(0, 1, "á", Character),
    KEY(0, 2, "â", Character),
    KEY(0, 3, "ã", Character),
    KEY(0, 4, "ä", Character),
    KEY(0, 5, "å", Character),
    KEY(0, 6, "ą", Character),
    KEY(0, 7, "æ", Character),
    KEY(0, 8, "ç", Character),
    KEY(0, 9, "ć", Character),

    KEY(1, 0, "è", Character),
    KEY(1, 1, "é", Character),
    KEY(1, 2, "ê", Character),
    KEY(1, 3, "ë", Character),
    KEY(1, 4, "ę", Character),
    KEY(1, 5, "ğ", Character),
    KEY(1, 6, "ì", Character),
    KEY(1, 7, "í", Character),
    KEY(1, 8, "î", Character),
    KEY(1, 9, "ï", Character),

    KEY(2, 0, "ı", Character),
    KEY(2, 1, "ł", Character),
    KEY(2, 2, "ñ", Character),
    KEY(2, 3, "ń", Character),
    KEY(2, 4, "ò", Character),
    KEY(2, 5, "ó", Character),
    KEY(2, 6, "ô", Character),
    KEY(2, 7, "õ", Character),
    KEY(2, 8, "ö", Character),
    KEY(2, 9, "ø", Character),

    KEY(3, 0, "œ", Character),
    KEY(3, 1, "ś", Character),
    KEY(3, 2, "ş", Character),
    KEY(3, 3, "š", Character),
    KEY(3, 4, "ß", Character),
    KEY(3, 5, "ù", Character),
    KEY(3, 6, "ú", Character),
    KEY(3, 7, "û", Character),
    KEY(3, 8, "ü", Character),
    KEY(3, 9, "ý", Character),

    KEY(4, 0, "ÿ", Character),
    KEY(4, 1, "ź", Character),
    KEY(4, 2, "ż", Character),
    KEY(4, 3, "ž", Character),
    KEY(4, 4, "ð", Character),
    KEY(4, 5, "þ", Character),
    KEY(4, 6, nullptr, None),
    KEY(4, 7, nullptr, None),
    KEY(4, 8, nullptr, None),
    KEY(4, 9, nullptr, None),

    KEYHOT(5, 0, "Shift", "L2", Shift),
    KEYHOT(5, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(5, 2, "ABC", "L3", SpecialsMode),
    KEYSPANHOT(5, 3, 4, 1, "Space", "Tri", Space),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(6, 0, ArrowDown, ArrowDown),
    KEYGLYPH(6, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(6, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(6, 3, "R1", ArrowRight, ArrowRight),
    KEY(6, 4, nullptr, None),
    KEY(6, 5, "...", Menu),
    KEYHOT(6, 6, "Gyro", "R3", Settings),
    KEY(6, 7, nullptr, None),
    KEYSPANHOT(6, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 65> kSpecialsPage1UpperKeys = {{
    KEY(0, 0, "À", Character),
    KEY(0, 1, "Á", Character),
    KEY(0, 2, "Â", Character),
    KEY(0, 3, "Ã", Character),
    KEY(0, 4, "Ä", Character),
    KEY(0, 5, "Å", Character),
    KEY(0, 6, "Ą", Character),
    KEY(0, 7, "Æ", Character),
    KEY(0, 8, "Ç", Character),
    KEY(0, 9, "Ć", Character),

    KEY(1, 0, "È", Character),
    KEY(1, 1, "É", Character),
    KEY(1, 2, "Ê", Character),
    KEY(1, 3, "Ë", Character),
    KEY(1, 4, "Ę", Character),
    KEY(1, 5, "Ğ", Character),
    KEY(1, 6, "Ì", Character),
    KEY(1, 7, "Í", Character),
    KEY(1, 8, "Î", Character),
    KEY(1, 9, "Ï", Character),

    KEY(2, 0, "İ", Character),
    KEY(2, 1, "Ł", Character),
    KEY(2, 2, "Ñ", Character),
    KEY(2, 3, "Ń", Character),
    KEY(2, 4, "Ò", Character),
    KEY(2, 5, "Ó", Character),
    KEY(2, 6, "Ô", Character),
    KEY(2, 7, "Õ", Character),
    KEY(2, 8, "Ö", Character),
    KEY(2, 9, "Ø", Character),

    KEY(3, 0, "Œ", Character),
    KEY(3, 1, "Ś", Character),
    KEY(3, 2, "Ş", Character),
    KEY(3, 3, "Š", Character),
    KEY(3, 4, "ß", Character),
    KEY(3, 5, "Ù", Character),
    KEY(3, 6, "Ú", Character),
    KEY(3, 7, "Û", Character),
    KEY(3, 8, "Ü", Character),
    KEY(3, 9, "Ý", Character),

    KEY(4, 0, "Ÿ", Character),
    KEY(4, 1, "Ź", Character),
    KEY(4, 2, "Ż", Character),
    KEY(4, 3, "Ž", Character),
    KEY(4, 4, "Ð", Character),
    KEY(4, 5, "Þ", Character),
    KEY(4, 6, nullptr, None),
    KEY(4, 7, nullptr, None),
    KEY(4, 8, nullptr, None),
    KEY(4, 9, nullptr, None),

    KEYHOT(5, 0, "Shift", "L2", Shift),
    KEYHOT(5, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(5, 2, "ABC", "L3", SpecialsMode),
    KEYSPANHOT(5, 3, 4, 1, "Space", "Tri", Space),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(6, 0, ArrowDown, ArrowDown),
    KEYGLYPH(6, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(6, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(6, 3, "R1", ArrowRight, ArrowRight),
    KEY(6, 4, nullptr, None),
    KEY(6, 5, "...", Menu),
    KEYHOT(6, 6, "Gyro", "R3", Settings),
    KEY(6, 7, nullptr, None),
    KEYSPANHOT(6, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 55> kSpecialsPage2Keys = {{
    KEY(0, 0, "ÿ", Character),
    KEY(0, 1, "ź", Character),
    KEY(0, 2, "ż", Character),
    KEY(0, 3, "ž", Character),
    KEY(0, 4, "ð", Character),
    KEY(0, 5, "þ", Character),
    KEY(0, 6, nullptr, None),
    KEY(0, 7, nullptr, None),
    KEY(0, 8, nullptr, None),
    KEY(0, 9, nullptr, None),

    KEY(1, 0, nullptr, None),
    KEY(1, 1, nullptr, None),
    KEY(1, 2, nullptr, None),
    KEY(1, 3, nullptr, None),
    KEY(1, 4, nullptr, None),
    KEY(1, 5, nullptr, None),
    KEY(1, 6, nullptr, None),
    KEY(1, 7, nullptr, None),
    KEY(1, 8, nullptr, None),
    KEY(1, 9, nullptr, None),

    KEY(2, 0, nullptr, None),
    KEY(2, 1, nullptr, None),
    KEY(2, 2, nullptr, None),
    KEY(2, 3, nullptr, None),
    KEY(2, 4, nullptr, None),
    KEY(2, 5, nullptr, None),
    KEY(2, 6, nullptr, None),
    KEY(2, 7, nullptr, None),
    KEY(2, 8, nullptr, None),
    KEY(2, 9, nullptr, None),

    KEY(3, 0, nullptr, None),
    KEY(3, 1, nullptr, None),
    KEY(3, 2, nullptr, None),
    KEY(3, 3, nullptr, None),
    KEY(3, 4, nullptr, None),
    KEY(3, 5, nullptr, None),
    KEY(3, 6, nullptr, None),
    KEY(3, 7, nullptr, None),
    KEY(3, 8, nullptr, None),
    KEY(3, 9, nullptr, None),

    KEYHOT(5, 0, "Shift", "L2", Shift),
    KEYHOT(5, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(5, 2, "ABC", "L3", SpecialsMode),
    KEYSPANHOT(5, 3, 4, 1, "Space", "Tri", Space),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(6, 0, ArrowDown, ArrowDown),
    KEYGLYPH(6, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(6, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(6, 3, "R1", ArrowRight, ArrowRight),
    KEY(6, 4, nullptr, None),
    KEY(6, 5, "...", Menu),
    KEYHOT(6, 6, "Gyro", "R3", Settings),
    KEY(6, 7, nullptr, None),
    KEYSPANHOT(6, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr std::array<ImeKbKeySpec, 55> kSpecialsPage2UpperKeys = {{
    KEY(0, 0, "Ÿ", Character),
    KEY(0, 1, "Ź", Character),
    KEY(0, 2, "Ż", Character),
    KEY(0, 3, "Ž", Character),
    KEY(0, 4, "Ð", Character),
    KEY(0, 5, "Þ", Character),
    KEY(0, 6, nullptr, None),
    KEY(0, 7, nullptr, None),
    KEY(0, 8, nullptr, None),
    KEY(0, 9, nullptr, None),

    KEY(1, 0, nullptr, None),
    KEY(1, 1, nullptr, None),
    KEY(1, 2, nullptr, None),
    KEY(1, 3, nullptr, None),
    KEY(1, 4, nullptr, None),
    KEY(1, 5, nullptr, None),
    KEY(1, 6, nullptr, None),
    KEY(1, 7, nullptr, None),
    KEY(1, 8, nullptr, None),
    KEY(1, 9, nullptr, None),

    KEY(2, 0, nullptr, None),
    KEY(2, 1, nullptr, None),
    KEY(2, 2, nullptr, None),
    KEY(2, 3, nullptr, None),
    KEY(2, 4, nullptr, None),
    KEY(2, 5, nullptr, None),
    KEY(2, 6, nullptr, None),
    KEY(2, 7, nullptr, None),
    KEY(2, 8, nullptr, None),
    KEY(2, 9, nullptr, None),

    KEY(3, 0, nullptr, None),
    KEY(3, 1, nullptr, None),
    KEY(3, 2, nullptr, None),
    KEY(3, 3, nullptr, None),
    KEY(3, 4, nullptr, None),
    KEY(3, 5, nullptr, None),
    KEY(3, 6, nullptr, None),
    KEY(3, 7, nullptr, None),
    KEY(3, 8, nullptr, None),
    KEY(3, 9, nullptr, None),

    KEYHOT(5, 0, "Shift", "L2", Shift),
    KEYHOT(5, 1, "@#:", "L2+Tri", SymbolsMode),
    KEYHOT(5, 2, "ABC", "L3", SpecialsMode),
    KEYSPANHOT(5, 3, 4, 1, "Space", "Tri", Space),
    KEY(5, 7, nullptr, None),
    KEYSPANHOT(5, 8, 2, 1, "Backspace", "Sq", Backspace),

    KEYGLYPH(6, 0, ArrowDown, ArrowDown),
    KEYGLYPH(6, 1, ArrowUp, ArrowUp),
    KEYHOTGLYPH(6, 2, "L1", ArrowLeft, ArrowLeft),
    KEYHOTGLYPH(6, 3, "R1", ArrowRight, ArrowRight),
    KEY(6, 4, nullptr, None),
    KEY(6, 5, "...", Menu),
    KEYHOT(6, 6, "Gyro", "R3", Settings),
    KEY(6, 7, nullptr, None),
    KEYSPANHOT(6, 8, 2, 1, nullptr, "R2", Done),
}};

constexpr ImeKbLayoutModel kLatinLowerModel = MakeLayoutModel(kLatinLowerKeys);
constexpr ImeKbLayoutModel kLatinUpperModel = MakeLayoutModel(kLatinUpperKeys);
constexpr ImeKbLayoutModel kSymbolsPage1Model = MakeLayoutModel(kSymbolsPage1Keys);
constexpr ImeKbLayoutModel kSymbolsPage2Model = MakeLayoutModel(kSymbolsPage2Keys);
constexpr ImeKbLayoutModel kSpecialsPage1Model =
    MakeLayoutModel(kSpecialsPage1Keys, static_cast<u8>(kSpecialsKeyRows));
constexpr ImeKbLayoutModel kSpecialsPage2Model =
    MakeLayoutModel(kSpecialsPage2Keys, static_cast<u8>(kSpecialsKeyRows));
constexpr ImeKbLayoutModel kSpecialsPage1UpperModel =
    MakeLayoutModel(kSpecialsPage1UpperKeys, static_cast<u8>(kSpecialsKeyRows));
constexpr ImeKbLayoutModel kSpecialsPage2UpperModel =
    MakeLayoutModel(kSpecialsPage2UpperKeys, static_cast<u8>(kSpecialsKeyRows));

constexpr std::array<ImeTopPanelElementSpec, 2> kTopPanelDefaultElements = {{
    ImeTopPanelElementSpec{ImeTopPanelElementId::Prediction, 0, 9},
    ImeTopPanelElementSpec{ImeTopPanelElementId::Close, 9, 1},
}};

constexpr ImeTopPanelLayoutConfig kTopPanelDefaultLayout{
    kTopPanelDefaultElements.data(),
    kTopPanelDefaultElements.size(),
    10,
    static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRow),
    static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRows),
};

const ImeKbLayoutModel& GetDefaultLayoutModel() {
    return kLatinLowerModel;
}

const ImeTopPanelLayoutConfig& GetDefaultTopPanelLayoutConfig() {
    return kTopPanelDefaultLayout;
}

#undef KEY
#undef KEYHOT
#undef KEYSPAN
#undef KEYSPANHOT
#undef KEYGLYPH
#undef KEYHOTGLYPH
#undef KEYSPANGLYPH
#undef KEYSPANHOTGLYPH

} // namespace

namespace {

void AddImeKeyLabelGlyphs(ImFontGlyphRangesBuilder& builder, const char* label) {
    if (!label || label[0] == '\0') {
        return;
    }
    builder.AddText(label);
}

void AddImeLayoutGlyphs(ImFontGlyphRangesBuilder& builder, const ImeKbLayoutModel& model,
                        const ImeKbLayoutSelection& selection,
                        const OrbisImeLanguage supported_languages) {
    if (!model.keys || model.key_count == 0) {
        return;
    }

    for (std::size_t i = 0; i < model.key_count; ++i) {
        const ImeKbKeySpec& key = model.keys[i];
        const bool is_done = key.action == ImeKbKeyAction::Done;
        const char* label = key.label;
        if (!is_done) {
            label = ResolveSymbolOverrideLabel(selection, supported_languages, key, label);
            label = ResolveShiftOverrideLabel(selection, key, label);
        }
        AddImeKeyLabelGlyphs(builder, label);
        AddImeKeyLabelGlyphs(builder, key.hotkey_label);
    }
}

} // namespace

void AddImeKeyboardGlyphsToFontRanges(ImFontGlyphRangesBuilder& builder) {
    // Resolve labels via real layout selection paths so accent/symbol overrides stay complete.
    constexpr std::array<OrbisImeLanguage, 5> kLanguageMasks = {
        static_cast<OrbisImeLanguage>(0), OrbisImeLanguage::ENGLISH_US,
        OrbisImeLanguage::ENGLISH_GB,     OrbisImeLanguage::JAPANESE,
        OrbisImeLanguage::KOREAN,
    };
    constexpr std::array<ImeKbLayoutFamily, 3> kFamilies = {
        ImeKbLayoutFamily::Latin,
        ImeKbLayoutFamily::Symbols,
        ImeKbLayoutFamily::Specials,
    };
    constexpr std::array<ImeKbCaseState, 3> kCaseStates = {
        ImeKbCaseState::Lower,
        ImeKbCaseState::Upper,
        ImeKbCaseState::CapsLock,
    };

    for (const OrbisImeLanguage language_mask : kLanguageMasks) {
        for (const ImeKbLayoutFamily family : kFamilies) {
            for (const ImeKbCaseState case_state : kCaseStates) {
                for (u8 page = 0; page < 2; ++page) {
                    const ImeKbLayoutSelection selection{
                        .family = family,
                        .case_state = case_state,
                        .page = page,
                    };
                    const ImeKbLayoutModel& model = GetImeKeyboardLayout(selection);
                    AddImeLayoutGlyphs(builder, model, selection, language_mask);
                }
            }
        }
    }

    // Done key label is injected dynamically from game params.
    AddImeKeyLabelGlyphs(builder, GetEnterLabel(OrbisImeEnterLabel::Default));
    AddImeKeyLabelGlyphs(builder, GetEnterLabel(OrbisImeEnterLabel::Go));
    AddImeKeyLabelGlyphs(builder, GetEnterLabel(OrbisImeEnterLabel::Search));
    AddImeKeyLabelGlyphs(builder, GetEnterLabel(OrbisImeEnterLabel::Send));
}

ImeViewportMetrics ComputeImeViewportMetrics(bool use_over2k) {
    ImeViewportMetrics metrics{};
    metrics.base_w = use_over2k ? 3840.0f : 1920.0f;
    metrics.base_h = use_over2k ? 2160.0f : 1080.0f;

    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 base_pos = viewport ? viewport->WorkPos : ImVec2{0.0f, 0.0f};
    ImVec2 base_size = viewport ? viewport->WorkSize : io.DisplaySize;
    if (base_size.x <= 0.0f || base_size.y <= 0.0f) {
        base_pos = {0.0f, 0.0f};
        base_size = io.DisplaySize;
    }

    metrics.size = base_size;
    metrics.offset = base_pos;

    const auto out_res = DebugState.output_resolution;
    if (out_res.first != 0 && out_res.second != 0) {
        const float fb_scale_x =
            io.DisplayFramebufferScale.x > 0.0f ? io.DisplayFramebufferScale.x : 1.0f;
        const float fb_scale_y =
            io.DisplayFramebufferScale.y > 0.0f ? io.DisplayFramebufferScale.y : 1.0f;
        const float viewport_w = static_cast<float>(out_res.first) / fb_scale_x;
        const float viewport_h = static_cast<float>(out_res.second) / fb_scale_y;

        float offset_x = (base_size.x - viewport_w) * 0.5f;
        float offset_y = (base_size.y - viewport_h) * 0.5f;
        if (offset_x < 0.0f) {
            offset_x = 0.0f;
        }
        if (offset_y < 0.0f) {
            offset_y = 0.0f;
        }

        metrics.size = {viewport_w, viewport_h};
        metrics.offset = {base_pos.x + offset_x, base_pos.y + offset_y};
    }

    metrics.scale_x = metrics.size.x / metrics.base_w;
    metrics.scale_y = metrics.size.y / metrics.base_h;
    metrics.ui_scale = std::min(metrics.scale_x, metrics.scale_y);
    return metrics;
}

ImePanelMetrics ComputeImePanelMetrics(const ImePanelMetricsConfig& config) {
    ImePanelMetrics metrics{};
    metrics.panel_w = config.panel_w;
    metrics.panel_h = config.panel_h;
    metrics.padding_x = metrics.panel_w * (kPadX / kPanelBaseW);
    metrics.padding_bottom =
        metrics.panel_h * (config.multiline ? (kPadBottomMulti / kPanelBaseHMulti)
                                            : (kPadBottomSingle / kPanelBaseHSingle));

    const float panel_scale = metrics.panel_w / kPanelBaseW;
    metrics.label_h = config.show_title ? (kLabelH * panel_scale) : metrics.padding_bottom;
    metrics.input_h = metrics.panel_h * (config.multiline ? (kInputHMulti / kPanelBaseHMulti)
                                                          : (kInputHSingle / kPanelBaseHSingle));
    metrics.predict_h = metrics.panel_h * (config.multiline ? (kPredictH / kPanelBaseHMulti)
                                                            : (kPredictH / kPanelBaseHSingle));
    metrics.close_w = metrics.panel_w * (kCloseW / kPanelBaseW);
    metrics.keys_h = metrics.panel_h * (config.multiline ? (kKeysH / kPanelBaseHMulti)
                                                         : (kKeysH / kPanelBaseHSingle));
    metrics.key_gap = metrics.panel_w * (kKeyGap / kPanelBaseW);
    metrics.corner_radius = metrics.panel_w * kCornerRatio;

    const float base_font_size = config.base_font_size > 0.0f ? config.base_font_size : 1.0f;
    metrics.label_font_scale = (metrics.label_h * 0.85f) / base_font_size;
    metrics.input_font_scale =
        (metrics.input_h * (config.multiline
                                ? (kMultiLineTextFill / static_cast<float>(kMultiLineVisibleLines))
                                : kSingleLineTextFill)) /
        base_font_size;
    metrics.key_font_scale = (metrics.panel_h * kKeyFontRatio) / base_font_size;

    metrics.input_pos_local = {metrics.padding_x, metrics.label_h};
    metrics.input_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.input_h};
    metrics.input_pos_screen = {config.window_pos.x + metrics.input_pos_local.x,
                                config.window_pos.y + metrics.input_pos_local.y};

    const float remaining_gap =
        metrics.panel_h - (metrics.label_h + metrics.input_h + metrics.predict_h + metrics.keys_h +
                           metrics.padding_bottom);
    metrics.predict_gap = std::max(0.0f, remaining_gap * 0.5f);

    metrics.predict_pos = {config.window_pos.x, config.window_pos.y + metrics.label_h +
                                                    metrics.input_h + metrics.predict_gap};
    metrics.predict_size = {metrics.panel_w * (kPredictW / kPanelBaseW), metrics.predict_h};
    metrics.close_pos = {config.window_pos.x + metrics.panel_w - metrics.close_w,
                         metrics.predict_pos.y};
    metrics.close_size = {metrics.close_w, metrics.predict_h};

    metrics.kb_pos = {config.window_pos.x + metrics.padding_x,
                      metrics.predict_pos.y + metrics.predict_h + metrics.predict_gap};
    metrics.kb_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.keys_h};

    metrics.key_h =
        (metrics.kb_size.y - metrics.key_gap * static_cast<float>(kKeyRows - 1)) / kKeyRows;
    if (metrics.key_h < 8.0f) {
        metrics.key_h = 8.0f;
    }

    return metrics;
}

ImeKbLayoutId ResolveImeKeyboardLayoutId(const ImeKbLayoutSelection& selection) {
    switch (selection.family) {
    case ImeKbLayoutFamily::Latin:
        switch (selection.case_state) {
        case ImeKbCaseState::Upper:
            return ImeKbLayoutId::LatinUpper;
        case ImeKbCaseState::CapsLock:
            return ImeKbLayoutId::LatinCapsLock;
        case ImeKbCaseState::Lower:
        default:
            return ImeKbLayoutId::LatinLower;
        }
    case ImeKbLayoutFamily::Symbols:
        return (selection.page % 2 == 0) ? ImeKbLayoutId::SymbolsPage1
                                         : ImeKbLayoutId::SymbolsPage2;
    case ImeKbLayoutFamily::Specials:
        return (selection.page % 2 == 0) ? ImeKbLayoutId::SpecialsPage1
                                         : ImeKbLayoutId::SpecialsPage2;
    default:
        return ImeKbLayoutId::LatinLower;
    }
}

const ImeKbLayoutModel& GetImeKeyboardLayout(ImeKbLayoutId id) {
    switch (id) {
    case ImeKbLayoutId::LatinLower:
        return kLatinLowerModel;
    case ImeKbLayoutId::LatinUpper:
        return kLatinUpperModel;
    case ImeKbLayoutId::LatinCapsLock:
        return kLatinUpperModel;
    case ImeKbLayoutId::SymbolsPage1:
        return kSymbolsPage1Model;
    case ImeKbLayoutId::SymbolsPage2:
        return kSymbolsPage2Model;
    case ImeKbLayoutId::SpecialsPage1:
        return kSpecialsPage1Model;
    case ImeKbLayoutId::SpecialsPage2:
        return kSpecialsPage2Model;
    default:
        return GetDefaultLayoutModel();
    }
}

const ImeKbLayoutModel& GetImeKeyboardLayout(const ImeKbLayoutSelection& selection) {
    if (selection.family == ImeKbLayoutFamily::Specials &&
        selection.case_state != ImeKbCaseState::Lower) {
        const bool page1 = (selection.page % 2) == 0;
        return page1 ? kSpecialsPage1UpperModel : kSpecialsPage2UpperModel;
    }
    return GetImeKeyboardLayout(ResolveImeKeyboardLayoutId(selection));
}

const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig() {
    return GetDefaultTopPanelLayoutConfig();
}

const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig(ImeKbLayoutId id) {
    switch (id) {
    case ImeKbLayoutId::LatinLower:
    case ImeKbLayoutId::LatinUpper:
    case ImeKbLayoutId::LatinCapsLock:
    case ImeKbLayoutId::SymbolsPage1:
    case ImeKbLayoutId::SymbolsPage2:
    case ImeKbLayoutId::SpecialsPage1:
    case ImeKbLayoutId::SpecialsPage2:
    default:
        return GetDefaultTopPanelLayoutConfig();
    }
}

const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig(const ImeKbLayoutSelection& selection) {
    return GetImeTopPanelLayoutConfig(ResolveImeKeyboardLayoutId(selection));
}

ImU32 ImeColorToImU32(const OrbisImeColor& color) {
    return IM_COL32(color.r, color.g, color.b, color.a);
}

ImVec4 ImeColorToImVec4(const OrbisImeColor& color) {
    return ImGui::ColorConvertU32ToFloat4(ImeColorToImU32(color));
}

ImeStyleConfig GetDefaultImeStyleConfig() {
    return ImeStyleConfig{};
}

ImeStyleConfig ResolveImeStyleConfig(const OrbisImeParamExtended* extended) {
    ImeStyleConfig style = GetDefaultImeStyleConfig();
    if (!extended) {
        return style;
    }
    if (!True(extended->option & OrbisImeExtOption::SET_COLOR)) {
        return style;
    }

    style.color_base = extended->color_base;
    style.color_line = extended->color_line;
    style.color_text_field = extended->color_text_field;
    style.color_preedit = extended->color_preedit;
    style.color_button_default = extended->color_button_default;
    style.color_button_function = extended->color_button_function;
    style.color_button_symbol = extended->color_button_symbol;
    style.color_text = extended->color_text;
    style.color_special = extended->color_special;
    return style;
}

void ApplyImeStyleToKeyboardDrawParams(const ImeStyleConfig& style, ImeKbDrawParams& params) {
    params.key_bg_default = ImeColorToImU32(style.color_button_default);
    params.key_bg_function = ImeColorToImU32(style.color_button_function);
    params.key_bg_symbol = ImeColorToImU32(style.color_button_symbol);
    params.key_border = ImeColorToImU32(style.color_line);
    params.key_done = ImeColorToImU32(style.color_special);
    params.key_text = ImeColorToImU32(style.color_text);

    ImVec4 hotkey = ImeColorToImVec4(style.color_text);
    hotkey.x = std::clamp(hotkey.x * 0.92f, 0.0f, 1.0f);
    hotkey.y = std::clamp(hotkey.y * 0.92f, 0.0f, 1.0f);
    hotkey.z = std::clamp(hotkey.z * 0.92f, 0.0f, 1.0f);
    params.key_hotkey_text = ImGui::ColorConvertFloat4ToU32(hotkey);
}

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state) {
    state.done_pressed = false;
    state.pressed_action = ImeKbKeyAction::None;
    state.pressed_label = nullptr;
    state.pressed_keycode = 0;
    state.pressed_character = u'\0';
    state.selected_row = -1;
    state.selected_col = -1;
    state.selected_center = {};
    state.hovered = false;
    state.clicked = false;

    auto* draw = ImGui::GetWindowDrawList();
    if (!draw || layout.cols <= 0 || layout.rows <= 0 || layout.size.x <= 0.0f ||
        layout.size.y <= 0.0f) {
        return;
    }

    const ImeKbLayoutModel* model = params.layout_model;
    if (!model) {
        model = &GetImeKeyboardLayout(params.selection);
    }
    if (!model || !model->keys || model->key_count == 0) {
        model = &GetDefaultLayoutModel();
    }

    const int grid_cols = layout.cols;
    const int grid_rows = layout.rows;
    const float key_gap_x = layout.key_gap_x;
    const float key_gap_y = layout.key_gap_y;
    const float key_h = layout.key_h;
    const float key_w = (layout.size.x - key_gap_x * (grid_cols - 1)) / grid_cols;
    const int fixed_bottom_rows = std::clamp(layout.fixed_bottom_rows, 0, grid_rows);

    thread_local std::vector<float> row_heights;
    row_heights.assign(static_cast<std::size_t>(grid_rows), key_h);
    if (fixed_bottom_rows > 0 && layout.bottom_row_h > 0.0f) {
        for (int row = grid_rows - fixed_bottom_rows; row < grid_rows; ++row) {
            row_heights[static_cast<std::size_t>(row)] = layout.bottom_row_h;
        }
    }

    thread_local std::vector<float> row_offsets;
    row_offsets.resize(static_cast<std::size_t>(grid_rows));
    float y_cursor = layout.pos.y;
    for (int row = 0; row < grid_rows; ++row) {
        row_offsets[static_cast<std::size_t>(row)] = y_cursor;
        y_cursor += row_heights[static_cast<std::size_t>(row)];
        if (row + 1 < grid_rows) {
            y_cursor += key_gap_y;
        }
    }

    const auto span_row_height = [&](int row, int span) {
        float total = 0.0f;
        for (int i = 0; i < span; ++i) {
            total += row_heights[static_cast<std::size_t>(row + i)];
        }
        if (span > 1) {
            total += key_gap_y * static_cast<float>(span - 1);
        }
        return total;
    };

    const auto idx = [grid_cols](int row, int col) { return row * grid_cols + col; };
    thread_local std::vector<const ImeKbKeySpec*> occupied;
    thread_local std::vector<bool> is_anchor;
    occupied.assign(static_cast<std::size_t>(grid_rows * grid_cols), nullptr);
    is_anchor.assign(static_cast<std::size_t>(grid_rows * grid_cols), false);

    for (std::size_t i = 0; i < model->key_count; ++i) {
        const ImeKbKeySpec& key = model->keys[i];
        if (key.col_span == 0 || key.row_span == 0) {
            continue;
        }
        if (key.row >= grid_rows || key.col >= grid_cols) {
            continue;
        }

        const int row_start = key.row;
        const int col_start = key.col;
        const int row_end = std::min(grid_rows, static_cast<int>(key.row + key.row_span));
        const int col_end = std::min(grid_cols, static_cast<int>(key.col + key.col_span));
        is_anchor[static_cast<std::size_t>(idx(row_start, col_start))] = true;
        for (int row = row_start; row < row_end; ++row) {
            for (int col = col_start; col < col_end; ++col) {
                occupied[static_cast<std::size_t>(idx(row, col))] = &key;
            }
        }
    }

    struct RenderedKey {
        const ImeKbKeySpec* key = nullptr;
        int row = 0;
        int col = 0;
        u8 col_span = 1;
        u8 row_span = 1;
        ImVec2 pos{};
        ImVec2 size{};
        ImVec2 center{};
        ImU32 bg = 0;
        const char* label = nullptr;
        const char* hotkey_label = nullptr;
        ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None;
        bool underline_label = false;
        bool is_done = false;
        bool disabled_visual = false;
        bool selectable = false;
    };

    thread_local std::vector<RenderedKey> rendered_keys;
    rendered_keys.clear();
    rendered_keys.reserve(static_cast<std::size_t>(grid_cols * grid_rows));

    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            const std::size_t cell_index = static_cast<std::size_t>(idx(row, col));
            const ImeKbKeySpec* key = occupied[cell_index];
            if (key && !is_anchor[cell_index]) {
                continue;
            }

            u8 col_span = 1;
            u8 row_span = 1;
            if (key) {
                col_span =
                    static_cast<u8>(std::min(grid_cols - col, static_cast<int>(key->col_span)));
                row_span =
                    static_cast<u8>(std::min(grid_rows - row, static_cast<int>(key->row_span)));
            }

            const float x = layout.pos.x + col * (key_w + key_gap_x);
            const float y = row_offsets[static_cast<std::size_t>(row)];
            const float w = key_w * col_span + key_gap_x * (col_span - 1);
            const float h = span_row_height(row, row_span);
            ImVec2 pos{x, y};
            ImVec2 size{w, h};

            const auto slot_base_bg = [&](int slot_row) {
                const bool function_row =
                    fixed_bottom_rows > 0 && slot_row >= std::max(0, grid_rows - fixed_bottom_rows);
                if (function_row) {
                    return params.key_bg_function;
                }
                if (params.selection.family == ImeKbLayoutFamily::Symbols) {
                    return params.key_bg_symbol;
                }
                return params.key_bg_default;
            };

            ImU32 bg = slot_base_bg(row);
            bool is_done = false;
            const char* label = nullptr;
            const char* hotkey_label = nullptr;
            ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None;
            bool underline_label = false;
            bool disabled_visual = false;
            if (key) {
                const bool symbols_layout = params.selection.family == ImeKbLayoutFamily::Symbols;
                const bool typing_key = key->action == ImeKbKeyAction::Character;
                const bool function_key =
                    key->action != ImeKbKeyAction::Character && key->action != ImeKbKeyAction::None;
                if (symbols_layout && typing_key) {
                    bg = params.key_bg_symbol;
                } else if (function_key) {
                    bg = params.key_bg_function;
                } else if (key->action == ImeKbKeyAction::None) {
                    bg = slot_base_bg(row);
                } else {
                    bg = params.key_bg_default;
                }
                is_done = key->action == ImeKbKeyAction::Done;
                if (is_done) {
                    bg = params.key_done;
                    label = GetEnterLabel(params.enter_label);
                } else {
                    label = key->label;
                }
                hotkey_label = key->hotkey_label;
                glyph = key->glyph;

                if (!is_done) {
                    label = ResolveSymbolOverrideLabel(params.selection, params.supported_languages,
                                                       *key, label);
                    label = ResolveShiftOverrideLabel(params.selection, *key, label);
                    underline_label = key->action == ImeKbKeyAction::Shift &&
                                      params.selection.case_state == ImeKbCaseState::CapsLock;
                }
            }

            rendered_keys.push_back(RenderedKey{
                .key = key,
                .row = row,
                .col = col,
                .col_span = col_span,
                .row_span = row_span,
                .pos = pos,
                .size = size,
                .center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f},
                .bg = bg,
                .label = label,
                .hotkey_label = hotkey_label,
                .glyph = glyph,
                .underline_label = underline_label,
                .is_done = is_done,
                .disabled_visual = disabled_visual,
                .selectable = (key && key->action != ImeKbKeyAction::None),
            });
        }
    }

    struct ImeKbNavState {
        int cursor_row = -1;
        int cursor_col = -1;
        int fallback_prefer_col_dir = 0;
        ImeEdgeWrapNavState edge_wrap_nav{};
        int last_grid_rows = -1;
        int last_grid_cols = -1;
    };
    static std::unordered_map<ImGuiID, ImeKbNavState> s_nav_states;
    const ImGuiID nav_id = ImGui::GetID("##ImeKbGridNav");
    ImeKbNavState& nav_state = s_nav_states[nav_id];
    const bool grid_shape_changed =
        nav_state.last_grid_rows != grid_rows || nav_state.last_grid_cols != grid_cols;
    if (grid_shape_changed) {
        nav_state.last_grid_rows = grid_rows;
        nav_state.last_grid_cols = grid_cols;
        ResetImeEdgeWrapNav(nav_state.edge_wrap_nav);
    }
    if (params.reset_nav_state) {
        ResetImeEdgeWrapNav(nav_state.edge_wrap_nav);
    }

    const auto is_selectable_cell = [&](int row, int col) {
        if (row < 0 || row >= grid_rows || col < 0 || col >= grid_cols) {
            return false;
        }
        const ImeKbKeySpec* key = occupied[static_cast<std::size_t>(idx(row, col))];
        return key && key->action != ImeKbKeyAction::None;
    };

    const auto first_selectable_cell = [&]() -> std::pair<int, int> {
        for (int row = 0; row < grid_rows; ++row) {
            for (int col = 0; col < grid_cols; ++col) {
                if (is_selectable_cell(row, col)) {
                    return {row, col};
                }
            }
        }
        return {-1, -1};
    };

    const auto nearest_selectable_cell = [&](int from_row, int from_col) -> std::pair<int, int> {
        int best_row = -1;
        int best_col = -1;
        int best_distance = std::numeric_limits<int>::max();
        for (int row = 0; row < grid_rows; ++row) {
            for (int col = 0; col < grid_cols; ++col) {
                if (!is_selectable_cell(row, col)) {
                    continue;
                }
                const int distance = std::abs(row - from_row) + std::abs(col - from_col);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_row = row;
                    best_col = col;
                }
            }
        }
        return {best_row, best_col};
    };

    const auto nearest_selectable_cell_on_row = [&](int from_row, int from_col,
                                                    int prefer_col_dir) -> std::pair<int, int> {
        if (from_row < 0 || from_row >= grid_rows) {
            return {-1, -1};
        }

        int best_col = -1;
        int best_distance = std::numeric_limits<int>::max();
        bool best_in_direction = false;
        for (int col = 0; col < grid_cols; ++col) {
            if (!is_selectable_cell(from_row, col)) {
                continue;
            }

            const int distance = std::abs(col - from_col);
            const bool in_direction =
                (prefer_col_dir > 0 && col > from_col) || (prefer_col_dir < 0 && col < from_col);
            const bool better_tie = (prefer_col_dir != 0 && in_direction != best_in_direction)
                                        ? in_direction
                                        : col < best_col;
            if (distance < best_distance ||
                (distance == best_distance && (best_col < 0 || better_tie))) {
                best_distance = distance;
                best_col = col;
                best_in_direction = in_direction;
            }
        }

        return best_col >= 0 ? std::pair<int, int>{from_row, best_col}
                             : std::pair<int, int>{-1, -1};
    };

    const auto visible_cell_for_cursor = [&]() -> std::pair<int, int> {
        if (is_selectable_cell(nav_state.cursor_row, nav_state.cursor_col)) {
            return {nav_state.cursor_row, nav_state.cursor_col};
        }
        auto row_fallback = nearest_selectable_cell_on_row(
            nav_state.cursor_row, nav_state.cursor_col, nav_state.fallback_prefer_col_dir);
        if (row_fallback.first >= 0 && row_fallback.second >= 0) {
            return row_fallback;
        }
        return nearest_selectable_cell(nav_state.cursor_row, nav_state.cursor_col);
    };

    if (params.requested_selected_row >= 0 && params.requested_selected_col >= 0) {
        const int row = std::clamp(params.requested_selected_row, 0, std::max(0, grid_rows - 1));
        const int col = std::clamp(params.requested_selected_col, 0, std::max(0, grid_cols - 1));
        if (nearest_selectable_cell(row, col).first >= 0) {
            nav_state.cursor_row = row;
            nav_state.cursor_col = col;
            nav_state.fallback_prefer_col_dir = 0;
            ResetImeEdgeWrapNav(nav_state.edge_wrap_nav);
        }
    }

    if (nav_state.cursor_row < 0 || nav_state.cursor_row >= grid_rows || nav_state.cursor_col < 0 ||
        nav_state.cursor_col >= grid_cols) {
        const int anchor_row = std::clamp(nav_state.cursor_row, 0, std::max(0, grid_rows - 1));
        const int anchor_col = std::clamp(nav_state.cursor_col, 0, std::max(0, grid_cols - 1));
        auto [row, col] = nearest_selectable_cell(anchor_row, anchor_col);
        if (row < 0 || col < 0) {
            const auto first = first_selectable_cell();
            row = first.first;
            col = first.second;
        }
        nav_state.cursor_row = row;
        nav_state.cursor_col = col;
        nav_state.fallback_prefer_col_dir = 0;
        ResetImeEdgeWrapNav(nav_state.edge_wrap_nav);
    }

    constexpr double kEdgeWrapHoldDelaySec = 0.5;
    constexpr double kRepeatIntentWindowSec = 0.45;

    const auto move_cursor = [&](int step_row, int step_col, bool repeat_hint) {
        if (step_row == 0 && step_col == 0) {
            return;
        }

        int row = nav_state.cursor_row;
        int col = nav_state.cursor_col;
        const double now = ImGui::GetTime();
        if (row < 0 || col < 0) {
            const auto [first_row, first_col] = first_selectable_cell();
            nav_state.cursor_row = first_row;
            nav_state.cursor_col = first_col;
            nav_state.fallback_prefer_col_dir = 0;
            ResetImeEdgeWrapNav(nav_state.edge_wrap_nav);
            return;
        }

        const ImeKbKeySpec* origin_key =
            occupied[static_cast<std::size_t>(idx(nav_state.cursor_row, nav_state.cursor_col))];
        if (origin_key && origin_key->action == ImeKbKeyAction::None) {
            origin_key = nullptr;
        }
        const int max_steps = std::max(1, grid_rows * grid_cols);
        bool crossed_wrap = false;
        for (int i = 0; i < max_steps; ++i) {
            crossed_wrap = crossed_wrap || DoesImeKeyboardStepCrossGridEdge(
                                               row, col, step_row, step_col, grid_rows, grid_cols);
            const int next_row = row + step_row;
            const int next_col = col + step_col;
            row = (next_row + grid_rows) % grid_rows;
            col = (next_col + grid_cols) % grid_cols;

            const ImeKbKeySpec* candidate_key = occupied[static_cast<std::size_t>(idx(row, col))];
            // Treat each spanned key as one navigation node in every direction.
            if (origin_key && candidate_key == origin_key) {
                continue;
            }

            if (ShouldDelayImeEdgeWrap(nav_state.edge_wrap_nav, step_row, step_col, repeat_hint,
                                       crossed_wrap, now, kEdgeWrapHoldDelaySec,
                                       kRepeatIntentWindowSec)) {
                return;
            }

            nav_state.cursor_row = row;
            nav_state.cursor_col = col;
            nav_state.fallback_prefer_col_dir = step_col;
            CommitImeEdgeWrapStep(nav_state.edge_wrap_nav, step_row, step_col, now);
            return;
        }
    };

    if (params.allow_nav_input) {
        const bool imgui_move_left_once = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false) ||
                                          ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, false);
        const bool imgui_move_right_once = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, false) ||
                                           ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, false);
        const bool imgui_move_up_once = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, false) ||
                                        ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, false);
        const bool imgui_move_down_once = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, false) ||
                                          ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, false);
        const bool imgui_move_left_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) ||
            ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, true);
        const bool imgui_move_right_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) ||
            ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, true);
        const bool imgui_move_up_with_repeat = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) ||
                                               ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, true);
        const bool imgui_move_down_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) ||
            ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, true);
        const bool imgui_move_left_repeat = imgui_move_left_with_repeat && !imgui_move_left_once;
        const bool imgui_move_right_repeat = imgui_move_right_with_repeat && !imgui_move_right_once;
        const bool imgui_move_up_repeat = imgui_move_up_with_repeat && !imgui_move_up_once;
        const bool imgui_move_down_repeat = imgui_move_down_with_repeat && !imgui_move_down_once;
        const bool move_left =
            imgui_move_left_once || imgui_move_left_repeat || params.external_nav_left;
        const bool move_right =
            imgui_move_right_once || imgui_move_right_repeat || params.external_nav_right;
        const bool move_up = imgui_move_up_once || imgui_move_up_repeat || params.external_nav_up;
        const bool move_down =
            imgui_move_down_once || imgui_move_down_repeat || params.external_nav_down;
        const bool move_left_repeat =
            imgui_move_left_repeat || (params.external_nav_left && params.external_nav_left_repeat);
        const bool move_right_repeat =
            imgui_move_right_repeat ||
            (params.external_nav_right && params.external_nav_right_repeat);
        const bool move_up_repeat =
            imgui_move_up_repeat || (params.external_nav_up && params.external_nav_up_repeat);
        const bool move_down_repeat =
            imgui_move_down_repeat || (params.external_nav_down && params.external_nav_down_repeat);

        if (move_left) {
            move_cursor(0, -1, move_left_repeat);
        } else if (move_right) {
            move_cursor(0, 1, move_right_repeat);
        } else if (move_up) {
            move_cursor(-1, 0, move_up_repeat);
        } else if (move_down) {
            move_cursor(1, 0, move_down_repeat);
        }
    }

    const bool imgui_activate_selected_once = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false);
    const bool imgui_activate_selected_repeat = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, true);

    const auto activate_key = [&](const RenderedKey& render_key) {
        if (!render_key.key || !render_key.selectable) {
            return;
        }
        state.pressed_action = render_key.key->action;
        state.pressed_label = render_key.label;
        state.pressed_keycode = ResolveImeKeycode(*render_key.key, render_key.label);
        state.pressed_character = ResolveImeCharacter(*render_key.key, render_key.label);
        if (render_key.is_done) {
            state.done_pressed = true;
        }
    };

    int selected_render_index = -1;
    const auto [visible_row, visible_col] = visible_cell_for_cursor();
    if (is_selectable_cell(visible_row, visible_col)) {
        const ImeKbKeySpec* selected_spec =
            occupied[static_cast<std::size_t>(idx(visible_row, visible_col))];
        for (int i = 0; i < static_cast<int>(rendered_keys.size()); ++i) {
            const auto& key = rendered_keys[static_cast<std::size_t>(i)];
            if (key.key == selected_spec && key.selectable) {
                selected_render_index = i;
                break;
            }
        }
    }
    state.selected_row = nav_state.cursor_row;
    state.selected_col = nav_state.cursor_col;
    if (selected_render_index >= 0) {
        const auto& selected_key = rendered_keys[static_cast<std::size_t>(selected_render_index)];
        state.selected_center = selected_key.center;
    }

    bool activate_selected = false;
    if (params.allow_activate_input) {
        bool imgui_activate = imgui_activate_selected_once;
        if (selected_render_index >= 0) {
            const auto& selected_key =
                rendered_keys[static_cast<std::size_t>(selected_render_index)];
            if (selected_key.key && selected_key.key->action == ImeKbKeyAction::Backspace) {
                // Mirror PS4 OSK behavior: holding Cross on selected Backspace should auto-repeat.
                imgui_activate = imgui_activate_selected_repeat;
            }
        }
        activate_selected = imgui_activate || (!imgui_activate && params.external_activate_pressed);
    }

    const auto draw_key_glyph = [&](ImVec2 pos, ImVec2 size, ImeKbKeyGlyph glyph) {
        if (glyph == ImeKbKeyGlyph::None) {
            return;
        }

        const float thickness = std::max(1.2f, size.y * 0.05f);
        const ImU32 color = params.key_text;
        const float cx = pos.x + size.x * 0.5f;
        const float cy = pos.y + size.y * 0.5f;

        switch (glyph) {
        case ImeKbKeyGlyph::Backspace: {
            const float left = pos.x + size.x * 0.32f;
            const float right = pos.x + size.x * 0.78f;
            const float top = pos.y + size.y * 0.30f;
            const float bottom = pos.y + size.y * 0.70f;
            const ImVec2 tip{pos.x + size.x * 0.16f, cy};
            const std::array<ImVec2, 5> frame{
                ImVec2{left, top},
                ImVec2{right, top},
                ImVec2{right, bottom},
                ImVec2{left, bottom},
                tip,
            };
            draw->AddPolyline(frame.data(), static_cast<int>(frame.size()), color, true, thickness);

            const float cross_half_x = (right - left) * 0.18f;
            const float cross_half_y = (bottom - top) * 0.22f;
            const float cross_cx = left + (right - left) * 0.55f;
            draw->AddLine({cross_cx - cross_half_x, cy - cross_half_y},
                          {cross_cx + cross_half_x, cy + cross_half_y}, color, thickness);
            draw->AddLine({cross_cx + cross_half_x, cy - cross_half_y},
                          {cross_cx - cross_half_x, cy + cross_half_y}, color, thickness);
            break;
        }
        case ImeKbKeyGlyph::ArrowLeft: {
            const float tri_half_h = size.y * 0.18f;
            const float tri_w = size.x * 0.28f;
            const ImVec2 tip{cx - tri_w * 0.5f, cy};
            const ImVec2 top{cx + tri_w * 0.5f, cy - tri_half_h};
            const ImVec2 bottom{cx + tri_w * 0.5f, cy + tri_half_h};
            draw->AddTriangleFilled(tip, top, bottom, color);
            break;
        }
        case ImeKbKeyGlyph::ArrowRight: {
            const float tri_half_h = size.y * 0.18f;
            const float tri_w = size.x * 0.28f;
            const ImVec2 tip{cx + tri_w * 0.5f, cy};
            const ImVec2 top{cx - tri_w * 0.5f, cy - tri_half_h};
            const ImVec2 bottom{cx - tri_w * 0.5f, cy + tri_half_h};
            draw->AddTriangleFilled(tip, top, bottom, color);
            break;
        }
        case ImeKbKeyGlyph::ArrowUp: {
            const float tri_half_w = size.x * 0.18f;
            const float tri_h = size.y * 0.28f;
            const ImVec2 tip{cx, cy - tri_h * 0.5f};
            const ImVec2 left{cx - tri_half_w, cy + tri_h * 0.5f};
            const ImVec2 right{cx + tri_half_w, cy + tri_h * 0.5f};
            draw->AddTriangleFilled(tip, left, right, color);
            break;
        }
        case ImeKbKeyGlyph::ArrowDown: {
            const float tri_half_w = size.x * 0.18f;
            const float tri_h = size.y * 0.28f;
            const ImVec2 tip{cx, cy + tri_h * 0.5f};
            const ImVec2 left{cx - tri_half_w, cy - tri_h * 0.5f};
            const ImVec2 right{cx + tri_half_w, cy - tri_h * 0.5f};
            draw->AddTriangleFilled(tip, left, right, color);
            break;
        }
        case ImeKbKeyGlyph::None:
        default:
            break;
        }
    };

    const auto draw_key = [&](ImVec2 pos, ImVec2 size, ImU32 bg, const char* label,
                              const char* hotkey_label, ImeKbKeyGlyph glyph, float selected_alpha,
                              bool emphasize_main_label, bool underline_main_label,
                              bool disabled_visual) {
        if (selected_alpha > 0.0f) {
            ImVec4 selected_bg = ImGui::ColorConvertU32ToFloat4(bg);
            selected_bg.x = std::min(1.0f, selected_bg.x + 0.11f * selected_alpha);
            selected_bg.y = std::min(1.0f, selected_bg.y + 0.11f * selected_alpha);
            selected_bg.z = std::min(1.0f, selected_bg.z + 0.11f * selected_alpha);
            bg = ImGui::ColorConvertFloat4ToU32(selected_bg);
        }
        ImU32 key_text_color = params.key_text;
        ImU32 key_hotkey_color = params.key_hotkey_text;
        if (disabled_visual) {
            ImVec4 dim_bg = ImGui::ColorConvertU32ToFloat4(bg);
            dim_bg.x *= 0.72f;
            dim_bg.y *= 0.72f;
            dim_bg.z *= 0.72f;
            bg = ImGui::ColorConvertFloat4ToU32(dim_bg);
            key_text_color = IM_COL32(150, 150, 150, 255);
            key_hotkey_color = IM_COL32(122, 122, 122, 255);
        }

        draw->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, bg, layout.corner_radius);
        draw->AddRect(pos, {pos.x + size.x, pos.y + size.y}, params.key_border,
                      layout.corner_radius, 0, 1.0f);
        if (selected_alpha > 0.0f) {
            draw->AddRect(pos, {pos.x + size.x, pos.y + size.y},
                          ApplyImeAlpha(IM_COL32(248, 248, 248, 255), selected_alpha),
                          layout.corner_radius, 0, 2.0f);
        }
        ImFont* font = ImGui::GetFont();
        const float base_font_size = ImGui::GetFontSize();
        if (hotkey_label && hotkey_label[0] != '\0') {
            const float hotkey_padding = std::max(3.0f, size.y * 0.08f);
            const float hotkey_max_w = std::max(0.0f, size.x - hotkey_padding * 2.0f);
            float hotkey_font_size = std::max(6.0f, base_font_size * 0.58f);
            ImVec2 hotkey_size = font->CalcTextSizeA(
                hotkey_font_size, std::numeric_limits<float>::max(), -1.0f, hotkey_label);
            if (hotkey_max_w > 0.0f && hotkey_size.x > hotkey_max_w && hotkey_size.x > 0.0f) {
                hotkey_font_size *= hotkey_max_w / hotkey_size.x;
                hotkey_size = font->CalcTextSizeA(
                    hotkey_font_size, std::numeric_limits<float>::max(), -1.0f, hotkey_label);
            }
            ImVec2 hotkey_pos{pos.x + hotkey_padding, pos.y + hotkey_padding};
            draw->AddText(font, hotkey_font_size, hotkey_pos, key_hotkey_color, hotkey_label);
        }
        if ((!label || label[0] == '\0') && glyph != ImeKbKeyGlyph::None) {
            draw_key_glyph(pos, size, glyph);
        }
        if (label && label[0] != '\0') {
            float label_font_size =
                base_font_size * (emphasize_main_label ? kTypingKeyLabelScale : 1.0f);
            const float label_padding_x = std::max(4.0f, size.x * 0.08f);
            const float label_max_w = std::max(0.0f, size.x - label_padding_x * 2.0f);
            ImVec2 text_size = font->CalcTextSizeA(label_font_size,
                                                   std::numeric_limits<float>::max(), -1.0f, label);
            if (label_max_w > 0.0f && text_size.x > label_max_w && text_size.x > 0.0f) {
                label_font_size *= label_max_w / text_size.x;
                text_size = font->CalcTextSizeA(label_font_size, std::numeric_limits<float>::max(),
                                                -1.0f, label);
            }
            ImVec2 text_pos{pos.x + (size.x - text_size.x) * 0.5f,
                            pos.y + (size.y - text_size.y) * 0.5f};
            draw->AddText(font, label_font_size, text_pos, key_text_color, label);
            if (underline_main_label) {
                const float underline_pad = std::max(1.0f, label_font_size * 0.05f);
                const float underline_y =
                    std::min(pos.y + size.y - 3.0f, text_pos.y + text_size.y + underline_pad);
                const float underline_thickness = std::max(1.0f, label_font_size * 0.06f);
                draw->AddLine({text_pos.x, underline_y}, {text_pos.x + text_size.x, underline_y},
                              key_text_color, underline_thickness);
            }
        }
    };

    for (int i = 0; i < static_cast<int>(rendered_keys.size()); ++i) {
        const auto& key = rendered_keys[static_cast<std::size_t>(i)];
        const bool selected = params.show_selection_highlight && (i == selected_render_index);
        float selected_alpha = selected ? 1.0f : 0.0f;
        const bool has_fade =
            params.selection_fade_alpha && params.selection_fade_rows == layout.rows &&
            params.selection_fade_cols == layout.cols && key.row >= 0 && key.col >= 0 &&
            key.row < params.selection_fade_rows && key.col < params.selection_fade_cols;
        if (has_fade) {
            float& alpha =
                params.selection_fade_alpha[key.row * params.selection_fade_cols + key.col];
            selected_alpha = UpdateImeSelectorFadeAlpha(alpha, selected, params.delta_time);
        }
        const bool emphasize_main_label = key.key && key.key->action == ImeKbKeyAction::Character;
        const bool underline_main_label = key.underline_label;
        draw_key(key.pos, key.size, key.bg, key.label, key.hotkey_label, key.glyph, selected_alpha,
                 emphasize_main_label, underline_main_label, key.disabled_visual);

        if (!key.selectable) {
            continue;
        }

        ImGui::PushID(idx(key.row, key.col));
        ImGui::SetCursorScreenPos(key.pos);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        const bool key_activated = ImGui::InvisibleButton("##ImeGridKey", key.size);
        ImGui::PopItemFlag();
        if (ImGui::IsItemHovered()) {
            state.hovered = true;
        }
        if (key_activated) {
            state.clicked = true;
            nav_state.cursor_row = key.row;
            nav_state.cursor_col = key.col;
            nav_state.fallback_prefer_col_dir = 0;
            activate_key(key);
        }
        ImGui::PopID();
    }

    if (activate_selected && selected_render_index >= 0) {
        activate_key(rendered_keys[static_cast<std::size_t>(selected_render_index)]);
    }
}

} // namespace Libraries::Ime
