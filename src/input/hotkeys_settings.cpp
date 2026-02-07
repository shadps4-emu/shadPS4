// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <iomanip>
#include <iostream>
#include "common/path_util.h"
#include "hotkeys_settings.h"

using json = nlohmann::json;

std::shared_ptr<HotkeysSettingsManager> HotkeysSettingsManager::s_instance = nullptr;
std::mutex HotkeysSettingsManager::s_mutex;

// --------------------
// ctor/dtor + singleton
// --------------------
HotkeysSettingsManager::HotkeysSettingsManager() {
    // Load on construction
    Load();
}

HotkeysSettingsManager::~HotkeysSettingsManager() {
    Save();
}

std::shared_ptr<HotkeysSettingsManager> HotkeysSettingsManager::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance) {
        s_instance = std::make_shared<HotkeysSettingsManager>();
    }
    return s_instance;
}

void HotkeysSettingsManager::SetInstance(std::shared_ptr<HotkeysSettingsManager> instance) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = instance;
}

// --------------------
// Path helpers
// --------------------
std::filesystem::path HotkeysSettingsManager::GetDefaultConfigPath() const {
    const std::filesystem::path userDir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    return userDir / "hotkeys.json";
}

std::filesystem::path HotkeysSettingsManager::GetConfigPath() const {
    return GetDefaultConfigPath();
}

// --------------------
// Merge with latest defaults
// --------------------
void HotkeysSettingsManager::MergeWithLatestDefaults() {
    // Create a settings object with latest defaults
    HotkeysSettings latestDefaults;
    latestDefaults.SetDefaultValues();
    latestDefaults.UpdateToLatestVersion();

    bool needsSave = false;
    int originalVersion = m_settings.version;

    // Update to latest version structure
    m_settings.UpdateToLatestVersion();

    // Check for missing hotkeys from latest defaults
    for (const auto& [key, defaultValue] : latestDefaults.bindings) {
        if (m_settings.bindings.find(key) == m_settings.bindings.end()) {
            // Add missing default hotkey
            m_settings.bindings[key] = defaultValue;
            needsSave = true;
            std::cout << "[HotkeysSettings] Added missing default hotkey: " << key << " = "
                      << defaultValue << std::endl;
        }
    }

    // Save if we made any changes
    if (needsSave || originalVersion < m_settings.version) {
        std::cout << "[HotkeysSettings] Updated from version " << originalVersion << " to version "
                  << m_settings.version << std::endl;
        Save();
    }
}

// --------------------
// Load/Save helpers
// --------------------
bool HotkeysSettingsManager::LoadFromPath(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            std::cout << "[HotkeysSettings] Config file not found: " << path << std::endl;
            return false;
        }

        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "[HotkeysSettings] Failed to open config file: " << path << std::endl;
            return false;
        }

        json j;
        in >> j;

        // Load settings from JSON
        m_settings = j.get<HotkeysSettings>();

        std::cout << "[HotkeysSettings] Loaded version " << m_settings.version << " with "
                  << m_settings.bindings.size() << " hotkeys from: " << path << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[HotkeysSettings] Error loading from " << path << ": " << e.what()
                  << std::endl;
        return false;
    }
}

bool HotkeysSettingsManager::SaveToPath(const std::filesystem::path& path) const {
    try {
        // Ensure directory exists
        std::filesystem::create_directories(path.parent_path());

        json j = m_settings;

        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "[HotkeysSettings] Failed to open file for writing: " << path << std::endl;
            return false;
        }

        out << std::setw(4) << j;
        out.flush();

        if (out.fail()) {
            std::cerr << "[HotkeysSettings] Failed to write hotkeys to: " << path << std::endl;
            return false;
        }

        std::cout << "[HotkeysSettings] Saved version " << m_settings.version << " with "
                  << m_settings.bindings.size() << " hotkeys to: " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[HotkeysSettings] Error saving to " << path << ": " << e.what() << std::endl;
        return false;
    }
}

// --------------------
// Public Load/Save
// --------------------
bool HotkeysSettingsManager::Load() {
    const std::filesystem::path configPath = GetDefaultConfigPath();

    if (LoadFromPath(configPath)) {
        // Successfully loaded, merge with latest defaults
        MergeWithLatestDefaults();
        return true;
    } else {
        // File doesn't exist or failed to load, use latest defaults
        std::cout << "[HotkeysSettings] Creating new config with latest defaults" << std::endl;
        m_settings.SetDefaultValues();
        m_settings.UpdateToLatestVersion();
        // Save new config
        Save();
        return true;
    }
}

bool HotkeysSettingsManager::Save() const {
    return SaveToPath(GetDefaultConfigPath());
}

bool HotkeysSettingsManager::Load(const std::filesystem::path& customPath) {
    if (LoadFromPath(customPath)) {
        MergeWithLatestDefaults();
        return true;
    }
    return false;
}

bool HotkeysSettingsManager::Save(const std::filesystem::path& customPath) const {
    return SaveToPath(customPath);
}

// --------------------
// Hotkey accessors
// --------------------
std::string HotkeysSettingsManager::GetHotkey(const std::string& action) const {
    auto it = m_settings.bindings.find(action);
    if (it != m_settings.bindings.end()) {
        return it->second;
    }
    return ""; // Return empty string if not found
}

void HotkeysSettingsManager::SetHotkey(const std::string& action, const std::string& keyCombo) {
    m_settings.bindings[action] = keyCombo;
}

bool HotkeysSettingsManager::HasHotkey(const std::string& action) const {
    return m_settings.bindings.find(action) != m_settings.bindings.end();
}

void HotkeysSettingsManager::RemoveHotkey(const std::string& action) {
    m_settings.bindings.erase(action);
}