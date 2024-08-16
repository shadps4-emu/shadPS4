// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer.h"
#include "avplayer_common.h"

#include <algorithm>   // std::equal
#include <cctype>      // std::tolower
#include <string_view> // std::string_view

namespace Libraries::AvPlayer {

using namespace Kernel;

static bool ichar_equals(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

static bool iequals(std::string_view l, std::string_view r) {
    return std::ranges::equal(l, r, ichar_equals);
}

SceAvPlayerSourceType GetSourceType(std::string_view path) {
    if (path.empty()) {
        return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
    }

    std::string_view name = path;
    if (path.find("://") != std::string_view::npos) {
        // This path is a URI. Strip HTTP parameters from it.
        // schema://server.domain/path/file.ext/and/beyond?param=value#paragraph ->
        // -> schema://server.domain/path/to/file.ext/and/beyond
        name = path.substr(0, path.find_first_of("?#"));
        if (name.empty()) {
            return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
        }
    }

    // schema://server.domain/path/to/file.ext/and/beyond -> .ext/and/beyond
    auto ext = name.substr(name.rfind('.'));
    if (ext.empty()) {
        return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
    }

    // .ext/and/beyond -> .ext
    ext = ext.substr(0, ext.find('/'));

    if (iequals(ext, ".mp4") || iequals(ext, ".m4v") || iequals(ext, ".m3d") ||
        iequals(ext, ".m4a") || iequals(ext, ".mov")) {
        return SCE_AVPLAYER_SOURCE_TYPE_FILE_MP4;
    }

    if (iequals(ext, ".m3u8")) {
        return SCE_AVPLAYER_SOURCE_TYPE_HLS;
    }

    return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
}

} // namespace Libraries::AvPlayer
