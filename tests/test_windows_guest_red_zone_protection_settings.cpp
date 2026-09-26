// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Windows static guest red-zone protection

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "common/path_util.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

class WindowsGuestRedZoneProtectionProfileTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        root = fs::temp_directory_path() /
               ("shadps4_red_zone_settings_test_" + std::to_string(suffix));
        custom_configs = root / "custom_configs";
        fs::create_directories(custom_configs);

        Common::FS::SetUserPath(Common::FS::PathType::UserDir, root);
        Common::FS::SetUserPath(Common::FS::PathType::CustomConfigs, custom_configs);

        state = std::make_shared<EmulatorState>();
        EmulatorState::SetInstance(state);
        settings = std::make_shared<EmulatorSettingsImpl>();
        EmulatorSettingsImpl::SetInstance(settings);
    }

    void TearDown() override {
        EmulatorSettingsImpl::SetInstance(nullptr);
        EmulatorState::SetInstance(nullptr);
        settings.reset();
        state.reset();

        std::error_code error;
        fs::remove_all(root, error);
    }

    void WriteGameConfig(const std::string& serial, const json& config) const {
        std::ofstream output(custom_configs / (serial + ".json"));
        ASSERT_TRUE(output.is_open());
        output << config;
    }

    fs::path root;
    fs::path custom_configs;
    std::shared_ptr<EmulatorState> state;
    std::shared_ptr<EmulatorSettingsImpl> settings;
};

TEST(WindowsGuestRedZoneProtectionSettingsTest, DisabledIsTheDefault) {
    EmulatorSettingsImpl settings;

    EXPECT_EQ(settings.GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::Disabled);
}

TEST(WindowsGuestRedZoneProtectionSettingsTest, GameOverrideDoesNotChangeGlobalMode) {
    EmulatorSettingsImpl settings;
    settings.SetWindowsGuestRedZoneProtectionMode(WindowsGuestRedZoneProtectionMode::StaticPatching,
                                                  true);

    EXPECT_EQ(settings.GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::StaticPatching);

    settings.SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(settings.GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::Disabled);
}

TEST(WindowsGuestRedZoneProtectionSettingsTest, JsonUsesTheExistingPerGameKey) {
    WindowsGuestRedZoneProtectionSettings source;
    source.windows_guest_red_zone_protection_mode.value =
        WindowsGuestRedZoneProtectionMode::StaticPatching;

    const nlohmann::json encoded = source;
    ASSERT_TRUE(encoded.contains("windows_guest_red_zone_protection_mode"));

    const auto decoded = encoded.get<WindowsGuestRedZoneProtectionSettings>();
    EXPECT_EQ(decoded.windows_guest_red_zone_protection_mode.value,
              WindowsGuestRedZoneProtectionMode::StaticPatching);
}

TEST_F(WindowsGuestRedZoneProtectionProfileTest, MissingProfileClearsExperimentalOverrides) {
    settings->SetWindowsGuestRedZoneProtectionMode(
        WindowsGuestRedZoneProtectionMode::StaticPatching, true);
    settings->SetReadbacksMode(GpuReadbacksMode::Precise, true);

    EXPECT_FALSE(settings->Load("CUSA00001"));
    EXPECT_EQ(settings->GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::Disabled);
    EXPECT_EQ(settings->GetReadbacksMode(), GpuReadbacksMode::Disabled);
}

TEST_F(WindowsGuestRedZoneProtectionProfileTest, NewProfileReplacesExperimentalOverrides) {
    json enabled;
    enabled["WindowsGuestRedZoneProtection"]["windows_guest_red_zone_protection_mode"] =
        "StaticPatching";
    enabled["GPU"]["readbacks_mode"] = GpuReadbacksMode::Precise;
    WriteGameConfig("CUSA00001", enabled);
    WriteGameConfig("CUSA00002", json::object());

    ASSERT_TRUE(settings->Load("CUSA00001"));
    EXPECT_EQ(settings->GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::StaticPatching);
    EXPECT_EQ(settings->GetReadbacksMode(), GpuReadbacksMode::Precise);

    ASSERT_TRUE(settings->Load("CUSA00002"));
    EXPECT_EQ(settings->GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::Disabled);
    EXPECT_EQ(settings->GetReadbacksMode(), GpuReadbacksMode::Disabled);
}

TEST_F(WindowsGuestRedZoneProtectionProfileTest, GlobalConfigCannotEnableProtection) {
    json global;
    global["WindowsGuestRedZoneProtection"]["windows_guest_red_zone_protection_mode"] =
        "StaticPatching";
    std::ofstream output(root / "config.json");
    ASSERT_TRUE(output.is_open());
    output << global;
    output.close();

    ASSERT_TRUE(settings->Load());
    EXPECT_EQ(settings->GetWindowsGuestRedZoneProtectionMode(),
              WindowsGuestRedZoneProtectionMode::Disabled);
}
