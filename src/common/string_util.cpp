// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <sstream>
#include <string>
#include "common/string_util.h"

namespace Common {

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::istringstream iss(str);
    std::vector<std::string> output(1);

    while (std::getline(iss, *output.rbegin(), delimiter)) {
        output.emplace_back();
    }

    output.pop_back();
    return output;
}

} // namespace Common
