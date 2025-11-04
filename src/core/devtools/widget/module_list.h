//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>
#include "common/config.h"
#include "common/elf_info.h"
#include "common/path_util.h"

namespace Core::Devtools::Widget {

class ModuleList {
public:
    ModuleList() = default;
    ~ModuleList() = default;

    void Draw();
    bool open = false;

    static bool IsSystemModule(const std::filesystem::path& path) {
        const auto sys_modules_path = Config::getSysModulesPath();

        const auto abs_path = std::filesystem::absolute(path).lexically_normal();
        const auto abs_sys_path = std::filesystem::absolute(sys_modules_path).lexically_normal();

        const auto path_str = abs_path.string();
        const auto sys_path_str = abs_sys_path.string();

        return path_str.starts_with(sys_path_str);
    }

    static bool IsSystemModule(const std::string& name) {
        const auto game_modules_path = Common::ElfInfo::Instance().GetGameFolder() / "sce_module";
        const auto prx_path = game_modules_path / name;

        if (!std::filesystem::exists(prx_path)) {
            return true;
        }
        return false;
    }

    static void AddModule(const std::string& name, std::filesystem::path path) {
        if (name == "eboot.bin") {
            return;
        }
        std::scoped_lock lock(modules_mutex);
        modules.push_back({name, IsSystemModule(path), true});
    }

    static void AddModule(std::string name) {
        name = name + ".prx";
        std::scoped_lock lock(modules_mutex);

        bool is_sys_module = IsSystemModule(name);
        bool is_lle = false;
        auto it = std::find_if(modules.begin(), modules.end(),
                               [&name, is_sys_module, is_lle](const ModuleInfo& entry) {
                                   return entry.name == name && !entry.is_lle;
                               });

        if (it == modules.end()) {
            modules.push_back({name, is_sys_module, is_lle});
        }
    }

private:
    struct ModuleInfo {
        std::string name;
        bool is_sys_module;
        bool is_lle;
    };

    static inline std::mutex modules_mutex;

    static inline std::vector<ModuleInfo> modules;
};

} // namespace Core::Devtools::Widget