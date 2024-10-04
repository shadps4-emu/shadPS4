// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <vector>
#include "types.h"

namespace Config {
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
bool isFullscreenMode();
bool getPlayBGM();
int getBGMvolume();

std::string getLogFilter();
std::string getLogType();
std::string getUserName();
std::string getUpdateChannel();

bool getUseSpecialPad();
int getSpecialPadClass();

u32 getScreenWidth();
u32 getScreenHeight();
s32 getGpuId();

bool debugDump();
bool showSplash();
bool autoUpdate();
bool nullGpu();
bool copyGPUCmdBuffers();
bool dumpShaders();
bool isRdocEnabled();
u32 vblankDiv();

void setDebugDump(bool enable);
void setShowSplash(bool enable);
void setAutoUpdate(bool enable);
void setNullGpu(bool enable);
void setCopyGPUCmdBuffers(bool enable);
void setDumpShaders(bool enable);
void setVblankDiv(u32 value);
void setGpuId(s32 selectedGpuId);
void setScreenWidth(u32 width);
void setScreenHeight(u32 height);
void setFullscreenMode(bool enable);
void setPlayBGM(bool enable);
void setBGMvolume(int volume);
void setLanguage(u32 language);
void setNeoMode(bool enable);
void setUserName(const std::string& type);
void setUpdateChannel(const std::string& type);

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
bool vkMarkersEnabled();
bool vkCrashDiagnosticEnabled();

// Gui
void setMainWindowGeometry(u32 x, u32 y, u32 w, u32 h);
void setGameInstallDir(const std::filesystem::path& dir);
void setAddonInstallDir(const std::filesystem::path& dir);
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
std::filesystem::path getGameInstallDir();
std::filesystem::path getAddonInstallDir();
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
}; // namespace Config
