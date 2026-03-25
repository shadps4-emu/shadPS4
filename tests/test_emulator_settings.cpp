// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "common/path_util.h"
#include "common/scm_rev.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"

namespace fs = std::filesystem;
using json = nlohmann::json;


class TempDir {
public:
	TempDir() {
		auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
		temp_path = fs::temp_directory_path() /
			("shadps4_test_" + std::to_string(ns) + "_" +
				std::to_string(reinterpret_cast<uintptr_t>(this)));
		fs::create_directories(temp_path);
	}
	~TempDir() {
		std::error_code ec;
		fs::remove_all(temp_path, ec);
	}
	const fs::path& path() const { return temp_path; }

private:
	fs::path temp_path;
};

static void WriteJson(const fs::path& p, const json& j) {
	std::ofstream out(p);
	ASSERT_TRUE(out.is_open()) << "Cannot write: " << p;
	out << std::setw(2) << j;
}

static json ReadJson(const fs::path& p) {
	std::ifstream in(p);
	EXPECT_TRUE(in.is_open()) << "Cannot read: " << p;
	json j;
	in >> j;
	return j;
}

class EmulatorSettingsTest : public ::testing::Test {
protected:
	void SetUp() override {
		temp_dir = std::make_unique<TempDir>();
		const fs::path root = temp_dir->path();

		using PT = Common::FS::PathType;
		const struct {
			PT type;
			const char* sub;
		} dirs[] = {
			{PT::UserDir,       ""},
			{PT::LogDir,        "log"},
			{PT::ScreenshotsDir,"screenshots"},
			{PT::ShaderDir,     "shader"},
			{PT::GameDataDir,   "data"},
			{PT::TempDataDir,   "temp"},
			{PT::SysModuleDir,  "sys_modules"},
			{PT::DownloadDir,   "download"},
			{PT::CapturesDir,   "captures"},
			{PT::CheatsDir,     "cheats"},
			{PT::PatchesDir,    "patches"},
			{PT::MetaDataDir,   "game_data"},
			{PT::CustomTrophy,  "custom_trophy"},
			{PT::CustomConfigs, "custom_configs"},
			{PT::CacheDir,      "cache"},
			{PT::FontsDir,      "fonts"},
			{PT::HomeDir,       "home"},
		};
		for (const auto& d : dirs) {
			fs::path p = d.sub[0] ? (root / d.sub) : root;
			fs::create_directories(p);
			Common::FS::SetUserPath(d.type, p);
		}

		temp_state = std::make_shared<EmulatorState>();
		EmulatorState::SetInstance(temp_state);

		temp_settings = std::make_shared<EmulatorSettingsImpl>();
		EmulatorSettingsImpl::SetInstance(temp_settings);
	}

	void TearDown() override {
		EmulatorSettingsImpl::SetInstance(nullptr);
		EmulatorState::SetInstance(nullptr);
		temp_settings.reset();
		temp_state.reset();
		temp_dir.reset();
	}

	fs::path ConfigJson() const {
		return temp_dir->path() / "config.json";
	}
	fs::path GameConfig(const std::string& serial) const {
		return temp_dir->path() / "custom_configs" / (serial + ".json");
	}

	std::unique_ptr<TempDir>              temp_dir;
	std::shared_ptr<EmulatorSettingsImpl> temp_settings;
	std::shared_ptr<EmulatorState>        temp_state;
};

//tests Settting<T> template , default , Global override modes

TEST(SettingTest, DefaultCtorZeroInitialises) {
	Setting<int> s;
	EXPECT_EQ(s.value, 0);
	EXPECT_EQ(s.default_value, 0);
	EXPECT_FALSE(s.game_specific_value.has_value());
}

TEST(SettingTest, ValueCtorSetsBothValueAndDefault) {
	Setting<int> s{ 42 };
	EXPECT_EQ(s.value, 42);
	EXPECT_EQ(s.default_value, 42);
}

TEST(SettingTest, GetDefaultPrefersGameSpecificOverBase) {
	Setting<int> s{ 10 };
	s.value = 20;
	s.game_specific_value = 99;
	EXPECT_EQ(s.get(ConfigMode::Default), 99);
}

TEST(SettingTest, GetDefaultFallsBackToBaseWhenNoOverride) {
	Setting<int> s{ 10 };
	s.value = 20;
	EXPECT_EQ(s.get(ConfigMode::Default), 20);
}

TEST(SettingTest, GetGlobalIgnoresGameSpecific) {
	Setting<int> s{ 10 };
	s.value = 20;
	s.game_specific_value = 99;
	EXPECT_EQ(s.get(ConfigMode::Global), 20);
}

TEST(SettingTest, GetCleanAlwaysReturnsFactoryDefault) {
	Setting<int> s{ 10 };
	s.value = 20;
	s.game_specific_value = 99;
	EXPECT_EQ(s.get(ConfigMode::Clean), 10);
}

TEST(SettingTest, SetWritesToBaseOnly) {
	Setting<int> s{ 0 };
	s.game_specific_value = 55;
	s.set(77);
	EXPECT_EQ(s.value, 77);
	EXPECT_EQ(s.game_specific_value.value(), 55); // override untouched
}

TEST(SettingTest, ResetGameSpecificClearsOverride) {
	Setting<int> s{ 0 };
	s.game_specific_value = 55;
	s.reset_game_specific();
	EXPECT_FALSE(s.game_specific_value.has_value());
	// base and default must be intact
	EXPECT_EQ(s.value, 0);
	EXPECT_EQ(s.default_value, 0);
}

TEST(SettingTest, BoolSettingAllModes) {
	Setting<bool> s{ false };
	s.value = true;
	s.game_specific_value = false;
	EXPECT_FALSE(s.get(ConfigMode::Default));
	EXPECT_TRUE(s.get(ConfigMode::Global));
	EXPECT_FALSE(s.get(ConfigMode::Clean));
}

TEST(SettingTest, StringSettingAllModes) {
	Setting<std::string> s{ "shadow" };
	s.value = "rule";
	s.game_specific_value = "override";
	EXPECT_EQ(s.get(ConfigMode::Default), "override");
	EXPECT_EQ(s.get(ConfigMode::Global), "rule");
	EXPECT_EQ(s.get(ConfigMode::Clean), "shadow");
}

TEST(SettingTest, NoGameSpecificDefaultAndGlobalAgree) {
	Setting<int> s{ 7 };
	s.value = 7;
	EXPECT_EQ(s.get(ConfigMode::Default), s.get(ConfigMode::Global));
}

//tests for default settings

TEST_F(EmulatorSettingsTest, SetDefaultValuesResetsAllGroupsToFactory) {
	//set random values
	temp_settings->SetNeo(true);
	temp_settings->SetWindowWidth(3840u);
	temp_settings->SetGpuId(2);
	temp_settings->SetDebugDump(true);
	temp_settings->SetCursorState(HideCursorState::Always);

	temp_settings->SetDefaultValues();//reset to defaults
	//check if values are reset to defaults
	EXPECT_FALSE(temp_settings->IsNeo());
	EXPECT_EQ(temp_settings->GetWindowWidth(), 1280u);
	EXPECT_EQ(temp_settings->GetGpuId(), -1);
	EXPECT_FALSE(temp_settings->IsDebugDump());
	EXPECT_EQ(temp_settings->GetCursorState(), static_cast<int>(HideCursorState::Idle));
}

TEST_F(EmulatorSettingsTest, SetDefaultValuesClearsGameSpecificOverrides) {
	//check that game-specific overrides are cleared by SetDefaultValues
	json game;
	game["General"]["neo_mode"] = true;
	WriteJson(GameConfig("CUSA00001"), game);
	temp_settings->Load("CUSA00001");

	temp_settings->SetDefaultValues();
	temp_settings->SetConfigMode(ConfigMode::Default);

	EXPECT_FALSE(temp_settings->IsNeo());//default is false should be loaded instead of override
}