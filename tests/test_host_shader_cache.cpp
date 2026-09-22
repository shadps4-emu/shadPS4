// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>

#include <gtest/gtest.h>

#include "video_core/host_shader_cache.h"

namespace {

constexpr u32 SpirvMagic = 0x07230203;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
        const auto test_name = std::string{test_info->test_suite_name()} + "_" + test_info->name();
        path = std::filesystem::path{testing::TempDir()} / test_name;
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    std::filesystem::path path;
};

std::vector<u32> MakeSpirv(u32 payload) {
    return {SpirvMagic, 0x00010300, 0, 4, payload};
}

TEST(HostShaderCache, StoresAndLoadsPermutation) {
    TemporaryDirectory temporary;
    Storage::HostShaderCache cache{temporary.path};
    const auto spirv = MakeSpirv(1);

    ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation1", spirv));
    const auto loaded = cache.Load("tiling.comp", "generation1", "permutation1");

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, spirv);
}

TEST(HostShaderCache, AppendsPermutationsToCurrentGeneration) {
    TemporaryDirectory temporary;
    const auto first = MakeSpirv(1);
    const auto second = MakeSpirv(2);

    {
        Storage::HostShaderCache cache{temporary.path};
        ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation1", first));
    }

    Storage::HostShaderCache cache{temporary.path};
    ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation2", second));

    EXPECT_EQ(cache.Load("tiling.comp", "generation1", "permutation1"), first);
    EXPECT_EQ(cache.Load("tiling.comp", "generation1", "permutation2"), second);
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator{temporary.path / "tiling.comp" /
                                                                "generation1"},
                            std::filesystem::directory_iterator{}),
              2);
}

TEST(HostShaderCache, ReplacesPreviousShaderGeneration) {
    TemporaryDirectory temporary;
    Storage::HostShaderCache cache{temporary.path};

    ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation1", MakeSpirv(1)));
    ASSERT_TRUE(cache.Store("tiling.comp", "generation2", "permutation1", MakeSpirv(2)));

    EXPECT_FALSE(std::filesystem::exists(temporary.path / "tiling.comp" / "generation1"));
    EXPECT_TRUE(std::filesystem::exists(temporary.path / "tiling.comp" / "generation2"));
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator{temporary.path / "tiling.comp"},
                            std::filesystem::directory_iterator{}),
              1);
}

TEST(HostShaderCache, RejectsAndReplacesCorruptedBinary) {
    TemporaryDirectory temporary;
    Storage::HostShaderCache cache{temporary.path};
    const auto spirv = MakeSpirv(1);
    ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation1", spirv));

    const auto cache_file = temporary.path / "tiling.comp" / "generation1" / "permutation1.spv";
    std::fstream file{cache_file, std::ios::binary | std::ios::in | std::ios::out};
    ASSERT_TRUE(file.is_open());
    file.seekp(-4, std::ios::end);
    file.put('\0');
    file.close();

    EXPECT_FALSE(cache.Load("tiling.comp", "generation1", "permutation1").has_value());
    ASSERT_TRUE(cache.Store("tiling.comp", "generation1", "permutation1", spirv));
    EXPECT_EQ(cache.Load("tiling.comp", "generation1", "permutation1"), spirv);
}

TEST(HostShaderCache, RejectsUnsafeCacheKeys) {
    TemporaryDirectory temporary;
    Storage::HostShaderCache cache{temporary.path};

    EXPECT_FALSE(cache.Store("../tiling.comp", "generation1", "permutation1", MakeSpirv(1)));
    EXPECT_FALSE(cache.Store("tiling.comp", "../generation1", "permutation1", MakeSpirv(1)));
    EXPECT_FALSE(cache.Store("tiling.comp", "generation1", "../permutation1", MakeSpirv(1)));
    EXPECT_TRUE(std::filesystem::is_empty(temporary.path));
}

} // namespace
