// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <string>
#include <fmt/core.h>
#include <fmt/xchar.h> // for wstring support
#include <toml.hpp>

#include "common/config.h"
#include "common/logging/formatter.h"
#include "common/path_util.h"
#include "common/scm_rev.h"

namespace toml {
template <typename TC, typename K>
std::filesystem::path find_fs_path_or(const basic_value<TC>& v, const K& ky,
                                      std::filesystem::path opt) {
    try {
        auto str = find<std::string>(v, ky);
        if (str.empty()) {
            return opt;
        }
        std::u8string u8str{(char8_t*)&str.front(), (char8_t*)&str.back() + 1};
        return std::filesystem::path{u8str};
    } catch (...) {
        return opt;
    }
}
} // namespace toml

namespace Config {

// General
static int volumeSlider = 100;
static bool isNeo = false;
static bool isDevKit = false;
static bool isPSNSignedIn = false;
static bool isTrophyPopupDisabled = false;
static double trophyNotificationDuration = 6.0;
static bool enableDiscordRPC = false;
static std::string logFilter = "";
static std::string logType = "sync";
static std::string userName = "shadPS4";
static std::string chooseHomeTab = "General";
static bool isShowSplash = false;
static std::string isSideTrophy = "right";
static bool compatibilityData = false;
static bool checkCompatibilityOnStartup = false;

// Input
static int cursorState = HideCursorState::Idle;
static int cursorHideTimeout = 5; // 5 seconds (default)
static bool useSpecialPad = false;
static int specialPadClass = 1;
static bool isMotionControlsEnabled = true;
static bool useUnifiedInputConfig = true;
static std::string micDevice = "Default Device";

// These two entries aren't stored in the config
static bool overrideControllerColor = false;
static int controllerCustomColorRGB[3] = {0, 0, 255};

// GPU
static u32 windowWidth = 1280;
static u32 windowHeight = 720;
static u32 internalScreenWidth = 1280;
static u32 internalScreenHeight = 720;
static bool isNullGpu = false;
static bool shouldCopyGPUBuffers = false;
static bool readbacksEnabled = false;
static bool readbackLinearImagesEnabled = false;
static bool directMemoryAccessEnabled = false;
static bool shouldDumpShaders = false;
static bool shouldPatchShaders = false;
static u32 vblankDivider = 1;
static bool isFullscreen = false;
static std::string fullscreenMode = "Windowed";
static bool isHDRAllowed = false;

// Vulkan
static s32 gpuId = -1;
static bool vkValidation = false;
static bool vkValidationSync = false;
static bool vkValidationGpu = false;
static bool vkCrashDiagnostic = false;
static bool vkHostMarkers = false;
static bool vkGuestMarkers = false;
static bool rdocEnable = false;

// Debug
static bool isDebugDump = false;
static bool isShaderDebug = false;
static bool isSeparateLogFilesEnabled = false;
static bool isFpsColor = true;

// GUI
static bool load_game_size = true;
static std::vector<GameInstallDir> settings_install_dirs = {};
std::vector<bool> install_dirs_enabled = {};
std::filesystem::path settings_addon_install_dir = {};
std::filesystem::path save_data_path = {};

// Settings
u32 m_language = 1; // english

// Keys
static std::string trophyKey = "";

// Config version, used to determine if a user's config file is outdated.
static std::string config_version = Common::g_scm_rev;

int getVolumeSlider() {
    return volumeSlider;
}
bool allowHDR() {
    return isHDRAllowed;
}

bool GetUseUnifiedInputConfig() {
    return useUnifiedInputConfig;
}

void SetUseUnifiedInputConfig(bool use) {
    useUnifiedInputConfig = use;
}

bool GetOverrideControllerColor() {
    return overrideControllerColor;
}

void SetOverrideControllerColor(bool enable) {
    overrideControllerColor = enable;
}

int* GetControllerCustomColor() {
    return controllerCustomColorRGB;
}

void SetControllerCustomColor(int r, int b, int g) {
    controllerCustomColorRGB[0] = r;
    controllerCustomColorRGB[1] = b;
    controllerCustomColorRGB[2] = g;
}

std::string getTrophyKey() {
    return trophyKey;
}

void setTrophyKey(std::string key) {
    trophyKey = key;
}

bool GetLoadGameSizeEnabled() {
    return load_game_size;
}

std::filesystem::path GetSaveDataPath() {
    if (save_data_path.empty()) {
        return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "savedata";
    }
    return save_data_path;
}

void setVolumeSlider(int volumeValue) {
    volumeSlider = volumeValue;
}

void setLoadGameSizeEnabled(bool enable) {
    load_game_size = enable;
}

bool isNeoModeConsole() {
    return isNeo;
}

bool isDevKitConsole() {
    return isDevKit;
}

bool getIsFullscreen() {
    return isFullscreen;
}

std::string getFullscreenMode() {
    return fullscreenMode;
}

bool getisTrophyPopupDisabled() {
    return isTrophyPopupDisabled;
}

bool getEnableDiscordRPC() {
    return enableDiscordRPC;
}

s16 getCursorState() {
    return cursorState;
}

int getCursorHideTimeout() {
    return cursorHideTimeout;
}

std::string getMicDevice() {
    return micDevice;
}

double getTrophyNotificationDuration() {
    return trophyNotificationDuration;
}

u32 getWindowWidth() {
    return windowWidth;
}

u32 getWindowHeight() {
    return windowHeight;
}

u32 getInternalScreenWidth() {
    return internalScreenHeight;
}

u32 getInternalScreenHeight() {
    return internalScreenHeight;
}

s32 getGpuId() {
    return gpuId;
}

std::string getLogFilter() {
    return logFilter;
}

std::string getLogType() {
    return logType;
}

std::string getUserName() {
    return userName;
}

std::string getChooseHomeTab() {
    return chooseHomeTab;
}

bool getUseSpecialPad() {
    return useSpecialPad;
}

int getSpecialPadClass() {
    return specialPadClass;
}

bool getIsMotionControlsEnabled() {
    return isMotionControlsEnabled;
}

bool debugDump() {
    return isDebugDump;
}

bool collectShadersForDebug() {
    return isShaderDebug;
}

bool showSplash() {
    return isShowSplash;
}

std::string sideTrophy() {
    return isSideTrophy;
}

bool nullGpu() {
    return isNullGpu;
}

bool copyGPUCmdBuffers() {
    return shouldCopyGPUBuffers;
}

bool readbacks() {
    return readbacksEnabled;
}

bool readbackLinearImages() {
    return readbackLinearImagesEnabled;
}

bool directMemoryAccess() {
    return directMemoryAccessEnabled;
}

bool dumpShaders() {
    return shouldDumpShaders;
}

bool patchShaders() {
    return shouldPatchShaders;
}

bool isRdocEnabled() {
    return rdocEnable;
}

bool fpsColor() {
    return isFpsColor;
}

u32 vblankDiv() {
    return vblankDivider;
}

bool vkValidationEnabled() {
    return vkValidation;
}

bool vkValidationSyncEnabled() {
    return vkValidationSync;
}

bool vkValidationGpuEnabled() {
    return vkValidationGpu;
}

bool getVkCrashDiagnosticEnabled() {
    return vkCrashDiagnostic;
}

bool getVkHostMarkersEnabled() {
    return vkHostMarkers;
}

bool getVkGuestMarkersEnabled() {
    return vkGuestMarkers;
}

void setVkCrashDiagnosticEnabled(bool enable) {
    vkCrashDiagnostic = enable;
}

void setVkHostMarkersEnabled(bool enable) {
    vkHostMarkers = enable;
}

void setVkGuestMarkersEnabled(bool enable) {
    vkGuestMarkers = enable;
}

bool getCompatibilityEnabled() {
    return compatibilityData;
}

bool getCheckCompatibilityOnStartup() {
    return checkCompatibilityOnStartup;
}

void setGpuId(s32 selectedGpuId) {
    gpuId = selectedGpuId;
}

void setWindowWidth(u32 width) {
    windowWidth = width;
}

void setWindowHeight(u32 height) {
    windowHeight = height;
}

void setInternalScreenWidth(u32 width) {
    internalScreenWidth = width;
}

void setInternalScreenHeight(u32 height) {
    internalScreenHeight = height;
}

void setDebugDump(bool enable) {
    isDebugDump = enable;
}

void setCollectShaderForDebug(bool enable) {
    isShaderDebug = enable;
}

void setShowSplash(bool enable) {
    isShowSplash = enable;
}

void setSideTrophy(std::string side) {
    isSideTrophy = side;
}

void setNullGpu(bool enable) {
    isNullGpu = enable;
}

void setAllowHDR(bool enable) {
    isHDRAllowed = enable;
}

void setCopyGPUCmdBuffers(bool enable) {
    shouldCopyGPUBuffers = enable;
}

void setReadbacks(bool enable) {
    readbacksEnabled = enable;
}

void setDirectMemoryAccess(bool enable) {
    directMemoryAccessEnabled = enable;
}

void setDumpShaders(bool enable) {
    shouldDumpShaders = enable;
}

void setVkValidation(bool enable) {
    vkValidation = enable;
}

void setVkSyncValidation(bool enable) {
    vkValidationSync = enable;
}

void setRdocEnabled(bool enable) {
    rdocEnable = enable;
}

void setVblankDiv(u32 value) {
    vblankDivider = value;
}

void setIsFullscreen(bool enable) {
    isFullscreen = enable;
}

void setFullscreenMode(std::string mode) {
    fullscreenMode = mode;
}

void setisTrophyPopupDisabled(bool disable) {
    isTrophyPopupDisabled = disable;
}

void setEnableDiscordRPC(bool enable) {
    enableDiscordRPC = enable;
}

void setCursorState(s16 newCursorState) {
    cursorState = newCursorState;
}

void setCursorHideTimeout(int newcursorHideTimeout) {
    cursorHideTimeout = newcursorHideTimeout;
}

void setMicDevice(std::string device) {
    micDevice = device;
}

void setTrophyNotificationDuration(double newTrophyNotificationDuration) {
    trophyNotificationDuration = newTrophyNotificationDuration;
}

void setLanguage(u32 language) {
    m_language = language;
}

void setNeoMode(bool enable) {
    isNeo = enable;
}

void setLogType(const std::string& type) {
    logType = type;
}

void setLogFilter(const std::string& type) {
    logFilter = type;
}

void setSeparateLogFilesEnabled(bool enabled) {
    isSeparateLogFilesEnabled = enabled;
}

void setUserName(const std::string& type) {
    userName = type;
}

void setChooseHomeTab(const std::string& type) {
    chooseHomeTab = type;
}

void setUseSpecialPad(bool use) {
    useSpecialPad = use;
}

void setSpecialPadClass(int type) {
    specialPadClass = type;
}

void setIsMotionControlsEnabled(bool use) {
    isMotionControlsEnabled = use;
}

void setCompatibilityEnabled(bool use) {
    compatibilityData = use;
}

void setCheckCompatibilityOnStartup(bool use) {
    checkCompatibilityOnStartup = use;
}

bool addGameInstallDir(const std::filesystem::path& dir, bool enabled) {
    for (const auto& install_dir : settings_install_dirs) {
        if (install_dir.path == dir) {
            return false;
        }
    }
    settings_install_dirs.push_back({dir, enabled});
    return true;
}

void removeGameInstallDir(const std::filesystem::path& dir) {
    auto iterator =
        std::find_if(settings_install_dirs.begin(), settings_install_dirs.end(),
                     [&dir](const GameInstallDir& install_dir) { return install_dir.path == dir; });
    if (iterator != settings_install_dirs.end()) {
        settings_install_dirs.erase(iterator);
    }
}

void setGameInstallDirEnabled(const std::filesystem::path& dir, bool enabled) {
    auto iterator =
        std::find_if(settings_install_dirs.begin(), settings_install_dirs.end(),
                     [&dir](const GameInstallDir& install_dir) { return install_dir.path == dir; });
    if (iterator != settings_install_dirs.end()) {
        iterator->enabled = enabled;
    }
}

void setAddonInstallDir(const std::filesystem::path& dir) {
    settings_addon_install_dir = dir;
}

void setGameInstallDirs(const std::vector<std::filesystem::path>& dirs_config) {
    settings_install_dirs.clear();
    for (const auto& dir : dirs_config) {
        settings_install_dirs.push_back({dir, true});
    }
}

void setAllGameInstallDirs(const std::vector<GameInstallDir>& dirs_config) {
    settings_install_dirs = dirs_config;
}

void setSaveDataPath(const std::filesystem::path& path) {
    save_data_path = path;
}

const std::vector<std::filesystem::path> getGameInstallDirs() {
    std::vector<std::filesystem::path> enabled_dirs;
    for (const auto& dir : settings_install_dirs) {
        if (dir.enabled) {
            enabled_dirs.push_back(dir.path);
        }
    }
    return enabled_dirs;
}

const std::vector<bool> getGameInstallDirsEnabled() {
    std::vector<bool> enabled_dirs;
    for (const auto& dir : settings_install_dirs) {
        enabled_dirs.push_back(dir.enabled);
    }
    return enabled_dirs;
}

std::filesystem::path getAddonInstallDir() {
    if (settings_addon_install_dir.empty()) {
        // Default for users without a config file or a config file from before this option existed
        return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "addcont";
    }
    return settings_addon_install_dir;
}

u32 GetLanguage() {
    return m_language;
}

bool getSeparateLogFilesEnabled() {
    return isSeparateLogFilesEnabled;
}

bool getPSNSignedIn() {
    return isPSNSignedIn;
}

void setPSNSignedIn(bool sign) {
    isPSNSignedIn = sign;
}

void load(const std::filesystem::path& path) {
    // If the configuration file does not exist, create it and return
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        save(path);
        return;
    }

    toml::value data;

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifs.open(path, std::ios_base::binary);
        data = toml::parse(ifs, std::string{fmt::UTF(path.filename().u8string()).data});
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    if (data.contains("General")) {
        const toml::value& general = data.at("General");

        volumeSlider = toml::find_or<int>(general, "volumeSlider", volumeSlider);
        isNeo = toml::find_or<bool>(general, "isPS4Pro", isNeo);
        isDevKit = toml::find_or<bool>(general, "isDevKit", isDevKit);
        isPSNSignedIn = toml::find_or<bool>(general, "isPSNSignedIn", isPSNSignedIn);
        isTrophyPopupDisabled =
            toml::find_or<bool>(general, "isTrophyPopupDisabled", isTrophyPopupDisabled);
        trophyNotificationDuration = toml::find_or<double>(general, "trophyNotificationDuration",
                                                           trophyNotificationDuration);
        enableDiscordRPC = toml::find_or<bool>(general, "enableDiscordRPC", enableDiscordRPC);
        logFilter = toml::find_or<std::string>(general, "logFilter", logFilter);
        logType = toml::find_or<std::string>(general, "logType", logType);
        userName = toml::find_or<std::string>(general, "userName", userName);
        isShowSplash = toml::find_or<bool>(general, "showSplash", isShowSplash);
        isSideTrophy = toml::find_or<std::string>(general, "sideTrophy", isSideTrophy);
        compatibilityData = toml::find_or<bool>(general, "compatibilityEnabled", compatibilityData);
        checkCompatibilityOnStartup = toml::find_or<bool>(general, "checkCompatibilityOnStartup",
                                                          checkCompatibilityOnStartup);
        chooseHomeTab = toml::find_or<std::string>(general, "chooseHomeTab", chooseHomeTab);
    }

    if (data.contains("Input")) {
        const toml::value& input = data.at("Input");

        cursorState = toml::find_or<int>(input, "cursorState", cursorState);
        cursorHideTimeout = toml::find_or<int>(input, "cursorHideTimeout", cursorHideTimeout);
        useSpecialPad = toml::find_or<bool>(input, "useSpecialPad", useSpecialPad);
        specialPadClass = toml::find_or<int>(input, "specialPadClass", specialPadClass);
        isMotionControlsEnabled =
            toml::find_or<bool>(input, "isMotionControlsEnabled", isMotionControlsEnabled);
        useUnifiedInputConfig =
            toml::find_or<bool>(input, "useUnifiedInputConfig", useUnifiedInputConfig);
        micDevice = toml::find_or<std::string>(input, "micDevice", micDevice);
    }

    if (data.contains("GPU")) {
        const toml::value& gpu = data.at("GPU");

        windowWidth = toml::find_or<int>(gpu, "screenWidth", windowWidth);
        windowHeight = toml::find_or<int>(gpu, "screenHeight", windowHeight);
        internalScreenWidth = toml::find_or<int>(gpu, "internalScreenWidth", internalScreenWidth);
        internalScreenHeight =
            toml::find_or<int>(gpu, "internalScreenHeight", internalScreenHeight);
        isNullGpu = toml::find_or<bool>(gpu, "nullGpu", isNullGpu);
        shouldCopyGPUBuffers = toml::find_or<bool>(gpu, "copyGPUBuffers", shouldCopyGPUBuffers);
        readbacksEnabled = toml::find_or<bool>(gpu, "readbacks", readbacksEnabled);
        readbackLinearImagesEnabled =
            toml::find_or<bool>(gpu, "readbackLinearImages", readbackLinearImagesEnabled);
        directMemoryAccessEnabled =
            toml::find_or<bool>(gpu, "directMemoryAccess", directMemoryAccessEnabled);
        shouldDumpShaders = toml::find_or<bool>(gpu, "dumpShaders", shouldDumpShaders);
        shouldPatchShaders = toml::find_or<bool>(gpu, "patchShaders", shouldPatchShaders);
        vblankDivider = toml::find_or<int>(gpu, "vblankDivider", vblankDivider);
        isFullscreen = toml::find_or<bool>(gpu, "Fullscreen", isFullscreen);
        fullscreenMode = toml::find_or<std::string>(gpu, "FullscreenMode", fullscreenMode);
        isHDRAllowed = toml::find_or<bool>(gpu, "allowHDR", isHDRAllowed);
    }

    if (data.contains("Vulkan")) {
        const toml::value& vk = data.at("Vulkan");

        gpuId = toml::find_or<int>(vk, "gpuId", gpuId);
        vkValidation = toml::find_or<bool>(vk, "validation", vkValidation);
        vkValidationSync = toml::find_or<bool>(vk, "validation_sync", vkValidationSync);
        vkValidationGpu = toml::find_or<bool>(vk, "validation_gpu", vkValidationGpu);
        vkCrashDiagnostic = toml::find_or<bool>(vk, "crashDiagnostic", vkCrashDiagnostic);
        vkHostMarkers = toml::find_or<bool>(vk, "hostMarkers", vkHostMarkers);
        vkGuestMarkers = toml::find_or<bool>(vk, "guestMarkers", vkGuestMarkers);
        rdocEnable = toml::find_or<bool>(vk, "rdocEnable", rdocEnable);
    }

    std::string current_version = {};
    if (data.contains("Debug")) {
        const toml::value& debug = data.at("Debug");

        isDebugDump = toml::find_or<bool>(debug, "DebugDump", isDebugDump);
        isSeparateLogFilesEnabled =
            toml::find_or<bool>(debug, "isSeparateLogFilesEnabled", isSeparateLogFilesEnabled);
        isShaderDebug = toml::find_or<bool>(debug, "CollectShader", isShaderDebug);
        isFpsColor = toml::find_or<bool>(debug, "FPSColor", isFpsColor);
        current_version = toml::find_or<std::string>(debug, "ConfigVersion", current_version);
    }

    if (data.contains("GUI")) {
        const toml::value& gui = data.at("GUI");

        load_game_size = toml::find_or<bool>(gui, "loadGameSizeEnabled", load_game_size);

        const auto install_dir_array =
            toml::find_or<std::vector<std::u8string>>(gui, "installDirs", {});

        try {
            install_dirs_enabled = toml::find<std::vector<bool>>(gui, "installDirsEnabled");
        } catch (...) {
            // If it does not exist, assume that all are enabled.
            install_dirs_enabled.resize(install_dir_array.size(), true);
        }

        if (install_dirs_enabled.size() < install_dir_array.size()) {
            install_dirs_enabled.resize(install_dir_array.size(), true);
        }

        settings_install_dirs.clear();
        for (size_t i = 0; i < install_dir_array.size(); i++) {
            settings_install_dirs.push_back(
                {std::filesystem::path{install_dir_array[i]}, install_dirs_enabled[i]});
        }

        save_data_path = toml::find_fs_path_or(gui, "saveDataPath", save_data_path);

        settings_addon_install_dir =
            toml::find_fs_path_or(gui, "addonInstallDir", settings_addon_install_dir);
    }

    if (data.contains("Settings")) {
        const toml::value& settings = data.at("Settings");
        m_language = toml::find_or<int>(settings, "consoleLanguage", m_language);
    }

    if (data.contains("Keys")) {
        const toml::value& keys = data.at("Keys");
        trophyKey = toml::find_or<std::string>(keys, "TrophyKey", trophyKey);
    }

    // Run save after loading to generate any missing fields with default values.
    if (config_version != current_version) {
        fmt::print("Outdated config detected, updating config file.\n");
        save(path);
    }
}

void sortTomlSections(toml::ordered_value& data) {
    toml::ordered_value ordered_data;
    std::vector<std::string> section_order = {"General", "Input", "GPU", "Vulkan",
                                              "Debug",   "Keys",  "GUI", "Settings"};

    for (const auto& section : section_order) {
        if (data.contains(section)) {
            std::vector<std::string> keys;
            for (const auto& item : data.at(section).as_table()) {
                keys.push_back(item.first);
            }

            std::sort(keys.begin(), keys.end(), [](const std::string& a, const std::string& b) {
                return std::lexicographical_compare(
                    a.begin(), a.end(), b.begin(), b.end(), [](char a_char, char b_char) {
                        return std::tolower(a_char) < std::tolower(b_char);
                    });
            });

            toml::ordered_value ordered_section;
            for (const auto& key : keys) {
                ordered_section[key] = data.at(section).at(key);
            }

            ordered_data[section] = ordered_section;
        }
    }

    data = ordered_data;
}

void save(const std::filesystem::path& path) {
    toml::ordered_value data;

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        try {
            std::ifstream ifs;
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifs.open(path, std::ios_base::binary);
            data = toml::parse<toml::ordered_type_config>(
                ifs, std::string{fmt::UTF(path.filename().u8string()).data});
        } catch (const std::exception& ex) {
            fmt::print("Exception trying to parse config file. Exception: {}\n", ex.what());
            return;
        }
    } else {
        if (error) {
            fmt::print("Filesystem error: {}\n", error.message());
        }
        fmt::print("Saving new configuration file {}\n", fmt::UTF(path.u8string()));
    }

    data["General"]["volumeSlider"] = volumeSlider;
    data["General"]["isPS4Pro"] = isNeo;
    data["General"]["isDevKit"] = isDevKit;
    data["General"]["isPSNSignedIn"] = isPSNSignedIn;
    data["General"]["isTrophyPopupDisabled"] = isTrophyPopupDisabled;
    data["General"]["trophyNotificationDuration"] = trophyNotificationDuration;
    data["General"]["enableDiscordRPC"] = enableDiscordRPC;
    data["General"]["logFilter"] = logFilter;
    data["General"]["logType"] = logType;
    data["General"]["userName"] = userName;
    data["General"]["chooseHomeTab"] = chooseHomeTab;
    data["General"]["showSplash"] = isShowSplash;
    data["General"]["sideTrophy"] = isSideTrophy;
    data["General"]["compatibilityEnabled"] = compatibilityData;
    data["General"]["checkCompatibilityOnStartup"] = checkCompatibilityOnStartup;
    data["Input"]["cursorState"] = cursorState;
    data["Input"]["cursorHideTimeout"] = cursorHideTimeout;
    data["Input"]["useSpecialPad"] = useSpecialPad;
    data["Input"]["specialPadClass"] = specialPadClass;
    data["Input"]["isMotionControlsEnabled"] = isMotionControlsEnabled;
    data["Input"]["useUnifiedInputConfig"] = useUnifiedInputConfig;
    data["Input"]["micDevice"] = micDevice;
    data["GPU"]["screenWidth"] = windowWidth;
    data["GPU"]["screenHeight"] = windowHeight;
    data["GPU"]["internalScreenWidth"] = internalScreenWidth;
    data["GPU"]["internalScreenHeight"] = internalScreenHeight;
    data["GPU"]["nullGpu"] = isNullGpu;
    data["GPU"]["copyGPUBuffers"] = shouldCopyGPUBuffers;
    data["GPU"]["readbacks"] = readbacksEnabled;
    data["GPU"]["readbackLinearImages"] = readbackLinearImagesEnabled;
    data["GPU"]["directMemoryAccess"] = directMemoryAccessEnabled;
    data["GPU"]["dumpShaders"] = shouldDumpShaders;
    data["GPU"]["patchShaders"] = shouldPatchShaders;
    data["GPU"]["vblankDivider"] = vblankDivider;
    data["GPU"]["Fullscreen"] = isFullscreen;
    data["GPU"]["FullscreenMode"] = fullscreenMode;
    data["GPU"]["allowHDR"] = isHDRAllowed;
    data["Vulkan"]["gpuId"] = gpuId;
    data["Vulkan"]["validation"] = vkValidation;
    data["Vulkan"]["validation_sync"] = vkValidationSync;
    data["Vulkan"]["validation_gpu"] = vkValidationGpu;
    data["Vulkan"]["crashDiagnostic"] = vkCrashDiagnostic;
    data["Vulkan"]["hostMarkers"] = vkHostMarkers;
    data["Vulkan"]["guestMarkers"] = vkGuestMarkers;
    data["Vulkan"]["rdocEnable"] = rdocEnable;
    data["Debug"]["DebugDump"] = isDebugDump;
    data["Debug"]["CollectShader"] = isShaderDebug;
    data["Debug"]["isSeparateLogFilesEnabled"] = isSeparateLogFilesEnabled;
    data["Debug"]["FPSColor"] = isFpsColor;
    data["Debug"]["ConfigVersion"] = config_version;
    data["Keys"]["TrophyKey"] = trophyKey;

    std::vector<std::string> install_dirs;
    std::vector<bool> install_dirs_enabled;

    // temporary structure for ordering
    struct DirEntry {
        std::string path_str;
        bool enabled;
    };

    std::vector<DirEntry> sorted_dirs;
    for (const auto& dirInfo : settings_install_dirs) {
        sorted_dirs.push_back(
            {std::string{fmt::UTF(dirInfo.path.u8string()).data}, dirInfo.enabled});
    }

    // Sort directories alphabetically
    std::sort(sorted_dirs.begin(), sorted_dirs.end(), [](const DirEntry& a, const DirEntry& b) {
        return std::lexicographical_compare(
            a.path_str.begin(), a.path_str.end(), b.path_str.begin(), b.path_str.end(),
            [](char a_char, char b_char) { return std::tolower(a_char) < std::tolower(b_char); });
    });

    for (const auto& entry : sorted_dirs) {
        install_dirs.push_back(entry.path_str);
        install_dirs_enabled.push_back(entry.enabled);
    }

    data["GUI"]["installDirs"] = install_dirs;
    data["GUI"]["installDirsEnabled"] = install_dirs_enabled;
    data["GUI"]["saveDataPath"] = std::string{fmt::UTF(save_data_path.u8string()).data};
    data["GUI"]["loadGameSizeEnabled"] = load_game_size;

    data["GUI"]["addonInstallDir"] =
        std::string{fmt::UTF(settings_addon_install_dir.u8string()).data};
    data["Settings"]["consoleLanguage"] = m_language;

    // Sorting of TOML sections
    sortTomlSections(data);

    std::ofstream file(path, std::ios::binary);
    file << data;
    file.close();
}

void setDefaultValues() {
    // General
    volumeSlider = 100;
    isNeo = false;
    isDevKit = false;
    isPSNSignedIn = false;
    isTrophyPopupDisabled = false;
    trophyNotificationDuration = 6.0;
    enableDiscordRPC = false;
    logFilter = "";
    logType = "sync";
    userName = "shadPS4";
    chooseHomeTab = "General";
    isShowSplash = false;
    isSideTrophy = "right";
    compatibilityData = false;
    checkCompatibilityOnStartup = false;

    // Input
    cursorState = HideCursorState::Idle;
    cursorHideTimeout = 5;
    useSpecialPad = false;
    specialPadClass = 1;
    isMotionControlsEnabled = true;
    useUnifiedInputConfig = true;
    overrideControllerColor = false;
    controllerCustomColorRGB[0] = 0;
    controllerCustomColorRGB[1] = 0;
    controllerCustomColorRGB[2] = 255;
    micDevice = "Default Device";

    // GPU
    windowWidth = 1280;
    windowHeight = 720;
    internalScreenWidth = 1280;
    internalScreenHeight = 720;
    isNullGpu = false;
    shouldCopyGPUBuffers = false;
    readbacksEnabled = false;
    readbackLinearImagesEnabled = false;
    directMemoryAccessEnabled = false;
    shouldDumpShaders = false;
    shouldPatchShaders = false;
    vblankDivider = 1;
    isFullscreen = false;
    fullscreenMode = "Windowed";
    isHDRAllowed = false;

    // Vulkan
    gpuId = -1;
    vkValidation = false;
    vkValidationSync = false;
    vkValidationGpu = false;
    vkCrashDiagnostic = false;
    vkHostMarkers = false;
    vkGuestMarkers = false;
    rdocEnable = false;

    // Debug
    isDebugDump = false;
    isShaderDebug = false;
    isSeparateLogFilesEnabled = false;
    isFpsColor = true;

    // GUI
    load_game_size = true;

    // Settings
    m_language = 1;
}

constexpr std::string_view GetDefaultKeyboardConfig() {
    return R"(#Feeling lost? Check out the Help section!

# Keyboard bindings

triangle = kp8
circle = kp6
cross = kp2
square = kp4
# Alternatives for users without a keypad
triangle = c
circle = b
cross = n
square = v

l1 = q
r1 = u
l2 = e
r2 = o
l3 = x
r3 = m

options = enter
touchpad_center = space

pad_up = up
pad_down = down
pad_left = left
pad_right = right

axis_left_x_minus = a
axis_left_x_plus = d
axis_left_y_minus = w
axis_left_y_plus = s

axis_right_x_minus = j
axis_right_x_plus = l
axis_right_y_minus = i
axis_right_y_plus = k

# Controller bindings

triangle = triangle
cross = cross
square = square
circle = circle

l1 = l1
l2 = l2
l3 = l3
r1 = r1
r2 = r2
r3 = r3

options = options
touchpad_center = back

pad_up = pad_up
pad_down = pad_down
pad_left = pad_left
pad_right = pad_right

axis_left_x = axis_left_x
axis_left_y = axis_left_y
axis_right_x = axis_right_x
axis_right_y = axis_right_y

# Range of deadzones: 1 (almost none) to 127 (max)
analog_deadzone = leftjoystick, 2, 127
analog_deadzone = rightjoystick, 2, 127

override_controller_color = false, 0, 0, 255
)";
}
std::filesystem::path GetFoolproofKbmConfigFile(const std::string& game_id) {
    // Read configuration file of the game, and if it doesn't exist, generate it from default
    // If that doesn't exist either, generate that from getDefaultConfig() and try again
    // If even the folder is missing, we start with that.

    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "input_config";
    const auto config_file = config_dir / (game_id + ".ini");
    const auto default_config_file = config_dir / "default.ini";

    // Ensure the config directory exists
    if (!std::filesystem::exists(config_dir)) {
        std::filesystem::create_directories(config_dir);
    }

    // Check if the default config exists
    if (!std::filesystem::exists(default_config_file)) {
        // If the default config is also missing, create it from getDefaultConfig()
        const auto default_config = GetDefaultKeyboardConfig();
        std::ofstream default_config_stream(default_config_file);
        if (default_config_stream) {
            default_config_stream << default_config;
        }
    }

    // if empty, we only need to execute the function up until this point
    if (game_id.empty()) {
        return default_config_file;
    }

    // If game-specific config doesn't exist, create it from the default config
    if (!std::filesystem::exists(config_file)) {
        std::filesystem::copy(default_config_file, config_file);
    }
    return config_file;
}

} // namespace Config
