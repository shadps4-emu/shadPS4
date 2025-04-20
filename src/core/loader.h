// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace Loader {

constexpr static u32 PkgMagic = 0x544e437f;

enum class FileTypes {
    Unknown,
    Pkg,
};

FileTypes DetectFileType(const std::filesystem::path& filepath);
} // namespace Loader
