// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include "common/logging/log.h"
#include "emulator_settings.h"
#include "emulator_state.h"

#include <SDL3/SDL_messagebox.h>

using json = nlohmann::json;

// ── Singleton storage ─────────────────────────────────────────────────
std::shared_ptr<EmulatorSettingsImpl> EmulatorSettingsImpl::s_instance = nullptr;
std::mutex EmulatorSettingsImpl::s_mutex;

// ── nlohmann helpers for std::filesystem::path ───────────────────────
namespace nlohmann {
template <>
struct adl_serializer<std::filesystem::path> {
    static void to_json(json& j, const std::filesystem::path& p) {
        j = p.string();
    }
    static void from_json(const json& j, std::filesystem::path& p) {
        p = j.get<std::string>();
    }
};
} // namespace nlohmann

// ── Helpers ───────────────────────────────────────────────────────────

void EmulatorSettingsImpl::PrintChangedSummary(const std::vector<std::string>& changed) {
    if (changed.empty()) {
        LOG_DEBUG(EmuSettings, "No game-specific overrides applied");
        return;
    }
    LOG_DEBUG(EmuSettings, "Game-specific overrides applied:");
    for (const auto& k : changed)
        LOG_DEBUG(EmuSettings, "    * {}", k);
}

// ── Singleton ────────────────────────────────────────────────────────
EmulatorSettingsImpl::EmulatorSettingsImpl() = default;

EmulatorSettingsImpl::~EmulatorSettingsImpl() {
    Save();
}

std::shared_ptr<EmulatorSettingsImpl> EmulatorSettingsImpl::GetInstance() {
    std::lock_guard lock(s_mutex);
    if (!s_instance)
        s_instance = std::make_shared<EmulatorSettingsImpl>();
    return s_instance;
}

void EmulatorSettingsImpl::SetInstance(std::shared_ptr<EmulatorSettingsImpl> instance) {
    std::lock_guard lock(s_mutex);
    s_instance = std::move(instance);
}

// --------------------
// General helpers
// --------------------
bool EmulatorSettingsImpl::AddGameInstallDir(const std::filesystem::path& dir, bool enabled) {
    for (const auto& d : m_general.install_dirs.value)
        if (d.path == dir)
            return false;
    m_general.install_dirs.value.push_back({dir, enabled});
    return true;
}

std::vector<std::filesystem::path> EmulatorSettingsImpl::GetGameInstallDirs() const {
    std::vector<std::filesystem::path> out;
    for (const auto& d : m_general.install_dirs.value)
        if (d.enabled)
            out.push_back(d.path);
    return out;
}

const std::vector<GameInstallDir>& EmulatorSettingsImpl::GetAllGameInstallDirs() const {
    return m_general.install_dirs.value;
}

void EmulatorSettingsImpl::SetAllGameInstallDirs(const std::vector<GameInstallDir>& dirs) {
    m_general.install_dirs.value = dirs;
}

void EmulatorSettingsImpl::RemoveGameInstallDir(const std::filesystem::path& dir) {
    auto iterator =
        std::find_if(m_general.install_dirs.value.begin(), m_general.install_dirs.value.end(),
                     [&dir](const GameInstallDir& install_dir) { return install_dir.path == dir; });
    if (iterator != m_general.install_dirs.value.end()) {
        m_general.install_dirs.value.erase(iterator);
    }
}

void EmulatorSettingsImpl::SetGameInstallDirEnabled(const std::filesystem::path& dir,
                                                    bool enabled) {
    auto iterator =
        std::find_if(m_general.install_dirs.value.begin(), m_general.install_dirs.value.end(),
                     [&dir](const GameInstallDir& install_dir) { return install_dir.path == dir; });
    if (iterator != m_general.install_dirs.value.end()) {
        iterator->enabled = enabled;
    }
}

void EmulatorSettingsImpl::SetGameInstallDirs(
    const std::vector<std::filesystem::path>& dirs_config) {
    m_general.install_dirs.value.clear();
    for (const auto& dir : dirs_config) {
        m_general.install_dirs.value.push_back({dir, true});
    }
}

const std::vector<bool> EmulatorSettingsImpl::GetGameInstallDirsEnabled() {
    std::vector<bool> enabled_dirs;
    for (const auto& dir : m_general.install_dirs.value) {
        enabled_dirs.push_back(dir.enabled);
    }
    return enabled_dirs;
}

std::filesystem::path EmulatorSettingsImpl::GetHomeDir() {
    if (m_general.home_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::HomeDir);
    }
    return m_general.home_dir.value;
}

void EmulatorSettingsImpl::SetHomeDir(const std::filesystem::path& dir) {
    m_general.home_dir.value = dir;
}

std::filesystem::path EmulatorSettingsImpl::GetSysModulesDir() {
    if (m_general.sys_modules_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::SysModuleDir);
    }
    return m_general.sys_modules_dir.value;
}

void EmulatorSettingsImpl::SetSysModulesDir(const std::filesystem::path& dir) {
    m_general.sys_modules_dir.value = dir;
}

std::filesystem::path EmulatorSettingsImpl::GetFontsDir() {
    if (m_general.font_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::FontsDir);
    }
    return m_general.font_dir.value;
}

void EmulatorSettingsImpl::SetFontsDir(const std::filesystem::path& dir) {
    m_general.font_dir.value = dir;
}

// ── Game-specific override management ────────────────────────────────
void EmulatorSettingsImpl::ClearGameSpecificOverrides() {
    ClearGroupOverrides(m_general);
    ClearGroupOverrides(m_debug);
    ClearGroupOverrides(m_input);
    ClearGroupOverrides(m_audio);
    ClearGroupOverrides(m_gpu);
    ClearGroupOverrides(m_vulkan);
    LOG_DEBUG(EmuSettings, "All game-specific overrides cleared");
}

void EmulatorSettingsImpl::ResetGameSpecificValue(const std::string& key) {
    // Walk every overrideable group until we find the matching key.
    auto tryGroup = [&key](auto& group) {
        for (auto& item : group.GetOverrideableFields()) {
            if (key == item.key) {
                item.reset_game_specific(&group);
                return true;
            }
        }
        return false;
    };
    if (tryGroup(m_general))
        return;
    if (tryGroup(m_debug))
        return;
    if (tryGroup(m_input))
        return;
    if (tryGroup(m_audio))
        return;
    if (tryGroup(m_gpu))
        return;
    if (tryGroup(m_vulkan))
        return;
    LOG_WARNING(EmuSettings, "ResetGameSpecificValue: key '{}' not found", key);
}

bool EmulatorSettingsImpl::Save(const std::string& serial) {
    try {
        if (!serial.empty()) {
            const auto cfgDir = Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs);
            std::filesystem::create_directories(cfgDir);
            const auto path = cfgDir / (serial + ".json");

            json j = json::object();

            json generalObj = json::object();
            SaveGroupGameSpecific(m_general, generalObj);
            j["General"] = generalObj;

            json debugObj = json::object();
            SaveGroupGameSpecific(m_debug, debugObj);
            j["Debug"] = debugObj;

            json inputObj = json::object();
            SaveGroupGameSpecific(m_input, inputObj);
            j["Input"] = inputObj;

            json audioObj = json::object();
            SaveGroupGameSpecific(m_audio, audioObj);
            j["Audio"] = audioObj;

            json gpuObj = json::object();
            SaveGroupGameSpecific(m_gpu, gpuObj);
            j["GPU"] = gpuObj;

            json vulkanObj = json::object();
            SaveGroupGameSpecific(m_vulkan, vulkanObj);
            j["Vulkan"] = vulkanObj;

            std::ofstream out(path);
            if (!out) {
                LOG_ERROR(EmuSettings, "Failed to open game config for writing: {}", path.string());
                return false;
            }
            out << std::setw(4) << j;
            return !out.fail();

        } else {
            // ── Global config.json ─────────────────────────────────────
            const auto path =
                Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.json";

            SetConfigVersion(Common::g_scm_rev);
            json j;
            j["General"] = m_general;
            j["Debug"] = m_debug;
            j["Input"] = m_input;
            j["Audio"] = m_audio;
            j["GPU"] = m_gpu;
            j["Vulkan"] = m_vulkan;
            j["Users"] = m_userManager.GetUsers();
            std::ofstream out(path);
            if (!out) {
                LOG_ERROR(EmuSettings, "Failed to open config for writing: {}", path.string());
                return false;
            }
            out << std::setw(4) << j;
            return !out.fail();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(EmuSettings, "Error saving settings: {}", e.what());
        return false;
    }
}

// ── Load ──────────────────────────────────────────────────────────────

bool EmulatorSettingsImpl::Load(const std::string& serial) {
    try {
        if (serial.empty()) {
            // ── Global config ──────────────────────────────────────────
            const auto userDir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
            const auto configPath = userDir / "config.json";
            LOG_DEBUG(EmuSettings, "Loading global config from: {}", configPath.string());

            if (std::ifstream in{configPath}; in.good()) {
                json gj;
                in >> gj;

                auto mergeGroup = [&gj](auto& group, const char* section) {
                    if (!gj.contains(section))
                        return;
                    json current = group;
                    current.update(gj.at(section));
                    group = current.get<std::remove_reference_t<decltype(group)>>();
                };

                mergeGroup(m_general, "General");
                mergeGroup(m_debug, "Debug");
                mergeGroup(m_input, "Input");
                mergeGroup(m_audio, "Audio");
                mergeGroup(m_gpu, "GPU");
                mergeGroup(m_vulkan, "Vulkan");

                if (gj.contains("Users"))
                    m_userManager.GetUsers() = gj.at("Users").get<Users>();
                LOG_DEBUG(EmuSettings, "Global config loaded successfully");
            } else {
                if (std::filesystem::exists(Common::FS::GetUserPath(Common::FS::PathType::UserDir) /
                                            "config.toml")) {
                    SDL_MessageBoxButtonData btns[2]{
                        {0, 0, "No"},
                        {0, 1, "Yes"},
                    };
                    SDL_MessageBoxData msg_box{
                        0,
                        nullptr,
                        "Config Migration",
                        "The shadPS4 config backend has been updated, and you only have "
                        "the old version of the config. Do you wish to update it "
                        "automatically, or continue with the default config?",
                        2,
                        btns,
                        nullptr,
                    };
                    int result;
                    SDL_ShowMessageBox(&msg_box, &result);
                    if (result == 1) {
                        SDL_ShowSimpleMessageBox(
                            0, "Error", "sike this actually isn't implemented yet lol", nullptr);
                        std::quick_exit(1);
                    }
                }
                LOG_DEBUG(EmuSettings, "Global config not found - using defaults");
                SetDefaultValues();
                if (m_userManager.GetUsers().user.empty())
                    m_userManager.GetUsers().user = m_userManager.CreateDefaultUser();
                Save();
            }
            if (GetConfigVersion() != Common::g_scm_rev) {
                Save();
            }
            return true;
        } else {
            // ── Per-game override file ─────────────────────────────────
            // Never reloads global settings.  Only applies
            // game_specific_value overrides on top of the already-loaded
            // base configuration.
            const auto gamePath =
                Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (serial + ".json");
            LOG_DEBUG(EmuSettings, "Applying game config: {}", gamePath.string());

            if (!std::filesystem::exists(gamePath)) {
                LOG_DEBUG(EmuSettings, "No game-specific config found for {}", serial);
                return false;
            }

            std::ifstream in(gamePath);
            if (!in) {
                LOG_ERROR(EmuSettings, "Failed to open game config: {}", gamePath.string());
                return false;
            }

            json gj;
            in >> gj;

            std::vector<std::string> changed;

            // ApplyGroupOverrides now correctly stores values as
            // game_specific_value (see make_override in the header).
            // ConfigMode::Default will then resolve them at getter call
            // time without ever touching the base values.
            if (gj.contains("General"))
                ApplyGroupOverrides(m_general, gj.at("General"), changed);
            if (gj.contains("Debug"))
                ApplyGroupOverrides(m_debug, gj.at("Debug"), changed);
            if (gj.contains("Input"))
                ApplyGroupOverrides(m_input, gj.at("Input"), changed);
            if (gj.contains("Audio"))
                ApplyGroupOverrides(m_audio, gj.at("Audio"), changed);
            if (gj.contains("GPU"))
                ApplyGroupOverrides(m_gpu, gj.at("GPU"), changed);
            if (gj.contains("Vulkan"))
                ApplyGroupOverrides(m_vulkan, gj.at("Vulkan"), changed);

            PrintChangedSummary(changed);
            EmulatorState::GetInstance()->SetGameSpecifigConfigUsed(true);
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR(EmuSettings, "Error loading settings: {}", e.what());
        return false;
    }
}

void EmulatorSettingsImpl::SetDefaultValues() {
    m_general = GeneralSettings{};
    m_debug = DebugSettings{};
    m_input = InputSettings{};
    m_audio = AudioSettings{};
    m_gpu = GPUSettings{};
    m_vulkan = VulkanSettings{};
}

std::vector<std::string> EmulatorSettingsImpl::GetAllOverrideableKeys() const {
    std::vector<std::string> keys;
    auto addGroup = [&keys](const auto& fields) {
        for (const auto& item : fields)
            keys.push_back(item.key);
    };
    addGroup(m_general.GetOverrideableFields());
    addGroup(m_debug.GetOverrideableFields());
    addGroup(m_input.GetOverrideableFields());
    addGroup(m_audio.GetOverrideableFields());
    addGroup(m_gpu.GetOverrideableFields());
    addGroup(m_vulkan.GetOverrideableFields());
    return keys;
}