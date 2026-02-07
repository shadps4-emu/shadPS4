// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct HotkeysSettings {
    int version{1};
    std::unordered_map<std::string, std::string> bindings;

    HotkeysSettings() {
        SetDefaultValues();
    }

    void SetDefaultValues() {
        bindings.clear();
        // Version 1 defaults
        bindings["renderdoc_capture"] = "f12";
        bindings["fullscreen"] = "f11";
        bindings["show_fps"] = "f10";
        bindings["pause"] = "f9";
        bindings["reload_inputs"] = "f8";
        bindings["toggle_mouse_to_joystick"] = "f7";
        bindings["toggle_mouse_to_gyro"] = "f6";
        bindings["toggle_mouse_to_touchpad"] = "delete";
        bindings["quit"] = "lctrl, lshift, end";
        bindings["volume_up"] = "kpplus";
        bindings["volume_down"] = "kpminus";
        version = 1;
    }

    // Add new defaults for version 2
    void AddVersion2Defaults() {
        if (version < 2) {
            // add new hotkeys for version 2
            // version = 2; and increase version
        }
    }

    // Apply all version updates
    void UpdateToLatestVersion() {
        // AddVersion2Defaults();
        //  Add more version updates here as needed
    }
};

namespace nlohmann {
template <>
struct adl_serializer<HotkeysSettings> {
    static void to_json(json& j, const HotkeysSettings& s) {
        j = json::object();
        j["version"] = s.version;
        j["hotkeys"] = json::object();
        for (const auto& [key, value] : s.bindings) {
            j["hotkeys"][key] = value;
        }
    }

    static void from_json(const json& j, HotkeysSettings& s) {
        s.bindings.clear();
        s.version = j["version"].get<int>();
        // Load hotkeys
        if (j.contains("hotkeys") && j["hotkeys"].is_object()) {
            for (auto& [key, value] : j["hotkeys"].items()) {
                if (value.is_string()) {
                    s.bindings[key] = value.get<std::string>();
                }
            }
        }
    }
};
} // namespace nlohmann

class HotkeysSettingsManager {
public:
    HotkeysSettingsManager();
    ~HotkeysSettingsManager();

    static std::shared_ptr<HotkeysSettingsManager> GetInstance();
    static void SetInstance(std::shared_ptr<HotkeysSettingsManager> instance);

    bool Save() const;
    bool Load();
    bool Save(const std::filesystem::path& customPath) const;
    bool Load(const std::filesystem::path& customPath);

    // Hotkey accessors
    std::string GetHotkey(const std::string& action) const;
    void SetHotkey(const std::string& action, const std::string& keyCombo);
    bool HasHotkey(const std::string& action) const;
    void RemoveHotkey(const std::string& action);

    // Get all hotkeys
    const std::unordered_map<std::string, std::string>& GetAllHotkeys() const {
        return m_settings.bindings;
    }

    // Get current version
    int GetVersion() const {
        return m_settings.version;
    }

    // Reset to latest defaults
    void ResetToDefaults() {
        m_settings.SetDefaultValues();
        m_settings.UpdateToLatestVersion();
    }

    // Add new hotkeys (preserves user customizations)
    void AddNewHotkeys(const std::unordered_map<std::string, std::string>& newHotkeys) {
        for (const auto& [action, combo] : newHotkeys) {
            // Only add if not already present (preserve user changes)
            if (m_settings.bindings.find(action) == m_settings.bindings.end()) {
                m_settings.bindings[action] = combo;
            }
        }
    }

    // Update to latest version (adds missing defaults)
    void UpdateToLatestVersion() {
        m_settings.UpdateToLatestVersion();
    }

    // Get the config file path
    std::filesystem::path GetConfigPath() const;

private:
    HotkeysSettings m_settings;

    static std::shared_ptr<HotkeysSettingsManager> s_instance;
    static std::mutex s_mutex;

    std::filesystem::path GetDefaultConfigPath() const;
    bool LoadFromPath(const std::filesystem::path& path);
    bool SaveToPath(const std::filesystem::path& path) const;

    // Merge loaded settings with latest defaults (adds missing, preserves user values)
    void MergeWithLatestDefaults();
};