// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm> // std::equal
#include <cctype>    // std::tolower

#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/avplayer/avplayer_common.h"

namespace Libraries::AvPlayer {

static bool iequals(std::string_view l, std::string_view r) {
    return std::ranges::equal(l, r, [](u8 a, u8 b) { return std::tolower(a) == std::tolower(b); });
}

AvPlayerSourceType GetSourceType(std::string_view path) {
    if (path.empty()) {
        return AvPlayerSourceType::Unknown;
    }

    std::string_view name = path;
    if (path.find("://") != std::string_view::npos) {
        // This path is a URI. Strip HTTP parameters from it.
        // schema://server.domain/path/file.ext/and/beyond?param=value#paragraph ->
        // -> schema://server.domain/path/to/file.ext/and/beyond
        name = path.substr(0, path.find_first_of("?#"));
        if (name.empty()) {
            return AvPlayerSourceType::Unknown;
        }
    }

    // schema://server.domain/path/to/file.ext/and/beyond -> .ext/and/beyond
    auto ext = name.substr(name.rfind('.'));
    if (ext.empty()) {
        return AvPlayerSourceType::Unknown;
    }

    // .ext/and/beyond -> .ext
    ext = ext.substr(0, ext.find('/'));

    if (iequals(ext, ".mp4") || iequals(ext, ".m4v") || iequals(ext, ".m3d") ||
        iequals(ext, ".m4a") || iequals(ext, ".mov")) {
        return AvPlayerSourceType::FileMp4;
    }

    if (iequals(ext, ".m3u8")) {
        return AvPlayerSourceType::Hls;
    }

    return AvPlayerSourceType::Unknown;
}

} // namespace Libraries::AvPlayer
