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
        temp_path = fs::temp_directory_path() / ("shadps4_test_" + std::to_string(ns) + "_" +
                                                 std::to_string(reinterpret_cast<uintptr_t>(this)));
        fs::create_directories(temp_path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(temp_path, ec);
    }
    const fs::path& path() const {
        return temp_path;
    }

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

class TestWrapper : public ::testing::Test {
protected:
    TestWrapper() {
        Common::Log::Setup("shad_test.log");
    }
};

class EmulatorSettingsTest : public TestWrapper {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TempDir>();
        const fs::path root = temp_dir->path();

        using PT = Common::FS::PathType;
        const struct {
            PT type;
            const char* sub;
        } dirs[] = {
            {PT::UserDir, ""},
            {PT::LogDir, "log"},
            {PT::ScreenshotsDir, "screenshots"},
            {PT::ShaderDir, "shader"},
            {PT::GameDataDir, "data"},
            {PT::TempDataDir, "temp"},
            {PT::SysModuleDir, "sys_modules"},
            {PT::DownloadDir, "download"},
            {PT::CapturesDir, "captures"},
            {PT::CheatsDir, "cheats"},
            {PT::PatchesDir, "patches"},
            {PT::MetaDataDir, "game_data"},
            {PT::CustomTrophy, "custom_trophy"},
            {PT::CustomConfigs, "custom_configs"},
            {PT::CacheDir, "cache"},
            {PT::FontsDir, "fonts"},
            {PT::HomeDir, "home"},
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

    std::unique_ptr<TempDir> temp_dir;
    std::shared_ptr<EmulatorSettingsImpl> temp_settings;
    std::shared_ptr<EmulatorState> temp_state;
};

// tests Settting<T> template , default , Global override modes

TEST(SettingTest, DefaultCtorZeroInitialises) {
    Setting<int> s;
    EXPECT_EQ(s.value, 0);
    EXPECT_EQ(s.default_value, 0);
    EXPECT_FALSE(s.game_specific_value.has_value());
}

TEST(SettingTest, ValueCtorSetsBothValueAndDefault) {
    Setting<int> s{42};
    EXPECT_EQ(s.value, 42);
    EXPECT_EQ(s.default_value, 42);
}

TEST(SettingTest, GetDefaultPrefersGameSpecificOverBase) {
    Setting<int> s{10};
    s.value = 20;
    s.game_specific_value = 99;
    EXPECT_EQ(s.get(ConfigMode::Default), 99);
}

TEST(SettingTest, GetDefaultFallsBackToBaseWhenNoOverride) {
    Setting<int> s{10};
    s.value = 20;
    EXPECT_EQ(s.get(ConfigMode::Default), 20);
}

TEST(SettingTest, GetGlobalIgnoresGameSpecific) {
    Setting<int> s{10};
    s.value = 20;
    s.game_specific_value = 99;
    EXPECT_EQ(s.get(ConfigMode::Global), 20);
}

TEST(SettingTest, GetCleanAlwaysReturnsFactoryDefault) {
    Setting<int> s{10};
    s.value = 20;
    s.game_specific_value = 99;
    EXPECT_EQ(s.get(ConfigMode::Clean), 10);
}

TEST(SettingTest, SetWritesToBaseOnly) {
    Setting<int> s{0};
    s.game_specific_value = 55;
    s.set(77);
    EXPECT_EQ(s.value, 77);
    EXPECT_EQ(s.game_specific_value.value(), 55); // override untouched
}

TEST(SettingTest, ResetGameSpecificClearsOverride) {
    Setting<int> s{0};
    s.game_specific_value = 55;
    s.reset_game_specific();
    EXPECT_FALSE(s.game_specific_value.has_value());
    // base and default must be intact
    EXPECT_EQ(s.value, 0);
    EXPECT_EQ(s.default_value, 0);
}

TEST(SettingTest, BoolSettingAllModes) {
    Setting<bool> s{false};
    s.value = true;
    s.game_specific_value = false;
    EXPECT_FALSE(s.get(ConfigMode::Default));
    EXPECT_TRUE(s.get(ConfigMode::Global));
    EXPECT_FALSE(s.get(ConfigMode::Clean));
}

TEST(SettingTest, StringSettingAllModes) {
    Setting<std::string> s{"shadow"};
    s.value = "rule";
    s.game_specific_value = "override";
    EXPECT_EQ(s.get(ConfigMode::Default), "override");
    EXPECT_EQ(s.get(ConfigMode::Global), "rule");
    EXPECT_EQ(s.get(ConfigMode::Clean), "shadow");
}

TEST(SettingTest, NoGameSpecificDefaultAndGlobalAgree) {
    Setting<int> s{7};
    s.value = 7;
    EXPECT_EQ(s.get(ConfigMode::Default), s.get(ConfigMode::Global));
}

// tests for default settings

TEST_F(EmulatorSettingsTest, SetDefaultValuesResetsAllGroupsToFactory) {
    // set random values
    temp_settings->SetNeo(true);
    temp_settings->SetWindowWidth(3840u);
    temp_settings->SetGpuId(2);
    temp_settings->SetDebugDump(true);
    temp_settings->SetCursorState(HideCursorState::Always);

    temp_settings->SetDefaultValues(); // reset to defaults
    // check if values are reset to defaults
    EXPECT_FALSE(temp_settings->IsNeo());
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280u);
    EXPECT_EQ(temp_settings->GetGpuId(), -1);
    EXPECT_FALSE(temp_settings->IsDebugDump());
    EXPECT_EQ(temp_settings->GetCursorState(), static_cast<int>(HideCursorState::Idle));
}

TEST_F(EmulatorSettingsTest, SetDefaultValuesClearsGameSpecificOverrides) {
    // check that game-specific overrides are cleared by SetDefaultValues
    json game;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA00001"), game);
    temp_settings->Load("CUSA00001");

    temp_settings->SetDefaultValues();
    temp_settings->SetConfigMode(ConfigMode::Default);

    EXPECT_FALSE(temp_settings->IsNeo()); // default is false should be loaded instead of override
}

// configModes tests

TEST_F(EmulatorSettingsTest, ConfigModeSetAndGetRoundTrips) {
    temp_settings->SetConfigMode(ConfigMode::Clean);
    EXPECT_EQ(temp_settings->GetConfigMode(), ConfigMode::Clean);
    temp_settings->SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(temp_settings->GetConfigMode(), ConfigMode::Global);
    temp_settings->SetConfigMode(ConfigMode::Default);
    EXPECT_EQ(temp_settings->GetConfigMode(), ConfigMode::Default);
}

TEST_F(EmulatorSettingsTest, ConfigModeCleanReturnFactoryDefaults) {
    temp_settings->SetWindowWidth(3840u);
    json game;
    game["GPU"]["window_width"] = 2560;
    WriteJson(GameConfig("CUSA00001"), game);
    temp_settings->Load("CUSA00001");

    temp_settings->SetConfigMode(ConfigMode::Clean);
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280); // factory default
}

TEST_F(EmulatorSettingsTest, ConfigModeGlobalIgnoresGameSpecific) {
    temp_settings->SetNeo(false);
    json game;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA00001"), game);
    temp_settings->Load("CUSA00001");

    temp_settings->SetConfigMode(ConfigMode::Global);
    EXPECT_FALSE(temp_settings->IsNeo());
}

TEST_F(EmulatorSettingsTest, ConfigModeDefaultResolvesGameSpecificWhenPresent) {
    temp_settings->SetNeo(false);
    json game;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA00001"), game);
    temp_settings->Load("CUSA00001");

    temp_settings->SetConfigMode(ConfigMode::Default);
    EXPECT_TRUE(temp_settings->IsNeo());
}

// tests for global config.json file

TEST_F(EmulatorSettingsTest, SaveCreatesConfigJson) {
    ASSERT_TRUE(temp_settings->Save());
    EXPECT_TRUE(fs::exists(ConfigJson()));
}

TEST_F(EmulatorSettingsTest, SaveWritesAllExpectedSections) {
    ASSERT_TRUE(temp_settings->Save());
    json j = ReadJson(ConfigJson());
    for (const char* section : {"General", "Log", "Debug", "Input", "Audio", "GPU", "Vulkan"})
        EXPECT_TRUE(j.contains(section)) << "Missing section: " << section;
}

TEST_F(EmulatorSettingsTest, LoadReturnsTrueForExistingFile) {
    temp_settings->Save();
    auto fresh = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(fresh);
    EXPECT_TRUE(fresh->Load());
}

TEST_F(EmulatorSettingsTest, RoundTripAllGroups) {
    temp_settings->SetNeo(true);
    temp_settings->SetDebugDump(true);
    temp_settings->SetWindowWidth(1920u);
    temp_settings->SetGpuId(1);
    temp_settings->SetCursorState(HideCursorState::Always);
    temp_settings->SetAudioBackend(AudioBackend::OpenAL);
    temp_settings->Save();

    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load();
    EXPECT_TRUE(f->IsNeo());
    EXPECT_TRUE(f->IsDebugDump());
    EXPECT_EQ(f->GetWindowWidth(), 1920u);
    EXPECT_EQ(f->GetGpuId(), 1);
    EXPECT_EQ(f->GetCursorState(), static_cast<int>(HideCursorState::Always));
    EXPECT_EQ(f->GetAudioBackend(), static_cast<u32>(AudioBackend::OpenAL));
}

TEST_F(EmulatorSettingsTest, LoadMissingFileCreatesDefaultsOnDisk) {
    ASSERT_FALSE(fs::exists(ConfigJson()));
    temp_settings->Load();
    EXPECT_TRUE(fs::exists(ConfigJson()));
    EXPECT_FALSE(temp_settings->IsNeo()); // defaults
}

TEST_F(EmulatorSettingsTest, LoadMissingSectionDoesNotZeroOtherSections) {
    temp_settings->SetNeo(true);
    temp_settings->Save();
    json j = ReadJson(ConfigJson());
    j.erase("GPU");
    WriteJson(ConfigJson(), j);

    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load();

    EXPECT_TRUE(f->IsNeo());              // belongs to General, should be loaded
    EXPECT_EQ(f->GetWindowWidth(), 1280); // GPU fell back to default
}

TEST_F(EmulatorSettingsTest, LoadPreservesUnknownKeysOnResave) {
    temp_settings->Save();
    json j = ReadJson(ConfigJson());
    j["General"]["future_feature"] = "preserved";
    WriteJson(ConfigJson(), j);

    // A fresh load + save (triggered by version mismatch) must keep the key
    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load();
    f->Save();

    json after = ReadJson(ConfigJson());
    EXPECT_EQ(after["General"]["future_feature"], "preserved");
}

TEST_F(EmulatorSettingsTest, LoadUnknownTopLevelSectionPreserved) {
    temp_settings->Save();
    json j = ReadJson(ConfigJson());
    j["FutureSection"]["key"] = 42;
    WriteJson(ConfigJson(), j);

    temp_settings->SetNeo(true);
    temp_settings->Save(); // merge path

    json after = ReadJson(ConfigJson());
    EXPECT_TRUE(after.contains("FutureSection"));
    EXPECT_EQ(after["FutureSection"]["key"], 42);
}

TEST_F(EmulatorSettingsTest, LoadCorruptJsonDoesNotCrash) {
    {
        std::ofstream out(ConfigJson());
        out << "{NOT VALID JSON!!!";
    }
    EXPECT_NO_THROW(temp_settings->Load());
}

TEST_F(EmulatorSettingsTest, LoadEmptyJsonObjectDoesNotCrash) {
    WriteJson(ConfigJson(), json::object());
    EXPECT_NO_THROW(temp_settings->Load());
}

// tests for per game config

TEST_F(EmulatorSettingsTest, SaveSerialCreatesPerGameFile) {
    ASSERT_TRUE(temp_settings->Save("CUSA01234"));
    EXPECT_TRUE(fs::exists(GameConfig("CUSA01234")));
}

TEST_F(EmulatorSettingsTest, LoadSerialReturnsFalseWhenFileAbsent) {
    EXPECT_FALSE(temp_settings->Load("CUSA99999"));
}

TEST_F(EmulatorSettingsTest, LoadSerialAppliesOverrideToGameSpecificValue) {
    temp_settings->SetNeo(false);
    json game;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA01234"), game);

    ASSERT_TRUE(temp_settings->Load("CUSA01234"));
    temp_settings->SetConfigMode(ConfigMode::Default);
    EXPECT_TRUE(temp_settings->IsNeo());
}

TEST_F(EmulatorSettingsTest, LoadSerialBaseValueUntouched) {
    temp_settings->SetWindowWidth(1280);
    json game;
    game["GPU"]["window_width"] = 3840;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280);
}

TEST_F(EmulatorSettingsTest, LoadSerialOverridesMultipleGroups) {
    temp_settings->SetNeo(false);
    temp_settings->SetWindowWidth(1280u);
    temp_settings->SetDebugDump(false);

    json game;
    game["General"]["neo_mode"] = true;
    game["GPU"]["window_width"] = 3840;
    game["Debug"]["debug_dump"] = true;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->SetConfigMode(ConfigMode::Default);
    EXPECT_TRUE(temp_settings->IsNeo());
    EXPECT_EQ(temp_settings->GetWindowWidth(), 3840);
    EXPECT_TRUE(temp_settings->IsDebugDump());
}

TEST_F(EmulatorSettingsTest, LoadSerialUnrecognisedKeyIgnored) {
    json game;
    game["GPU"]["key_that_does_not_exist"] = 999;
    WriteJson(GameConfig("CUSA01234"), game);
    EXPECT_NO_THROW(temp_settings->Load("CUSA01234"));
}

TEST_F(EmulatorSettingsTest, LoadSerialTypeMismatch_DoesNotCrash) {
    json game;
    game["GPU"]["window_width"] = "not_a_number";
    WriteJson(GameConfig("CUSA01234"), game);
    EXPECT_NO_THROW(temp_settings->Load("CUSA01234"));
    // base unchanged
    temp_settings->SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280u);
}

TEST_F(EmulatorSettingsTest, LoadSerialCorruptFileDoesNotCrash) {
    {
        std::ofstream out(GameConfig("CUSA01234"));
        out << "{{{{totally broken";
    }
    EXPECT_NO_THROW(temp_settings->Load("CUSA01234"));
}

TEST_F(EmulatorSettingsTest, SaveSerialWritesGameSpecificValueWhenOverrideLoaded) {
    temp_settings->SetWindowWidth(1280);
    json game;
    game["GPU"]["window_width"] = 3840;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->Save("CUSA01234");

    json saved = ReadJson(GameConfig("CUSA01234"));
    EXPECT_EQ(saved["GPU"]["window_width"].get<unsigned>(), 3840);
}

TEST_F(EmulatorSettingsTest, SaveSerialWritesBaseValueWhenNoOverrideSet) {
    temp_settings->SetWindowWidth(2560);
    temp_settings->Save("CUSA01234");

    json saved = ReadJson(GameConfig("CUSA01234"));
    EXPECT_EQ(saved["GPU"]["window_width"].get<unsigned>(), 2560);
}

TEST_F(EmulatorSettingsTest, MultipleSerialsDoNotInterfere) {
    json g1;
    g1["General"]["neo_mode"] = true;
    g1["GPU"]["window_width"] = 3840;
    WriteJson(GameConfig("CUSA00001"), g1);

    json g2;
    g2["General"]["neo_mode"] = false;
    g2["GPU"]["window_width"] = 1920;
    WriteJson(GameConfig("CUSA00002"), g2);

    {
        auto s = std::make_shared<EmulatorSettingsImpl>();
        EmulatorSettingsImpl::SetInstance(s);
        s->Load();
        s->Load("CUSA00001");
        s->SetConfigMode(ConfigMode::Default);
        EXPECT_TRUE(s->IsNeo());
        EXPECT_EQ(s->GetWindowWidth(), 3840);
    }
    {
        auto s = std::make_shared<EmulatorSettingsImpl>();
        EmulatorSettingsImpl::SetInstance(s);
        s->Load();
        s->Load("CUSA00002");
        s->SetConfigMode(ConfigMode::Default);
        EXPECT_FALSE(s->IsNeo());
        EXPECT_EQ(s->GetWindowWidth(), 1920);
    }
}

// ClearGameSpecificOverrides tests

TEST_F(EmulatorSettingsTest, ClearGameSpecificOverridesRemovesAllGroups) {
    json game;
    game["General"]["neo_mode"] = true;
    game["GPU"]["window_width"] = 3840;
    game["Debug"]["debug_dump"] = true;
    game["Input"]["cursor_state"] = 2;
    game["Audio"]["audio_backend"] = 1;
    game["Vulkan"]["gpu_id"] = 2;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->ClearGameSpecificOverrides();
    temp_settings->SetConfigMode(ConfigMode::Default);

    EXPECT_FALSE(temp_settings->IsNeo());
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280);
    EXPECT_FALSE(temp_settings->IsDebugDump());
    EXPECT_EQ(temp_settings->GetCursorState(), static_cast<int>(HideCursorState::Idle));
    EXPECT_EQ(temp_settings->GetGpuId(), -1);
}

TEST_F(EmulatorSettingsTest, ClearGameSpecificOverridesDoesNotTouchBaseValues) {
    temp_settings->SetWindowWidth(1920);
    json game;
    game["GPU"]["window_width"] = 3840;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->ClearGameSpecificOverrides();

    temp_settings->SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1920);
}

TEST_F(EmulatorSettingsTest, ClearGameSpecificOverrides_NoopWhenNothingLoaded) {
    EXPECT_NO_THROW(temp_settings->ClearGameSpecificOverrides());
}

// ResetGameSpecificValue tests

TEST_F(EmulatorSettingsTest, ResetGameSpecificValue_ClearsNamedKey) {
    temp_settings->SetWindowWidth(1280);
    json game;
    game["GPU"]["window_width"] = 3840;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->SetConfigMode(ConfigMode::Default);
    ASSERT_EQ(temp_settings->GetWindowWidth(), 3840);

    temp_settings->ResetGameSpecificValue("window_width");
    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280);
}

TEST_F(EmulatorSettingsTest, ResetGameSpecificValueOnlyAffectsTargetKey) {
    json game;
    game["GPU"]["window_width"] = 3840;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");

    temp_settings->ResetGameSpecificValue("window_width");
    temp_settings->SetConfigMode(ConfigMode::Default);

    EXPECT_EQ(temp_settings->GetWindowWidth(), 1280); // cleared
    EXPECT_TRUE(temp_settings->IsNeo());              // still set
}

TEST_F(EmulatorSettingsTest, ResetGameSpecificValueUnknownKeyNoOp) {
    EXPECT_NO_THROW(temp_settings->ResetGameSpecificValue("does_not_exist"));
}

// GameInstallDir tests

TEST_F(EmulatorSettingsTest, AddGameInstallDirAddsEnabled) {
    fs::path dir = temp_dir->path() / "games";
    fs::create_directories(dir);
    EXPECT_TRUE(temp_settings->AddGameInstallDir(dir));
    ASSERT_EQ(temp_settings->GetGameInstallDirs().size(), 1u);
    EXPECT_EQ(temp_settings->GetGameInstallDirs()[0], dir);
}

TEST_F(EmulatorSettingsTest, AddGameInstallDirRejectsDuplicate) {
    fs::path dir = temp_dir->path() / "games";
    fs::create_directories(dir);
    temp_settings->AddGameInstallDir(dir);
    EXPECT_FALSE(temp_settings->AddGameInstallDir(dir));
    EXPECT_EQ(temp_settings->GetGameInstallDirs().size(), 1u);
}

TEST_F(EmulatorSettingsTest, RemoveGameInstallDirRemovesEntry) {
    fs::path dir = temp_dir->path() / "games";
    fs::create_directories(dir);
    temp_settings->AddGameInstallDir(dir);
    temp_settings->RemoveGameInstallDir(dir);
    EXPECT_TRUE(temp_settings->GetGameInstallDirs().empty());
}

TEST_F(EmulatorSettingsTest, RemoveGameInstallDirNoopForMissing) {
    EXPECT_NO_THROW(temp_settings->RemoveGameInstallDir("/nonexistent/path"));
}

TEST_F(EmulatorSettingsTest, SetGameInstallDirEnabledDisablesDir) {
    fs::path dir = temp_dir->path() / "games";
    fs::create_directories(dir);
    temp_settings->AddGameInstallDir(dir, true);
    temp_settings->SetGameInstallDirEnabled(dir, false);
    EXPECT_TRUE(temp_settings->GetGameInstallDirs().empty());
}

TEST_F(EmulatorSettingsTest, SetGameInstallDirEnabledReEnablesDir) {
    fs::path dir = temp_dir->path() / "games";
    fs::create_directories(dir);
    temp_settings->AddGameInstallDir(dir, false);
    ASSERT_TRUE(temp_settings->GetGameInstallDirs().empty());
    temp_settings->SetGameInstallDirEnabled(dir, true);
    EXPECT_EQ(temp_settings->GetGameInstallDirs().size(), 1u);
}

TEST_F(EmulatorSettingsTest, SetAllGameInstallDirsReplacesExistingList) {
    fs::path d1 = temp_dir->path() / "g1";
    fs::path d2 = temp_dir->path() / "g2";
    fs::create_directories(d1);
    fs::create_directories(d2);
    temp_settings->AddGameInstallDir(d1);

    temp_settings->SetAllGameInstallDirs({{d2, true}});
    ASSERT_EQ(temp_settings->GetGameInstallDirs().size(), 1u);
    EXPECT_EQ(temp_settings->GetGameInstallDirs()[0], d2);
}

TEST_F(EmulatorSettingsTest, GameInstallDirsFullRoundTripWithEnabledFlags) {
    fs::path d1 = temp_dir->path() / "g1";
    fs::path d2 = temp_dir->path() / "g2";
    fs::create_directories(d1);
    fs::create_directories(d2);
    temp_settings->AddGameInstallDir(d1, true);
    temp_settings->AddGameInstallDir(d2, false);
    temp_settings->Save();

    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load();

    const auto& all = f->GetAllGameInstallDirs();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].path, d1);
    EXPECT_TRUE(all[0].enabled);
    EXPECT_EQ(all[1].path, d2);
    EXPECT_FALSE(all[1].enabled);
}

TEST_F(EmulatorSettingsTest, GetGameInstallDirsEnabledReflectsState) {
    fs::path d1 = temp_dir->path() / "g1";
    fs::path d2 = temp_dir->path() / "g2";
    fs::create_directories(d1);
    fs::create_directories(d2);
    temp_settings->AddGameInstallDir(d1, true);
    temp_settings->AddGameInstallDir(d2, false);

    auto enabled = temp_settings->GetGameInstallDirsEnabled();
    ASSERT_EQ(enabled.size(), 2u);
    EXPECT_TRUE(enabled[0]);
    EXPECT_FALSE(enabled[1]);
}

// GetAllOverrideableKeys tests

TEST_F(EmulatorSettingsTest, GetAllOverrideableKeysIsNonEmpty) {
    EXPECT_FALSE(temp_settings->GetAllOverrideableKeys().empty());
}

TEST_F(EmulatorSettingsTest, GetAllOverrideableKeysContainsRepresentativeKeys) {
    auto keys = temp_settings->GetAllOverrideableKeys();
    auto has = [&](const char* k) { return std::find(keys.begin(), keys.end(), k) != keys.end(); };
    // General
    EXPECT_TRUE(has("neo_mode"));
    EXPECT_TRUE(has("volume_slider"));
    // GPU
    EXPECT_TRUE(has("window_width"));
    EXPECT_TRUE(has("null_gpu"));
    EXPECT_TRUE(has("vblank_frequency"));
    // Vulkan
    EXPECT_TRUE(has("gpu_id"));
    EXPECT_TRUE(has("pipeline_cache_enabled"));
    // Debug
    EXPECT_TRUE(has("debug_dump"));
    // Input
    EXPECT_TRUE(has("cursor_state"));
    // Audio
    EXPECT_TRUE(has("audio_backend"));
}

TEST_F(EmulatorSettingsTest, GetAllOverrideableKeysNoDuplicates) {
    auto keys = temp_settings->GetAllOverrideableKeys();
    std::vector<std::string> sorted = keys;
    std::sort(sorted.begin(), sorted.end());
    auto it = std::unique(sorted.begin(), sorted.end());
    EXPECT_EQ(it, sorted.end()) << "Duplicate key found in overrideable keys list";
}

// Per-group GetOverrideableFields tests

TEST_F(EmulatorSettingsTest, GetGeneralOverrideableFieldsNonEmpty) {
    EXPECT_FALSE(temp_settings->GetGeneralOverrideableFields().empty());
}

TEST_F(EmulatorSettingsTest, GetGPUOverrideableFieldsContainsWindowAndFullscreen) {
    auto fields = temp_settings->GetGPUOverrideableFields();
    auto has = [&](const char* k) {
        return std::any_of(fields.begin(), fields.end(),
                           [k](const OverrideItem& f) { return std::string(f.key) == k; });
    };
    EXPECT_TRUE(has("window_width"));
    EXPECT_TRUE(has("window_height"));
    EXPECT_TRUE(has("full_screen"));
    EXPECT_TRUE(has("vblank_frequency"));
}

TEST_F(EmulatorSettingsTest, GetVulkanOverrideableFieldsContainsGpuId) {
    auto fields = temp_settings->GetVulkanOverrideableFields();
    bool found = std::any_of(fields.begin(), fields.end(),
                             [](const OverrideItem& f) { return std::string(f.key) == "gpu_id"; });
    EXPECT_TRUE(found);
}

// Path accessors tests
TEST_F(EmulatorSettingsTest, GetHomeDirReturnsCustomWhenSet) {
    fs::path dir = temp_dir->path() / "custom_home";
    fs::create_directories(dir);
    temp_settings->SetHomeDir(dir);
    EXPECT_EQ(temp_settings->GetHomeDir(), dir);
}
TEST_F(EmulatorSettingsTest, GetSysModulesDirFallsBackToPathUtilWhenEmpty) {
    // default_value is empty; GetSysModulesDir falls back to GetUserPath(SysModuleDir)
    auto result = temp_settings->GetSysModulesDir();
    EXPECT_FALSE(result.empty());
}
TEST_F(EmulatorSettingsTest, GetFontsDirFallsBackToPathUtilWhenEmpty) {
    auto result = temp_settings->GetFontsDir();
    EXPECT_FALSE(result.empty());
}

// edge cases tests

TEST_F(EmulatorSettingsTest, VersionMismatchPreservesSettings) {
    temp_settings->SetNeo(true);
    temp_settings->SetWindowWidth(2560u);
    temp_settings->Save();

    // Force a stale version string so the mismatch branch fires
    json j = ReadJson(ConfigJson());
    j["Debug"]["config_version"] = "old_hash_0000";
    WriteJson(ConfigJson(), j);

    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load(); // triggers version-bump Save() internally

    EXPECT_TRUE(f->IsNeo());
    EXPECT_EQ(f->GetWindowWidth(), 2560u);
}

TEST_F(EmulatorSettingsTest, DoubleGlobalLoadIsIdempotent) {
    temp_settings->SetNeo(true);
    temp_settings->SetWindowWidth(2560u);
    temp_settings->Save();

    auto f = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(f);
    f->Load(""); // first — loads from disk
    f->Load(""); // second — must not reset anything

    EXPECT_TRUE(f->IsNeo());
    EXPECT_EQ(f->GetWindowWidth(), 2560u);
}

TEST_F(EmulatorSettingsTest, ConfigUsedFlagTrueWhenFileExists) {
    json game;
    game["General"]["neo_mode"] = true;
    WriteJson(GameConfig("CUSA01234"), game);
    temp_settings->Load("CUSA01234");
    EXPECT_TRUE(EmulatorState::GetInstance()->IsGameSpecifigConfigUsed());
}

TEST_F(EmulatorSettingsTest, ConfigUsedFlagFalseWhenFileAbsent) {
    temp_settings->Load("CUSA99999");
    EXPECT_FALSE(EmulatorState::GetInstance()->IsGameSpecifigConfigUsed());
}

TEST_F(EmulatorSettingsTest, DestructorNoSaveIfLoadNeverCalled) {
    temp_settings->SetNeo(true);
    temp_settings->Save();
    auto t0 = fs::last_write_time(ConfigJson());

    {
        // Create and immediately destroy without calling Load()
        auto untouched = std::make_shared<EmulatorSettingsImpl>();
        // destructor fires here
    }

    auto t1 = fs::last_write_time(ConfigJson());
    EXPECT_EQ(t0, t1) << "Destructor wrote config.json without a prior Load()";
}

TEST_F(EmulatorSettingsTest, DestructorSavesAfterSuccessfulLoad) {
    temp_settings->SetNeo(true);
    temp_settings->Save();

    {
        auto s = std::make_shared<EmulatorSettingsImpl>();
        EmulatorSettingsImpl::SetInstance(s);
        s->Load();
        s->SetWindowWidth(2560u); // mutate after successful load
        // destructor should write this change
    }

    auto verify = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(verify);
    verify->Load();
    EXPECT_EQ(verify->GetWindowWidth(), 2560);
}
