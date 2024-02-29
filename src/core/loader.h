// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

enum FileTypes { FILETYPE_UNKNOWN, FILETYPE_PKG };
FileTypes detectFileType(const std::string& filepath);
