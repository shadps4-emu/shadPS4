// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/types.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::SaveData {

enum class DbSortKey : u32 {
    DirName = 0,
    UserParam = 1,
    Blocks = 2,
    Mtime = 3,
    FreeBlocks = 5,
};

enum class DbSortOrder : u32 {
    Asc = 0,
    Desc = 1,
};

struct SaveDbEntry {
    s64 id = 0;
    std::string title_id;
    std::string dir_name;
    std::string main_title;
    std::string sub_title;
    std::string detail;
    std::string tmp_dir_name;
    bool is_broken = false;
    u32 user_param = 0;
    u64 blocks = 96;
    u64 free_blocks = 96;
    u64 size_kib = 3072;
    s64 mtime = 0;
    bool fake_broken = false;
    u64 account_id = 0;
    u32 user_id = 0;
    bool faked_owner = false;
    std::string cloud_icon_url;
    s32 cloud_revision = 0;
    std::string game_title_id;
    std::string account_id_in_usb_path = "0000000000000000";
};

// Per-user path layout (8-char lowercase hex user ID)
[[nodiscard]] std::string FormatUidHex(Libraries::UserService::OrbisUserServiceUserId user_id);
[[nodiscard]] std::filesystem::path GetUserHomeDir(
    Libraries::UserService::OrbisUserServiceUserId user_id);
[[nodiscard]] std::filesystem::path GetSaveRootPath(
    Libraries::UserService::OrbisUserServiceUserId user_id);
[[nodiscard]] std::filesystem::path GetSaveTitlePath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id);
[[nodiscard]] std::filesystem::path GetSaveDirPath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
    std::string_view dir_name);
[[nodiscard]] std::filesystem::path GetBackupDirPath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
    std::string_view dir_name);
[[nodiscard]] std::filesystem::path GetMetaRootPath(
    Libraries::UserService::OrbisUserServiceUserId user_id);
[[nodiscard]] std::filesystem::path GetMetaTitlePath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id);
[[nodiscard]] std::filesystem::path GetMetaIconPath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
    std::string_view dir_name);
[[nodiscard]] std::filesystem::path GetMetaBackupIconPath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
    std::string_view dir_name);
[[nodiscard]] std::filesystem::path GetMetaBackupSfoPath(
    Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
    std::string_view dir_name);
[[nodiscard]] std::filesystem::path GetDbPath(
    Libraries::UserService::OrbisUserServiceUserId user_id);

[[nodiscard]] std::string UtcTimeToIso8601(s64 epoch_sec);
[[nodiscard]] s64 Iso8601ToUtcTime(std::string_view iso);
[[nodiscard]] u64 ComputeFreeBlocks(const std::filesystem::path& save_dir, u64 total_blocks);

class SaveDb {
public:
    static void CloseAll();

    static bool EnsureDatabase(Libraries::UserService::OrbisUserServiceUserId user_id);

    static bool UpsertCreating(Libraries::UserService::OrbisUserServiceUserId user_id,
                               const SaveDbEntry& entry);
    static bool Finalize(Libraries::UserService::OrbisUserServiceUserId user_id,
                         const SaveDbEntry& entry);
    static bool MarkBroken(Libraries::UserService::OrbisUserServiceUserId user_id,
                           std::string_view title_id, std::string_view dir_name,
                           bool broken = true);
    static bool SetFakeBroken(Libraries::UserService::OrbisUserServiceUserId user_id,
                              std::string_view title_id, std::string_view dir_name,
                              bool fake_broken);
    static bool Remove(Libraries::UserService::OrbisUserServiceUserId user_id,
                       std::string_view title_id, std::string_view dir_name);
    static bool RemoveTitle(Libraries::UserService::OrbisUserServiceUserId user_id,
                            std::string_view title_id);
    static bool RemoveUser(Libraries::UserService::OrbisUserServiceUserId user_id);

    [[nodiscard]] static std::optional<SaveDbEntry> GetEntry(
        Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
        std::string_view dir_name);

    static bool QueryDirNames(Libraries::UserService::OrbisUserServiceUserId user_id,
                              std::string_view title_id,
                              std::optional<std::string_view> dir_pattern, DbSortKey sort_key,
                              DbSortOrder sort_order, bool include_broken,
                              std::vector<SaveDbEntry>& out_entries);

    [[nodiscard]] static std::optional<u32> GetCount(
        Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
        bool include_broken = false);
    [[nodiscard]] static std::optional<u64> GetTotalSizeBytes(
        Libraries::UserService::OrbisUserServiceUserId user_id,
        std::optional<std::string_view> title_id = std::nullopt);

    static bool RebuildFromDisk(Libraries::UserService::OrbisUserServiceUserId user_id,
                                std::optional<std::string_view> title_id = std::nullopt);
};

} // namespace Libraries::SaveData
