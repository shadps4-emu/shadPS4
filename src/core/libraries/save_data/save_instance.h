// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "common/assert.h"
#include "common/singleton.h"
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "core/libraries/system/userservice.h"
#include "save_db.h"
#include "savedata_error.h"

namespace Libraries::SaveData {

template <size_t N>
class CString {
    std::array<char, N> data{};

public:
    CString() = default;

    CString(std::string_view str) {
        FromString(str);
    }

    void FromString(std::string_view str) {
        ASSERT_MSG(str.size() < N, "String too long ({} >= {})", str.size(), N);
        const size_t len = std::min(str.size(), N - 1);
        std::memcpy(data.data(), str.data(), len);
        std::memset(data.data() + len, 0, N - len);
    }

    [[nodiscard]] std::string to_string() const {
        return std::string{to_view()};
    }

    [[nodiscard]] std::string_view to_view() const {
        return std::string_view{data.data(), ::strnlen(data.data(), N)};
    }

    [[nodiscard]] bool operator==(const CString& o) const {
        return std::memcmp(data.data(), o.data.data(), N) == 0;
    }

    [[nodiscard]] const char* c_str() const {
        return data.data();
    }

    [[nodiscard]] constexpr size_t max_size() const {
        return N - 1;
    }

    [[nodiscard]] bool empty() const {
        return data[0] == '\0';
    }
};

// Validates that png_data is an 8-bit 228x128 RGB PNG matching SceShellCore requirements.
[[nodiscard]] bool ValidateIconPng(std::span<const u8> png_data);

// Truncates UTF-8 text cleanly without exceeding max_bytes or splitting a multi-byte sequence.
[[nodiscard]] std::string TruncateUtf8(std::string_view str, size_t max_bytes);

inline void TruncateUtf8(std::string_view src, char* dst, size_t dst_cap) {
    if (dst == nullptr || dst_cap == 0) {
        return;
    }
    const std::string clean = TruncateUtf8(src, dst_cap - 1);
    std::memset(dst, 0, dst_cap);
    std::memcpy(dst, clean.data(), clean.size());
}

class SaveInstance {
public:
    static constexpr u32 IconWidth = 228;
    static constexpr u32 IconHeight = 128;
    static constexpr size_t IconMaxBytes = 116736; // 228 * 128 * 4

    static std::filesystem::path MakeDirSavePath(
        Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view game_serial,
        std::string_view dir_name);

    static std::filesystem::path GetParamSFOPath(const std::filesystem::path& save_path);
    static std::filesystem::path GetIconPath(const std::filesystem::path& save_path);

    static void SetupDefaultParamSFO(PSF& param_sfo, std::string_view dir_name,
                                     std::string_view title_id,
                                     Libraries::UserService::OrbisUserServiceUserId user_id = 0,
                                     u64 blocks = 96, bool update = true);

    static bool WriteParamSFOAtomic(const PSF& sfo, const std::filesystem::path& sfo_path);

    static SaveDbEntry BuildDbEntryFromSfo(
        const PSF& sfo, Libraries::UserService::OrbisUserServiceUserId user_id,
        std::string_view title_id, std::string_view game_title_id, std::string_view dir_name,
        const std::filesystem::path& save_path, u64 default_blocks, bool is_broken = false,
        bool fake_broken = false);

    SaveInstance(int slot_num, Libraries::UserService::OrbisUserServiceUserId user_id,
                 std::string save_title_id, std::string game_title_id, std::string dir_name,
                 u64 max_blocks = 96, bool read_only = false);

    ~SaveInstance();

    [[nodiscard]] std::string GetMountPoint() const {
        return mount_point;
    }

    [[nodiscard]] std::filesystem::path GetSavePath() const {
        return save_path;
    }

    [[nodiscard]] std::filesystem::path GetParamSFOPath() const {
        return param_sfo_path;
    }

    [[nodiscard]] std::filesystem::path GetIconPath() const {
        return icon_path;
    }

    [[nodiscard]] PSF& GetParamSFO() {
        return param_sfo;
    }

    [[nodiscard]] const PSF& GetParamSFO() const {
        return param_sfo;
    }

    [[nodiscard]] std::string_view GetTitleId() const {
        return title_id;
    }

    [[nodiscard]] std::string_view GetGameTitleId() const {
        return game_title_id;
    }

    [[nodiscard]] std::string_view GetDirName() const {
        return dir_name;
    }

    [[nodiscard]] Libraries::UserService::OrbisUserServiceUserId GetUserId() const {
        return user_id;
    }

    [[nodiscard]] u64 GetMaxBlocks() const {
        return max_blocks;
    }

    [[nodiscard]] bool IsMounted() const {
        return mounted;
    }

    [[nodiscard]] bool IsReadOnly() const {
        return read_only;
    }

    [[nodiscard]] bool Exists() const {
        return exists;
    }

    void MarkSfoDirty() {
        sfo_dirty = true;
    }

    bool SyncMetaIcon(std::span<const u8> icon_bytes = {}) const;

    bool CreateFiles();
    bool SaveParamSFO(bool finalize_db = true);
    bool Mount();
    bool Umount();

private:
    Core::FileSys::MntPoints* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    int slot_num;
    Libraries::UserService::OrbisUserServiceUserId user_id;
    std::string title_id;
    std::string game_title_id;
    std::string dir_name;
    u64 max_blocks = 96;

    std::filesystem::path save_path;
    std::filesystem::path param_sfo_path;
    std::filesystem::path icon_path;
    std::string mount_point;

    PSF param_sfo;
    bool exists = false;
    bool mounted = false;
    bool read_only = false;
    bool sfo_dirty = false;
    bool destruct_off = false;
    bool fake_broken = false;
};

} // namespace Libraries::SaveData
