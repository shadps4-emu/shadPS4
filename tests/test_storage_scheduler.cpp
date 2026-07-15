// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "core/emulator_settings.h"
#include "core/file_sys/storage_scheduler.h"

namespace {

using Core::FileSys::StorageTimingModel;

constexpr StorageTimingModel MakeTimingModel(u32 bandwidth_mibps,
                                             bool disable_time_stretching = false,
                                             bool unlimited_sequential_read_speed = false) {
    return StorageTimingModel{
        {bandwidth_mibps, disable_time_stretching, unlimited_sequential_read_speed}};
}

TEST(StorageSchedulerTest, PreservesEveryUserBandwidthInRange) {
    for (u32 bandwidth = 50; bandwidth <= 200; ++bandwidth) {
        EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(bandwidth), bandwidth);
    }
}

TEST(StorageSchedulerTest, OutOfRangeValuesNormalizeSafely) {
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(0), 0u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(1), 50u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(49), 50u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(50), 50u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(137), 137u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(200), 200u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(201), 0u);
    EXPECT_EQ(Core::FileSys::NormalizeReadBandwidth(1'000), 0u);
}

TEST(StorageSchedulerTest, DisabledProfileAddsNoTransferDelay) {
    constexpr auto model = MakeTimingModel(0);
    EXPECT_EQ(model.TransferDuration(0), std::chrono::nanoseconds::zero());
    EXPECT_EQ(model.TransferDuration(1024 * 1024), std::chrono::nanoseconds::zero());
}

TEST(StorageSchedulerTest, UserBandwidthsProduceExactSequentialCeilings) {
    constexpr auto model75 = MakeTimingModel(75);
    constexpr auto model100 = MakeTimingModel(100);
    constexpr auto model125 = MakeTimingModel(125);

    EXPECT_EQ(model75.TransferDuration(75 * 1024 * 1024), std::chrono::seconds{1});
    EXPECT_EQ(model100.TransferDuration(100 * 1024 * 1024), std::chrono::seconds{1});
    EXPECT_EQ(model125.TransferDuration(125 * 1024 * 1024), std::chrono::seconds{1});
    EXPECT_EQ(model100.TransferDuration(512 * 1024), std::chrono::milliseconds{5});
    EXPECT_EQ(model125.TransferDuration(1024 * 1024), std::chrono::milliseconds{8});

    constexpr auto model137 = MakeTimingModel(137);
    EXPECT_EQ(model137.TransferDuration(137 * 1024 * 1024), std::chrono::seconds{1});
}

TEST(StorageSchedulerTest, SmallSequentialReadsAggregateWithoutPerReadLatency) {
    constexpr auto model = MakeTimingModel(75);
    constexpr auto individual = model.ServiceDuration(4 * 1024, true);
    EXPECT_NEAR((individual * 128).count(), model.TransferDuration(512 * 1024).count(), 128);
}

TEST(StorageSchedulerTest, RandomReadAddsMechanicalPositioningOnce) {
    constexpr auto model = MakeTimingModel(100);
    constexpr auto sequential = model.ServiceDuration(4 * 1024, true);
    constexpr auto positioned = model.ServiceDuration(4 * 1024, false);
    EXPECT_EQ(positioned - sequential,
              StorageTimingModel::AverageSeek + StorageTimingModel::AverageRotation);
}

TEST(StorageSchedulerTest, DisableTimeStretchingOnlyRemovesSlowdownFactor) {
    constexpr auto stretched = MakeTimingModel(100);
    constexpr auto unstretched = MakeTimingModel(100, true);
    constexpr auto normal_sequential = stretched.ServiceDuration(512 * 1024, true);
    constexpr auto normal_positioned = stretched.ServiceDuration(512 * 1024, false);

    EXPECT_EQ(unstretched.ServiceDuration(512 * 1024, true, 250), normal_sequential);
    EXPECT_EQ(unstretched.ServiceDuration(512 * 1024, false, 250), normal_positioned);
    EXPECT_EQ(stretched.ServiceDuration(512 * 1024, true, 250), normal_sequential * 5 / 2);
    EXPECT_EQ(stretched.ServiceDuration(512 * 1024, false, 250), normal_positioned * 5 / 2);
}

TEST(StorageSchedulerTest, UnlimitedSequentialReadSpeedOnlyUncapsContiguousReads) {
    constexpr auto limited = MakeTimingModel(100);
    constexpr auto unlimited = MakeTimingModel(100, false, true);
    constexpr auto normal_positioned = limited.ServiceDuration(512 * 1024, false);

    EXPECT_EQ(unlimited.ServiceDuration(512 * 1024, true), std::chrono::nanoseconds::zero());
    EXPECT_EQ(unlimited.ServiceDuration(512 * 1024, false), normal_positioned);
}

TEST(StorageSchedulerTest, UsesFiosDefaultChunkSize) {
    EXPECT_EQ(StorageTimingModel::MaxChunkSize, 512u * 1024u);
    constexpr auto model = MakeTimingModel(100);
    EXPECT_EQ(model.TransferDuration(StorageTimingModel::MaxChunkSize),
              std::chrono::milliseconds{5});
}

TEST(StorageSchedulerTest, PriorityMappingClampsToSdkRange) {
    EXPECT_EQ(Core::FileSys::StoragePriorityIndex(-1'000), 0u);
    EXPECT_EQ(Core::FileSys::StoragePriorityIndex(-128), 0u);
    EXPECT_EQ(Core::FileSys::StoragePriorityIndex(0), 128u);
    EXPECT_EQ(Core::FileSys::StoragePriorityIndex(127), 255u);
    EXPECT_EQ(Core::FileSys::StoragePriorityIndex(1'000), 255u);
}

TEST(StorageSchedulerTest, BandwidthDefaultsToNativeSpeed) {
    EmulatorSettingsImpl settings;
    EXPECT_EQ(settings.GetApp0ReadBandwidthMiBps(), 0u);
    EXPECT_FALSE(settings.IsApp0ReadDisableTimeStretching());
    EXPECT_FALSE(settings.IsApp0ReadUnlimitedSequentialReadSpeed());
}

TEST(StorageSchedulerTest, BandwidthSupportsPerGameOverride) {
    EmulatorSettingsImpl settings;
    settings.SetApp0ReadBandwidthMiBps(100);
    settings.SetApp0ReadBandwidthMiBps(75, true);
    settings.SetApp0ReadDisableTimeStretching(false);
    settings.SetApp0ReadDisableTimeStretching(true, true);
    settings.SetApp0ReadUnlimitedSequentialReadSpeed(false);
    settings.SetApp0ReadUnlimitedSequentialReadSpeed(true, true);

    settings.SetConfigMode(ConfigMode::Global);
    EXPECT_EQ(settings.GetApp0ReadBandwidthMiBps(), 100u);
    EXPECT_FALSE(settings.IsApp0ReadDisableTimeStretching());
    EXPECT_FALSE(settings.IsApp0ReadUnlimitedSequentialReadSpeed());
    settings.SetConfigMode(ConfigMode::Default);
    EXPECT_EQ(settings.GetApp0ReadBandwidthMiBps(), 75u);
    EXPECT_TRUE(settings.IsApp0ReadDisableTimeStretching());
    EXPECT_TRUE(settings.IsApp0ReadUnlimitedSequentialReadSpeed());
    settings.SetConfigMode(ConfigMode::Clean);
    EXPECT_EQ(settings.GetApp0ReadBandwidthMiBps(), 0u);
    EXPECT_FALSE(settings.IsApp0ReadDisableTimeStretching());
    EXPECT_FALSE(settings.IsApp0ReadUnlimitedSequentialReadSpeed());
}

TEST(StorageSchedulerTest, BandwidthIsSerializedAndOverrideable) {
    GeneralSettings settings;
    const nlohmann::json json = settings;
    EXPECT_EQ(json.at("app0_read_bandwidth_mibps"), 0u);
    EXPECT_FALSE(json.at("app0_read_disable_time_stretching"));
    EXPECT_FALSE(json.at("app0_read_unlimited_sequential_read_speed"));

    const auto overrides = settings.GetOverrideableFields();
    EXPECT_NE(std::ranges::find_if(overrides,
                                   [](const OverrideItem& item) {
                                       return std::string{item.key} == "app0_read_bandwidth_mibps";
                                   }),
              overrides.end());
    EXPECT_NE(std::ranges::find_if(overrides,
                                   [](const OverrideItem& item) {
                                       return std::string{item.key} ==
                                              "app0_read_disable_time_stretching";
                                   }),
              overrides.end());
    EXPECT_NE(std::ranges::find_if(overrides,
                                   [](const OverrideItem& item) {
                                       return std::string{item.key} ==
                                              "app0_read_unlimited_sequential_read_speed";
                                   }),
              overrides.end());
}

} // namespace
