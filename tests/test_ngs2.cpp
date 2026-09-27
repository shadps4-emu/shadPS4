// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include <gtest/gtest.h>

#include "core/libraries/error_codes.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/ngs2/ngs2_error.h"

namespace Libraries::Ngs2 {
namespace {

TEST(Ngs2Test, RackQueryBufferSizeInitializesSuccessfulOutput) {
    OrbisNgs2ContextBufferInfo info;
    std::memset(&info, 0xA5, sizeof(info));
    const size_t poisoned_size = info.hostBufferSize;
    const uintptr_t poisoned_user_data = info.userData;

    EXPECT_EQ(RackQueryBufferSize(nullptr, &info), ORBIS_OK);
    EXPECT_EQ(info.hostBuffer, nullptr);
    EXPECT_GT(info.hostBufferSize, 0);
    EXPECT_NE(info.hostBufferSize, poisoned_size);
    for (const auto value : info.reserved) {
        EXPECT_EQ(value, 0);
    }
    EXPECT_EQ(info.userData, poisoned_user_data);
}

TEST(Ngs2Test, RackQueryBufferSizeRejectsInvalidArguments) {
    EXPECT_EQ(RackQueryBufferSize(nullptr, nullptr), ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS);

    OrbisNgs2RackOption option{};
    option.size = sizeof(option) - 1;
    OrbisNgs2ContextBufferInfo info{};
    EXPECT_EQ(RackQueryBufferSize(&option, &info), ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE);
}

TEST(Ngs2Test, SystemSetupRejectsUnsupportedSampleRate) {
    OrbisNgs2SystemOption option{};
    option.size = sizeof(option);
    option.maxGrainSamples = 512;
    option.numGrainSamples = 256;
    option.sampleRate = 44000;

    OrbisNgs2ContextBufferInfo info{};
    EXPECT_EQ(SystemSetup(&option, &info, nullptr, nullptr), ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE);
}

TEST(Ngs2Test, SystemSetupRejectsMisalignedMaxGrainSamples) {
    OrbisNgs2SystemOption option{};
    option.size = sizeof(option);
    option.maxGrainSamples = 500;
    option.numGrainSamples = 256;
    option.sampleRate = 48000;

    OrbisNgs2ContextBufferInfo info{};
    EXPECT_EQ(SystemSetup(&option, &info, nullptr, nullptr),
              ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES);
}

TEST(Ngs2Test, SystemSetupDoesNotHandOutAHandleForAnInvalidOption) {
    OrbisNgs2SystemOption option{};
    option.size = sizeof(option);
    option.maxGrainSamples = 512;
    option.numGrainSamples = 256;
    option.sampleRate = 44000;

    u8 storage[256]{};
    OrbisNgs2ContextBufferInfo info{};
    info.hostBuffer = storage;
    info.hostBufferSize = sizeof(storage);

    OrbisNgs2Handle handle = 0;
    EXPECT_EQ(SystemSetup(&option, &info, nullptr, &handle), ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE);
    EXPECT_EQ(handle, 0);
}

TEST(Ngs2Test, SystemSetupQueriesBufferSizeWithDefaultOption) {
    OrbisNgs2ContextBufferInfo info{};
    EXPECT_EQ(SystemSetup(nullptr, &info, nullptr, nullptr), ORBIS_OK);
    EXPECT_EQ(info.hostBuffer, nullptr);
    EXPECT_GT(info.hostBufferSize, 0);
}

} // namespace
} // namespace Libraries::Ngs2
