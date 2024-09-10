// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

namespace Common {

/// Make a string lowercase
[[nodiscard]] std::string ToLower(std::string_view str);

void ToLowerInPlace(std::string& str);

std::vector<std::string> SplitString(const std::string& str, char delimiter);

#ifdef _WIN32
[[nodiscard]] std::string UTF16ToUTF8(std::wstring_view input);
[[nodiscard]] std::wstring UTF8ToUTF16W(std::string_view str);
#endif

} // namespace Common
