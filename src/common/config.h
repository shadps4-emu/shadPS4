// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <vector>
#include "types.h"

namespace Config {

struct GameInstallDir {
    std::filesystem::path path;
    bool enabled;
};

enum HideCursorState : int { Never, Idle, Always };

void load(const std::filesystem::path& path, bool is_game_specific = false);
void save(const std::filesystem::path& path);

int getVolumeSlider();
void setVolumeSlider(int volumeValue);
std::string getTrophyKey();
void setTrophyKey(std::string key);
bool getIsFullscreen();
void setIsFullscreen(bool enable);
std::string getFullscreenMode();
void setFullscreenMode(std::string mode);
std::string getPresentMode();
void setPresentMode(std::string mode);
u32 getWindowWidth();
u32 getWindowHeight();
void setWindowWidth(u32 width);
void setWindowHeight(u32 height);
u32 getInternalScreenWidth();
u32 getInternalScreenHeight();
void setInternalScreenWidth(u32 width);
void setInternalScreenHeight(u32 height);
bool debugDump();
void setDebugDump(bool enable);
s32 getGpuId();
void setGpuId(s32 selectedGpuId);
bool allowHDR();
void setAllowHDR(bool enable);
bool collectShadersForDebug();
void setCollectShaderForDebug(bool enable);
bool showSplash();
void setShowSplash(bool enable);
std::string sideTrophy();
void setSideTrophy(std::string side);
bool nullGpu();
void setNullGpu(bool enable);
bool copyGPUCmdBuffers();
void setCopyGPUCmdBuffers(bool enable);
bool readbacks();
void setReadbacks(bool enable);
bool readbackLinearImages();
void setReadbackLinearImages(bool enable);
bool directMemoryAccess();
void setDirectMemoryAccess(bool enable);
bool dumpShaders();
void setDumpShaders(bool enable);
u32 vblankDiv();
void setVblankDiv(u32 value);
bool getisTrophyPopupDisabled();
void setisTrophyPopupDisabled(bool disable);
s16 getCursorState();
void setCursorState(s16 cursorState);
bool vkValidationEnabled();
void setVkValidation(bool enable);
bool vkValidationSyncEnabled();
void setVkSyncValidation(bool enable);
bool getVkCrashDiagnosticEnabled();
void setVkCrashDiagnosticEnabled(bool enable);
bool getVkHostMarkersEnabled();
void setVkHostMarkersEnabled(bool enable);
bool getVkGuestMarkersEnabled();
void setVkGuestMarkersEnabled(bool enable);
bool getEnableDiscordRPC();
void setEnableDiscordRPC(bool enable);
bool isRdocEnabled();
void setRdocEnabled(bool enable);
std::string getLogType();
void setLogType(const std::string& type);
std::string getLogFilter();
void setLogFilter(const std::string& type);
double getTrophyNotificationDuration();
void setTrophyNotificationDuration(double newTrophyNotificationDuration);
int getCursorHideTimeout();
std::string getMicDevice();
void setCursorHideTimeout(int newcursorHideTimeout);
void setMicDevice(std::string device);
void setSeparateLogFilesEnabled(bool enabled);
bool getSeparateLogFilesEnabled();
u32 GetLanguage();
void setLanguage(u32 language);
void setUseSpecialPad(bool use);
bool getUseSpecialPad();
void setSpecialPadClass(int type);
int getSpecialPadClass();
bool getPSNSignedIn();
void setPSNSignedIn(bool sign); // no ui setting
bool patchShaders();            // no set
bool fpsColor();                // no set
bool isNeoModeConsole();
void setNeoMode(bool enable);  // no ui setting
bool isDevKitConsole();        // no set
bool vkValidationGpuEnabled(); // no set
bool getIsMotionControlsEnabled();
void setIsMotionControlsEnabled(bool use);
std::string getDefaultControllerID();
void setDefaultControllerID(std::string id);
bool getBackgroundControllerInput();
void setBackgroundControllerInput(bool enable);
bool getLoggingEnabled();
void setLoggingEnabled(bool enable);
bool getFsrEnabled();
void setFsrEnabled(bool enable);
bool getRcasEnabled();
void setRcasEnabled(bool enable);
int getRcasAttenuation();
void setRcasAttenuation(int value);

// TODO
bool GetLoadGameSizeEnabled();
std::filesystem::path GetSaveDataPath();
void setLoadGameSizeEnabled(bool enable);
bool getCompatibilityEnabled();
bool getCheckCompatibilityOnStartup();
bool getIsConnectedToNetwork();
std::string getUserName();
std::string getChooseHomeTab();
bool GetUseUnifiedInputConfig();
void SetUseUnifiedInputConfig(bool use);
bool GetOverrideControllerColor();
void SetOverrideControllerColor(bool enable);
int* GetControllerCustomColor();
void SetControllerCustomColor(int r, int b, int g);
void setUserName(const std::string& type);
void setChooseHomeTab(const std::string& type);
void setGameInstallDirs(const std::vector<std::filesystem::path>& dirs_config);
void setAllGameInstallDirs(const std::vector<GameInstallDir>& dirs_config);
void setSaveDataPath(const std::filesystem::path& path);
void setCompatibilityEnabled(bool use);
void setCheckCompatibilityOnStartup(bool use);
// Gui
bool addGameInstallDir(const std::filesystem::path& dir, bool enabled = true);
void removeGameInstallDir(const std::filesystem::path& dir);
void setGameInstallDirEnabled(const std::filesystem::path& dir, bool enabled);
void setAddonInstallDir(const std::filesystem::path& dir);

const std::vector<std::filesystem::path> getGameInstallDirs();
const std::vector<bool> getGameInstallDirsEnabled();
std::filesystem::path getAddonInstallDir();

void setDefaultValues();

constexpr std::string_view GetDefaultGlobalConfig();
std::filesystem::path GetFoolproofInputConfigFile(const std::string& game_id = "");

}; // namespace Config
