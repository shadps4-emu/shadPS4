// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <optional>
#include <string>
#include <fmt/core.h>
#include <fmt/xchar.h> // for wstring support
#include <toml.hpp>

#include "common/config.h"
#include "common/logging/formatter.h"
#include "common/path_util.h"
#include "common/scm_rev.h"

using std::nullopt;
using std::optional;
using std::string;

namespace toml {
template <typename TC, typename K>
std::filesystem::path find_fs_path_or(const basic_value<TC>& v, const K& ky,
                                      std::filesystem::path opt) {
    try {
        auto str = find<string>(v, ky);
        if (str.empty()) {
            return opt;
        }
        std::u8string u8str{(char8_t*)&str.front(), (char8_t*)&str.back() + 1};
        return std::filesystem::path{u8str};
    } catch (...) {
        return opt;
    }
}

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
    } else if constexpr (std::is_same_v<T, double>) {
        if (it->second.is_floating()) {
            return toml::get<double>(it->second);
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
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

namespace Config {

template <typename T>
class ConfigEntry {
public:
    T base_value;
    optional<T> game_specific_value;
    ConfigEntry(const T& t = T()) : base_value(t), game_specific_value(nullopt) {}
    ConfigEntry operator=(const T& t) {
        base_value = t;
        return *this;
    }
    const T get() const {
        return game_specific_value.has_value() ? *game_specific_value : base_value;
    }
    void setFromToml(const toml::value& v, const std::string& key, bool is_game_specific = false) {
        if (is_game_specific) {
            game_specific_value = toml::get_optional<T>(v, key);
        } else {
            base_value = toml::get_optional<T>(v, key).value_or(base_value);
        }
    }
    void set(const T value, bool is_game_specific = false) {
        is_game_specific ? game_specific_value = value : base_value = value;
    }
    void setTomlValue(toml::ordered_value& data, const std::string& header, const std::string& key,
                      bool is_game_specific = false) {
        if (is_game_specific) {
            if (game_specific_value == std::nullopt) {
                fmt::print("Attempted to save std::nullopt value to {}-{}, matching config entry "
                           "may not be correctly set-up\n",
                           header, key);
                return;
            }
            data[header][key] = game_specific_value.value_or(base_value);
            game_specific_value = std::nullopt;
        } else {
            data[header][key] = base_value;
        }
    }
    // operator T() {
    //     return get();
    // }
};

// General
static ConfigEntry<int> volumeSlider(100);
static ConfigEntry<bool> isNeo(false);
static ConfigEntry<bool> isDevKit(false);
static ConfigEntry<bool> isPSNSignedIn(false);
static ConfigEntry<bool> isTrophyPopupDisabled(false);
static ConfigEntry<double> trophyNotificationDuration(6.0);
static ConfigEntry<string> logFilter("");
static ConfigEntry<string> logType("sync");
static ConfigEntry<string> userName("shadPS4");
static ConfigEntry<string> chooseHomeTab("General");
static ConfigEntry<bool> isShowSplash(false);
static ConfigEntry<string> isSideTrophy("right");
static ConfigEntry<bool> isConnectedToNetwork(false);
static bool enableDiscordRPC = false;
static bool checkCompatibilityOnStartup = false;
static bool compatibilityData = false;

// Input
static ConfigEntry<int> cursorState(HideCursorState::Idle);
static ConfigEntry<int> cursorHideTimeout(5); // 5 seconds (default)
static ConfigEntry<bool> useSpecialPad(false);
static ConfigEntry<int> specialPadClass(1);
static ConfigEntry<bool> isMotionControlsEnabled(true);
static ConfigEntry<bool> useUnifiedInputConfig(true);
static ConfigEntry<string> micDevice("Default Device");
static ConfigEntry<string> defaultControllerID("");
static ConfigEntry<bool> backgroundControllerInput(false);

// GPU
static ConfigEntry<u32> windowWidth(1280);
static ConfigEntry<u32> windowHeight(720);
static ConfigEntry<u32> internalScreenWidth(1280);
static ConfigEntry<u32> internalScreenHeight(720);
static ConfigEntry<bool> isNullGpu(false);
static ConfigEntry<bool> shouldCopyGPUBuffers(false);
static ConfigEntry<bool> readbacksEnabled(false);
static ConfigEntry<bool> readbackLinearImagesEnabled(false);
static ConfigEntry<bool> directMemoryAccessEnabled(false);
static ConfigEntry<bool> shouldDumpShaders(false);
static ConfigEntry<bool> shouldPatchShaders(false);
static ConfigEntry<u32> vblankFrequency(60);
static ConfigEntry<bool> isFullscreen(false);
static ConfigEntry<string> fullscreenMode("Windowed");
static ConfigEntry<string> presentMode("Mailbox");
static ConfigEntry<bool> isHDRAllowed(false);
static ConfigEntry<bool> fsrEnabled(true);
static ConfigEntry<bool> rcasEnabled(true);
static ConfigEntry<int> rcasAttenuation(250);

// Vulkan
static ConfigEntry<s32> gpuId(-1);
static ConfigEntry<bool> vkValidation(false);
static ConfigEntry<bool> vkValidationSync(false);
static ConfigEntry<bool> vkValidationGpu(false);
static ConfigEntry<bool> vkCrashDiagnostic(false);
static ConfigEntry<bool> vkHostMarkers(false);
static ConfigEntry<bool> vkGuestMarkers(false);
static ConfigEntry<bool> rdocEnable(false);

// Debug
static ConfigEntry<bool> isDebugDump(false);
static ConfigEntry<bool> isShaderDebug(false);
static ConfigEntry<bool> isSeparateLogFilesEnabled(false);
static ConfigEntry<bool> isFpsColor(true);
static ConfigEntry<bool> logEnabled(true);

// GUI
static bool load_game_size = true;
static std::vector<GameInstallDir> settings_install_dirs = {};
std::vector<bool> install_dirs_enabled = {};
std::filesystem::path settings_addon_install_dir = {};
std::filesystem::path save_data_path = {};

// Settings
ConfigEntry<u32> m_language(1); // english

// Keys
static string trophyKey = "";

// Config version, used to determine if a user's config file is outdated.
static string config_version = Common::g_scm_rev;

// These two entries aren't stored in the config
static bool overrideControllerColor = false;
static int controllerCustomColorRGB[3] = {0, 0, 255};

int getVolumeSlider() {
    return volumeSlider.get();
}
bool allowHDR() {
    return isHDRAllowed.get();
}

bool GetUseUnifiedInputConfig() {
    return useUnifiedInputConfig.get();
}

void SetUseUnifiedInputConfig(bool use) {
    useUnifiedInputConfig.base_value = use;
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

bool getLoggingEnabled() {
    return logEnabled.get();
}

void SetControllerCustomColor(int r, int b, int g) {
    controllerCustomColorRGB[0] = r;
    controllerCustomColorRGB[1] = b;
    controllerCustomColorRGB[2] = g;
}

string getTrophyKey() {
    return trophyKey;
}

void setTrophyKey(string key) {
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

void setVolumeSlider(int volumeValue, bool is_game_specific) {
    volumeSlider.set(volumeValue, is_game_specific);
}

void setLoadGameSizeEnabled(bool enable) {
    load_game_size = enable;
}

bool isNeoModeConsole() {
    return isNeo.get();
}

bool isDevKitConsole() {
    return isDevKit.get();
}

bool getIsFullscreen() {
    return isFullscreen.get();
}

string getFullscreenMode() {
    return fullscreenMode.get();
}

std::string getPresentMode() {
    return presentMode.get();
}

bool getisTrophyPopupDisabled() {
    return isTrophyPopupDisabled.get();
}

bool getEnableDiscordRPC() {
    return enableDiscordRPC;
}

s16 getCursorState() {
    return cursorState.get();
}

int getCursorHideTimeout() {
    return cursorHideTimeout.get();
}

string getMicDevice() {
    return micDevice.get();
}

double getTrophyNotificationDuration() {
    return trophyNotificationDuration.get();
}

u32 getWindowWidth() {
    return windowWidth.get();
}

u32 getWindowHeight() {
    return windowHeight.get();
}

u32 getInternalScreenWidth() {
    return internalScreenHeight.get();
}

u32 getInternalScreenHeight() {
    return internalScreenHeight.get();
}

s32 getGpuId() {
    return gpuId.get();
}

string getLogFilter() {
    return logFilter.get();
}

string getLogType() {
    return logType.get();
}

string getUserName() {
    return userName.get();
}

string getChooseHomeTab() {
    return chooseHomeTab.get();
}

bool getUseSpecialPad() {
    return useSpecialPad.get();
}

int getSpecialPadClass() {
    return specialPadClass.get();
}

bool getIsMotionControlsEnabled() {
    return isMotionControlsEnabled.get();
}

bool debugDump() {
    return isDebugDump.get();
}

bool collectShadersForDebug() {
    return isShaderDebug.get();
}

bool showSplash() {
    return isShowSplash.get();
}

string sideTrophy() {
    return isSideTrophy.get();
}

bool nullGpu() {
    return isNullGpu.get();
}

bool copyGPUCmdBuffers() {
    return shouldCopyGPUBuffers.get();
}

bool readbacks() {
    return readbacksEnabled.get();
}

bool readbackLinearImages() {
    return readbackLinearImagesEnabled.get();
}

bool directMemoryAccess() {
    return directMemoryAccessEnabled.get();
}

bool dumpShaders() {
    return shouldDumpShaders.get();
}

bool patchShaders() {
    return shouldPatchShaders.get();
}

bool isRdocEnabled() {
    return rdocEnable.get();
}

bool fpsColor() {
    return isFpsColor.get();
}

bool isLoggingEnabled() {
    return logEnabled.get();
}

u32 vblankFreq() {
    if (vblankFrequency.get() < 60) {
        vblankFrequency = 60;
    }
    return vblankFrequency.get();
}

bool vkValidationEnabled() {
    return vkValidation.get();
}

bool vkValidationSyncEnabled() {
    return vkValidationSync.get();
}

bool vkValidationGpuEnabled() {
    return vkValidationGpu.get();
}

bool getVkCrashDiagnosticEnabled() {
    return vkCrashDiagnostic.get();
}

bool getVkHostMarkersEnabled() {
    return vkHostMarkers.get();
}

bool getVkGuestMarkersEnabled() {
    return vkGuestMarkers.get();
}

void setVkCrashDiagnosticEnabled(bool enable, bool is_game_specific) {
    vkCrashDiagnostic.set(enable, is_game_specific);
}

void setVkHostMarkersEnabled(bool enable, bool is_game_specific) {
    vkHostMarkers.set(enable, is_game_specific);
}

void setVkGuestMarkersEnabled(bool enable, bool is_game_specific) {
    vkGuestMarkers.set(enable, is_game_specific);
}

bool getCompatibilityEnabled() {
    return compatibilityData;
}

bool getCheckCompatibilityOnStartup() {
    return checkCompatibilityOnStartup;
}

bool getIsConnectedToNetwork() {
    return isConnectedToNetwork.get();
}

void setConnectedToNetwork(bool enable, bool is_game_specific) {
    isConnectedToNetwork.set(enable, is_game_specific);
}

void setGpuId(s32 selectedGpuId, bool is_game_specific) {
    gpuId.set(selectedGpuId, is_game_specific);
}

void setWindowWidth(u32 width, bool is_game_specific) {
    windowWidth.set(width, is_game_specific);
}

void setWindowHeight(u32 height, bool is_game_specific) {
    windowHeight.set(height, is_game_specific);
}

void setInternalScreenWidth(u32 width) {
    internalScreenWidth.base_value = width;
}

void setInternalScreenHeight(u32 height) {
    internalScreenHeight.base_value = height;
}

void setDebugDump(bool enable, bool is_game_specific) {
    isDebugDump.set(enable, is_game_specific);
}

void setLoggingEnabled(bool enable, bool is_game_specific) {
    logEnabled.set(enable, is_game_specific);
}

void setCollectShaderForDebug(bool enable, bool is_game_specific) {
    isShaderDebug.set(enable, is_game_specific);
}

void setShowSplash(bool enable, bool is_game_specific) {
    isShowSplash.set(enable, is_game_specific);
}

void setSideTrophy(string side, bool is_game_specific) {
    isSideTrophy.set(side, is_game_specific);
}

void setNullGpu(bool enable, bool is_game_specific) {
    isNullGpu.set(enable, is_game_specific);
}

void setAllowHDR(bool enable, bool is_game_specific) {
    isHDRAllowed.set(enable, is_game_specific);
}

void setCopyGPUCmdBuffers(bool enable, bool is_game_specific) {
    shouldCopyGPUBuffers.set(enable, is_game_specific);
}

void setReadbacks(bool enable, bool is_game_specific) {
    readbacksEnabled.set(enable, is_game_specific);
}

void setReadbackLinearImages(bool enable, bool is_game_specific) {
    readbackLinearImagesEnabled.set(enable, is_game_specific);
}

void setDirectMemoryAccess(bool enable, bool is_game_specific) {
    directMemoryAccessEnabled.set(enable, is_game_specific);
}

void setDumpShaders(bool enable, bool is_game_specific) {
    shouldDumpShaders.set(enable, is_game_specific);
}

void setVkValidation(bool enable, bool is_game_specific) {
    vkValidation.set(enable, is_game_specific);
}

void setVkSyncValidation(bool enable, bool is_game_specific) {
    vkValidationSync.set(enable, is_game_specific);
}

void setRdocEnabled(bool enable, bool is_game_specific) {
    rdocEnable.set(enable, is_game_specific);
}

void setVblankFreq(u32 value, bool is_game_specific) {
    vblankFrequency.set(value, is_game_specific);
}

void setIsFullscreen(bool enable, bool is_game_specific) {
    isFullscreen.set(enable, is_game_specific);
}

void setFullscreenMode(string mode, bool is_game_specific) {
    fullscreenMode.set(mode, is_game_specific);
}

void setPresentMode(std::string mode, bool is_game_specific) {
    presentMode.set(mode, is_game_specific);
}

void setisTrophyPopupDisabled(bool disable, bool is_game_specific) {
    isTrophyPopupDisabled.set(disable, is_game_specific);
}

void setEnableDiscordRPC(bool enable) {
    enableDiscordRPC = enable;
}

void setCursorState(s16 newCursorState, bool is_game_specific) {
    cursorState.set(newCursorState, is_game_specific);
}

void setCursorHideTimeout(int newcursorHideTimeout, bool is_game_specific) {
    cursorHideTimeout.set(newcursorHideTimeout, is_game_specific);
}

void setMicDevice(string device, bool is_game_specific) {
    micDevice.set(device, is_game_specific);
}

void setTrophyNotificationDuration(double newTrophyNotificationDuration, bool is_game_specific) {
    trophyNotificationDuration.set(newTrophyNotificationDuration, is_game_specific);
}

void setLanguage(u32 language, bool is_game_specific) {
    m_language.set(language, is_game_specific);
}

void setNeoMode(bool enable, bool is_game_specific) {
    isNeo.set(enable, is_game_specific);
}

void setDevKitConsole(bool enable, bool is_game_specific) {
    isDevKit.set(enable, is_game_specific);
}

void setLogType(const string& type, bool is_game_specific) {
    logType.set(type, is_game_specific);
}

void setLogFilter(const string& type, bool is_game_specific) {
    logFilter.set(type, is_game_specific);
}

void setSeparateLogFilesEnabled(bool enabled, bool is_game_specific) {
    isSeparateLogFilesEnabled.set(enabled, is_game_specific);
}

void setUserName(const string& name, bool is_game_specific) {
    userName.set(name, is_game_specific);
}

void setChooseHomeTab(const string& type, bool is_game_specific) {
    chooseHomeTab.set(type, is_game_specific);
}

void setUseSpecialPad(bool use) {
    useSpecialPad.base_value = use;
}

void setSpecialPadClass(int type) {
    specialPadClass.base_value = type;
}

void setIsMotionControlsEnabled(bool use, bool is_game_specific) {
    isMotionControlsEnabled.set(use, is_game_specific);
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
    return m_language.get();
}

bool getSeparateLogFilesEnabled() {
    return isSeparateLogFilesEnabled.get();
}

bool getPSNSignedIn() {
    return isPSNSignedIn.get();
}

void setPSNSignedIn(bool sign, bool is_game_specific) {
    isPSNSignedIn.set(sign, is_game_specific);
}

string getDefaultControllerID() {
    return defaultControllerID.get();
}

void setDefaultControllerID(string id) {
    defaultControllerID.base_value = id;
}

bool getBackgroundControllerInput() {
    return backgroundControllerInput.get();
}

void setBackgroundControllerInput(bool enable, bool is_game_specific) {
    backgroundControllerInput.set(enable, is_game_specific);
}

bool getFsrEnabled() {
    return fsrEnabled.get();
}

void setFsrEnabled(bool enable, bool is_game_specific) {
    fsrEnabled.set(enable, is_game_specific);
}

bool getRcasEnabled() {
    return rcasEnabled.get();
}

void setRcasEnabled(bool enable, bool is_game_specific) {
    rcasEnabled.set(enable, is_game_specific);
}

int getRcasAttenuation() {
    return rcasAttenuation.get();
}

void setRcasAttenuation(int value, bool is_game_specific) {
    rcasAttenuation.set(value, is_game_specific);
}

void load(const std::filesystem::path& path, bool is_game_specific) {
    // If the configuration file does not exist, create it and return, unless it is game specific
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        if (!is_game_specific) {
            save(path);
        }
        return;
    }

    toml::value data;

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifs.open(path, std::ios_base::binary);
        data = toml::parse(ifs, string{fmt::UTF(path.filename().u8string()).data});
    } catch (std::exception& ex) {
        fmt::print("Got exception trying to load config file. Exception: {}\n", ex.what());
        return;
    }

    if (data.contains("General")) {
        const toml::value& general = data.at("General");

        volumeSlider.setFromToml(general, "volumeSlider", is_game_specific);
        isNeo.setFromToml(general, "isPS4Pro", is_game_specific);
        isDevKit.setFromToml(general, "isDevKit", is_game_specific);
        isPSNSignedIn.setFromToml(general, "isPSNSignedIn", is_game_specific);
        isTrophyPopupDisabled.setFromToml(general, "isTrophyPopupDisabled", is_game_specific);
        trophyNotificationDuration.setFromToml(general, "trophyNotificationDuration",
                                               is_game_specific);
        enableDiscordRPC = toml::find_or<bool>(general, "enableDiscordRPC", enableDiscordRPC);
        logFilter.setFromToml(general, "logFilter", is_game_specific);
        logType.setFromToml(general, "logType", is_game_specific);
        userName.setFromToml(general, "userName", is_game_specific);
        isShowSplash.setFromToml(general, "showSplash", is_game_specific);
        isSideTrophy.setFromToml(general, "sideTrophy", is_game_specific);
        compatibilityData = toml::find_or<bool>(general, "compatibilityEnabled", compatibilityData);
        checkCompatibilityOnStartup = toml::find_or<bool>(general, "checkCompatibilityOnStartup",
                                                          checkCompatibilityOnStartup);

        isConnectedToNetwork.setFromToml(general, "isConnectedToNetwork", is_game_specific);
        chooseHomeTab.setFromToml(general, "chooseHomeTab", is_game_specific);
        defaultControllerID.setFromToml(general, "defaultControllerID", is_game_specific);
    }

    if (data.contains("Input")) {
        const toml::value& input = data.at("Input");

        cursorState.setFromToml(input, "cursorState", is_game_specific);
        cursorHideTimeout.setFromToml(input, "cursorHideTimeout", is_game_specific);
        useSpecialPad.setFromToml(input, "useSpecialPad", is_game_specific);
        specialPadClass.setFromToml(input, "specialPadClass", is_game_specific);
        isMotionControlsEnabled.setFromToml(input, "isMotionControlsEnabled", is_game_specific);
        useUnifiedInputConfig.setFromToml(input, "useUnifiedInputConfig", is_game_specific);
        micDevice.setFromToml(input, "micDevice", is_game_specific);
        backgroundControllerInput.setFromToml(input, "backgroundControllerInput", is_game_specific);
    }

    if (data.contains("GPU")) {
        const toml::value& gpu = data.at("GPU");

        windowWidth.setFromToml(gpu, "screenWidth", is_game_specific);
        windowHeight.setFromToml(gpu, "screenHeight", is_game_specific);
        internalScreenWidth.setFromToml(gpu, "internalScreenWidth", is_game_specific);
        internalScreenHeight.setFromToml(gpu, "internalScreenHeight", is_game_specific);
        isNullGpu.setFromToml(gpu, "nullGpu", is_game_specific);
        shouldCopyGPUBuffers.setFromToml(gpu, "copyGPUBuffers", is_game_specific);
        readbacksEnabled.setFromToml(gpu, "readbacks", is_game_specific);
        readbackLinearImagesEnabled.setFromToml(gpu, "readbackLinearImages", is_game_specific);
        directMemoryAccessEnabled.setFromToml(gpu, "directMemoryAccess", is_game_specific);
        shouldDumpShaders.setFromToml(gpu, "dumpShaders", is_game_specific);
        shouldPatchShaders.setFromToml(gpu, "patchShaders", is_game_specific);
        vblankFrequency.setFromToml(gpu, "vblankFrequency", is_game_specific);
        isFullscreen.setFromToml(gpu, "Fullscreen", is_game_specific);
        fullscreenMode.setFromToml(gpu, "FullscreenMode", is_game_specific);
        presentMode.setFromToml(gpu, "presentMode", is_game_specific);
        isHDRAllowed.setFromToml(gpu, "allowHDR", is_game_specific);
        fsrEnabled.setFromToml(gpu, "fsrEnabled", is_game_specific);
        rcasEnabled.setFromToml(gpu, "rcasEnabled", is_game_specific);
        rcasAttenuation.setFromToml(gpu, "rcasAttenuation", is_game_specific);
    }

    if (data.contains("Vulkan")) {
        const toml::value& vk = data.at("Vulkan");

        gpuId.setFromToml(vk, "gpuId", is_game_specific);
        vkValidation.setFromToml(vk, "validation", is_game_specific);
        vkValidationSync.setFromToml(vk, "validation_sync", is_game_specific);
        vkValidationGpu.setFromToml(vk, "validation_gpu", is_game_specific);
        vkCrashDiagnostic.setFromToml(vk, "crashDiagnostic", is_game_specific);
        vkHostMarkers.setFromToml(vk, "hostMarkers", is_game_specific);
        vkGuestMarkers.setFromToml(vk, "guestMarkers", is_game_specific);
        rdocEnable.setFromToml(vk, "rdocEnable", is_game_specific);
    }

    string current_version = {};
    if (data.contains("Debug")) {
        const toml::value& debug = data.at("Debug");

        isDebugDump.setFromToml(debug, "DebugDump", is_game_specific);
        isSeparateLogFilesEnabled.setFromToml(debug, "isSeparateLogFilesEnabled", is_game_specific);
        isShaderDebug.setFromToml(debug, "CollectShader", is_game_specific);
        isFpsColor.setFromToml(debug, "FPSColor", is_game_specific);
        logEnabled.setFromToml(debug, "logEnabled", is_game_specific);
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
        m_language.setFromToml(settings, "consoleLanguage", is_game_specific);
    }

    if (data.contains("Keys")) {
        const toml::value& keys = data.at("Keys");
        trophyKey = toml::find_or<string>(keys, "TrophyKey", trophyKey);
    }

    // Run save after loading to generate any missing fields with default values.
    if (config_version != current_version && !is_game_specific) {
        save(path);
    }
}

void sortTomlSections(toml::ordered_value& data) {
    toml::ordered_value ordered_data;
    std::vector<string> section_order = {"General", "Input", "GPU", "Vulkan",
                                         "Debug",   "Keys",  "GUI", "Settings"};

    for (const auto& section : section_order) {
        if (data.contains(section)) {
            std::vector<string> keys;
            for (const auto& item : data.at(section).as_table()) {
                keys.push_back(item.first);
            }

            std::sort(keys.begin(), keys.end(), [](const string& a, const string& b) {
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

void save(const std::filesystem::path& path, bool is_game_specific) {
    toml::ordered_value data;

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        try {
            std::ifstream ifs;
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifs.open(path, std::ios_base::binary);
            data = toml::parse<toml::ordered_type_config>(
                ifs, string{fmt::UTF(path.filename().u8string()).data});
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

    // Entries saved by the game-specific settings GUI
    volumeSlider.setTomlValue(data, "General", "volumeSlider", is_game_specific);
    isTrophyPopupDisabled.setTomlValue(data, "General", "isTrophyPopupDisabled", is_game_specific);
    trophyNotificationDuration.setTomlValue(data, "General", "trophyNotificationDuration",
                                            is_game_specific);
    logFilter.setTomlValue(data, "General", "logFilter", is_game_specific);
    logType.setTomlValue(data, "General", "logType", is_game_specific);
    userName.setTomlValue(data, "General", "userName", is_game_specific);
    chooseHomeTab.setTomlValue(data, "General", "chooseHomeTab", is_game_specific);
    isShowSplash.setTomlValue(data, "General", "showSplash", is_game_specific);
    isSideTrophy.setTomlValue(data, "General", "sideTrophy", is_game_specific);
    isNeo.setTomlValue(data, "General", "isPS4Pro", is_game_specific);
    isDevKit.setTomlValue(data, "General", "isDevKit", is_game_specific);
    isPSNSignedIn.setTomlValue(data, "General", "isPSNSignedIn", is_game_specific);
    isConnectedToNetwork.setTomlValue(data, "General", "isConnectedToNetwork", is_game_specific);

    cursorState.setTomlValue(data, "Input", "cursorState", is_game_specific);
    cursorHideTimeout.setTomlValue(data, "Input", "cursorHideTimeout", is_game_specific);
    isMotionControlsEnabled.setTomlValue(data, "Input", "isMotionControlsEnabled",
                                         is_game_specific);
    micDevice.setTomlValue(data, "Input", "micDevice", is_game_specific);
    backgroundControllerInput.setTomlValue(data, "Input", "backgroundControllerInput",
                                           is_game_specific);

    windowWidth.setTomlValue(data, "GPU", "screenWidth", is_game_specific);
    windowHeight.setTomlValue(data, "GPU", "screenHeight", is_game_specific);
    isNullGpu.setTomlValue(data, "GPU", "nullGpu", is_game_specific);
    shouldCopyGPUBuffers.setTomlValue(data, "GPU", "copyGPUBuffers", is_game_specific);
    readbacksEnabled.setTomlValue(data, "GPU", "readbacks", is_game_specific);
    readbackLinearImagesEnabled.setTomlValue(data, "GPU", "readbackLinearImages", is_game_specific);
    shouldDumpShaders.setTomlValue(data, "GPU", "dumpShaders", is_game_specific);
    vblankFrequency.setTomlValue(data, "GPU", "vblankFrequency", is_game_specific);
    isFullscreen.setTomlValue(data, "GPU", "Fullscreen", is_game_specific);
    fullscreenMode.setTomlValue(data, "GPU", "FullscreenMode", is_game_specific);
    presentMode.setTomlValue(data, "GPU", "presentMode", is_game_specific);
    isHDRAllowed.setTomlValue(data, "GPU", "allowHDR", is_game_specific);
    fsrEnabled.setTomlValue(data, "GPU", "fsrEnabled", is_game_specific);
    rcasEnabled.setTomlValue(data, "GPU", "rcasEnabled", is_game_specific);
    rcasAttenuation.setTomlValue(data, "GPU", "rcasAttenuation", is_game_specific);
    directMemoryAccessEnabled.setTomlValue(data, "GPU", "directMemoryAccess", is_game_specific);

    gpuId.setTomlValue(data, "Vulkan", "gpuId", is_game_specific);
    vkValidation.setTomlValue(data, "Vulkan", "validation", is_game_specific);
    vkValidationSync.setTomlValue(data, "Vulkan", "validation_sync", is_game_specific);
    vkCrashDiagnostic.setTomlValue(data, "Vulkan", "crashDiagnostic", is_game_specific);
    vkHostMarkers.setTomlValue(data, "Vulkan", "hostMarkers", is_game_specific);
    vkGuestMarkers.setTomlValue(data, "Vulkan", "guestMarkers", is_game_specific);
    rdocEnable.setTomlValue(data, "Vulkan", "rdocEnable", is_game_specific);

    isDebugDump.setTomlValue(data, "Debug", "DebugDump", is_game_specific);
    isShaderDebug.setTomlValue(data, "Debug", "CollectShader", is_game_specific);
    isSeparateLogFilesEnabled.setTomlValue(data, "Debug", "isSeparateLogFilesEnabled",
                                           is_game_specific);
    logEnabled.setTomlValue(data, "Debug", "logEnabled", is_game_specific);

    m_language.setTomlValue(data, "Settings", "consoleLanguage", is_game_specific);

    // All other entries
    if (!is_game_specific) {
        std::vector<string> install_dirs;
        std::vector<bool> install_dirs_enabled;

        // temporary structure for ordering
        struct DirEntry {
            string path_str;
            bool enabled;
        };

        std::vector<DirEntry> sorted_dirs;
        for (const auto& dirInfo : settings_install_dirs) {
            sorted_dirs.push_back(
                {string{fmt::UTF(dirInfo.path.u8string()).data}, dirInfo.enabled});
        }

        // Sort directories alphabetically
        std::sort(sorted_dirs.begin(), sorted_dirs.end(), [](const DirEntry& a, const DirEntry& b) {
            return std::lexicographical_compare(
                a.path_str.begin(), a.path_str.end(), b.path_str.begin(), b.path_str.end(),
                [](char a_char, char b_char) {
                    return std::tolower(a_char) < std::tolower(b_char);
                });
        });

        for (const auto& entry : sorted_dirs) {
            install_dirs.push_back(entry.path_str);
            install_dirs_enabled.push_back(entry.enabled);
        }

        // Non game-specific entries
        data["General"]["enableDiscordRPC"] = enableDiscordRPC;
        data["General"]["compatibilityEnabled"] = compatibilityData;
        data["General"]["checkCompatibilityOnStartup"] = checkCompatibilityOnStartup;
        data["GUI"]["installDirs"] = install_dirs;
        data["GUI"]["installDirsEnabled"] = install_dirs_enabled;
        data["GUI"]["saveDataPath"] = string{fmt::UTF(save_data_path.u8string()).data};
        data["GUI"]["loadGameSizeEnabled"] = load_game_size;
        data["GUI"]["addonInstallDir"] =
            string{fmt::UTF(settings_addon_install_dir.u8string()).data};

        data["Debug"]["ConfigVersion"] = config_version;
        data["Keys"]["TrophyKey"] = trophyKey;

        // Do not save these entries in the game-specific dialog since they are not in the GUI
        data["General"]["defaultControllerID"] = defaultControllerID.base_value;

        data["Input"]["useSpecialPad"] = useSpecialPad.base_value;
        data["Input"]["specialPadClass"] = specialPadClass.base_value;
        data["Input"]["useUnifiedInputConfig"] = useUnifiedInputConfig.base_value;

        data["GPU"]["internalScreenWidth"] = internalScreenWidth.base_value;
        data["GPU"]["internalScreenHeight"] = internalScreenHeight.base_value;
        data["GPU"]["patchShaders"] = shouldPatchShaders.base_value;

        data["Vulkan"]["validation_gpu"] = vkValidationGpu.base_value;

        data["Debug"]["FPSColor"] = isFpsColor.base_value;
    }

    // Sorting of TOML sections
    sortTomlSections(data);

    std::ofstream file(path, std::ios::binary);
    file << data;
    file.close();
}

void setDefaultValues(bool is_game_specific) {

    // Entries with game-specific settings that are in the game-specific setings GUI but not in
    // the global settings GUI
    if (is_game_specific) {
        readbacksEnabled.set(false, is_game_specific);
        readbackLinearImagesEnabled.set(false, is_game_specific);
        isNeo.set(false, is_game_specific);
        isDevKit.set(false, is_game_specific);
        isPSNSignedIn.set(false, is_game_specific);
        isConnectedToNetwork.set(false, is_game_specific);
        directMemoryAccessEnabled.set(false, is_game_specific);
    }

    // Entries with game-specific settings that are in both the game-specific and global GUI
    // GS - General
    volumeSlider.set(100, is_game_specific);
    isTrophyPopupDisabled.set(false, is_game_specific);
    trophyNotificationDuration.set(6.0, is_game_specific);
    logFilter.set("", is_game_specific);
    logType.set("sync", is_game_specific);
    userName.set("shadPS4", is_game_specific);
    chooseHomeTab.set("General", is_game_specific);
    isShowSplash.set(false, is_game_specific);
    isSideTrophy.set("right", is_game_specific);

    // GS - Input
    cursorState.set(HideCursorState::Idle, is_game_specific);
    cursorHideTimeout.set(5, is_game_specific);
    isMotionControlsEnabled.set(true, is_game_specific);
    micDevice.set("Default Device", is_game_specific);
    backgroundControllerInput.set(false, is_game_specific);

    // GS - GPU
    windowWidth.set(1280, is_game_specific);
    windowHeight.set(720, is_game_specific);
    isNullGpu.set(false, is_game_specific);
    shouldCopyGPUBuffers.set(false, is_game_specific);
    shouldDumpShaders.set(false, is_game_specific);
    vblankFrequency.set(60, is_game_specific);
    isFullscreen.set(false, is_game_specific);
    fullscreenMode.set("Windowed", is_game_specific);
    presentMode.set("Mailbox", is_game_specific);
    isHDRAllowed.set(false, is_game_specific);
    fsrEnabled.set(true, is_game_specific);
    rcasEnabled.set(true, is_game_specific);
    rcasAttenuation.set(250, is_game_specific);

    // GS - Vulkan
    gpuId.set(-1, is_game_specific);
    vkValidation.set(false, is_game_specific);
    vkValidationSync.set(false, is_game_specific);
    vkValidationGpu.set(false, is_game_specific);
    vkCrashDiagnostic.set(false, is_game_specific);
    vkHostMarkers.set(false, is_game_specific);
    vkGuestMarkers.set(false, is_game_specific);
    rdocEnable.set(false, is_game_specific);

    // GS - Debug
    isDebugDump.set(false, is_game_specific);
    isShaderDebug.set(false, is_game_specific);
    isSeparateLogFilesEnabled.set(false, is_game_specific);
    logEnabled.set(true, is_game_specific);

    // GS - Settings
    m_language.set(1, is_game_specific);

    // All other entries
    if (!is_game_specific) {

        // General
        enableDiscordRPC = false;
        compatibilityData = false;
        checkCompatibilityOnStartup = false;

        // Input
        useSpecialPad.base_value = false;
        specialPadClass.base_value = 1;
        useUnifiedInputConfig.base_value = true;
        overrideControllerColor = false;
        controllerCustomColorRGB[0] = 0;
        controllerCustomColorRGB[1] = 0;
        controllerCustomColorRGB[2] = 255;

        // GPU
        shouldPatchShaders.base_value = false;
        internalScreenWidth.base_value = 1280;
        internalScreenHeight.base_value = 720;

        // GUI
        load_game_size = true;

        // Debug
        isFpsColor.base_value = true;
    }
}

constexpr std::string_view GetDefaultGlobalConfig() {
    return R"(# Anything put here will be loaded for all games,
# alongside the game's config or default.ini depending on your preference.

hotkey_renderdoc_capture = f12
hotkey_fullscreen = f11
hotkey_show_fps = f10
hotkey_pause = f9
hotkey_reload_inputs = f8
hotkey_toggle_mouse_to_joystick = f7
hotkey_toggle_mouse_to_gyro = f6
hotkey_quit = lctrl, lshift, end
)";
}

constexpr std::string_view GetDefaultInputConfig() {
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
std::filesystem::path GetFoolproofInputConfigFile(const string& game_id) {
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
        const auto default_config = GetDefaultInputConfig();
        std::ofstream default_config_stream(default_config_file);
        if (default_config_stream) {
            default_config_stream << default_config;
        }
    }

    // if empty, we only need to execute the function up until this point
    if (game_id.empty()) {
        return default_config_file;
    }

    // Create global config if it doesn't exist yet
    if (game_id == "global" && !std::filesystem::exists(config_file)) {
        if (!std::filesystem::exists(config_file)) {
            const auto global_config = GetDefaultGlobalConfig();
            std::ofstream global_config_stream(config_file);
            if (global_config_stream) {
                global_config_stream << global_config;
            }
        }
    }

    // If game-specific config doesn't exist, create it from the default config
    if (!std::filesystem::exists(config_file)) {
        std::filesystem::copy(default_config_file, config_file);
    }
    return config_file;
}

void resetGameSpecificValue(std::string entry) {
    if (entry == "volumeSlider") {
        volumeSlider.game_specific_value = std::nullopt;
    }
}

} // namespace Config
