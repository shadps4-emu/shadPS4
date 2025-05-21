//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <mutex>
#include <algorithm>

namespace Core::Devtools::Widget {

class ModuleList {
public:
    ModuleList() = default;
    ~ModuleList() = default;

    void Draw();
    bool open = false;
    char search_box[256] = "";

    static void AddModule(const std::string& name) {
        if (name == "eboot.bin") {
            return;
        }
        std::scoped_lock lock(s_modules_mutex);
        s_modules.push_back({name});
    }

private:

    struct ModuleInfo {
        std::string name;
    };

    struct ModuleListEntry {
        std::string name; 
    };

    static inline std::vector<ModuleListEntry> s_modules;
    static inline std::mutex s_modules_mutex;

    std::vector<ModuleInfo> modules;
};

} // namespace Core::Devtools::Widget