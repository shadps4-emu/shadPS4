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
std::string getLogFilter();
std::string getLogType();
std::string getUserName();

u32 getScreenWidth();
u32 getScreenHeight();
s32 getGpuId();

bool debugDump();
bool isLleLibc();
bool showSplash();
bool nullGpu();
bool dumpShaders();
bool dumpPM4();
bool isRdocEnabled();
bool isMarkersEnabled();
u32 vblankDiv();

void setDebugDump(bool enable);
void setShowSplash(bool enable);
void setNullGpu(bool enable);
void setDumpShaders(bool enable);
void setDumpPM4(bool enable);
void setVblankDiv(u32 value);
void setGpuId(s32 selectedGpuId);
void setScreenWidth(u32 width);
void setScreenHeight(u32 height);
void setFullscreenMode(bool enable);
void setLanguage(u32 language);
void setNeoMode(bool enable);
void setUserName(std::string type);

void setLogType(std::string type);
void setLogFilter(std::string type);

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
void setSliderPositon(u32 pos);
void setSliderPositonGrid(u32 pos);
void setTableMode(u32 mode);
void setMainWindowWidth(u32 width);
void setMainWindowHeight(u32 height);
void setPkgViewer(std::vector<std::string> pkgList);
void setElfViewer(std::vector<std::string> elfList);
void setRecentFiles(std::vector<std::string> recentFiles);

u32 getMainWindowGeometryX();
u32 getMainWindowGeometryY();
u32 getMainWindowGeometryW();
u32 getMainWindowGeometryH();
std::string getGameInstallDir();
u32 getMainWindowTheme();
u32 getIconSize();
u32 getIconSizeGrid();
u32 getSliderPositon();
u32 getSliderPositonGrid();
u32 getTableMode();
u32 getMainWindowWidth();
u32 getMainWindowHeight();
std::vector<std::string> getPkgViewer();
std::vector<std::string> getElfViewer();
std::vector<std::string> getRecentFiles();

void setDefaultValues();

// settings
u32 GetLanguage();
}; // namespace Config
