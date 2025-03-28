// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <string>
#include <fmt/core.h>
#include <fmt/xchar.h> // for wstring support
#include <toml.hpp>

#include "common/path_util.h"
#include "config.h"
#include "logging/formatter.h"
#include "version.h"

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

static bool isNeo = false;
static bool isDevKit = false;
static bool playBGM = false;
static bool isTrophyPopupDisabled = false;
static int BGMvolume = 50;
static bool enableDiscordRPC = false;
static u32 screenWidth = 1280;
static u32 screenHeight = 720;
static s32 gpuId = -1; // Vulkan physical device index. Set to negative for auto select
static std::string logFilter;
static std::string logType = "sync";
static std::string userName = "shadPS4";
static std::string updateChannel;
static std::string chooseHomeTab;
static std::string backButtonBehavior = "left";
static bool useSpecialPad = false;
static int specialPadClass = 1;
static bool isMotionControlsEnabled = true;
static bool isDebugDump = false;
static bool isShaderDebug = false;
static bool isShowSplash = false;
static bool isAutoUpdate = false;
static bool isAlwaysShowChangelog = false;
static std::string isSideTrophy = "right";
static bool isNullGpu = false;
static bool shouldCopyGPUBuffers = false;
static bool shouldDumpShaders = false;
static bool shouldPatchShaders = true;
static u32 vblankDivider = 1;
static bool vkValidation = false;
static bool vkValidationSync = false;
static bool vkValidationGpu = false;
static bool vkCrashDiagnostic = false;
static bool vkHostMarkers = false;
static bool vkGuestMarkers = false;
static bool rdocEnable = false;
static bool isFpsColor = true;
static bool isSeparateLogFilesEnabled = false;
static s16 cursorState = HideCursorState::Idle;
static int cursorHideTimeout = 5; // 5 seconds (default)
static double trophyNotificationDuration = 6.0;
static bool useUnifiedInputConfig = true;
static bool overrideControllerColor = false;
static int controllerCustomColorRGB[3] = {0, 0, 255};
static bool separateupdatefolder = false;
static bool compatibilityData = false;
static bool checkCompatibilityOnStartup = false;
static std::string trophyKey;

// Gui
static bool load_game_size = true;
static std::vector<GameInstallDir> settings_install_dirs = {};
std::vector<bool> install_dirs_enabled = {};
std::filesystem::path settings_addon_install_dir = {};
std::filesystem::path save_data_path = {};
u32 main_window_geometry_x = 400;
u32 main_window_geometry_y = 400;
u32 main_window_geometry_w = 1280;
u32 main_window_geometry_h = 720;
u32 mw_themes = 0;
u32 m_icon_size = 36;
u32 m_icon_size_grid = 69;
u32 m_slider_pos = 0;
u32 m_slider_pos_grid = 0;
u32 m_table_mode = 0;
u32 m_window_size_W = 1280;
u32 m_window_size_H = 720;
std::vector<std::string> m_pkg_viewer;
std::vector<std::string> m_elf_viewer;
std::vector<std::string> m_recent_files;
std::string emulator_language = "en_US";
static int backgroundImageOpacity = 50;
static bool showBackgroundImage = true;
static bool isFullscreen = false;
static std::string fullscreenMode = "Windowed";
static bool isHDRAllowed = false;
static bool showLabelsUnderIcons = true;

// Language
u32 m_language = 1; // english

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
        return Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir);
    }
    return save_data_path;
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

bool getShowLabelsUnderIcons() {
    return showLabelsUnderIcons;
}

bool setShowLabelsUnderIcons() {
    return false;
}

std::string getFullscreenMode() {
    return fullscreenMode;
}

bool getisTrophyPopupDisabled() {
    return isTrophyPopupDisabled;
}

bool getPlayBGM() {
    return playBGM;
}

int getBGMvolume() {
    return BGMvolume;
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

double getTrophyNotificationDuration() {
    return trophyNotificationDuration;
}

u32 getScreenWidth() {
    return screenWidth;
}

u32 getScreenHeight() {
    return screenHeight;
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

std::string getUpdateChannel() {
    return updateChannel;
}

std::string getChooseHomeTab() {
    return chooseHomeTab;
}

std::string getBackButtonBehavior() {
    return backButtonBehavior;
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

bool autoUpdate() {
    return isAutoUpdate;
}

bool alwaysShowChangelog() {
    return isAlwaysShowChangelog;
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

bool getSeparateUpdateEnabled() {
    return separateupdatefolder;
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

void setScreenWidth(u32 width) {
    screenWidth = width;
}

void setScreenHeight(u32 height) {
    screenHeight = height;
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

void setAutoUpdate(bool enable) {
    isAutoUpdate = enable;
}

void setAlwaysShowChangelog(bool enable) {
    isAlwaysShowChangelog = enable;
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
static void setShowLabelsUnderIcons(bool enable) {
    showLabelsUnderIcons = enable;
}

void setFullscreenMode(std::string mode) {
    fullscreenMode = mode;
}

void setisTrophyPopupDisabled(bool disable) {
    isTrophyPopupDisabled = disable;
}

void setPlayBGM(bool enable) {
    playBGM = enable;
}

void setBGMvolume(int volume) {
    BGMvolume = volume;
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

void setUpdateChannel(const std::string& type) {
    updateChannel = type;
}
void setChooseHomeTab(const std::string& type) {
    chooseHomeTab = type;
}

void setBackButtonBehavior(const std::string& type) {
    backButtonBehavior = type;
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

void setSeparateUpdateEnabled(bool use) {
    separateupdatefolder = use;
}

void setCompatibilityEnabled(bool use) {
    compatibilityData = use;
}

void setCheckCompatibilityOnStartup(bool use) {
    checkCompatibilityOnStartup = use;
}

void setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h) {
    main_window_geometry_x = x;
    main_window_geometry_y = y;
    main_window_geometry_w = w;
    main_window_geometry_h = h;
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

void setMainWindowTheme(u32 theme) {
    mw_themes = theme;
}

void setIconSize(u32 size) {
    m_icon_size = size;
}

void setIconSizeGrid(u32 size) {
    m_icon_size_grid = size;
}

void setSliderPosition(u32 pos) {
    m_slider_pos = pos;
}

void setSliderPositionGrid(u32 pos) {
    m_slider_pos_grid = pos;
}

void setTableMode(u32 mode) {
    m_table_mode = mode;
}

void setMainWindowWidth(u32 width) {
    m_window_size_W = width;
}

void setMainWindowHeight(u32 height) {
    m_window_size_H = height;
}

void setPkgViewer(const std::vector<std::string>& pkgList) {
    m_pkg_viewer.resize(pkgList.size());
    m_pkg_viewer = pkgList;
}

void setElfViewer(const std::vector<std::string>& elfList) {
    m_elf_viewer.resize(elfList.size());
    m_elf_viewer = elfList;
}

void setRecentFiles(const std::vector<std::string>& recentFiles) {
    m_recent_files.resize(recentFiles.size());
    m_recent_files = recentFiles;
}

void setEmulatorLanguage(std::string language) {
    emulator_language = language;
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

u32 getMainWindowGeometryX() {
    return main_window_geometry_x;
}

u32 getMainWindowGeometryY() {
    return main_window_geometry_y;
}

u32 getMainWindowGeometryW() {
    return main_window_geometry_w;
}

u32 getMainWindowGeometryH() {
    return main_window_geometry_h;
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

u32 getMainWindowTheme() {
    return mw_themes;
}

u32 getIconSize() {
    return m_icon_size;
}

u32 getIconSizeGrid() {
    return m_icon_size_grid;
}

u32 getSliderPosition() {
    return m_slider_pos;
}

u32 getSliderPositionGrid() {
    return m_slider_pos_grid;
}

u32 getTableMode() {
    return m_table_mode;
}

u32 getMainWindowWidth() {
    return m_window_size_W;
}

u32 getMainWindowHeight() {
    return m_window_size_H;
}

std::vector<std::string> getPkgViewer() {
    return m_pkg_viewer;
}

std::vector<std::string> getElfViewer() {
    return m_elf_viewer;
}

std::vector<std::string> getRecentFiles() {
    return m_recent_files;
}

std::string getEmulatorLanguage() {
    return emulator_language;
}

u32 GetLanguage() {
    return m_language;
}

bool getSeparateLogFilesEnabled() {
    return isSeparateLogFilesEnabled;
}

int getBackgroundImageOpacity() {
    return backgroundImageOpacity;
}

void setBackgroundImageOpacity(int opacity) {
    backgroundImageOpacity = std::clamp(opacity, 0, 100);
}

bool getShowBackgroundImage() {
    return showBackgroundImage;
}

void setShowBackgroundImage(bool show) {
    showBackgroundImage = show;
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

        isNeo = toml::find_or<bool>(general, "isPS4Pro", false);
        isDevKit = toml::find_or<bool>(general, "isDevKit", false);
        playBGM = toml::find_or<bool>(general, "playBGM", false);
        isTrophyPopupDisabled = toml::find_or<bool>(general, "isTrophyPopupDisabled", false);
        trophyNotificationDuration =
            toml::find_or<double>(general, "trophyNotificationDuration", 5.0);
        BGMvolume = toml::find_or<int>(general, "BGMvolume", 50);
        enableDiscordRPC = toml::find_or<bool>(general, "enableDiscordRPC", true);
        logFilter = toml::find_or<std::string>(general, "logFilter", "");
        logType = toml::find_or<std::string>(general, "logType", "sync");
        userName = toml::find_or<std::string>(general, "userName", "shadPS4");
        if (Common::isRelease) {
            updateChannel = toml::find_or<std::string>(general, "updateChannel", "Release");
        } else {
            updateChannel = toml::find_or<std::string>(general, "updateChannel", "Nightly");
        }
        isShowSplash = toml::find_or<bool>(general, "showSplash", true);
        isAutoUpdate = toml::find_or<bool>(general, "autoUpdate", false);
        isAlwaysShowChangelog = toml::find_or<bool>(general, "alwaysShowChangelog", false);
        isSideTrophy = toml::find_or<std::string>(general, "sideTrophy", "right");
        separateupdatefolder = toml::find_or<bool>(general, "separateUpdateEnabled", false);
        compatibilityData = toml::find_or<bool>(general, "compatibilityEnabled", false);
        checkCompatibilityOnStartup =
            toml::find_or<bool>(general, "checkCompatibilityOnStartup", false);
        chooseHomeTab = toml::find_or<std::string>(general, "chooseHomeTab", "Release");
    }

    if (data.contains("Input")) {
        const toml::value& input = data.at("Input");

        cursorState = toml::find_or<int>(input, "cursorState", HideCursorState::Idle);
        cursorHideTimeout = toml::find_or<int>(input, "cursorHideTimeout", 5);
        backButtonBehavior = toml::find_or<std::string>(input, "backButtonBehavior", "left");
        useSpecialPad = toml::find_or<bool>(input, "useSpecialPad", false);
        specialPadClass = toml::find_or<int>(input, "specialPadClass", 1);
        isMotionControlsEnabled = toml::find_or<bool>(input, "isMotionControlsEnabled", true);
        useUnifiedInputConfig = toml::find_or<bool>(input, "useUnifiedInputConfig", true);
    }

    if (data.contains("GPU")) {
        const toml::value& gpu = data.at("GPU");

        screenWidth = toml::find_or<int>(gpu, "screenWidth", screenWidth);
        screenHeight = toml::find_or<int>(gpu, "screenHeight", screenHeight);
        isNullGpu = toml::find_or<bool>(gpu, "nullGpu", false);
        shouldCopyGPUBuffers = toml::find_or<bool>(gpu, "copyGPUBuffers", false);
        shouldDumpShaders = toml::find_or<bool>(gpu, "dumpShaders", false);
        shouldPatchShaders = toml::find_or<bool>(gpu, "patchShaders", true);
        vblankDivider = toml::find_or<int>(gpu, "vblankDivider", 1);
        isFullscreen = toml::find_or<bool>(gpu, "Fullscreen", false);
        fullscreenMode = toml::find_or<std::string>(gpu, "FullscreenMode", "Windowed");
        isHDRAllowed = toml::find_or<bool>(gpu, "allowHDR", false);
    }

    if (data.contains("Vulkan")) {
        const toml::value& vk = data.at("Vulkan");

        gpuId = toml::find_or<int>(vk, "gpuId", -1);
        vkValidation = toml::find_or<bool>(vk, "validation", false);
        vkValidationSync = toml::find_or<bool>(vk, "validation_sync", false);
        vkValidationGpu = toml::find_or<bool>(vk, "validation_gpu", true);
        vkCrashDiagnostic = toml::find_or<bool>(vk, "crashDiagnostic", false);
        vkHostMarkers = toml::find_or<bool>(vk, "hostMarkers", false);
        vkGuestMarkers = toml::find_or<bool>(vk, "guestMarkers", false);
        rdocEnable = toml::find_or<bool>(vk, "rdocEnable", false);
    }

    if (data.contains("Debug")) {
        const toml::value& debug = data.at("Debug");

        isDebugDump = toml::find_or<bool>(debug, "DebugDump", false);
        isSeparateLogFilesEnabled = toml::find_or<bool>(debug, "isSeparateLogFilesEnabled", false);
        isShaderDebug = toml::find_or<bool>(debug, "CollectShader", false);
        isFpsColor = toml::find_or<bool>(debug, "FPSColor", true);
    }

    if (data.contains("GUI")) {
        const toml::value& gui = data.at("GUI");

        load_game_size = toml::find_or<bool>(gui, "loadGameSizeEnabled", true);
        m_icon_size = toml::find_or<int>(gui, "iconSize", 0);
        m_icon_size_grid = toml::find_or<int>(gui, "iconSizeGrid", 0);
        m_slider_pos = toml::find_or<int>(gui, "sliderPos", 0);
        m_slider_pos_grid = toml::find_or<int>(gui, "sliderPosGrid", 0);
        mw_themes = toml::find_or<int>(gui, "theme", 0);
        m_window_size_W = toml::find_or<int>(gui, "mw_width", 0);
        m_window_size_H = toml::find_or<int>(gui, "mw_height", 0);

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

        save_data_path = toml::find_fs_path_or(gui, "saveDataPath", {});

        settings_addon_install_dir = toml::find_fs_path_or(gui, "addonInstallDir", {});
        main_window_geometry_x = toml::find_or<int>(gui, "geometry_x", 0);
        main_window_geometry_y = toml::find_or<int>(gui, "geometry_y", 0);
        main_window_geometry_w = toml::find_or<int>(gui, "geometry_w", 0);
        main_window_geometry_h = toml::find_or<int>(gui, "geometry_h", 0);
        m_pkg_viewer = toml::find_or<std::vector<std::string>>(gui, "pkgDirs", {});
        m_elf_viewer = toml::find_or<std::vector<std::string>>(gui, "elfDirs", {});
        m_recent_files = toml::find_or<std::vector<std::string>>(gui, "recentFiles", {});
        m_table_mode = toml::find_or<int>(gui, "gameTableMode", 0);
        emulator_language = toml::find_or<std::string>(gui, "emulatorLanguage", "en_US");
        backgroundImageOpacity = toml::find_or<int>(gui, "backgroundImageOpacity", 50);
        showBackgroundImage = toml::find_or<bool>(gui, "showBackgroundImage", true);
    }

    if (data.contains("Settings")) {
        const toml::value& settings = data.at("Settings");

        m_language = toml::find_or<int>(settings, "consoleLanguage", 1);
    }

    if (data.contains("Keys")) {
        const toml::value& keys = data.at("Keys");
        trophyKey = toml::find_or<std::string>(keys, "TrophyKey", "");
    }

    // Check if the loaded language is in the allowed list
    const std::vector<std::string> allowed_languages = {
        "ar_SA", "da_DK", "de_DE", "el_GR", "en_US", "es_ES", "fa_IR", "fi_FI", "fr_FR", "hu_HU",
        "id_ID", "it_IT", "ja_JP", "ko_KR", "lt_LT", "nb_NO", "nl_NL", "pl_PL", "pt_BR", "pt_PT",
        "ro_RO", "ru_RU", "sq_AL", "sv_SE", "tr_TR", "uk_UA", "vi_VN", "zh_CN", "zh_TW"};

    if (std::find(allowed_languages.begin(), allowed_languages.end(), emulator_language) ==
        allowed_languages.end()) {
        emulator_language = "en_US"; // Default to en_US if not in the list
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

    data["General"]["isPS4Pro"] = isNeo;
    data["General"]["isDevKit"] = isDevKit;
    data["General"]["isTrophyPopupDisabled"] = isTrophyPopupDisabled;
    data["General"]["trophyNotificationDuration"] = trophyNotificationDuration;
    data["General"]["playBGM"] = playBGM;
    data["General"]["BGMvolume"] = BGMvolume;
    data["General"]["enableDiscordRPC"] = enableDiscordRPC;
    data["General"]["logFilter"] = logFilter;
    data["General"]["logType"] = logType;
    data["General"]["userName"] = userName;
    data["General"]["updateChannel"] = updateChannel;
    data["General"]["chooseHomeTab"] = chooseHomeTab;
    data["General"]["showSplash"] = isShowSplash;
    data["General"]["autoUpdate"] = isAutoUpdate;
    data["General"]["alwaysShowChangelog"] = isAlwaysShowChangelog;
    data["General"]["sideTrophy"] = isSideTrophy;
    data["General"]["separateUpdateEnabled"] = separateupdatefolder;
    data["General"]["compatibilityEnabled"] = compatibilityData;
    data["General"]["checkCompatibilityOnStartup"] = checkCompatibilityOnStartup;
    data["Input"]["cursorState"] = cursorState;
    data["Input"]["cursorHideTimeout"] = cursorHideTimeout;
    data["Input"]["backButtonBehavior"] = backButtonBehavior;
    data["Input"]["useSpecialPad"] = useSpecialPad;
    data["Input"]["specialPadClass"] = specialPadClass;
    data["Input"]["isMotionControlsEnabled"] = isMotionControlsEnabled;
    data["Input"]["useUnifiedInputConfig"] = useUnifiedInputConfig;
    data["GPU"]["screenWidth"] = screenWidth;
    data["GPU"]["screenHeight"] = screenHeight;
    data["GPU"]["nullGpu"] = isNullGpu;
    data["GPU"]["copyGPUBuffers"] = shouldCopyGPUBuffers;
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
    data["GUI"]["emulatorLanguage"] = emulator_language;
    data["GUI"]["backgroundImageOpacity"] = backgroundImageOpacity;
    data["GUI"]["showBackgroundImage"] = showBackgroundImage;
    data["Settings"]["consoleLanguage"] = m_language;

    // Sorting of TOML sections
    sortTomlSections(data);

    std::ofstream file(path, std::ios::binary);
    file << data;
    file.close();

    saveMainWindow(path);
}

void saveMainWindow(const std::filesystem::path& path) {
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

    data["GUI"]["mw_width"] = m_window_size_W;
    data["GUI"]["mw_height"] = m_window_size_H;
    data["GUI"]["theme"] = mw_themes;
    data["GUI"]["iconSize"] = m_icon_size;
    data["GUI"]["sliderPos"] = m_slider_pos;
    data["GUI"]["iconSizeGrid"] = m_icon_size_grid;
    data["GUI"]["sliderPosGrid"] = m_slider_pos_grid;
    data["GUI"]["gameTableMode"] = m_table_mode;
    data["GUI"]["geometry_x"] = main_window_geometry_x;
    data["GUI"]["geometry_y"] = main_window_geometry_y;
    data["GUI"]["geometry_w"] = main_window_geometry_w;
    data["GUI"]["geometry_h"] = main_window_geometry_h;
    data["GUI"]["pkgDirs"] = m_pkg_viewer;
    data["GUI"]["elfDirs"] = m_elf_viewer;
    data["GUI"]["recentFiles"] = m_recent_files;

    // Sorting of TOML sections
    sortTomlSections(data);

    std::ofstream file(path, std::ios::binary);
    file << data;
    file.close();
}

void setDefaultValues() {
    isHDRAllowed = false;
    isNeo = false;
    isDevKit = false;
    isFullscreen = false;
    isTrophyPopupDisabled = false;
    playBGM = false;
    BGMvolume = 50;
    enableDiscordRPC = true;
    screenWidth = 1280;
    screenHeight = 720;
    logFilter = "";
    logType = "sync";
    userName = "shadPS4";
    if (Common::isRelease) {
        updateChannel = "Release";
    } else {
        updateChannel = "Nightly";
    }
    chooseHomeTab = "General";
    cursorState = HideCursorState::Idle;
    cursorHideTimeout = 5;
    trophyNotificationDuration = 6.0;
    backButtonBehavior = "left";
    useSpecialPad = false;
    specialPadClass = 1;
    isDebugDump = false;
    isShaderDebug = false;
    isShowSplash = false;
    isAutoUpdate = false;
    isAlwaysShowChangelog = false;
    isSideTrophy = "right";
    isNullGpu = false;
    shouldDumpShaders = false;
    vblankDivider = 1;
    vkValidation = false;
    vkValidationSync = false;
    vkValidationGpu = false;
    vkCrashDiagnostic = false;
    vkHostMarkers = false;
    vkGuestMarkers = false;
    rdocEnable = false;
    emulator_language = "en_US";
    m_language = 1;
    gpuId = -1;
    separateupdatefolder = false;
    compatibilityData = false;
    checkCompatibilityOnStartup = false;
    backgroundImageOpacity = 50;
    showBackgroundImage = true;
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
touchpad = space

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
touchpad = back

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
