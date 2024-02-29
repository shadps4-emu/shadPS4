// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

enum class FileTypes {
     Unknown,
     Pkg,
};

FileTypes DetectFileType(const std::string& filepath);
