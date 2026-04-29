// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include <toml.hpp>
#include "common/logging/formatter.h"
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
        const auto u8 = p.u8string();
        j = std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
    }
    static void from_json(const json& j, std::filesystem::path& p) {
        const std::string s = j.get<std::string>();
        p = std::filesystem::path(
            std::u8string_view(reinterpret_cast<const char8_t*>(s.data()), s.size()));
    }
};
} // namespace nlohmann

namespace toml {
// why is it so hard to avoid exceptions with this library
template <typename T>
std::optional<T> get_optional(const toml::value& v, const std::string& key) {
    if (!v.is_table())
        return std::nullopt;
    const auto& tbl = v.as_table();
    auto it = tbl.find(key);
    if (it == tbl.end())
        return std::nullopt;

    if constexpr (std::is_same_v<T, int>) {
        if (it->second.is_integer()) {
            return static_cast<int>(toml::get<int>(it->second));
        }
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        if (it->second.is_integer()) {
            return static_cast<u32>(toml::get<unsigned int>(it->second));
        }
    } else if constexpr (std::is_same_v<T, unsigned long long>) {
        if (it->second.is_integer()) {
            return static_cast<long long>(toml::get<unsigned long long>(it->second));
        }
    } else if constexpr (std::is_same_v<T, double>) {
        if (it->second.is_floating()) {
            return toml::get<double>(it->second);
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (it->second.is_string()) {
            return toml::get<std::string>(it->second);
        }
    } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
        if (it->second.is_string()) {
            return toml::get<std::string>(it->second);
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        if (it->second.is_boolean()) {
            return toml::get<bool>(it->second);
        }
    } else {
        static_assert([] { return false; }(), "Unsupported type in get_optional<T>");
    }

    return std::nullopt;
}

} // namespace toml

// ── Helpers ───────────────────────────────────────────────────────────

void EmulatorSettingsImpl::PrintChangedSummary(const std::vector<std::string>& changed) {
    if (changed.empty()) {
        LOG_DEBUG(Config, "No game-specific overrides applied");
        return;
    }
    LOG_DEBUG(Config, "Game-specific overrides applied:");
    for (const auto& k : changed)
        LOG_DEBUG(Config, "    * {}", k);
}

// ── Singleton ────────────────────────────────────────────────────────
EmulatorSettingsImpl::EmulatorSettingsImpl() = default;

EmulatorSettingsImpl::~EmulatorSettingsImpl() {
    if (m_loaded)
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

std::filesystem::path EmulatorSettingsImpl::GetAddonInstallDir() {
    if (m_general.addon_install_dir.value.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "addcont";
    }
    return m_general.addon_install_dir.value;
}

void EmulatorSettingsImpl::SetAddonInstallDir(const std::filesystem::path& dir) {
    m_general.addon_install_dir.value = dir;
}

// ── Game-specific override management ────────────────────────────────
void EmulatorSettingsImpl::ClearGameSpecificOverrides() {
    ClearGroupOverrides(m_general);
    ClearGroupOverrides(m_log);
    ClearGroupOverrides(m_debug);
    ClearGroupOverrides(m_input);
    ClearGroupOverrides(m_audio);
    ClearGroupOverrides(m_gpu);
    ClearGroupOverrides(m_vulkan);
    LOG_DEBUG(Config, "All game-specific overrides cleared");
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
    if (tryGroup(m_log))
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
    LOG_WARNING(Config, "ResetGameSpecificValue: key '{}' not found", key);
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

            json logObj = json::object();
            SaveGroupGameSpecific(m_log, logObj);
            j["Log"] = logObj;

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
                LOG_ERROR(Config, "Failed to open game config for writing: {}", path.string());
                return false;
            }
            out << std::setw(2) << j;
            return !out.fail();

        } else {
            // ── Global config.json ─────────────────────────────────────
            const auto path =
                Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.json";

            SetConfigVersion(Common::g_scm_rev);

            json j;
            j["General"] = m_general;
            j["Log"] = m_log;
            j["Debug"] = m_debug;
            j["Input"] = m_input;
            j["Audio"] = m_audio;
            j["GPU"] = m_gpu;
            j["Vulkan"] = m_vulkan;

            // Read the existing file so we can preserve keys unknown to this build
            json existing = json::object();
            if (std::ifstream existingIn{path}; existingIn.good()) {
                try {
                    existingIn >> existing;
                } catch (...) {
                    existing = json::object();
                }
            }

            // Merge: update each section's known keys, but leave unknown keys intact
            for (auto& [section, val] : j.items()) {
                if (existing.contains(section) && existing[section].is_object() && val.is_object())
                    existing[section].update(val); // overwrites known keys, keeps unknown ones
                else
                    existing[section] = val;
            }

            std::ofstream out(path);
            if (!out) {
                LOG_ERROR(Config, "Failed to open config for writing: {}", path.string());
                return false;
            }
            out << std::setw(2) << existing;
            return !out.fail();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(Config, "Error saving settings: {}", e.what());
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
            LOG_DEBUG(Config, "Loading global config from: {}", configPath.string());

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
                mergeGroup(m_log, "Log");
                mergeGroup(m_debug, "Debug");
                mergeGroup(m_input, "Input");
                mergeGroup(m_audio, "Audio");
                mergeGroup(m_gpu, "GPU");
                mergeGroup(m_vulkan, "Vulkan");

                LOG_DEBUG(Config, "Global config loaded successfully");
            } else {
                if (std::filesystem::exists(Common::FS::GetUserPath(Common::FS::PathType::UserDir) /
                                            "config.toml")) {
                    SDL_MessageBoxButtonData btns[2]{
                        {0, 0, "Defaults"},
                        {0, 1, "Update"},
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
                    int result = 1;
                    SDL_ShowMessageBox(&msg_box, &result);
                    if (result == 1) {
                        if (TransferSettings()) {
                            m_loaded = true;
                            Save();
                            return true;
                        } else {
                            SDL_ShowSimpleMessageBox(0, "Config Migration",
                                                     "Error transferring settings, exiting.",
                                                     nullptr);
                            std::quick_exit(1);
                        }
                    }
                }
                LOG_DEBUG(Config, "Global config not found - using defaults");
                SetDefaultValues();
                Save();
            }
            if (GetConfigVersion() != Common::g_scm_rev) {
                Save();
            }
            m_loaded = true;
            return true;
        } else {
            // ── Per-game override file ─────────────────────────────────
            // Never reloads global settings. Only applies
            // game_specific_value overrides on top of the already-loaded
            // base configuration.
            const auto gamePath =
                Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) / (serial + ".json");
            LOG_DEBUG(Config, "Applying game config: {}", gamePath.string());

            if (!std::filesystem::exists(gamePath)) {
                LOG_DEBUG(Config, "No game-specific config found for {}", serial);
                return false;
            }

            std::ifstream in(gamePath);
            if (!in) {
                LOG_ERROR(Config, "Failed to open game config: {}", gamePath.string());
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
            if (gj.contains("Log"))
                ApplyGroupOverrides(m_log, gj.at("Log"), changed);
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
        LOG_ERROR(Config, "Error loading settings: {}", e.what());
        return false;
    }
}

void EmulatorSettingsImpl::SetDefaultValues() {
    m_general = GeneralSettings{};
    m_log = LogSettings{};
    m_debug = DebugSettings{};
    m_input = InputSettings{};
    m_audio = AudioSettings{};
    m_gpu = GPUSettings{};
    m_vulkan = VulkanSettings{};
}

bool EmulatorSettingsImpl::TransferSettings() {
    toml::value og_data;
    json new_data = json::object();
    try {
        auto path = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml";
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifs.open(path, std::ios_base::binary);
        og_data = toml::parse(ifs, std::string{fmt::UTF(path.filename().u8string()).data});
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return false;
    }
    auto setFromToml = [&]<typename T>(Setting<T>& n, toml::value const& t, std::string k) {
        n = toml::get_optional<T>(t, k).value_or(n.default_value);
    };
    if (og_data.contains("General")) {
        const toml::value& general = og_data.at("General");
        auto& s = m_general;

        setFromToml(s.volume_slider, general, "volumeSlider");
        setFromToml(s.neo_mode, general, "isPS4Pro");
        setFromToml(s.dev_kit_mode, general, "isDevKit");
        setFromToml(s.trophy_popup_disabled, general, "isTrophyPopupDisabled");
        setFromToml(s.trophy_notification_duration, general, "trophyNotificationDuration");
        setFromToml(s.discord_rpc_enabled, general, "enableDiscordRPC");
        setFromToml(s.show_splash, general, "showSplash");
        setFromToml(s.trophy_notification_side, general, "sideTrophy");
        setFromToml(s.connected_to_network, general, "isConnectedToNetwork");
        setFromToml(s.sys_modules_dir, general, "sysModulesPath");
        setFromToml(s.font_dir, general, "fontsPath");
        // setFromToml(, general, "userName");
        // setFromToml(s.defaultControllerID, general, "defaultControllerID");
    }

    if (og_data.contains("Log")) {
        const toml::value& log = og_data.at("Log");
        auto& s = m_log;

        setFromToml(s.append, log, "append");
        setFromToml(s.enable, log, "enable");
        setFromToml(s.filter, log, "filter");
        setFromToml(s.max_skip_duration, log, "maxSkipDuration");
        setFromToml(s.separate, log, "separate");
        setFromToml(s.size_limit, log, "sizeLimit");
        setFromToml(s.skip_duplicate, log, "skipDuplicate");
        setFromToml(s.sync, log, "sync");
#ifdef _WIN32
        setFromToml(s.type, log, "type");
#endif
    }

    if (og_data.contains("Input")) {
        const toml::value& input = og_data.at("Input");
        auto& s = m_input;

        setFromToml(s.cursor_state, input, "cursorState");
        setFromToml(s.cursor_hide_timeout, input, "cursorHideTimeout");
        setFromToml(s.use_special_pad, input, "useSpecialPad");
        setFromToml(s.special_pad_class, input, "specialPadClass");
        setFromToml(s.motion_controls_enabled, input, "isMotionControlsEnabled");
        setFromToml(s.use_unified_input_config, input, "useUnifiedInputConfig");
        setFromToml(s.background_controller_input, input, "backgroundControllerInput");
        setFromToml(s.usb_device_backend, input, "usbDeviceBackend");
    }

    if (og_data.contains("Audio")) {
        const toml::value& audio = og_data.at("Audio");
        auto& s = m_audio;

        setFromToml(s.sdl_mic_device, audio, "micDevice");
        setFromToml(s.sdl_main_output_device, audio, "mainOutputDevice");
        setFromToml(s.sdl_padSpk_output_device, audio, "padSpkOutputDevice");
    }

    if (og_data.contains("GPU")) {
        const toml::value& gpu = og_data.at("GPU");
        auto& s = m_gpu;

        setFromToml(s.window_width, gpu, "screenWidth");
        setFromToml(s.window_height, gpu, "screenHeight");
        setFromToml(s.internal_screen_width, gpu, "internalScreenWidth");
        setFromToml(s.internal_screen_height, gpu, "internalScreenHeight");
        setFromToml(s.null_gpu, gpu, "nullGpu");
        setFromToml(s.copy_gpu_buffers, gpu, "copyGPUBuffers");
        setFromToml(s.readbacks_mode, gpu, "readbacksMode");
        setFromToml(s.readback_linear_images_enabled, gpu, "readbackLinearImages");
        setFromToml(s.direct_memory_access_enabled, gpu, "directMemoryAccess");
        setFromToml(s.dump_shaders, gpu, "dumpShaders");
        setFromToml(s.patch_shaders, gpu, "patchShaders");
        setFromToml(s.vblank_frequency, gpu, "vblankFrequency");
        setFromToml(s.full_screen, gpu, "Fullscreen");
        setFromToml(s.full_screen_mode, gpu, "FullscreenMode");
        setFromToml(s.present_mode, gpu, "presentMode");
        setFromToml(s.hdr_allowed, gpu, "allowHDR");
        setFromToml(s.fsr_enabled, gpu, "fsrEnabled");
        setFromToml(s.rcas_enabled, gpu, "rcasEnabled");
        setFromToml(s.rcas_attenuation, gpu, "rcasAttenuation");
    }

    if (og_data.contains("Vulkan")) {
        const toml::value& vk = og_data.at("Vulkan");
        auto& s = m_vulkan;

        setFromToml(s.gpu_id, vk, "gpuId");
        setFromToml(s.vkvalidation_enabled, vk, "validation");
        setFromToml(s.vkvalidation_core_enabled, vk, "validation_core");
        setFromToml(s.vkvalidation_sync_enabled, vk, "validation_sync");
        setFromToml(s.vkvalidation_gpu_enabled, vk, "validation_gpu");
        setFromToml(s.vkcrash_diagnostic_enabled, vk, "crashDiagnostic");
        setFromToml(s.vkhost_markers, vk, "hostMarkers");
        setFromToml(s.vkguest_markers, vk, "guestMarkers");
        setFromToml(s.renderdoc_enabled, vk, "rdocEnable");
        setFromToml(s.pipeline_cache_enabled, vk, "pipelineCacheEnable");
        setFromToml(s.pipeline_cache_archived, vk, "pipelineCacheArchive");
    }

    if (og_data.contains("Debug")) {
        const toml::value& debug = og_data.at("Debug");
        auto& s = m_debug;

        setFromToml(s.debug_dump, debug, "DebugDump");
        setFromToml(s.shader_collect, debug, "CollectShader");
        setFromToml(m_general.show_fps_counter, debug, "showFpsCounter");
    }

    if (og_data.contains("Settings")) {
        const toml::value& settings = og_data.at("Settings");
        auto& s = m_general;
        setFromToml(s.console_language, settings, "consoleLanguage");
    }

    if (og_data.contains("GUI")) {
        const toml::value& gui = og_data.at("GUI");
        auto& s = m_general;

        // Transfer install directories
        try {
            const auto install_dir_array =
                toml::find_or<std::vector<std::string>>(gui, "installDirs", {});
            std::vector<bool> install_dirs_enabled;

            try {
                install_dirs_enabled = toml::find<std::vector<bool>>(gui, "installDirsEnabled");
            } catch (...) {
                // If it does not exist, assume that all are enabled.
                install_dirs_enabled.resize(install_dir_array.size(), true);
            }

            if (install_dirs_enabled.size() < install_dir_array.size()) {
                install_dirs_enabled.resize(install_dir_array.size(), true);
            }

            std::vector<GameInstallDir> settings_install_dirs;
            for (size_t i = 0; i < install_dir_array.size(); i++) {
                settings_install_dirs.push_back(
                    {std::filesystem::path{install_dir_array[i]}, install_dirs_enabled[i]});
            }
            s.install_dirs.value = settings_install_dirs;
        } catch (const std::exception& e) {
            LOG_WARNING(Config, "Failed to transfer install directories: {}", e.what());
        }

        // Transfer addon install directory
        try {
            std::string addon_install_dir_str;
            if (gui.contains("addonInstallDir")) {
                const auto& addon_value = gui.at("addonInstallDir");
                if (addon_value.is_string()) {
                    addon_install_dir_str = toml::get<std::string>(addon_value);
                    if (!addon_install_dir_str.empty()) {
                        s.addon_install_dir.value = std::filesystem::path{addon_install_dir_str};
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_WARNING(Config, "Failed to transfer addon install directory: {}", e.what());
        }
    }
    if (og_data.contains("General")) {
        const toml::value& general = og_data.at("General");
        auto& s = m_general;
        // Transfer sysmodules install directory
        try {
            std::string sysmodules_install_dir_str;
            if (general.contains("sysModulesPath")) {
                const auto& sysmodule_value = general.at("sysModulesPath");
                if (sysmodule_value.is_string()) {
                    sysmodules_install_dir_str = toml::get<std::string>(sysmodule_value);
                    if (!sysmodules_install_dir_str.empty()) {
                        s.sys_modules_dir.value = std::filesystem::path{sysmodules_install_dir_str};
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_WARNING(Config, "Failed to transfer sysmodules install directory: {}", e.what());
        }

        // Transfer font install directory
        try {
            std::string font_install_dir_str;
            if (general.contains("fontsPath")) {
                const auto& font_value = general.at("fontsPath");
                if (font_value.is_string()) {
                    font_install_dir_str = toml::get<std::string>(font_value);
                    if (!font_install_dir_str.empty()) {
                        s.font_dir.value = std::filesystem::path{font_install_dir_str};
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_WARNING(Config, "Failed to transfer font install directory: {}", e.what());
        }
    }

    return true;
}

std::vector<std::string> EmulatorSettingsImpl::GetAllOverrideableKeys() const {
    std::vector<std::string> keys;
    auto addGroup = [&keys](const auto& fields) {
        for (const auto& item : fields)
            keys.push_back(item.key);
    };
    addGroup(m_general.GetOverrideableFields());
    addGroup(m_log.GetOverrideableFields());
    addGroup(m_debug.GetOverrideableFields());
    addGroup(m_input.GetOverrideableFields());
    addGroup(m_audio.GetOverrideableFields());
    addGroup(m_gpu.GetOverrideableFields());
    addGroup(m_vulkan.GetOverrideableFields());
    return keys;
}
