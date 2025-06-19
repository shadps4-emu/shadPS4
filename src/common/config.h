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

void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);
void saveMainWindow(const std::filesystem::path& path);

std::string getTrophyKey();
void setTrophyKey(std::string key);
bool GetLoadGameSizeEnabled();
std::filesystem::path GetSaveDataPath();
void setLoadGameSizeEnabled(bool enable);
bool getIsFullscreen();
std::string getFullscreenMode();
bool isNeoModeConsole();
bool isDevKitConsole();
bool getisTrophyPopupDisabled();
bool getEnableDiscordRPC();
bool getCompatibilityEnabled();
bool getCheckCompatibilityOnStartup();
bool getPSNSignedIn();

std::string getLogFilter();
std::string getLogType();
std::string getUserName();
std::string getChooseHomeTab();

s16 getCursorState();
int getCursorHideTimeout();
double getTrophyNotificationDuration();
std::string getBackButtonBehavior();
bool getUseSpecialPad();
int getSpecialPadClass();
bool getIsMotionControlsEnabled();
bool GetUseUnifiedInputConfig();
void SetUseUnifiedInputConfig(bool use);
bool GetOverrideControllerColor();
void SetOverrideControllerColor(bool enable);
int* GetControllerCustomColor();
void SetControllerCustomColor(int r, int b, int g);

u32 getScreenWidth();
u32 getScreenHeight();
s32 getGpuId();
bool allowHDR();

bool debugDump();
bool collectShadersForDebug();
bool showSplash();
std::string sideTrophy();
bool nullGpu();
bool copyGPUCmdBuffers();
bool dumpShaders();
bool patchShaders();
bool isRdocEnabled();
bool fpsColor();
u32 vblankDiv();

void setDebugDump(bool enable);
void setCollectShaderForDebug(bool enable);
void setShowSplash(bool enable);
void setSideTrophy(std::string side);
void setNullGpu(bool enable);
void setAllowHDR(bool enable);
void setCopyGPUCmdBuffers(bool enable);
void setDumpShaders(bool enable);
void setVblankDiv(u32 value);
void setGpuId(s32 selectedGpuId);
void setScreenWidth(u32 width);
void setScreenHeight(u32 height);
void setIsFullscreen(bool enable);
void setFullscreenMode(std::string mode);
void setisTrophyPopupDisabled(bool disable);
void setEnableDiscordRPC(bool enable);
void setLanguage(u32 language);
void setNeoMode(bool enable);
void setUserName(const std::string& type);
void setChooseHomeTab(const std::string& type);
void setGameInstallDirs(const std::vector<std::filesystem::path>& dirs_config);
void setAllGameInstallDirs(const std::vector<GameInstallDir>& dirs_config);
void setSaveDataPath(const std::filesystem::path& path);
void setCompatibilityEnabled(bool use);
void setCheckCompatibilityOnStartup(bool use);
void setPSNSignedIn(bool sign);

void setCursorState(s16 cursorState);
void setCursorHideTimeout(int newcursorHideTimeout);
void setTrophyNotificationDuration(double newTrophyNotificationDuration);
void setBackButtonBehavior(const std::string& type);
void setUseSpecialPad(bool use);
void setSpecialPadClass(int type);
void setIsMotionControlsEnabled(bool use);

void setLogType(const std::string& type);
void setLogFilter(const std::string& type);
void setSeparateLogFilesEnabled(bool enabled);
bool getSeparateLogFilesEnabled();
void setVkValidation(bool enable);
void setVkSyncValidation(bool enable);
void setRdocEnabled(bool enable);

bool vkValidationEnabled();
bool vkValidationSyncEnabled();
bool vkValidationGpuEnabled();
bool getVkCrashDiagnosticEnabled();
bool getVkHostMarkersEnabled();
bool getVkGuestMarkersEnabled();
void setVkCrashDiagnosticEnabled(bool enable);
void setVkHostMarkersEnabled(bool enable);
void setVkGuestMarkersEnabled(bool enable);

// Gui
bool addGameInstallDir(const std::filesystem::path& dir, bool enabled = true);
void removeGameInstallDir(const std::filesystem::path& dir);
void setGameInstallDirEnabled(const std::filesystem::path& dir, bool enabled);
void setAddonInstallDir(const std::filesystem::path& dir);
void setMainWindowTheme(u32 theme);
void setElfViewer(const std::vector<std::string>& elfList);

const std::vector<std::filesystem::path> getGameInstallDirs();
const std::vector<bool> getGameInstallDirsEnabled();
std::filesystem::path getAddonInstallDir();
u32 getMainWindowTheme();
std::vector<std::string> getElfViewer();

void setDefaultValues();

// todo: name and function location pending
std::filesystem::path GetFoolproofKbmConfigFile(const std::string& game_id = "");

// settings
u32 GetLanguage();
}; // namespace Config
