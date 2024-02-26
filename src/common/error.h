// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace Common {

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in error.cpp.
[[nodiscard]] std::string GetLastErrorMsg();

// Like GetLastErrorMsg(), but passing an explicit error code.
// Defined in error.cpp.
[[nodiscard]] std::string NativeErrorToString(int e);

} // namespace Common
