// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <system_error>

#include "core/file_sys/ifile.h"

namespace Core::FileSys {

bool IsZArchiveFile(const std::filesystem::path& path) {
    std::error_code ec;
    return path.extension() == ".zar" && std::filesystem::is_regular_file(path, ec) && !ec;
}

std::optional<std::vector<u8>> ReadGameFile(const std::filesystem::path& game_root,
                                            std::string_view rel_path) {
    std::error_code ec;
    if (!std::filesystem::is_directory(game_root, ec) || ec) {
        // Archive-backed roots need the real backend,not available in tests.
        return std::nullopt;
    }

    const std::filesystem::path full_path = game_root / rel_path;
    if (!std::filesystem::is_regular_file(full_path, ec) || ec) {
        return std::nullopt;
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    std::vector<u8> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!file.good() && !file.eof()) {
        return std::nullopt;
    }
    return data;
}

} // namespace Core::FileSys
