// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <miniz.h>

#include "video_core/cache_archive.h"

namespace {

using ArchiveEntry = std::pair<std::string_view, std::string_view>;

class ArchivePaths {
public:
    ArchivePaths() {
        const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
        const auto test_name = std::string{test_info->test_suite_name()} + "_" + test_info->name();
        directory = std::filesystem::path{testing::TempDir()} / test_name;
        Cleanup();
        std::filesystem::create_directories(directory);
        source = directory / "source.zip";
        compacted = directory / "compacted.zip";
    }

    ~ArchivePaths() {
        Cleanup();
    }

    void Cleanup() const {
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
    }

    std::filesystem::path directory;
    std::filesystem::path source;
    std::filesystem::path compacted;
};

bool CreateArchive(const std::filesystem::path& path, std::span<const ArchiveEntry> entries) {
    mz_zip_archive writer{};
    if (!mz_zip_writer_init_file(&writer, path.string().c_str(), 0)) {
        return false;
    }

    bool succeeded = true;
    for (const auto& [name, contents] : entries) {
        if (!mz_zip_writer_add_mem(&writer, name.data(), contents.data(), contents.size(),
                                   MZ_BEST_COMPRESSION)) {
            succeeded = false;
            break;
        }
    }
    if (succeeded) {
        succeeded = mz_zip_writer_finalize_archive(&writer);
    }
    mz_zip_writer_end(&writer);
    return succeeded;
}

std::vector<char> ReadEntry(mz_zip_archive& reader, std::string_view name) {
    const int index = mz_zip_reader_locate_file(&reader, name.data(), nullptr, 0);
    if (index < 0) {
        return {};
    }

    mz_zip_archive_file_stat stat{};
    if (!mz_zip_reader_file_stat(&reader, index, &stat)) {
        return {};
    }
    std::vector<char> contents(stat.m_uncomp_size);
    if (!mz_zip_reader_extract_to_mem(&reader, index, contents.data(), contents.size(), 0)) {
        return {};
    }
    return contents;
}

TEST(CacheArchive, CompactionKeepsLatestEntryByName) {
    ArchivePaths paths;
    constexpr std::array entries{
        ArchiveEntry{"shader.spv", "old shader"},
        ArchiveEntry{"pipeline.key", "pipeline"},
        ArchiveEntry{"shader.spv", "updated shader"},
    };
    ASSERT_TRUE(CreateArchive(paths.source, entries));

    ASSERT_TRUE(Storage::Detail::CompactArchive(paths.source, paths.compacted));

    mz_zip_archive reader{};
    ASSERT_TRUE(mz_zip_reader_init_file(&reader, paths.compacted.string().c_str(), 0));
    EXPECT_TRUE(mz_zip_validate_archive(&reader, 0));
    EXPECT_EQ(mz_zip_reader_get_num_files(&reader), 2u);
    const auto contents = ReadEntry(reader, "shader.spv");
    EXPECT_EQ(std::string_view(contents.data(), contents.size()), "updated shader");
    mz_zip_reader_end(&reader);
}

TEST(CacheArchive, CompactionUpdatesAnEntryAcrossRuns) {
    ArchivePaths paths;
    constexpr std::array initial_entries{ArchiveEntry{"shader.meta", "version 1"}};
    ASSERT_TRUE(CreateArchive(paths.source, initial_entries));

    mz_zip_archive archive{};
    ASSERT_TRUE(mz_zip_reader_init_file(&archive, paths.source.string().c_str(),
                                        MZ_ZIP_FLAG_READ_ALLOW_WRITING));
    ASSERT_TRUE(mz_zip_writer_init_from_reader(&archive, paths.source.string().c_str()));
    constexpr std::string_view updated_contents{"version 2"};
    ASSERT_TRUE(mz_zip_writer_add_mem(&archive, "shader.meta", updated_contents.data(),
                                      updated_contents.size(), MZ_BEST_COMPRESSION));
    ASSERT_TRUE(mz_zip_writer_finalize_archive(&archive));
    mz_zip_writer_end(&archive);

    ASSERT_TRUE(Storage::Detail::CompactArchive(paths.source, paths.compacted));

    mz_zip_archive reader{};
    ASSERT_TRUE(mz_zip_reader_init_file(&reader, paths.compacted.string().c_str(), 0));
    EXPECT_EQ(mz_zip_reader_get_num_files(&reader), 1u);
    const auto contents = ReadEntry(reader, "shader.meta");
    EXPECT_EQ(std::string_view(contents.data(), contents.size()), updated_contents);
    mz_zip_reader_end(&reader);
}

TEST(CacheArchive, CompactionRemovesStaleDestinationWhenSourceIsMissing) {
    ArchivePaths paths;
    constexpr std::string_view stale_contents{"stale compacted archive"};
    {
        std::ofstream destination{paths.compacted, std::ios::binary};
        ASSERT_TRUE(destination.is_open());
        destination.write(stale_contents.data(), stale_contents.size());
    }

    ASSERT_FALSE(Storage::Detail::CompactArchive(paths.source, paths.compacted));
    EXPECT_FALSE(std::filesystem::exists(paths.compacted));
}

TEST(CacheArchive, CompactionCleansUpAfterWriterInitializationFailure) {
    ArchivePaths paths;
    constexpr std::array entries{ArchiveEntry{"shader.spv", "shader"}};
    ASSERT_TRUE(CreateArchive(paths.source, entries));
    paths.compacted = paths.directory / "missing" / "compacted.zip";

    ASSERT_FALSE(Storage::Detail::CompactArchive(paths.source, paths.compacted));
    EXPECT_FALSE(std::filesystem::exists(paths.compacted));
}

} // namespace
