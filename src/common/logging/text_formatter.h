// SPDX-FileCopyrightText: Copyright 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace Common::Log {

struct Entry;

/// Formats a log entry into the provided text buffer.
std::string FormatLogMessage(const Entry& entry);

/// Formats and prints a log entry to stderr.
void PrintMessage(const Entry& entry);

/// Prints the same message as `PrintMessage`, but colored according to the severity level.
void PrintColoredMessage(const Entry& entry);

} // namespace Common::Log
