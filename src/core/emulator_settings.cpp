// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <iomanip>
#include <iostream>
#include <common/path_util.h>
#include "emulator_settings.h"

using json = nlohmann::json;

std::shared_ptr<EmulatorSettings> EmulatorSettings::s_instance = nullptr;
std::mutex EmulatorSettings::s_mutex;

namespace nlohmann {
template <>
struct adl_serializer<std::filesystem::path> {
    static void to_json(json& j, const std::filesystem::path& p) {
        j = p.u8string();
    }
    static void from_json(const json& j, std::filesystem::path& p) {
        p = j.get<std::string>();
    }
};
} // namespace nlohmann

// --------------------
// Print summary
// --------------------
void EmulatorSettings::PrintChangedSummary(const std::vector<std::string>& changed) {
#ifndef NDEBUG
    if (changed.empty()) {
        std::cout << "[Settings] No game-specific overrides applied\n";
        return;
    }
    std::cout << "[Settings] Game-specific overrides applied:\n";
    for (const auto& k : changed)
        std::cout << "    * " << k << "\n";
#else
    (void)changed;
#endif
}

// --------------------
// ctor/dtor + singleton
// --------------------
EmulatorSettings::EmulatorSettings() {
    Load();
}
EmulatorSettings::~EmulatorSettings() {
    Save();
}

std::shared_ptr<EmulatorSettings> EmulatorSettings::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance)
        s_instance = std::make_shared<EmulatorSettings>();
    return s_instance;
}

void EmulatorSettings::SetInstance(std::shared_ptr<EmulatorSettings> instance) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = instance;
}

// --------------------
// General helpers
// --------------------
bool EmulatorSettings::AddGameInstallDir(const std::filesystem::path& dir, bool enabled) {
    for (const auto& d : m_general.install_dirs.value)
        if (d.path == dir)
            return false;
    m_general.install_dirs.value.push_back({dir, enabled});
    return true;
}

std::vector<std::filesystem::path> EmulatorSettings::GetGameInstallDirs() const {
    std::vector<std::filesystem::path> out;
    for (const auto& d : m_general.install_dirs.value)
        if (d.enabled)
            out.push_back(d.path);
    return out;
}

void EmulatorSettings::SetAllGameInstallDirs(const std::vector<GameInstallDir>& dirs) {
    m_general.install_dirs.value = dirs;
}

std::filesystem::path EmulatorSettings::GetHomeDir() {
    if (m_general.home_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::HomeDir);
    }
    return m_general.home_dir.value;
}

void EmulatorSettings::SetHomeDir(const std::filesystem::path& dir) {
    m_general.home_dir.value = dir;
}

std::filesystem::path EmulatorSettings::GetSysModulesDir() {
    if (m_general.sys_modules_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::SysModuleDir);
    }
    return m_general.sys_modules_dir.value;
}

void EmulatorSettings::SetSysModulesDir(const std::filesystem::path& dir) {
    m_general.sys_modules_dir.value = dir;
}

// --------------------
// Save
// --------------------
bool EmulatorSettings::Save(const std::string& serial) const {
    try {
        if (!serial.empty()) {
            const std::filesystem::path cfgDir =
                Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs);
            std::filesystem::create_directories(cfgDir);
            const std::filesystem::path path = cfgDir / (serial + ".json");

            json j = json::object();

            // Only write overrideable fields for each group
            json generalObj = json::object();
            for (auto& item : m_general.GetOverrideableFields()) {
                // To write the field we need to access it; use pointer-to-member via helper:
                // reuse the apply function with a special json that we construct from the
                // Setting<T>. To do that, we inspect the json by serializing the member through the
                // group object.
                json memberJson;
                // create a temporary group copy and call its overrideable getter to find the
                // member. A simpler approach: serialize entire group and pick field by key
                json whole = m_general;
                if (whole.contains(item.key))
                    generalObj[item.key] = whole[item.key];
            }
            j["General"] = generalObj;

            // Debug
            json debugObj = json::object();
            for (auto& item : m_debug.GetOverrideableFields()) {
                json whole = m_debug;
                if (whole.contains(item.key))
                    debugObj[item.key] = whole[item.key];
            }
            j["Debug"] = debugObj;

            std::ofstream out(path);
            if (!out.is_open()) {
                std::cerr << "Failed to open file for writing: " << path << std::endl;
                return false;
            }
            out << std::setw(4) << j;
            out.flush();
            if (out.fail()) {
                std::cerr << "Failed to write settings to: " << path << std::endl;
                return false;
            }
            return true;
        } else {
            const std::filesystem::path path =
                Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.json";
            json j;
            j["General"] = m_general;
            j["Debug"] = m_debug;
            j["Users"] = m_userManager.GetUsers();

            std::ofstream out(path);
            if (!out.is_open()) {
                std::cerr << "Failed to open file for writing: " << path << std::endl;
                return false;
            }
            out << std::setw(4) << j;
            out.flush();
            if (out.fail()) {
                std::cerr << "Failed to write settings to: " << path << std::endl;
                return false;
            }
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

// --------------------
// Load
// --------------------
bool EmulatorSettings::Load(const std::string& serial) {
    try {
        const std::filesystem::path userDir =
            Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        const std::filesystem::path configPath = userDir / "config.json";

        // Load global config if exists
        if (std::ifstream globalIn{configPath}; globalIn.good()) {
            json gj;
            globalIn >> gj;
            if (gj.contains("General")) {
                json current = m_general;         // JSON from existing struct with all defaults
                current.update(gj.at("General")); // merge only fields present in file
                m_general = current.get<GeneralSettings>(); // convert back
            }
            if (gj.contains("Debug")) {
                json current = m_debug;
                current.update(gj.at("Debug"));
                m_debug = current.get<DebugSettings>();
            }
            if (gj.contains("Users"))
                m_userManager.GetUsers() = gj.at("Users").get<Users>();
        } else {
            // ensure a default user exists
            if (m_userManager.GetUsers().user.empty())
                m_userManager.GetUsers().user = m_userManager.CreateDefaultUser();
        }

        // Load per-game overrides and apply
        if (!serial.empty()) {
            const std::filesystem::path gamePath =
                Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
                (serial + ".json");
            if (!std::filesystem::exists(gamePath))
                return false;

            std::ifstream in(gamePath);
            if (!in.is_open())
                return false;

            json gj;
            in >> gj;

            std::vector<std::string> changed;

            if (gj.contains("General")) {
                ApplyGroupOverrides<GeneralSettings>(m_general, gj.at("General"), changed);
            }
            if (gj.contains("Debug")) {
                ApplyGroupOverrides<DebugSettings>(m_debug, gj.at("Debug"), changed);
            }

            PrintChangedSummary(changed);
            return true;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading settings: " << e.what() << std::endl;
        return false;
    }
}
