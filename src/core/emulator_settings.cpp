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
            // ── Per-game config ─────────────────────────────────────
            const auto cfgDir = Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs);
            std::filesystem::create_directories(cfgDir);
            const auto path = cfgDir / (serial + ".json");

            // Read existing config to preserve unknown sections
            json existing_json;
            if (std::filesystem::exists(path)) {
                std::ifstream existing_in(path);
                if (existing_in.good()) {
                    existing_in >> existing_json;
                }
            }

            json j = existing_json.is_null() ? json::object() : existing_json;

            // Save game-specific overrides (only overrideable fields)
            json generalObj = json::object();
            SaveGroupGameSpecific(m_general, generalObj);

            // Merge with existing General section to preserve unknown fields within it
            if (j.contains("General") && j["General"].is_object()) {
                for (auto& [key, value] : j["General"].items()) {
                    if (!generalObj.contains(key)) {
                        generalObj[key] = value;
                    }
                }
            }
            j["General"] = generalObj;

            json debugObj = json::object();
            SaveGroupGameSpecific(m_debug, debugObj);
            if (j.contains("Debug") && j["Debug"].is_object()) {
                for (auto& [key, value] : j["Debug"].items()) {
                    if (!debugObj.contains(key)) {
                        debugObj[key] = value;
                    }
                }
            }
            j["Debug"] = debugObj;

            json inputObj = json::object();
            SaveGroupGameSpecific(m_input, inputObj);
            if (j.contains("Input") && j["Input"].is_object()) {
                for (auto& [key, value] : j["Input"].items()) {
                    if (!inputObj.contains(key)) {
                        inputObj[key] = value;
                    }
                }
            }
            j["Input"] = inputObj;

            json audioObj = json::object();
            SaveGroupGameSpecific(m_audio, audioObj);
            if (j.contains("Audio") && j["Audio"].is_object()) {
                for (auto& [key, value] : j["Audio"].items()) {
                    if (!audioObj.contains(key)) {
                        audioObj[key] = value;
                    }
                }
            }
            j["Audio"] = audioObj;

            json gpuObj = json::object();
            SaveGroupGameSpecific(m_gpu, gpuObj);
            if (j.contains("GPU") && j["GPU"].is_object()) {
                for (auto& [key, value] : j["GPU"].items()) {
                    if (!gpuObj.contains(key)) {
                        gpuObj[key] = value;
                    }
                }
            }
            j["GPU"] = gpuObj;

            json vulkanObj = json::object();
            SaveGroupGameSpecific(m_vulkan, vulkanObj);
            if (j.contains("Vulkan") && j["Vulkan"].is_object()) {
                for (auto& [key, value] : j["Vulkan"].items()) {
                    if (!vulkanObj.contains(key)) {
                        vulkanObj[key] = value;
                    }
                }
            }
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

            // Read existing config to preserve unknown sections
            json existing_json;
            if (std::filesystem::exists(path)) {
                std::ifstream existing_in(path);
                if (existing_in.good()) {
                    existing_in >> existing_json;
                }
            }

            // Start with unknown sections we've stored from previous loads
            json j = json::object();

            // Add all unknown sections first
            for (const auto& [section_name, section_data] : m_unknown_sections) {
                j[section_name] = section_data;
            }

            // Update schema version
            SetConfigVersion(Common::g_scm_rev);
            m_debug.config_schema_version.value = CURRENT_CONFIG_SCHEMA_VERSION;

            // Save known sections (this will overwrite any unknown sections with the same name)
            j["General"] = m_general;
            j["Debug"] = m_debug;
            j["Input"] = m_input;
            j["Audio"] = m_audio;
            j["GPU"] = m_gpu;
            j["Vulkan"] = m_vulkan;

            // Merge with existing JSON to preserve any sections that weren't loaded
            // (this is a safety net in case we missed some sections)
            if (!existing_json.is_null()) {
                for (auto& [key, value] : existing_json.items()) {
                    if (!j.contains(key)) {
                        j[key] = value;
                    }
                }
            }

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

            // Clear unknown sections from previous load
            m_unknown_sections.clear();

            if (std::ifstream in{configPath}; in.good()) {
                json j;
                in >> j;

                // Check schema version
                int file_schema_version = 1;
                if (j.contains("Debug") && j["Debug"].contains("config_schema_version")) {
                    file_schema_version = j["Debug"]["config_schema_version"].get<int>();
                }

                LOG_DEBUG(EmuSettings, "Config schema version: {} (current: {})",
                          file_schema_version, CURRENT_CONFIG_SCHEMA_VERSION);

                // Load known sections
                if (j.contains("General"))
                    j["General"].get_to(m_general);
                if (j.contains("Debug"))
                    j["Debug"].get_to(m_debug);
                if (j.contains("Input"))
                    j["Input"].get_to(m_input);
                if (j.contains("Audio"))
                    j["Audio"].get_to(m_audio);
                if (j.contains("GPU"))
                    j["GPU"].get_to(m_gpu);
                if (j.contains("Vulkan"))
                    j["Vulkan"].get_to(m_vulkan);

                // Store any unknown top-level sections
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (!IsKnownSection(it.key())) {
                        LOG_DEBUG(EmuSettings, "Preserving unknown section: {}", it.key());
                        m_unknown_sections[it.key()] = it.value();
                    }
                }

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
                        SDL_ShowSimpleMessageBox(0, "Error", "Migration not implemented yet",
                                                 nullptr);
                        std::quick_exit(1);
                    }
                }
                LOG_DEBUG(EmuSettings, "Global config not found - using defaults");
                SetDefaultValues();
                Save();
            }

            // Update schema version if needed
            if (m_debug.config_schema_version.value < CURRENT_CONFIG_SCHEMA_VERSION) {
                m_debug.config_schema_version.value = CURRENT_CONFIG_SCHEMA_VERSION;
                Save();
            }

            return true;

        } else {
            // ── Per-game override file ─────────────────────────────────
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

            // Apply overrides - these will set game_specific_value
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
    m_unknown_sections.clear();
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