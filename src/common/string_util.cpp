// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <sstream>
#include <string>
#include "common/string_util.h"
#include "common/types.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Common {

std::string ToLower(std::string_view input) {
    std::string str;
    str.resize(input.size());
    std::ranges::transform(input, str.begin(), tolower);
    return str;
}

void ToLowerInPlace(std::string& str) {
    std::ranges::transform(str, str.begin(), tolower);
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::istringstream iss(str);
    std::vector<std::string> output(1);

    while (std::getline(iss, *output.rbegin(), delimiter)) {
        output.emplace_back();
    }

    output.pop_back();
    return output;
}

#ifdef _WIN32
static std::wstring CPToUTF16(u32 code_page, std::string_view input) {
    const auto size =
        MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);

    if (size == 0) {
        return {};
    }

    std::wstring output(size, L'\0');

    if (size != MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()),
                                    &output[0], static_cast<int>(output.size()))) {
        output.clear();
    }

    return output;
}

std::string UTF16ToUTF8(std::wstring_view input) {
    const auto size = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        return {};
    }

    std::string output(size, '\0');

    if (size != WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                                    &output[0], static_cast<int>(output.size()), nullptr,
                                    nullptr)) {
        output.clear();
    }

    return output;
}

std::wstring UTF8ToUTF16W(std::string_view input) {
    return CPToUTF16(CP_UTF8, input);
}
#endif

} // namespace Common
