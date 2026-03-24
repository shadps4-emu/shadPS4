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

TEST(SettingTest, GetDefault_PrefersGameSpecificOverBase) {
	Setting<int> s{ 10 };
	s.value = 20;
	s.game_specific_value = 99;
	EXPECT_EQ(s.get(ConfigMode::Default), 99);
}

TEST(SettingTest, GetDefault_FallsBackToBaseWhenNoOverride) {
	Setting<int> s{ 10 };
	s.value = 20;
	EXPECT_EQ(s.get(ConfigMode::Default), 20);
}

TEST(SettingTest, GetGlobal_IgnoresGameSpecific) {
	Setting<int> s{ 10 };
	s.value = 20;
	s.game_specific_value = 99;
	EXPECT_EQ(s.get(ConfigMode::Global), 20);
}

TEST(SettingTest, GetClean_AlwaysReturnsFactoryDefault) {
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

TEST(SettingTest, BoolSetting_AllModes) {
	Setting<bool> s{ false };
	s.value = true;
	s.game_specific_value = false;
	EXPECT_FALSE(s.get(ConfigMode::Default));
	EXPECT_TRUE(s.get(ConfigMode::Global));
	EXPECT_FALSE(s.get(ConfigMode::Clean));
}

TEST(SettingTest, StringSetting_AllModes) {
	Setting<std::string> s{ "shadow" };
	s.value = "rule";
	s.game_specific_value = "override";
	EXPECT_EQ(s.get(ConfigMode::Default), "override");
	EXPECT_EQ(s.get(ConfigMode::Global), "rule");
	EXPECT_EQ(s.get(ConfigMode::Clean), "shadow");
}

TEST(SettingTest, NoGameSpecific_DefaultAndGlobalAgree) {
	Setting<int> s{ 7 };
	s.value = 7;
	EXPECT_EQ(s.get(ConfigMode::Default), s.get(ConfigMode::Global));
}