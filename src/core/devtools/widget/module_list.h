//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "common/path_util.h"

namespace Core::Devtools::Widget {

class ModuleList {
public:
    ModuleList() = default;
    ~ModuleList() = default;

    void Draw();
    bool open = false;

    bool IsSystemModule(const std::filesystem::path& path) {

        const auto sys_modules_path = Common::FS::GetUserPath(Common::FS::PathType::SysModuleDir);

        const auto canonical_path = std::filesystem::canonical(path);
        const auto canonical_sys_path = std::filesystem::canonical(sys_modules_path);

        const auto path_str = canonical_path.string();
        const auto sys_path_str = canonical_sys_path.string();

        return path_str.starts_with(sys_path_str);
    }

    static void AddModule(const std::string& name, std::filesystem::path path) {
        if (name == "eboot.bin") {
            return;
        }
        std::scoped_lock lock(s_modules_mutex);
        s_modules.push_back({name, path});
    }

private:
    struct ModuleInfo {
        std::string name;
        bool is_sys_module;
    };

    struct ModuleListEntry {
        std::string name;
        std::filesystem::path path;
    };

    static inline std::vector<ModuleListEntry> s_modules;
    static inline std::mutex s_modules_mutex;

    std::vector<ModuleInfo> modules;
};

} // namespace Core::Devtools::Widget