// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include <filesystem>
#include <iostream>

#include "common/singleton.h"

using namespace Config;

void Config::ConfigManager::loadConfigurations() {
    const auto configs_dir = Common::FS::GetUserPath(Common::FS::PathType::ConfigsDir);

    fmt::print("Iterating configuration files from: {}\n", configs_dir.string());

    bool configsLoaded = false;

    // Iterate over all files in the directory
    for (auto& file : std::filesystem::recursive_directory_iterator(configs_dir)) {
        // Skip any files that aren't toml files
        if (file.path().extension() != ".toml")
            continue;

        fmt::print("Loading configuration file {}\n", file.path().string());

        auto loadedConfiguration = std::make_shared<Config::Configuration>(file.path());
        if (!loadedConfiguration) {
            fmt::print("Error loading configuration file ({}).", file.path().string());
            continue;
        }

        // Validate title id
        auto& loadedTitleId = loadedConfiguration->getTitleId();
        if (loadedTitleId.empty()) {
            loadedConfiguration.reset();
            continue;
        }

        configurations[loadedTitleId] = loadedConfiguration;
        configsLoaded = true;
    }

    // If there were no configurations loaded, then create a global one
    if (!configsLoaded)
        configurations[c_globalTitleId] = std::make_shared<Config::Configuration>(
            configs_dir / (c_globalTitleId + c_configExtension));
}

void Config::ConfigManager::setCurrentConfigId(std::string titleId) {
    // Check if the current title id is empty for user error
    if (titleId.empty()) {
        currentTitleId = c_globalTitleId;
        return;
    }

    currentTitleId = titleId;
}

void Config::ConfigManager::setDefaultConfigId() {
    currentTitleId = c_globalTitleId;
}

const std::string Config::ConfigManager::getCurrentConfigId() {
    return currentTitleId;
}

std::shared_ptr<Config::Configuration> Config::ConfigManager::getCurrentConfig() {
    if (configurations.empty())
        return nullptr;

    if (currentTitleId.empty())
        return nullptr;

    if (configurations.find(currentTitleId) == configurations.end())
        return nullptr;

    return configurations[currentTitleId];
}

Config::Configuration::Configuration() {
    setDefaultValues();
}

Config::Configuration::Configuration(const std::filesystem::path& path) : Config::Configuration() {
    load(path);
}

void Config::Configuration::load(const std::filesystem::path& path) {
    // Validate that the incoming configuration file exists
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        save(path);
        fmt::print("err: configuration file path ({}) does not exist.\n", path.string());
        return;
    }

    // Attempt to parse the configuration data
    try {
        data = toml::parse(path);
    } catch (std::exception& ex) {
        fmt::print("err: could not load configuration (ex: {}).\n", ex.what());
        return;
    }
}

void Config::Configuration::save(const std::filesystem::path& path) {
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        try {
            data = toml::parse(path);
        } catch (const std::exception& ex) {
            return;
        }
    } else {
        if (error) {
        }

        fmt::print("saving new configuration file ({}).\n", path.string());
    }

    std::ofstream saveFile(path, std::ios::out);
    saveFile << data;
    saveFile.close();
}

bool Configuration::configVersionDifference(const std::filesystem::path& path) {
    // Validate that the incoming configuration file exists
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        fmt::print("err: configuration file path ({}) does not exist.\n", path.string());
        return true;
    }

    // Attempt to parse the configuration data
    toml::value oldConfigData;
    try {
        oldConfigData = toml::parse(path);
    } catch (std::exception& ex) {
        fmt::print("err: could not open old configuration file (ex: {}).\n", ex.what());
        return true;
    }

    // Iterate checking for new entries that do not exist in the provided config
    for (auto& item : c_defaultConfig) {
        // Get the key name
        auto& defaultFirst = item.first;
        auto& defaultSecond = item.second;

        // Check to see if the old configuration contains the key provided
        if (oldConfigData.contains(defaultFirst)) {
            // Check to see that the types match for the second
            if (oldConfigData[defaultFirst].type() != defaultSecond.type()) {
                return true;
            }
        } else
        {
            // If the key does not exist in the old config but exists in the new, mark difference
            return true;
        }

        auto& oldConfigSecond = oldConfigData[defaultFirst];

        // If there is a nested table, check the sub-entries
        if (defaultSecond.is_table()) {
            toml::table secondTable = defaultSecond.as_table();

            for (auto& tableItemPair : secondTable) {
                auto& secondItemFirst = tableItemPair.first;
                auto& secondItemSecond = tableItemPair.second;

                if (oldConfigSecond.contains(secondItemFirst)) {
                    // Check for type match
                    if (oldConfigSecond[secondItemFirst].type() != secondItemSecond.type()) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
        }
    }
    return false;
}

const std::string& Config::Configuration::getTitleId() const {
    return titleId;
}

bool Config::Configuration::isNeoMode() {
    return getValue<bool>("General", "isPS4Pro");
}

void Config::Configuration::setNeoMode(bool enable) {
    setValue("General", "isPS4Pro", enable);
}

bool Config::Configuration::isFullscreenMode() {
    return getValue<bool>("General", "Fullscreen");
}

void Config::Configuration::setFullscreenMode(bool enable) {
    setValue("General", "Fullscreen", enable);
}

std::string Config::Configuration::getLogFilter() {
    return getValue<std::string>("General", "logFilter");
}

void Config::Configuration::setLogFilter(const std::string& type) {
    setValue("General", "logFilter", type);
}

std::string Config::Configuration::getLogType() {
    return getValue<std::string>("General", "logType");
}

void Config::Configuration::setLogType(const std::string& type) {
    setValue("General", "logType", type);
}

std::string Config::Configuration::getUserName() {
    return getValue<std::string>("General", "userName");
}

void Config::Configuration::setUserName(const std::string& type) {
    setValue("General", "userName", type);
}

bool Config::Configuration::showSplash() {
    return getValue<bool>("General", "showSplash");
}

void Config::Configuration::setShowSplash(bool enable) {
    return setValue("General", "showSplash", enable);
}

bool Config::Configuration::getUseSpecialPad() {
    return getValue<bool>("General", "useSpecialPad");
}

void Config::Configuration::setUseSpecialPad(bool use) {
    setValue("General", "useSpecialPad", use);
}

int Config::Configuration::getSpecialPadClass() {
    return getValue<int>("General", "specialPadClass");
}

void Config::Configuration::setSpecialPadClass(int type) {
    setValue("General", "specialPadClass", type);
}

u32 Config::Configuration::getScreenWidth() {
    return getValue<uint32_t>("General", "screenWidth");
}

void Config::Configuration::setScreenWidth(u32 width) {
    setValue("General", "screenWidth", width);
}

u32 Config::Configuration::getScreenHeight() {
    return getValue<uint32_t>("General", "screenHeight");
}

void Config::Configuration::setScreenHeight(u32 height) {
    setValue("General", "screenHeight", height);
}

bool Config::Configuration::nullGpu() {
    return getValue<bool>("General", "nullGpu");
}

void Config::Configuration::setNullGpu(bool enable) {
    setValue("General", "nullGpu", enable);
}

bool Config::Configuration::copyGPUCmdBuffers() {
    return getValue<bool>("General", "copyGPUBuffers");
}

void Config::Configuration::setCopyGPUCmdBuffers(bool enable) {
    setValue("General", "copyGPUBuffers", enable);
}

bool Config::Configuration::dumpShaders() {
    return getValue<bool>("General", "dumpShaders");
}

void Config::Configuration::setDumpShaders(bool enable) {
    setValue("General", "dumpShader", enable);
}

bool Config::Configuration::dumpPM4() {
    return getValue<bool>("General", "dumpPM4");
}

void Config::Configuration::setDumpPM4(bool enable) {
    setValue("General", "dumpPM4", enable);
}

u32 Config::Configuration::vblankDiv() {
    return getValue<u32>("General", "vblankDivider");
}

void Config::Configuration::setVblankDiv(u32 value) {
    setValue("General", "vblankDivider", value);
}

s32 Config::Configuration::getGpuId() {
    return getValue<s32>("Vulkan", "gpuId");
}

void Config::Configuration::setGpuId(s32 selectedGpuId) {
    setValue("Vulkan", "gpuId", selectedGpuId);
}

bool Config::Configuration::vkValidationEnabled() {
    return getValue<bool>("Vulkan", "validation");
}

void Config::Configuration::setVkValidation(bool enable) {
    setValue("Vulkan", "validation", enable);
}

bool Config::Configuration::vkValidationSyncEnabled() {
    return getValue<bool>("Vulkan", "validationSync");
}

void Config::Configuration::setVkSyncValidation(bool enable) {
    setValue("Vulkan", "validationSync", enable);
}

bool Config::Configuration::vkValidationGpuEnabled() {
    return getValue<bool>("Vulkan", "validationGpu");
}

void Config::Configuration::setVkValidationGpuEnabled(bool enable) {
    setValue("Vulkan", "validationGpu", enable);
}

bool Config::Configuration::isRdocEnabled() {
    return getValue<bool>("Vulkan", "rdocEnable");
}

void Config::Configuration::setRdocEnabled(bool enable) {
    setValue("Vulkan", "rdocEnable", enable);
}

bool Config::Configuration::vkMarkersEnabled() {
    return getValue<bool>("Vulkan", "vkMarkersEnabled");
}

void Config::Configuration::setVkMarkersEnabled(bool enable) {
    setValue("Vulkan", "rdocMarkersEnable", enable);
}

bool Config::Configuration::debugDump() {
    return getValue<bool>("Vulkan", "debugDump");
}

void Config::Configuration::setDebugDump(bool enable) {
    setValue("Vulkan", "debugDump", enable);
}

bool Config::Configuration::vkCrashDiagnostic() {
    return getValue<bool>("Vulkan", "crashDiagnostic");
}

void Config::Configuration::setVkCrashDiagnostic(bool enable) {
    setValue("Vulkan", "crashDiagnostic", enable);
}

u32 Config::Configuration::getMainWindowTheme() {
    return getValue<u32>("GUI", "theme");
}

void Config::Configuration::setMainWindowTheme(u32 theme) {
    setValue("GUI", "theme", theme);
}

u32 Config::Configuration::getIconSize() {
    return getValue<u32>("GUI", "iconSize");
}

void Config::Configuration::setIconSize(u32 size) {
    setValue("GUI", "iconSize", size);
}

u32 Config::Configuration::getIconSizeGrid() {
    return getValue<u32>("GUI", "iconSizeGrid");
}

void Config::Configuration::setIconSizeGrid(u32 size) {
    setValue("GUI", "iconSizeGrid", size);
}

u32 Config::Configuration::getSliderPosition() {
    return getValue<u32>("GUI", "sliderPos");
}

void Config::Configuration::setSliderPosition(u32 pos) {
    setValue("GUI", "sliderPos", pos);
}

u32 Config::Configuration::getSliderPositionGrid() {
    return getValue<u32>("GUI", "sliderPosGrid");
}

void Config::Configuration::setSliderPositionGrid(u32 pos) {
    setValue("GUI", "sliderPosGrid", pos);
}

u32 Config::Configuration::getTableMode() {
    return getValue<u32>("GUI", "gameTableMode");
}

void Config::Configuration::setTableMode(u32 mode) {
    setValue("GUI", "gameTableMode", mode);
}

u32 Config::Configuration::getMainWindowWidth() {
    return getValue<u32>("GUI", "mwWidth");
}

void Config::Configuration::setMainWindowWidth(u32 width) {
    setValue("GUI", "mwWidth", width);
}

u32 Config::Configuration::getMainWindowHeight() {
    return getValue<u32>("GUI", "mwHeight");
}

void Config::Configuration::setMainWindowHeight(u32 height) {
    setValue("GUI", "mwHeight", height);
}

std::string Config::Configuration::getGameInstallDir() {
    return getValue<std::string>("GUI", "installDir");
}

void Config::Configuration::setGameInstallDir(const std::string& dir) {
    setValue("GUI", "installDir", dir);
}

u32 Config::Configuration::getMainWindowGeometryX() {
    return getValue<u32>("GUI", "geometryX");
}

u32 Config::Configuration::getMainWindowGeometryY() {
    return getValue<u32>("GUI", "geometryY");
}

u32 Config::Configuration::getMainWindowGeometryW() {
    return getValue<u32>("GUI", "geometryW");
}

u32 Config::Configuration::getMainWindowGeometryH() {
    return getValue<u32>("GUI", "geometryH");
}

void Config::Configuration::setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h) {
    setValue("GUI", "geometryX", x);
    setValue("GUI", "geometryY", y);
    setValue("GUI", "geometryW", w);
    setValue("GUI", "geometryH", h);
}

std::vector<std::string> Config::Configuration::getPkgViewer() {
    if (!data.contains("GUI"))
        return std::vector<std::string>();

    auto& gui = data["GUI"];

    return toml::find_or<std::vector<std::string>>(gui, "pkgDirs", {});
}

void Config::Configuration::setPkgViewer(const std::vector<std::string>& pkgList) {
    data["GUI"]["pkgDirs"] = pkgList;
}

std::vector<std::string> Config::Configuration::getElfViewer() {
    if (!data.contains("GUI"))
        return std::vector<std::string>();

    auto& gui = data["GUI"];

    return toml::find_or<std::vector<std::string>>(gui, "elfDirs", {});
}

void Config::Configuration::setElfViewer(const std::vector<std::string>& elfList) {
    data["GUI"]["elfDirs"] = elfList;
}

std::vector<std::string> Config::Configuration::getRecentFiles() {
    if (!data.contains("GUI"))
        return std::vector<std::string>();

    auto& gui = data["GUI"];

    return toml::find_or<std::vector<std::string>>(gui, "recentFiles", {});
}

void Config::Configuration::setRecentFiles(const std::vector<std::string>& recentFiles) {
    data["GUI"]["recentFiles"] = recentFiles;
}

std::string Config::Configuration::getEmulatorLanguage() {
    return getValue<std::string>("GUI", "emulatorLanguage");
}

void Config::Configuration::setEmulatorLanguage(std::string language) {
    setValue("GUI", "emulatorLanguage", language);
}

u32 Config::Configuration::getConsoleLanguage() {
    return getValue<u32>("Settings", "consoleLanauge");
}

void Config::Configuration::setLanguage(u32 language) {
    setValue("Settings", "consoleLanauge", language);
}

void Config::Configuration::setDefaultValues() {
    data = toml::table(c_defaultConfig);
}

void Config::load(const std::filesystem::path& path) {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    configManager->loadConfigurations();
}

void Config::save(const std::filesystem::path& path) {
    fmt::print("TODO: IMPLEMENT SAVE IN CONFIG MANAGER\n");
}

bool Config::isNeoMode() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->isNeoMode();
}

bool Config::isFullscreenMode() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->isFullscreenMode();
}

std::string Config::getLogFilter() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::string();

    return config->getLogFilter();
}

std::string Config::getLogType() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::string();

    return config->getLogType();
}

std::string Config::getUserName() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::string();

    return config->getUserName();
}

bool Config::getUseSpecialPad() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->getUseSpecialPad();
}

int Config::getSpecialPadClass() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getSpecialPadClass();
}

u32 Config::getScreenWidth() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getScreenWidth();
}

u32 Config::getScreenHeight() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getScreenHeight();
}

s32 Config::getGpuId() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return -1;

    return config->getGpuId();
}

bool Config::debugDump() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->debugDump();
}

bool Config::showSplash() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->showSplash();
}

bool Config::nullGpu() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->nullGpu();
}

bool Config::copyGPUCmdBuffers() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->copyGPUCmdBuffers();
}

bool Config::dumpShaders() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->dumpShaders();
}

bool Config::dumpPM4() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->dumpPM4();
}

bool Config::isRdocEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->isRdocEnabled();
}

bool Config::vkMarkersEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkMarkersEnabled();
}

bool Config::vkCrashDiagnosticEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkCrashDiagnostic();
}

u32 Config::vblankDiv() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->vblankDiv();
}

void Config::setDebugDump(bool enable) {}

void Config::setShowSplash(bool enable) {}

void Config::setNullGpu(bool enable) {}

void Config::setCopyGPUCmdBuffers(bool enable) {}

void Config::setDumpShaders(bool enable) {}

void Config::setDumpPM4(bool enable) {}

void Config::setVblankDiv(u32 value) {}

void Config::setGpuId(s32 selectedGpuId) {}

void Config::setScreenWidth(u32 width) {}

void Config::setScreenHeight(u32 height) {}

void Config::setFullscreenMode(bool enable) {}

void Config::setLanguage(u32 language) {}

void Config::setNeoMode(bool enable) {}

void Config::setUserName(const std::string& type) {}

void Config::setUseSpecialPad(bool use) {}

void Config::setSpecialPadClass(int type) {}

void Config::setLogType(const std::string& type) {}

void Config::setLogFilter(const std::string& type) {}

void Config::setVkValidation(bool enable) {}

void Config::setVkSyncValidation(bool enable) {}

void Config::setRdocEnabled(bool enable) {}

bool Config::vkValidationEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkValidationEnabled();
}

bool Config::vkValidationSyncEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkValidationSyncEnabled();
}

bool Config::vkValidationGpuEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkValidationGpuEnabled();
}

void Config::setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h) {}

void Config::setGameInstallDir(const std::string& dir) {}

void Config::setMainWindowTheme(u32 theme) {}

void Config::setIconSize(u32 size) {}

void Config::setIconSizeGrid(u32 size) {}

void Config::setSliderPosition(u32 pos) {}

void Config::setSliderPositionGrid(u32 pos) {}

void Config::setTableMode(u32 mode) {}

void Config::setMainWindowWidth(u32 width) {}

void Config::setMainWindowHeight(u32 height) {}

void Config::setPkgViewer(const std::vector<std::string>& pkgList) {}

void Config::setElfViewer(const std::vector<std::string>& elfList) {}

void Config::setRecentFiles(const std::vector<std::string>& recentFiles) {}

void Config::setEmulatorLanguage(std::string language) {}

u32 Config::getMainWindowGeometryX() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowGeometryX();
}

u32 Config::getMainWindowGeometryY() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowGeometryY();
}

u32 Config::getMainWindowGeometryW() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowGeometryW();
}

u32 Config::getMainWindowGeometryH() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowGeometryH();
}

std::string Config::getGameInstallDir() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::string();

    return config->getGameInstallDir();
}

u32 Config::getMainWindowTheme() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowTheme();
}

u32 Config::getIconSize() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getIconSize();
}

u32 Config::getIconSizeGrid() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getIconSizeGrid();
}

u32 Config::getSliderPosition() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getSliderPosition();
}

u32 Config::getSliderPositionGrid() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getSliderPositionGrid();
}

u32 Config::getTableMode() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getTableMode();
}

u32 Config::getMainWindowWidth() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->getMainWindowWidth();
}

u32 Config::getMainWindowHeight() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getMainWindowHeight();
}

std::vector<std::string> Config::getPkgViewer() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::vector<std::string>();

    return config->getPkgViewer();
}

std::vector<std::string> Config::getElfViewer() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::vector<std::string>();

    return config->getElfViewer();
}

std::vector<std::string> Config::getRecentFiles() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::vector<std::string>();

    return config->getRecentFiles();
}

std::string Config::getEmulatorLanguage() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return std::string();

    return config->getEmulatorLanguage();
}

void Config::setDefaultValues() {}

u32 Config::GetLanguage() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return 0;

    return config->getConsoleLanguage();
}

bool vkMarkersEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkMarkersEnabled() || config->vkCrashDiagnostic();
}

bool vkCrashDiagnosticEnabled() {
    const auto configManager = Common::Singleton<Config::ConfigManager>::Instance();
    auto config = configManager->getCurrentConfig();
    if (!config)
        return false;

    return config->vkCrashDiagnostic();
}