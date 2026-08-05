// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <string_view>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "core/emulator_settings.h"
#include "core/file_sys/storage_scheduler.h"

TEST(StorageConfigTest, NormalizesBandwidth) {
    using Core::FileSys::NormalizeReadBandwidth;

    EXPECT_EQ(NormalizeReadBandwidth(0), 0u);
    EXPECT_EQ(NormalizeReadBandwidth(1), 50u);
    EXPECT_EQ(NormalizeReadBandwidth(50), 50u);
    EXPECT_EQ(NormalizeReadBandwidth(137), 137u);
    EXPECT_EQ(NormalizeReadBandwidth(200), 200u);
    EXPECT_EQ(NormalizeReadBandwidth(201), 0u);
}

TEST(StorageConfigTest, SettingsAreSerializedAndOverrideable) {
    const GeneralSettings settings;
    const nlohmann::json json = settings;

    EXPECT_EQ(json.at("app0_read_bandwidth_mibps"), 0u);
    EXPECT_FALSE(json.at("app0_read_disable_time_stretching"));

    const auto overrides = settings.GetOverrideableFields();
    const auto has_override = [&](std::string_view key) {
        return std::ranges::any_of(overrides,
                                   [=](const OverrideItem& item) { return item.key == key; });
    };
    EXPECT_TRUE(has_override("app0_read_bandwidth_mibps"));
    EXPECT_TRUE(has_override("app0_read_disable_time_stretching"));
}
