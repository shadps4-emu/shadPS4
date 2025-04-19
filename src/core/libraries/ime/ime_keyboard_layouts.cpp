// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "ime_keyboard_layouts.h"

int c16rtomb(char* out, char16_t ch) {
    if (ch <= 0x7F) {
        out[0] = static_cast<char>(ch);
        out[1] = '\0';
        return 1;
    } else if (ch <= 0x7FF) {
        out[0] = 0xC0 | ((ch >> 6) & 0x1F);
        out[1] = 0x80 | (ch & 0x3F);
        out[2] = '\0';
        return 2;
    } else {
        out[0] = 0xE0 | ((ch >> 12) & 0x0F);
        out[1] = 0x80 | ((ch >> 6) & 0x3F);
        out[2] = 0x80 | (ch & 0x3F);
        out[3] = '\0';
        return 3;
    }
}

const std::vector<KeyEntry> kLayoutEnLettersUppercase = {
    // Row 0
    {0x31, u'1', KeyType::Character, 0, 0, 1, 1, "1", "", {None, None}},
    {0x32, u'2', KeyType::Character, 0, 1, 1, 1, "2", "", {None, None}},
    {0x33, u'3', KeyType::Character, 0, 2, 1, 1, "3", "", {None, None}},
    {0x34, u'4', KeyType::Character, 0, 3, 1, 1, "4", "", {None, None}},
    {0x35, u'5', KeyType::Character, 0, 4, 1, 1, "5", "", {None, None}},
    {0x36, u'6', KeyType::Character, 0, 5, 1, 1, "6", "", {None, None}},
    {0x37, u'7', KeyType::Character, 0, 6, 1, 1, "7", "", {None, None}},
    {0x38, u'8', KeyType::Character, 0, 7, 1, 1, "8", "", {None, None}},
    {0x39, u'9', KeyType::Character, 0, 8, 1, 1, "9", "", {None, None}},
    {0x30, u'0', KeyType::Character, 0, 9, 1, 1, "0", "", {None, None}},

    // Row 1
    {0x51, u'Q', KeyType::Character, 1, 0, 1, 1, "Q", "", {None, None}},
    {0x57, u'W', KeyType::Character, 1, 1, 1, 1, "W", "", {None, None}},
    {0x45, u'E', KeyType::Character, 1, 2, 1, 1, "E", "", {None, None}},
    {0x52, u'R', KeyType::Character, 1, 3, 1, 1, "R", "", {None, None}},
    {0x54, u'T', KeyType::Character, 1, 4, 1, 1, "T", "", {None, None}},
    {0x59, u'Y', KeyType::Character, 1, 5, 1, 1, "Y", "", {None, None}},
    {0x55, u'U', KeyType::Character, 1, 6, 1, 1, "U", "", {None, None}},
    {0x49, u'I', KeyType::Character, 1, 7, 1, 1, "I", "", {None, None}},
    {0x4F, u'O', KeyType::Character, 1, 8, 1, 1, "O", "", {None, None}},
    {0x50, u'P', KeyType::Character, 1, 9, 1, 1, "P", "", {None, None}},

    // Row 2
    {0x41, u'A', KeyType::Character, 2, 0, 1, 1, "A", "", {None, None}},
    {0x53, u'S', KeyType::Character, 2, 1, 1, 1, "S", "", {None, None}},
    {0x44, u'D', KeyType::Character, 2, 2, 1, 1, "D", "", {None, None}},
    {0x46, u'F', KeyType::Character, 2, 3, 1, 1, "F", "", {None, None}},
    {0x47, u'G', KeyType::Character, 2, 4, 1, 1, "G", "", {None, None}},
    {0x48, u'H', KeyType::Character, 2, 5, 1, 1, "H", "", {None, None}},
    {0x4A, u'J', KeyType::Character, 2, 6, 1, 1, "J", "", {None, None}},
    {0x4B, u'K', KeyType::Character, 2, 7, 1, 1, "K", "", {None, None}},
    {0x4C, u'L', KeyType::Character, 2, 8, 1, 1, "L", "", {None, None}},
    {0x22, u'"', KeyType::Character, 2, 9, 1, 1, "\"", "", {None, None}},

    // Row 3
    {0x5A, u'Z', KeyType::Character, 3, 0, 1, 1, "Z", "", {None, None}},
    {0x58, u'X', KeyType::Character, 3, 1, 1, 1, "X", "", {None, None}},
    {0x43, u'C', KeyType::Character, 3, 2, 1, 1, "C", "", {None, None}},
    {0x56, u'V', KeyType::Character, 3, 3, 1, 1, "V", "", {None, None}},
    {0x42, u'B', KeyType::Character, 3, 4, 1, 1, "B", "", {None, None}},
    {0x4E, u'N', KeyType::Character, 3, 5, 1, 1, "N", "", {None, None}},
    {0x4D, u'M', KeyType::Character, 3, 6, 1, 1, "M", "", {None, None}},
    {0x2D, u'-', KeyType::Character, 3, 7, 1, 1, "-", "", {None, None}},
    {0x5F, u'_', KeyType::Character, 3, 8, 1, 1, "_", "", {None, None}},
    {0x2F, u'/', KeyType::Character, 3, 9, 1, 1, "/", "", {None, None}},

    // Row 4
    {0x10, u'\0', KeyType::Function, 4, 0, 1, 1, "⬆", "L2", {L2, None}},
    {KC_SYM1, u'\0', KeyType::Function, 4, 1, 1, 1, "@#:", "△", {Triangle, None}}, // TODO:
    {KC_ACCENTS, u'\0', KeyType::Function, 4, 2, 1, 1, "à", "", {None, None}},     // TODO:
    {0x20, u' ', KeyType::Character, 4, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x08, u'\0', KeyType::Function, 4, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 5
    {0xF020, u'\0', KeyType::Function, 5, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 5, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 5, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 5, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 5, 4, 1, 1, "KB", "", {None, None}}, // TODO:
    {KC_OPT, u'\0', KeyType::Function, 5, 5, 1, 1, "...", "", {None, None}},
    {KC_GYRO, u'\0', KeyType::Function, 5, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO:
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x0D, u'\r', KeyType::Function, 5, 8, 2, 1, "Done", "R2", {R2, None}},

};

const std::vector<KeyEntry> kLayoutEnLettersLowercase = {
    // Row 0
    {0x31, u'1', KeyType::Character, 0, 0, 1, 1, "1", "", {None, None}},
    {0x32, u'2', KeyType::Character, 0, 1, 1, 1, "2", "", {None, None}},
    {0x33, u'3', KeyType::Character, 0, 2, 1, 1, "3", "", {None, None}},
    {0x34, u'4', KeyType::Character, 0, 3, 1, 1, "4", "", {None, None}},
    {0x35, u'5', KeyType::Character, 0, 4, 1, 1, "5", "", {None, None}},
    {0x36, u'6', KeyType::Character, 0, 5, 1, 1, "6", "", {None, None}},
    {0x37, u'7', KeyType::Character, 0, 6, 1, 1, "7", "", {None, None}},
    {0x38, u'8', KeyType::Character, 0, 7, 1, 1, "8", "", {None, None}},
    {0x39, u'9', KeyType::Character, 0, 8, 1, 1, "9", "", {None, None}},
    {0x30, u'0', KeyType::Character, 0, 9, 1, 1, "0", "", {None, None}},

    // Row 1
    {0x71, u'q', KeyType::Character, 1, 0, 1, 1, "q", "", {None, None}},
    {0x77, u'w', KeyType::Character, 1, 1, 1, 1, "w", "", {None, None}},
    {0x65, u'e', KeyType::Character, 1, 2, 1, 1, "e", "", {None, None}},
    {0x72, u'r', KeyType::Character, 1, 3, 1, 1, "r", "", {None, None}},
    {0x74, u't', KeyType::Character, 1, 4, 1, 1, "t", "", {None, None}},
    {0x79, u'y', KeyType::Character, 1, 5, 1, 1, "y", "", {None, None}},
    {0x75, u'u', KeyType::Character, 1, 6, 1, 1, "u", "", {None, None}},
    {0x69, u'i', KeyType::Character, 1, 7, 1, 1, "i", "", {None, None}},
    {0x6F, u'o', KeyType::Character, 1, 8, 1, 1, "o", "", {None, None}},
    {0x70, u'p', KeyType::Character, 1, 9, 1, 1, "p", "", {None, None}},

    // Row 2
    {0x61, u'a', KeyType::Character, 2, 0, 1, 1, "a", "", {None, None}},
    {0x73, u's', KeyType::Character, 2, 1, 1, 1, "s", "", {None, None}},
    {0x64, u'd', KeyType::Character, 2, 2, 1, 1, "d", "", {None, None}},
    {0x66, u'f', KeyType::Character, 2, 3, 1, 1, "f", "", {None, None}},
    {0x67, u'g', KeyType::Character, 2, 4, 1, 1, "g", "", {None, None}},
    {0x68, u'h', KeyType::Character, 2, 5, 1, 1, "h", "", {None, None}},
    {0x6A, u'j', KeyType::Character, 2, 6, 1, 1, "j", "", {None, None}},
    {0x6B, u'k', KeyType::Character, 2, 7, 1, 1, "k", "", {None, None}},
    {0x6C, u'l', KeyType::Character, 2, 8, 1, 1, "l", "", {None, None}},
    {0x22, u'"', KeyType::Character, 2, 9, 1, 1, "\"", "", {None, None}},

    // Row 3
    {0x7A, u'z', KeyType::Character, 3, 0, 1, 1, "z", "", {None, None}},
    {0x78, u'x', KeyType::Character, 3, 1, 1, 1, "x", "", {None, None}},
    {0x63, u'c', KeyType::Character, 3, 2, 1, 1, "c", "", {None, None}},
    {0x76, u'v', KeyType::Character, 3, 3, 1, 1, "v", "", {None, None}},
    {0x62, u'b', KeyType::Character, 3, 4, 1, 1, "b", "", {None, None}},
    {0x6E, u'n', KeyType::Character, 3, 5, 1, 1, "n", "", {None, None}},
    {0x6D, u'm', KeyType::Character, 3, 6, 1, 1, "m", "", {None, None}},
    {0x2D, u'-', KeyType::Character, 3, 7, 1, 1, "-", "", {None, None}},
    {0x5F, u'_', KeyType::Character, 3, 8, 1, 1, "_", "", {None, None}},
    {0x2F, u'/', KeyType::Character, 3, 9, 1, 1, "/", "", {None, None}},

    // Row 4
    {0x105, u'\0', KeyType::Function, 4, 0, 1, 1, "⬆", "L2", {L2, None}},
    {KC_SYM1, u'\0', KeyType::Function, 4, 1, 1, 1, "@#:", "△", {Triangle, None}}, // TODO
    {KC_ACCENTS, u'\0', KeyType::Function, 4, 2, 1, 1, "à", "", {None, None}},     // TODO
    {0x20, u' ', KeyType::Character, 4, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x00, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x08, u'\0', KeyType::Function, 4, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 5
    {0xF020, u'\0', KeyType::Function, 5, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 5, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 5, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 5, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 5, 4, 1, 1, "KB", "", {None, None}},    // TODO
    {KC_OPT, u'\0', KeyType::Function, 5, 5, 1, 1, "...", "", {None, None}},  // TODO
    {KC_GYRO, u'\0', KeyType::Function, 5, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x0D, u'\r', KeyType::Function, 5, 8, 2, 1, "Done", "R2", {R2, None}},

};

// From PS5
const std::vector<KeyEntry> kLayoutEnSymbols1 = {
    // Row 1
    {0x21, u'!', KeyType::Character, 0, 0, 1, 1, "!", "", {None, None}},
    {0x3F, u'?', KeyType::Character, 0, 1, 1, 1, "?", "", {None, None}},
    {0x22, u'"', KeyType::Character, 0, 2, 1, 1, "\"", "", {None, None}},
    {0x27, u'\'', KeyType::Character, 0, 3, 1, 1, "'", "", {None, None}},
    {0x23, u'#', KeyType::Character, 0, 4, 1, 1, "#", "", {None, None}},
    {0x25, u'%', KeyType::Character, 0, 5, 1, 1, "%", "", {None, None}},
    {0x28, u'(', KeyType::Character, 0, 6, 1, 1, "(", "", {None, None}},
    {0x29, u')', KeyType::Character, 0, 7, 1, 1, ")", "", {None, None}},
    {0xF001, u'\0', KeyType::Function, 0, 8, 1, 1, "()", "", {None, None}},
    {0x2F, u'/', KeyType::Character, 0, 9, 1, 1, "/", "", {None, None}},

    // Row 2
    {0x2D, u'-', KeyType::Character, 1, 0, 1, 1, "-", "", {None, None}},
    {0x5F, u'_', KeyType::Character, 1, 1, 1, 1, "_", "", {None, None}},
    {0x2C, u',', KeyType::Character, 1, 2, 1, 1, ",", "", {None, None}},
    {0x2E, u'.', KeyType::Character, 1, 3, 1, 1, ".", "", {None, None}},
    {0x3A, u':', KeyType::Character, 1, 4, 1, 1, ":", "", {None, None}},
    {0x3B, u';', KeyType::Character, 1, 5, 1, 1, ";", "", {None, None}},
    {0x2A, u'*', KeyType::Character, 1, 6, 1, 1, "*", "", {None, None}},
    {0x26, u'&', KeyType::Character, 1, 7, 1, 1, "&", "", {None, None}},
    {0x2B, u'+', KeyType::Character, 1, 8, 1, 1, "+", "", {None, None}},
    {0x3D, u'=', KeyType::Character, 1, 9, 1, 1, "=", "", {None, None}},

    // Row 3
    {0x3C, u'<', KeyType::Character, 2, 0, 1, 1, "<", "", {None, None}},
    {0x3E, u'>', KeyType::Character, 2, 1, 1, 1, ">", "", {None, None}},
    {0x40, u'@', KeyType::Character, 2, 2, 1, 1, "@", "", {None, None}},
    {0x5B, u'[', KeyType::Character, 2, 3, 1, 1, "[", "", {None, None}},
    {0x5D, u']', KeyType::Character, 2, 4, 1, 1, "]", "", {None, None}},
    {0xF002, u'\0', KeyType::Function, 2, 5, 1, 1, "[]", "", {None, None}},
    {0x7B, u'{', KeyType::Character, 2, 6, 1, 1, "{", "", {None, None}},
    {0x7D, u'}', KeyType::Character, 2, 7, 1, 1, "}", "", {None, None}},
    {0xF004, u'\0', KeyType::Function, 2, 8, 1, 1, "{}", "", {None, None}},
    {KC_SYM2, u'\0', KeyType::Function, 2, 9, 1, 2, "→", "", {None, None}},

    // Row 4
    {0x5C, u'\\', KeyType::Character, 3, 0, 1, 1, "\\", "", {None, None}},
    {0x7C, u'|', KeyType::Character, 3, 1, 1, 1, "|", "", {None, None}},
    {0x5E, u'^', KeyType::Character, 3, 2, 1, 1, "^", "", {None, None}},
    {0x60, u'`', KeyType::Character, 3, 3, 1, 1, "`", "", {None, None}},
    {0x24, u'$', KeyType::Character, 3, 4, 1, 1, "$", "", {None, None}},
    {0x20AC, u'\u20AC', KeyType::Character, 3, 5, 1, 1, "€", "", {None, None}},
    {0x00A3, u'\u00A3', KeyType::Character, 3, 6, 1, 1, "£", "", {None, None}},
    {0x00A5, u'\u00A5', KeyType::Character, 3, 7, 1, 1, "¥", "", {None, None}},
    {0x20A9, u'\u20A9', KeyType::Character, 3, 8, 1, 1, "₩", "", {None, None}},

    // Row 5
    {0x0000, u'\0', KeyType::Disabled, 4, 0, 1, 1, "", "", {None, None}},
    {KC_LETTERS, u'\0', KeyType::Function, 4, 1, 1, 1, "ABC", "L2+△", {L2, Triangle}}, // TODO:
    {0x0000, u'\0', KeyType::Disabled, 4, 2, 1, 1, "", "", {None, None}},
    {0x0020, u' ', KeyType::Character, 4, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x0008, u'\0', KeyType::Function, 4, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 6
    {0xF020, u'\0', KeyType::Function, 5, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 5, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 5, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 5, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 5, 4, 1, 1, "KB", "", {None, None}},    // TODO:
    {KC_OPT, u'\0', KeyType::Function, 5, 5, 1, 1, "...", "", {None, None}},  // TODO:
    {KC_GYRO, u'\0', KeyType::Function, 5, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO:
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x000D, u'\r', KeyType::Function, 5, 8, 2, 1, "Done", "R2", {R2, None}},

};

// From PS5
const std::vector<KeyEntry> kLayoutEnSymbols2 = {
    // Row 1
    {0x201C, u'“', KeyType::Character, 0, 0, 1, 1, "“", "", {None, None}},
    {0x201D, u'”', KeyType::Character, 0, 1, 1, 1, "”", "", {None, None}},
    {0x201E, u'„', KeyType::Character, 0, 2, 1, 1, "„", "", {None, None}},
    {0x00A1, u'¡', KeyType::Character, 0, 3, 1, 1, "¡", "", {None, None}},
    {0xF013, u'\0', KeyType::Function, 0, 4, 1, 1, "¡!", "", {None, None}},
    {0x00BF, u'¿', KeyType::Character, 0, 5, 1, 1, "¿", "", {None, None}},
    {0xF014, u'\0', KeyType::Function, 0, 6, 1, 1, "¿?", "", {None, None}},
    {0x007E, u'~', KeyType::Character, 0, 7, 1, 1, "~", "", {None, None}},
    {0x00B7, u'·', KeyType::Character, 0, 8, 1, 1, "·", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 0, 9, 1, 1, "", "", {None, None}},

    // Row 2
    {0x00D7, u'×', KeyType::Character, 1, 0, 1, 1, "×", "", {None, None}},
    {0x00F7, u'÷', KeyType::Character, 1, 1, 1, 1, "÷", "", {None, None}},
    {0x2039, u'‹', KeyType::Character, 1, 2, 1, 1, "‹", "", {None, None}},
    {0x203A, u'›', KeyType::Character, 1, 3, 1, 1, "›", "", {None, None}},
    {0x00AB, u'«', KeyType::Character, 1, 4, 1, 1, "«", "", {None, None}},
    {0x00BB, u'»', KeyType::Character, 1, 5, 1, 1, "»", "", {None, None}},
    {0x00BA, u'º', KeyType::Character, 1, 6, 1, 1, "º", "", {None, None}},
    {0x00AA, u'ª', KeyType::Character, 1, 7, 1, 1, "ª", "", {None, None}},
    {0x00B0, u'°', KeyType::Character, 1, 8, 1, 1, "°", "", {None, None}},
    {0x00A7, u'§', KeyType::Character, 1, 9, 1, 1, "§", "", {None, None}},

    // Row 3
    {KC_SYM1, u'\0', KeyType::Function, 2, 0, 1, 2, "←", "", {None, None}},
    {0x00A6, u'¦', KeyType::Character, 2, 1, 1, 1, "¦", "", {None, None}},
    {0x00B5, u'µ', KeyType::Character, 2, 2, 1, 1, "µ", "", {None, None}},
    {0x00AC, u'¬', KeyType::Character, 2, 3, 1, 1, "¬", "", {None, None}},
    {0x00B9, u'¹', KeyType::Character, 2, 4, 1, 1, "¹", "", {None, None}},
    {0x00B2, u'²', KeyType::Character, 2, 5, 1, 1, "²", "", {None, None}},
    {0x00B3, u'³', KeyType::Character, 2, 6, 1, 1, "³", "", {None, None}},
    {0x00BC, u'¼', KeyType::Character, 2, 7, 1, 1, "¼", "", {None, None}},
    {0x00BD, u'½', KeyType::Character, 2, 8, 1, 1, "½", "", {None, None}},
    {0x00BE, u'¾', KeyType::Character, 2, 9, 1, 1, "¾", "", {None, None}},

    // Row 4
    {0x00A2, u'¢', KeyType::Character, 3, 1, 1, 1, "¢", "", {None, None}},
    {0x00A4, u'¤', KeyType::Character, 3, 2, 1, 1, "¤", "", {None, None}},
    {0x2019, u'’', KeyType::Character, 3, 3, 1, 1, "’", "", {None, None}},
    {0x2018, u'‘', KeyType::Character, 3, 4, 1, 1, "‘", "", {None, None}},
    {0x201B, u'‛', KeyType::Character, 3, 5, 1, 1, "‛", "", {None, None}},
    {0x201A, u'‚', KeyType::Character, 3, 6, 1, 1, "‚", "", {None, None}},
    {0x2116, u'№', KeyType::Character, 3, 7, 1, 1, "№", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 3, 8, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 3, 9, 1, 1, "", "", {None, None}},

    // Row 5
    {0x0000, u'\0', KeyType::Disabled, 4, 0, 1, 1, "", "", {None, None}},
    {KC_LETTERS, u'\0', KeyType::Function, 4, 1, 1, 1, "ABC", "L2+△", {L2, Triangle}},
    {0x0000, u'\0', KeyType::Disabled, 4, 2, 1, 1, "", "", {None, None}},
    {0x20, u' ', KeyType::Character, 4, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x08, u'\0', KeyType::Function, 4, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 6
    {0xF020, u'\0', KeyType::Function, 5, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 5, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 5, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 5, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 5, 4, 1, 1, "KB", "", {None, None}},    // TODO
    {KC_OPT, u'\0', KeyType::Function, 5, 5, 1, 1, "...", "", {None, None}},  // TODO
    {KC_GYRO, u'\0', KeyType::Function, 5, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x0D, u'\r', KeyType::Function, 5, 8, 2, 1, "Done", "R2", {R2, None}},

};

const std::vector<KeyEntry> kLayoutEnAccentLettersUppercase = {
    // Row 0
    {0x00C0, u'À', KeyType::Character, 0, 0, 1, 1, "À", "", {None, None}},
    {0x00C1, u'Á', KeyType::Character, 0, 1, 1, 1, "Á", "", {None, None}},
    {0x00C2, u'Â', KeyType::Character, 0, 2, 1, 1, "Â", "", {None, None}},
    {0x00C3, u'Ã', KeyType::Character, 0, 3, 1, 1, "Ã", "", {None, None}},
    {0x00C4, u'Ä', KeyType::Character, 0, 4, 1, 1, "Ä", "", {None, None}},
    {0x00C5, u'Å', KeyType::Character, 0, 5, 1, 1, "Å", "", {None, None}},
    {0x0104, u'Ą', KeyType::Character, 0, 6, 1, 1, "Ą", "", {None, None}},
    {0x00C6, u'Æ', KeyType::Character, 0, 7, 1, 1, "Æ", "", {None, None}},
    {0x00C7, u'Ç', KeyType::Character, 0, 8, 1, 1, "Ç", "", {None, None}},
    {0x0106, u'Ć', KeyType::Character, 0, 9, 1, 1, "Ć", "", {None, None}},

    // Row 1
    {0x00C8, u'È', KeyType::Character, 1, 0, 1, 1, "È", "", {None, None}},
    {0x00C9, u'É', KeyType::Character, 1, 1, 1, 1, "É", "", {None, None}},
    {0x00CA, u'Ê', KeyType::Character, 1, 2, 1, 1, "Ê", "", {None, None}},
    {0x00CB, u'Ë', KeyType::Character, 1, 3, 1, 1, "Ë", "", {None, None}},
    {0x0118, u'Ę', KeyType::Character, 1, 4, 1, 1, "Ę", "", {None, None}},
    {0x011E, u'Ğ', KeyType::Character, 1, 5, 1, 1, "Ğ", "", {None, None}},
    {0x00CC, u'Ì', KeyType::Character, 1, 6, 1, 1, "Ì", "", {None, None}},
    {0x00CD, u'Í', KeyType::Character, 1, 7, 1, 1, "Í", "", {None, None}},
    {0x00CE, u'Î', KeyType::Character, 1, 8, 1, 1, "Î", "", {None, None}},
    {0x00CF, u'Ï', KeyType::Character, 1, 9, 1, 1, "Ï", "", {None, None}},

    // Row 2
    {0x0130, u'İ', KeyType::Character, 2, 0, 1, 1, "İ", "", {None, None}},
    {0x0141, u'Ł', KeyType::Character, 2, 1, 1, 1, "Ł", "", {None, None}},
    {0x00D1, u'Ñ', KeyType::Character, 2, 2, 1, 1, "Ñ", "", {None, None}},
    {0x0143, u'Ń', KeyType::Character, 2, 3, 1, 1, "Ń", "", {None, None}},
    {0x00D2, u'Ò', KeyType::Character, 2, 4, 1, 1, "Ò", "", {None, None}},
    {0x00D3, u'Ó', KeyType::Character, 2, 5, 1, 1, "Ó", "", {None, None}},
    {0x00D4, u'Ô', KeyType::Character, 2, 6, 1, 1, "Ô", "", {None, None}},
    {0x00D5, u'Õ', KeyType::Character, 2, 7, 1, 1, "Õ", "", {None, None}},
    {0x00D6, u'Ö', KeyType::Character, 2, 8, 1, 1, "Ö", "", {None, None}},
    {0x00D8, u'Ø', KeyType::Character, 2, 9, 1, 1, "Ø", "", {None, None}},

    // Row 3
    {0x0152, u'Œ', KeyType::Character, 3, 0, 1, 1, "Œ", "", {None, None}},
    {0x015A, u'Ś', KeyType::Character, 3, 1, 1, 1, "Ś", "", {None, None}},
    {0x015E, u'Ş', KeyType::Character, 3, 2, 1, 1, "Ş", "", {None, None}},
    {0x0160, u'Š', KeyType::Character, 3, 3, 1, 1, "Š", "", {None, None}},
    {0x00DF, u'ß', KeyType::Character, 3, 4, 1, 1, "ß", "", {None, None}},
    {0x00D9, u'Ù', KeyType::Character, 3, 5, 1, 1, "Ù", "", {None, None}},
    {0x00DA, u'Ú', KeyType::Character, 3, 6, 1, 1, "Ú", "", {None, None}},
    {0x00DB, u'Û', KeyType::Character, 3, 7, 1, 1, "Û", "", {None, None}},
    {0x00DC, u'Ü', KeyType::Character, 3, 8, 1, 1, "Ü", "", {None, None}},
    {0x00DD, u'Ý', KeyType::Character, 3, 9, 1, 1, "Ý", "", {None, None}},

    // Row 4
    {0x0178, u'Ÿ', KeyType::Character, 4, 0, 1, 1, "Ÿ", "", {None, None}},
    {0x0179, u'Ź', KeyType::Character, 4, 1, 1, 1, "Ź", "", {None, None}},
    {0x017B, u'Ż', KeyType::Character, 4, 2, 1, 1, "Ż", "", {None, None}},
    {0x017D, u'Ž', KeyType::Character, 4, 3, 1, 1, "Ž", "", {None, None}},
    {0x00D0, u'Ð', KeyType::Character, 4, 4, 1, 1, "Ð", "", {None, None}},
    {0x00DE, u'Þ', KeyType::Character, 4, 5, 1, 1, "Þ", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 6, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 8, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 9, 1, 1, "", "", {None, None}},

    // Row 5
    {0x0010, u'\0', KeyType::Function, 5, 0, 1, 1, "⬆", "L2", {L2, None}},
    {KC_LETTERS, u'\0', KeyType::Function, 5, 1, 1, 1, "ABC", "L2+△", {L3, Triangle}}, // TODO:
    {0x0000, u'\0', KeyType::Disabled, 5, 2, 1, 1, "", "", {None, None}},              // TODO:
    {0x0020, u' ', KeyType::Character, 5, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x0008, u'\0', KeyType::Function, 5, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 6
    {0xF020, u'\0', KeyType::Function, 6, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 6, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 6, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 6, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 6, 4, 1, 1, "KB", "", {None, None}},    // TODO
    {KC_OPT, u'\0', KeyType::Function, 6, 5, 1, 1, "...", "", {None, None}},  // TODO
    {KC_GYRO, u'\0', KeyType::Function, 6, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO
    {0x0000, u'\0', KeyType::Disabled, 6, 7, 1, 1, "", "", {None, None}},
    {0x000D, u'\r', KeyType::Function, 6, 8, 2, 1, "Done", "R2", {R2, None}},
};

const std::vector<KeyEntry> kLayoutEnAccentLettersLowercase = {
    // Row 0
    {0x00E0, u'à', KeyType::Character, 0, 0, 1, 1, "à", "", {None, None}},
    {0x00E1, u'á', KeyType::Character, 0, 1, 1, 1, "á", "", {None, None}},
    {0x00E2, u'â', KeyType::Character, 0, 2, 1, 1, "â", "", {None, None}},
    {0x00E3, u'ã', KeyType::Character, 0, 3, 1, 1, "ã", "", {None, None}},
    {0x00E4, u'ä', KeyType::Character, 0, 4, 1, 1, "ä", "", {None, None}},
    {0x00E5, u'å', KeyType::Character, 0, 5, 1, 1, "å", "", {None, None}},
    {0x0105, u'ą', KeyType::Character, 0, 6, 1, 1, "ą", "", {None, None}},
    {0x00E6, u'æ', KeyType::Character, 0, 7, 1, 1, "æ", "", {None, None}},
    {0x00E7, u'ç', KeyType::Character, 0, 8, 1, 1, "ç", "", {None, None}},
    {0x0107, u'ć', KeyType::Character, 0, 9, 1, 1, "ć", "", {None, None}},

    // Row 1
    {0x00E8, u'è', KeyType::Character, 1, 0, 1, 1, "è", "", {None, None}},
    {0x00E9, u'é', KeyType::Character, 1, 1, 1, 1, "é", "", {None, None}},
    {0x00EA, u'ê', KeyType::Character, 1, 2, 1, 1, "ê", "", {None, None}},
    {0x00EB, u'ë', KeyType::Character, 1, 3, 1, 1, "ë", "", {None, None}},
    {0x0119, u'ę', KeyType::Character, 1, 4, 1, 1, "ę", "", {None, None}},
    {0x011F, u'ğ', KeyType::Character, 1, 5, 1, 1, "ğ", "", {None, None}},
    {0x00EC, u'ì', KeyType::Character, 1, 6, 1, 1, "ì", "", {None, None}},
    {0x00ED, u'í', KeyType::Character, 1, 7, 1, 1, "í", "", {None, None}},
    {0x00EE, u'î', KeyType::Character, 1, 8, 1, 1, "î", "", {None, None}},
    {0x00EF, u'ï', KeyType::Character, 1, 9, 1, 1, "ï", "", {None, None}},

    // Row 2
    {0x0131, u'ı', KeyType::Character, 2, 0, 1, 1, "ı", "", {None, None}},
    {0x0142, u'ł', KeyType::Character, 2, 1, 1, 1, "ł", "", {None, None}},
    {0x00F1, u'ñ', KeyType::Character, 2, 2, 1, 1, "ñ", "", {None, None}},
    {0x0144, u'ń', KeyType::Character, 2, 3, 1, 1, "ń", "", {None, None}},
    {0x00F2, u'ò', KeyType::Character, 2, 4, 1, 1, "ò", "", {None, None}},
    {0x00F3, u'ó', KeyType::Character, 2, 5, 1, 1, "ó", "", {None, None}},
    {0x00F4, u'ô', KeyType::Character, 2, 6, 1, 1, "ô", "", {None, None}},
    {0x00F5, u'õ', KeyType::Character, 2, 7, 1, 1, "õ", "", {None, None}},
    {0x00F6, u'ö', KeyType::Character, 2, 8, 1, 1, "ö", "", {None, None}},
    {0x00F8, u'ø', KeyType::Character, 2, 9, 1, 1, "ø", "", {None, None}},

    // Row 3
    {0x0153, u'œ', KeyType::Character, 3, 0, 1, 1, "œ", "", {None, None}},
    {0x015B, u'ś', KeyType::Character, 3, 1, 1, 1, "ś", "", {None, None}},
    {0x015F, u'ş', KeyType::Character, 3, 2, 1, 1, "ş", "", {None, None}},
    {0x0161, u'š', KeyType::Character, 3, 3, 1, 1, "š", "", {None, None}},
    {0x00DF, u'ß', KeyType::Character, 3, 4, 1, 1, "ß", "", {None, None}},
    {0x00F9, u'ù', KeyType::Character, 3, 5, 1, 1, "ù", "", {None, None}},
    {0x00FA, u'ú', KeyType::Character, 3, 6, 1, 1, "ú", "", {None, None}},
    {0x00FB, u'û', KeyType::Character, 3, 7, 1, 1, "û", "", {None, None}},
    {0x00FC, u'ü', KeyType::Character, 3, 8, 1, 1, "ü", "", {None, None}},
    {0x00FD, u'ý', KeyType::Character, 3, 9, 1, 1, "ý", "", {None, None}},

    // Row 4
    {0x00FF, u'ÿ', KeyType::Character, 4, 0, 1, 1, "ÿ", "", {None, None}},
    {0x017A, u'ź', KeyType::Character, 4, 1, 1, 1, "ź", "", {None, None}},
    {0x017C, u'ż', KeyType::Character, 4, 2, 1, 1, "ż", "", {None, None}},
    {0x017E, u'ž', KeyType::Character, 4, 3, 1, 1, "ž", "", {None, None}},
    {0x00F0, u'ð', KeyType::Character, 4, 4, 1, 1, "ð", "", {None, None}},
    {0x00FE, u'þ', KeyType::Character, 4, 5, 1, 1, "þ", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 6, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 7, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 8, 1, 1, "", "", {None, None}},
    {0x0000, u'\0', KeyType::Disabled, 4, 9, 1, 1, "", "", {None, None}},

    // Row 5
    {0x0010, u'\0', KeyType::Function, 5, 0, 1, 1, "⬆", "L2", {L2, None}},
    {KC_LETTERS, u'\0', KeyType::Function, 5, 1, 1, 1, "ABC", "L2+△", {L3, Triangle}}, // TODO
    {0x0000, u'\0', KeyType::Disabled, 5, 2, 1, 1, "", "", {None, None}},              // TODO
    {0x0020, u' ', KeyType::Character, 5, 3, 4, 1, "Space", "△", {Triangle, None}},
    {0x0000, u'\0', KeyType::Disabled, 5, 7, 1, 1, "", "", {None, None}},
    {0x0008, u'\0', KeyType::Function, 5, 8, 2, 1, "⇦", "□", {Square, None}, true},

    // Row 6
    {0xF020, u'\0', KeyType::Function, 6, 0, 1, 1, "▲", "", {Up, None}},
    {0xF021, u'\0', KeyType::Function, 6, 1, 1, 1, "▼", "", {Down, None}},
    {0xF022, u'\0', KeyType::Function, 6, 2, 1, 1, "◀", "L1", {L1, None}},
    {0xF023, u'\0', KeyType::Function, 6, 3, 1, 1, "▶", "R1", {R1, None}},
    {KC_KB, u'\0', KeyType::Function, 6, 4, 1, 1, "KB", "", {None, None}},    // TODO
    {KC_OPT, u'\0', KeyType::Function, 6, 5, 1, 1, "...", "", {None, None}},  // TODO
    {KC_GYRO, u'\0', KeyType::Function, 6, 6, 1, 1, "+/⊗", "R3", {R3, None}}, // TODO
    {0x0000, u'\0', KeyType::Disabled, 6, 7, 1, 1, "", "", {None, None}},
    {0x000D, u'\r', KeyType::Function, 6, 8, 2, 1, "Done", "R2", {R2, None}},
};
