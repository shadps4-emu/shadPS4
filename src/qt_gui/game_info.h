// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

struct GameInfo {
    std::string path; // root path of game directory (normaly directory that contains eboot.bin)
    std::string icon_path; // path of icon0.png
    std::string pic_path;  // path of pic1.png

    // variables extracted from param.sfo
    std::string name = "Unknown";
    std::string serial = "Unknown";
    std::string app_ver = "Unknown";
    std::string version = "Unknown";
    std::string category = "Unknown";
    std::string fw = "Unknown";
};