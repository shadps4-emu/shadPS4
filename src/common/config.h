// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <toml.hpp>

#include "common/path_util.h"

#define u32 uint32_t
#define s32 int32_t

namespace Config {
// Name of the global configuration
static std::string c_globalTitleId = "config";
static std::string c_configExtension = ".toml";

class Configuration;

class ConfigManager {
private:
    // This is the map of title id <-> configuraation
    std::map<std::string, std::shared_ptr<Config::Configuration>> configurations;
    std::string currentTitleId = c_globalTitleId;

    // ConfigManager() = default;
    // virtual ~ConfigManager() = default;

public:
    void loadConfigurations();

    void setCurrentConfigId(std::string titleId);

    void setDefaultConfigId();

    const std::string getCurrentConfigId();

    std::shared_ptr<Config::Configuration> getCurrentConfig();
};

class Configuration {
private:
    const toml::table c_defaultConfig = {
        {"titleId", c_globalTitleId},
        {"General",
         toml::table{
             {"isPS4Pro", false},
             {"Fullscreen", false},
             {"logFilter", ""},
             {"logType", "async"},
             {"userName", "shadPS4"},
             {"showSplash", false},
             {"useSpecialPad", false},
             {"specialPadClass", 1},
             {"screenWidth", 1280},
             {"screenHeight", 720},
             {"nullGpu", false},
             {"copyGPUBuffers", false},
             {"dumpShaders", false},
             {"dumpPM4", false},
             {"vblankDivider", 1},
         }},
        {
            "Vulkan",
            toml::table{
                {"gpuId", (-1)},
                {"validation", false},
                {"validationSync", false}, // Breaking change
                {"validationGpu", false},  // Breaking change
                {"rdocEnable", false},
                {"rdocMarkersEnable", false},
                {"debugDump", false},
                {"crashDiagnostic", false},
            },
        },
        {
            "GUI",
            toml::table{{"theme", 0},
                        {"iconSize", 36},
                        {"iconSizeGrid", 69},
                        {"sliderPos", 0},
                        {"sliderPosGrid", 0},
                        {"gameTableMode", 0},
                        {"mwWidth", 1280}, // Breaking change
                        {"mwHeight", 720}, // Breaking change
                        {"installDir", ""},
                        {"geometryX", 400},  // Breaking change
                        {"geometryY", 400},  // Breaking change
                        {"geometryW", 1280}, // Breaking change
                        {"geometryH", 720},  // Breaking change
                        {"pkgDirs", toml::array{}},
                        {"elfDirs", toml::array{}},
                        {"recentFiles", toml::array{}},
                        {"emulatorLanguage", "en"}},
        },
        {
            "Settings",
            toml::table{{"consoleLanguage", 1}},
        },
    };

protected:
    // Title id of the game, or "global" for global settings
    std::string titleId;

    // Toml data, do not modify this directly
    toml::value data;

public:
    /// <summary>
    /// Create a new configuration with defaults
    /// </summary>
    Configuration();

    /// <summary>
    /// Load configuration from file path
    /// </summary>
    /// <param name="path">Path of configuration file</param>
    Configuration(const std::filesystem::path& path);

    /// <summary>
    /// Loads a configuration from file path
    /// </summary>
    /// <param name="path">Path of configuration file</param>
    void load(const std::filesystem::path& path);

    /// <summary>
    /// Saves a configuration to file path
    /// </summary>
    /// <param name="path">Path of configuration file</param>
    void save(const std::filesystem::path& path);

    /// <summary>
    /// This function will iterate through all of the section and entries in the default
    /// configuration
    ///
    /// It will check to make sure that all of the categories, and keys match in both name and type
    /// </summary>
    /// <param name="path">Path of configuration file to check</param>
    /// <returns>True if there is a structural difference in config files, false if no
    /// difference</returns>
    bool configVersionDifference(const std::filesystem::path& path);

    template <typename T>
    T getValue(const char* category, const char* key) {
        // Debug logging for profiling
        fmt::print("DBG: getValue ({}) ({})\n", category, key);

        if (!c_defaultConfig.contains(category))
            return T();

        const toml::value& defaultGeneral = c_defaultConfig.at(category);

        auto defaultValue = toml::find_or<T>(defaultGeneral, key, T());

        if (data.contains(category)) {
            const toml::value& general = data.at(category);

            return toml::find_or<T>(general, key, defaultValue);
        }

        return T();
    }

    template <typename T>
    void setValue(const char* category, const char* key, T value) {
        if (!data.contains(category))
            return;

        auto& dataCategory = data[category];

        if (!dataCategory.contains(key))
            return;

        data[category][key] = value;
    }

    /*
     * Alright, making some changes here, first is keeping the
     * default config and the C++ functions in the same order
     */

    const std::string& getTitleId() const;

#pragma region General settings
    bool isNeoMode();
    void setNeoMode(bool enable);

    bool isFullscreenMode();
    void setFullscreenMode(bool enable);

    std::string getLogFilter();
    void setLogFilter(const std::string& type);

    std::string getLogType();
    void setLogType(const std::string& type);

    std::string getUserName();
    void setUserName(const std::string& type);

    bool showSplash();
    void setShowSplash(bool enable);

    bool getUseSpecialPad();
    void setUseSpecialPad(bool use);

    int getSpecialPadClass();
    void setSpecialPadClass(int type);

    u32 getScreenWidth();
    void setScreenWidth(u32 width);

    u32 getScreenHeight();
    void setScreenHeight(u32 height);

    bool nullGpu();
    void setNullGpu(bool enable);

    bool copyGPUCmdBuffers();
    void setCopyGPUCmdBuffers(bool enable);

    bool dumpShaders();
    void setDumpShaders(bool enable);

    bool dumpPM4();
    void setDumpPM4(bool enable);

    u32 vblankDiv();
    void setVblankDiv(u32 value);

#pragma endregion

#pragma region Vulkan settings
    s32 getGpuId();
    void setGpuId(s32 selectedGpuId);

    bool vkValidationEnabled();
    void setVkValidation(bool enable);

    bool vkValidationSyncEnabled();
    void setVkSyncValidation(bool enable);

    bool vkValidationGpuEnabled();
    void setVkValidationGpuEnabled(bool enable);

    bool isRdocEnabled();
    void setRdocEnabled(bool enable);

    bool vkMarkersEnabled();
    void setVkMarkersEnabled(bool enable);

    bool debugDump();
    void setDebugDump(bool enable);

    bool vkCrashDiagnostic();
    void setVkCrashDiagnostic(bool enable);
#pragma endregion

#pragma region GUI settings
    u32 getMainWindowTheme();
    void setMainWindowTheme(u32 theme);

    u32 getIconSize();
    void setIconSize(u32 size);

    u32 getIconSizeGrid();
    void setIconSizeGrid(u32 size);

    u32 getSliderPosition();
    void setSliderPosition(u32 pos);

    u32 getSliderPositionGrid();
    void setSliderPositionGrid(u32 pos);

    u32 getTableMode();
    void setTableMode(u32 mode);

    u32 getMainWindowWidth();
    void setMainWindowWidth(u32 width);

    u32 getMainWindowHeight();
    void setMainWindowHeight(u32 height);

    std::string getGameInstallDir();
    void setGameInstallDir(const std::string& dir);

    u32 getMainWindowGeometryX();
    u32 getMainWindowGeometryY();
    u32 getMainWindowGeometryW();
    u32 getMainWindowGeometryH();
    void setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h);

    std::vector<std::string> getPkgViewer();
    void setPkgViewer(const std::vector<std::string>& pkgList);

    std::vector<std::string> getElfViewer();
    void setElfViewer(const std::vector<std::string>& elfList);

    std::vector<std::string> getRecentFiles();
    void setRecentFiles(const std::vector<std::string>& recentFiles);

    std::string getEmulatorLanguage();
    void setEmulatorLanguage(std::string language);
#pragma endregion

#pragma region Console settings
    u32 getConsoleLanguage();
    void setLanguage(u32 language);
#pragma endregion

    /// <summary>
    /// Sets the data to the default values
    /// </summary>
    void setDefaultValues();
};

/*
 * ============================================================
 * THIS IS TO TEST FUNCTIONALITY, EVENTUALLY NUKE BELOW
 *
 * NOTE: For all of these should fall back on defaults in event config
 * manager is busted for whatever reason (which should not happen)
 * ============================================================
 */
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
bool isFullscreenMode();
std::string getLogFilter();
std::string getLogType();
std::string getUserName();

bool getUseSpecialPad();
int getSpecialPadClass();

u32 getScreenWidth();
u32 getScreenHeight();
s32 getGpuId();

bool debugDump();
bool showSplash();
bool nullGpu();
bool copyGPUCmdBuffers();
bool dumpShaders();
bool dumpPM4();
bool isRdocEnabled();
bool vkMarkersEnabled();
bool vkCrashDiagnosticEnabled();
u32 vblankDiv();

void setDebugDump(bool enable);
void setShowSplash(bool enable);
void setNullGpu(bool enable);
void setCopyGPUCmdBuffers(bool enable);
void setDumpShaders(bool enable);
void setDumpPM4(bool enable);
void setVblankDiv(u32 value);
void setGpuId(s32 selectedGpuId);
void setScreenWidth(u32 width);
void setScreenHeight(u32 height);
void setFullscreenMode(bool enable);
void setLanguage(u32 language);
void setNeoMode(bool enable);
void setUserName(const std::string& type);

void setUseSpecialPad(bool use);
void setSpecialPadClass(int type);

void setLogType(const std::string& type);
void setLogFilter(const std::string& type);

void setVkValidation(bool enable);
void setVkSyncValidation(bool enable);
void setRdocEnabled(bool enable);

bool vkValidationEnabled();
bool vkValidationSyncEnabled();
bool vkValidationGpuEnabled();

// Gui
void setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h);
void setGameInstallDir(const std::string& dir);
void setMainWindowTheme(u32 theme);
void setIconSize(u32 size);
void setIconSizeGrid(u32 size);
void setSliderPosition(u32 pos);
void setSliderPositionGrid(u32 pos);
void setTableMode(u32 mode);
void setMainWindowWidth(u32 width);
void setMainWindowHeight(u32 height);
void setPkgViewer(const std::vector<std::string>& pkgList);
void setElfViewer(const std::vector<std::string>& elfList);
void setRecentFiles(const std::vector<std::string>& recentFiles);
void setEmulatorLanguage(std::string language);

u32 getMainWindowGeometryX();
u32 getMainWindowGeometryY();
u32 getMainWindowGeometryW();
u32 getMainWindowGeometryH();
std::string getGameInstallDir();
u32 getMainWindowTheme();
u32 getIconSize();
u32 getIconSizeGrid();
u32 getSliderPosition();
u32 getSliderPositionGrid();
u32 getTableMode();
u32 getMainWindowWidth();
u32 getMainWindowHeight();
std::vector<std::string> getPkgViewer();
std::vector<std::string> getElfViewer();
std::vector<std::string> getRecentFiles();
std::string getEmulatorLanguage();

void setDefaultValues();

// settings
u32 GetLanguage();
} // namespace Config