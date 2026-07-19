// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <zarchive/zarchivewriter.h>

#include "common/io_file.h"
#include "common/path_util.h"
#include "common/zar_fs.h"

namespace Common::FS::Zar {
namespace {

namespace fs = std::filesystem;

struct PackContext {
    fs::path path;
    std::ofstream output;
    bool failed{};
};

void NewOutputFile(s32 part_index, void* opaque) {
    auto* context = static_cast<PackContext*>(opaque);
    if (part_index != -1) {
        context->failed = true;
        return;
    }
    context->output.open(context->path, std::ios::binary | std::ios::trunc);
    context->failed = !context->output;
}

void WriteOutputData(const void* data, size_t length, void* opaque) {
    auto* context = static_cast<PackContext*>(opaque);
    context->output.write(static_cast<const char*>(data), static_cast<std::streamsize>(length));
    context->failed = context->failed || !context->output;
}

bool AddFile(ZArchiveWriter& writer, const char* path, std::string_view contents) {
    if (!writer.StartNewFile(path)) {
        return false;
    }
    writer.AppendData(contents.data(), contents.size());
    return true;
}

bool CreateArchive(const fs::path& path) {
    PackContext context{.path = path};
    {
        ZArchiveWriter writer{NewOutputFile, WriteOutputData, &context};
        if (context.failed || !writer.MakeDir("sce_sys", true) || !writer.MakeDir("data", true) ||
            !AddFile(writer, "sce_sys/Param.SFO", "parameter data") ||
            !AddFile(writer, "eboot.bin", "executable data") ||
            !AddFile(writer, "data/part.zar", "nested extension")) {
            return false;
        }
        writer.Finalize();
    }
    context.output.close();
    return !context.failed;
}

class ZarFsTest : public testing::Test {
protected:
    void SetUp() override {
        original_temp_dir = GetUserPath(PathType::TempDataDir);
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        test_dir = fs::temp_directory_path() / ("shadps4_zar_test_" + std::to_string(unique));
        ASSERT_TRUE(fs::create_directories(test_dir));
        archive_path = test_dir / "CUSA00001.zar";
        ASSERT_TRUE(CreateArchive(archive_path));
    }

    void TearDown() override {
        ClearCache();
        SetUserPath(PathType::TempDataDir, original_temp_dir);
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }

    fs::path test_dir;
    fs::path archive_path;
    fs::path original_temp_dir;
};

TEST_F(ZarFsTest, QueriesAndIteratesArchive) {
    EXPECT_TRUE(IsZarArchive(archive_path));
    EXPECT_TRUE(Exists(archive_path));
    EXPECT_TRUE(IsDirectory(archive_path));
    EXPECT_FALSE(IsZarInnerPath(archive_path));

    const auto param_path = archive_path / "SCE_SYS" / "param.sfo";
    EXPECT_TRUE(IsZarInnerPath(param_path));
    EXPECT_TRUE(Exists(param_path));
    EXPECT_TRUE(IsRegularFile(param_path));
    EXPECT_EQ(GetFileSize(param_path), 14);
    EXPECT_TRUE(GetLastWriteTime(param_path).has_value());

    std::set<std::string> entries;
    ASSERT_TRUE(IterateDirectory(archive_path, [&](const fs::path& path, bool is_file) {
        entries.emplace(path.filename().string() + (is_file ? ":file" : ":dir"));
    }));
    EXPECT_EQ(entries, (std::set<std::string>{"data:dir", "eboot.bin:file", "sce_sys:dir"}));
}

TEST_F(ZarFsTest, ReadsSeeksAndCopiesFiles) {
    const auto nested_zar_path = archive_path / "data" / "part.zar";
    EXPECT_TRUE(IsZarInnerPath(nested_zar_path));

    IOFile file{nested_zar_path, FileAccessMode::Read};
    ASSERT_TRUE(file.IsOpen());
    EXPECT_TRUE(file.IsZarBacked());
    ASSERT_TRUE(file.Seek(-9, SeekOrigin::End));

    std::string suffix(9, '\0');
    ASSERT_EQ(file.ReadRaw<char>(suffix.data(), suffix.size()), suffix.size());
    EXPECT_EQ(suffix, "extension");

    const auto copied_path = test_dir / "copied.bin";
    ASSERT_TRUE(CopyFile(archive_path / "eboot.bin", copied_path));
    std::ifstream copied{copied_path, std::ios::binary};
    const std::string copied_contents{std::istreambuf_iterator<char>{copied}, {}};
    EXPECT_EQ(copied_contents, "executable data");

    SetUserPath(PathType::TempDataDir, test_dir);
    IOFile materialized{archive_path / "eboot.bin", FileAccessMode::Read};
    ASSERT_TRUE(materialized.MaterializeToHost());
    EXPECT_FALSE(materialized.IsZarBacked());
    EXPECT_TRUE(fs::is_regular_file(materialized.GetPath()));
    EXPECT_EQ(materialized.ReadString(15), "executable data");
}

TEST_F(ZarFsTest, FindsArchivedGameById) {
    const auto found = FindGameByID(test_dir, "CUSA00001", 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, archive_path / "eboot.bin");
}

TEST_F(ZarFsTest, ResolvesLooseOverlayPaths) {
    EXPECT_EQ(GetLooseOverlayPath(archive_path, "-UPDATE"), test_dir / "CUSA00001-UPDATE");
    EXPECT_EQ(GetLooseOverlayPath(archive_path, "-patch"), test_dir / "CUSA00001-patch");
    EXPECT_EQ(GetLooseOverlayPath(archive_path, "-mods"), test_dir / "CUSA00001-mods");

    const auto directory_path = test_dir / "CUSA00002";
    ASSERT_TRUE(fs::create_directory(directory_path));
    EXPECT_EQ(GetLooseOverlayPath(directory_path, "-UPDATE"), test_dir / "CUSA00002-UPDATE");
}

TEST_F(ZarFsTest, KeepsOpenFileValidAfterCacheEviction) {
    auto file = OpenFile(archive_path / "eboot.bin");
    ASSERT_NE(file, nullptr);

    for (int i = 0; i < 9; ++i) {
        const auto other_archive = test_dir / ("other_" + std::to_string(i) + ".zar");
        ASSERT_TRUE(CreateArchive(other_archive));
        EXPECT_TRUE(Exists(other_archive / "eboot.bin"));
    }

    std::string contents(file->GetSize(), '\0');
    ASSERT_EQ(file->Read(contents.data(), contents.size()), contents.size());
    EXPECT_EQ(contents, "executable data");
}

} // Anonymous namespace
} // namespace Common::FS::Zar
