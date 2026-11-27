// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_db.h"

#include <charconv>
#include <chrono>
#include <cstring>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <fmt/format.h>
#include <sqlite3.h>

#include "common/logging/formatter.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/emulator_settings.h"
#include "core/file_format/psf.h"

namespace fs = std::filesystem;

namespace Libraries::SaveData {

namespace {

constexpr u64 BlockSizeBytes = 32768;
constexpr u64 KibPerBlock = 32;

std::mutex g_db_mutex;
std::unordered_map<std::string, sqlite3*> g_db_handles;

struct Stmt {
    sqlite3_stmt* ptr = nullptr;
    Stmt() = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    ~Stmt() {
        if (ptr != nullptr) {
            sqlite3_finalize(ptr);
        }
    }
    bool Prepare(sqlite3* db, const char* sql) {
        return sqlite3_prepare_v2(db, sql, -1, &ptr, nullptr) == SQLITE_OK;
    }
};

void BindText(sqlite3_stmt* stmt, int idx, std::string_view value) {
    sqlite3_bind_text(stmt, idx, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void BindOptionalText(sqlite3_stmt* stmt, int idx, std::string_view value) {
    if (value.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        BindText(stmt, idx, value);
    }
}

std::string ReadText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    if (text == nullptr) {
        return {};
    }
    const int bytes = sqlite3_column_bytes(stmt, col);
    return std::string(reinterpret_cast<const char*>(text), static_cast<size_t>(bytes));
}

std::string EscapeLikePattern(std::string_view pattern) {
    std::string escaped;
    escaped.reserve(pattern.size());
    for (const char ch : pattern) {
        if (ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

SaveDbEntry ReadEntryFromStmt(sqlite3_stmt* stmt) {
    SaveDbEntry entry;
    entry.id = sqlite3_column_int64(stmt, 0);
    entry.title_id = ReadText(stmt, 1);
    entry.dir_name = ReadText(stmt, 2);
    entry.main_title = ReadText(stmt, 3);
    entry.sub_title = ReadText(stmt, 4);
    entry.detail = ReadText(stmt, 5);
    entry.tmp_dir_name = ReadText(stmt, 6);
    entry.is_broken = sqlite3_column_int(stmt, 7) != 0;
    entry.user_param = static_cast<u32>(sqlite3_column_int64(stmt, 8));
    entry.blocks = static_cast<u64>(std::max<s64>(0, sqlite3_column_int64(stmt, 9)));
    entry.free_blocks = static_cast<u64>(std::max<s64>(0, sqlite3_column_int64(stmt, 10)));
    entry.size_kib = static_cast<u64>(std::max<s64>(0, sqlite3_column_int64(stmt, 11)));
    entry.mtime = Iso8601ToUtcTime(ReadText(stmt, 12));
    entry.fake_broken = sqlite3_column_int(stmt, 13) != 0;
    entry.account_id = static_cast<u64>(sqlite3_column_int64(stmt, 14));
    entry.user_id = static_cast<u32>(sqlite3_column_int64(stmt, 15));
    entry.faked_owner = sqlite3_column_int(stmt, 16) != 0;
    entry.cloud_icon_url = ReadText(stmt, 17);
    entry.cloud_revision = sqlite3_column_int(stmt, 18);
    entry.game_title_id = ReadText(stmt, 19);
    entry.account_id_in_usb_path = ReadText(stmt, 20);
    if (entry.account_id_in_usb_path.empty()) {
        entry.account_id_in_usb_path = "0000000000000000";
    }
    return entry;
}

constexpr const char* kSelectAllColumnsSql =
    "SELECT id, title_id, dir_name, main_title, sub_title, detail, tmp_dir_name, is_broken, "
    "user_param, blocks, free_blocks, size_kib, mtime, fake_broken, account_id, user_id, "
    "faked_owner, cloud_icon_url, cloud_revision, game_title_id, account_id_in_usb_path "
    "FROM savedata";

bool EnsureSchema(sqlite3* db) {
    static constexpr const char* kSchemaSql =
        "CREATE TABLE IF NOT EXISTS savedata ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "title_id NOT NULL,"
        "dir_name NOT NULL,"
        "main_title NOT NULL,"
        "sub_title,"
        "detail,"
        "tmp_dir_name,"
        "is_broken,"
        "user_param,"
        "blocks,"
        "free_blocks,"
        "size_kib,"
        "mtime NOT NULL,"
        "fake_broken,"
        "account_id,"
        "user_id,"
        "faked_owner,"
        "cloud_icon_url,"
        "cloud_revision,"
        "game_title_id NOT NULL,"
        "account_id_in_usb_path);"
        "CREATE INDEX IF NOT EXISTS savedata_index on savedata("
        "title_id, dir_name, user_param, size_kib, mtime, game_title_id);";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, kSchemaSql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        LOG_ERROR(Lib_SaveData, "SaveDb schema initialization failed: {}",
                  err_msg ? err_msg : sqlite3_errmsg(db));
        if (err_msg != nullptr) {
            sqlite3_free(err_msg);
        }
        return false;
    }
    return true;
}

sqlite3* OpenLocked(Libraries::UserService::OrbisUserServiceUserId user_id) {
    const fs::path db_path = GetDbPath(user_id);
    const std::string key = Common::FS::PathToUTF8String(db_path);
    if (const auto it = g_db_handles.find(key); it != g_db_handles.end()) {
        return it->second;
    }

    std::error_code ec;
    fs::create_directories(db_path.parent_path(), ec);
    if (ec) {
        LOG_ERROR(Lib_SaveData, "Failed to create savedata DB directory {}: {}",
                  fmt::UTF(db_path.parent_path().u8string()), ec.message());
        return nullptr;
    }

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(key.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) !=
        SQLITE_OK) {
        LOG_ERROR(Lib_SaveData, "Failed to open savedata.db at {}: {}", key,
                  db ? sqlite3_errmsg(db) : "OOM");
        if (db != nullptr) {
            sqlite3_close_v2(db);
        }
        return nullptr;
    }

    sqlite3_busy_timeout(db, 2000);
    sqlite3_exec(db, "PRAGMA case_sensitive_like = ON;", nullptr, nullptr, nullptr);
    if (!EnsureSchema(db)) {
        sqlite3_close_v2(db);
        return nullptr;
    }

    g_db_handles.emplace(key, db);
    return db;
}

void CloseLockedForUser(Libraries::UserService::OrbisUserServiceUserId user_id) {
    const fs::path db_path = GetDbPath(user_id);
    const std::string key = Common::FS::PathToUTF8String(db_path);
    if (const auto it = g_db_handles.find(key); it != g_db_handles.end()) {
        sqlite3_close_v2(it->second);
        g_db_handles.erase(it);
    }
}

bool UpsertEntryLocked(sqlite3* db, const SaveDbEntry& entry, bool is_broken_override) {
    std::optional<s64> existing_id;
    {
        Stmt sel;
        if (!sel.Prepare(db,
                         "SELECT id FROM savedata WHERE title_id = ? AND dir_name = ? LIMIT 1;")) {
            return false;
        }
        BindText(sel.ptr, 1, entry.title_id);
        BindText(sel.ptr, 2, entry.dir_name);
        const int rc = sqlite3_step(sel.ptr);
        if (rc == SQLITE_ROW) {
            existing_id = sqlite3_column_int64(sel.ptr, 0);
        } else if (rc != SQLITE_DONE) {
            return false;
        }
    }

    const s64 effective_mtime = entry.mtime != 0
                                    ? entry.mtime
                                    : std::chrono::duration_cast<std::chrono::seconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
    const std::string mtime_str = UtcTimeToIso8601(effective_mtime);
    const std::string game_tid = entry.game_title_id.empty() ? entry.title_id : entry.game_title_id;
    const u64 size_kib = entry.blocks * KibPerBlock;
    const std::string& usb_acct = entry.account_id_in_usb_path.empty()
                                      ? std::string("0000000000000000")
                                      : entry.account_id_in_usb_path;

    Stmt stmt;
    if (existing_id.has_value()) {
        if (!stmt.Prepare(
                db, "UPDATE savedata SET main_title=?, sub_title=?, detail=?, tmp_dir_name=?, "
                    "is_broken=?, user_param=?, blocks=?, free_blocks=?, size_kib=?, mtime=?, "
                    "fake_broken=?, account_id=?, user_id=?, faked_owner=?, cloud_icon_url=?, "
                    "cloud_revision=?, game_title_id=?, account_id_in_usb_path=? WHERE id=?;")) {
            return false;
        }
        BindText(stmt.ptr, 1, entry.main_title);
        BindOptionalText(stmt.ptr, 2, entry.sub_title);
        BindOptionalText(stmt.ptr, 3, entry.detail);
        BindOptionalText(stmt.ptr, 4, entry.tmp_dir_name);
        sqlite3_bind_int(stmt.ptr, 5, is_broken_override ? 1 : 0);
        sqlite3_bind_int64(stmt.ptr, 6, static_cast<s64>(entry.user_param));
        sqlite3_bind_int64(stmt.ptr, 7, static_cast<s64>(entry.blocks));
        sqlite3_bind_int64(stmt.ptr, 8, static_cast<s64>(entry.free_blocks));
        sqlite3_bind_int64(stmt.ptr, 9, static_cast<s64>(size_kib));
        BindText(stmt.ptr, 10, mtime_str);
        sqlite3_bind_int(stmt.ptr, 11, entry.fake_broken ? 1 : 0);
        sqlite3_bind_int64(stmt.ptr, 12, static_cast<s64>(entry.account_id));
        sqlite3_bind_int64(stmt.ptr, 13, static_cast<s64>(entry.user_id));
        sqlite3_bind_int(stmt.ptr, 14, entry.faked_owner ? 1 : 0);
        BindOptionalText(stmt.ptr, 15, entry.cloud_icon_url);
        if (entry.cloud_revision != 0) {
            sqlite3_bind_int(stmt.ptr, 16, entry.cloud_revision);
        } else {
            sqlite3_bind_null(stmt.ptr, 16);
        }
        BindText(stmt.ptr, 17, game_tid);
        BindText(stmt.ptr, 18, usb_acct);
        sqlite3_bind_int64(stmt.ptr, 19, *existing_id);
        return sqlite3_step(stmt.ptr) == SQLITE_DONE;
    }

    if (!stmt.Prepare(
            db, "INSERT INTO savedata (title_id, dir_name, main_title, sub_title, detail, "
                "tmp_dir_name, is_broken, user_param, blocks, free_blocks, size_kib, mtime, "
                "fake_broken, account_id, user_id, faked_owner, cloud_icon_url, cloud_revision, "
                "game_title_id, account_id_in_usb_path) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);")) {
        return false;
    }
    BindText(stmt.ptr, 1, entry.title_id);
    BindText(stmt.ptr, 2, entry.dir_name);
    BindText(stmt.ptr, 3, entry.main_title);
    BindOptionalText(stmt.ptr, 4, entry.sub_title);
    BindOptionalText(stmt.ptr, 5, entry.detail);
    BindOptionalText(stmt.ptr, 6, entry.tmp_dir_name);
    sqlite3_bind_int(stmt.ptr, 7, is_broken_override ? 1 : 0);
    sqlite3_bind_int64(stmt.ptr, 8, static_cast<s64>(entry.user_param));
    sqlite3_bind_int64(stmt.ptr, 9, static_cast<s64>(entry.blocks));
    sqlite3_bind_int64(stmt.ptr, 10, static_cast<s64>(entry.free_blocks));
    sqlite3_bind_int64(stmt.ptr, 11, static_cast<s64>(size_kib));
    BindText(stmt.ptr, 12, mtime_str);
    sqlite3_bind_int(stmt.ptr, 13, entry.fake_broken ? 1 : 0);
    sqlite3_bind_int64(stmt.ptr, 14, static_cast<s64>(entry.account_id));
    sqlite3_bind_int64(stmt.ptr, 15, static_cast<s64>(entry.user_id));
    sqlite3_bind_int(stmt.ptr, 16, entry.faked_owner ? 1 : 0);
    BindOptionalText(stmt.ptr, 17, entry.cloud_icon_url);
    if (entry.cloud_revision != 0) {
        sqlite3_bind_int(stmt.ptr, 18, entry.cloud_revision);
    } else {
        sqlite3_bind_null(stmt.ptr, 18);
    }
    BindText(stmt.ptr, 19, game_tid);
    BindText(stmt.ptr, 20, usb_acct);
    return sqlite3_step(stmt.ptr) == SQLITE_DONE;
}

bool ParseSfoToDbEntry(Libraries::UserService::OrbisUserServiceUserId user_id,
                       std::string_view title_id, std::string_view dir_name,
                       const fs::path& save_dir, SaveDbEntry& out_entry) {
    const fs::path sfo_path = save_dir / "sce_sys" / "param.sfo";
    PSF sfo;
    if (!sfo.Open(sfo_path)) {
        return false;
    }

    out_entry = SaveDbEntry{};
    out_entry.title_id = std::string(sfo.GetString("TITLE_ID").value_or(title_id));
    if (out_entry.title_id.empty()) {
        out_entry.title_id = std::string(title_id);
    }
    out_entry.dir_name = std::string(sfo.GetString("SAVEDATA_DIRECTORY").value_or(dir_name));
    if (out_entry.dir_name.empty()) {
        out_entry.dir_name = std::string(dir_name);
    }
    out_entry.main_title = std::string(sfo.GetString("MAINTITLE").value_or(""));
    out_entry.sub_title = std::string(sfo.GetString("SUBTITLE").value_or(""));
    out_entry.detail = std::string(sfo.GetString("DETAIL").value_or(""));
    out_entry.user_param = static_cast<u32>(sfo.GetInteger("SAVEDATA_LIST_PARAM").value_or(0));

    u64 blocks = 96;
    if (const auto blocks_bin = sfo.GetBinary("SAVEDATA_BLOCKS");
        blocks_bin && blocks_bin->size() >= sizeof(u64)) {
        std::memcpy(&blocks, blocks_bin->data(), sizeof(u64));
    }
    if (blocks == 0) {
        blocks = 96;
    }
    out_entry.blocks = blocks;
    out_entry.size_kib = blocks * KibPerBlock;
    out_entry.free_blocks = ComputeFreeBlocks(save_dir, blocks);

    if (const auto acct_bin = sfo.GetBinary("ACCOUNT_ID");
        acct_bin && acct_bin->size() >= sizeof(u64)) {
        std::memcpy(&out_entry.account_id, acct_bin->data(), sizeof(u64));
    }

    out_entry.user_id = static_cast<u32>(user_id);
    if (const auto params_bin = sfo.GetBinary("PARAMS"); params_bin && params_bin->size() >= 8) {
        u32 sfo_uid = 0;
        std::memcpy(&sfo_uid, params_bin->data() + 4, sizeof(u32));
        if (sfo_uid != 0) {
            out_entry.user_id = sfo_uid;
        }
    }

    out_entry.mtime =
        std::chrono::duration_cast<std::chrono::seconds>(sfo.GetLastWrite().time_since_epoch())
            .count();
    out_entry.game_title_id = out_entry.title_id;
    return true;
}

bool RebuildTitleLocked(sqlite3* db, Libraries::UserService::OrbisUserServiceUserId user_id,
                        std::string_view tid) {
    {
        Stmt del;
        if (!del.Prepare(db, "DELETE FROM savedata WHERE title_id = ?;")) {
            return false;
        }
        BindText(del.ptr, 1, tid);
        if (sqlite3_step(del.ptr) != SQLITE_DONE) {
            return false;
        }
    }

    const fs::path title_dir = GetSaveTitlePath(user_id, tid);
    std::error_code ec;
    if (!fs::exists(title_dir, ec) || !fs::is_directory(title_dir, ec)) {
        return true;
    }

    for (const auto& entry : fs::directory_iterator(title_dir, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }
        const std::string dir_name = entry.path().filename().string();
        if (dir_name.starts_with("sce_bu_")) {
            continue;
        }
        SaveDbEntry db_entry;
        if (ParseSfoToDbEntry(user_id, tid, dir_name, entry.path(), db_entry)) {
            db_entry.is_broken = false;
            UpsertEntryLocked(db, db_entry, false);

            // Keep savedata_meta icon synchronized when container icon exists
            const fs::path container_icon = entry.path() / "sce_sys" / "icon0.png";
            const fs::path meta_icon = GetMetaIconPath(user_id, tid, dir_name);
            if (fs::exists(container_icon, ec) && !fs::exists(meta_icon, ec)) {
                fs::create_directories(meta_icon.parent_path(), ec);
                fs::copy_file(container_icon, meta_icon, fs::copy_options::overwrite_existing, ec);
            }
        } else {
            SaveDbEntry broken;
            broken.title_id = std::string(tid);
            broken.dir_name = dir_name;
            broken.user_id = static_cast<u32>(user_id);
            broken.game_title_id = std::string(tid);
            broken.is_broken = true;
            UpsertEntryLocked(db, broken, true);
        }
    }
    return true;
}

} // namespace

std::string FormatUidHex(Libraries::UserService::OrbisUserServiceUserId user_id) {
    return fmt::format("{:08x}", static_cast<u32>(user_id));
}

fs::path GetUserHomeDir(Libraries::UserService::OrbisUserServiceUserId user_id) {
    return EmulatorSettings.GetHomeDir() / FormatUidHex(user_id);
}

fs::path GetSaveRootPath(Libraries::UserService::OrbisUserServiceUserId user_id) {
    return GetUserHomeDir(user_id) / "savedata";
}

fs::path GetSaveTitlePath(Libraries::UserService::OrbisUserServiceUserId user_id,
                          std::string_view title_id) {
    return GetSaveRootPath(user_id) / title_id;
}

fs::path GetSaveDirPath(Libraries::UserService::OrbisUserServiceUserId user_id,
                        std::string_view title_id, std::string_view dir_name) {
    return GetSaveTitlePath(user_id, title_id) / dir_name;
}

fs::path GetBackupDirPath(Libraries::UserService::OrbisUserServiceUserId user_id,
                          std::string_view title_id, std::string_view dir_name) {
    return GetSaveTitlePath(user_id, title_id) / fmt::format("sce_bu_{}", dir_name);
}

fs::path GetMetaRootPath(Libraries::UserService::OrbisUserServiceUserId user_id) {
    return GetUserHomeDir(user_id) / "savedata_meta" / "user";
}

fs::path GetMetaTitlePath(Libraries::UserService::OrbisUserServiceUserId user_id,
                          std::string_view title_id) {
    return GetMetaRootPath(user_id) / title_id;
}

fs::path GetMetaIconPath(Libraries::UserService::OrbisUserServiceUserId user_id,
                         std::string_view title_id, std::string_view dir_name) {
    return GetMetaTitlePath(user_id, title_id) / fmt::format("{}_icon0.png", dir_name);
}

fs::path GetMetaBackupIconPath(Libraries::UserService::OrbisUserServiceUserId user_id,
                               std::string_view title_id, std::string_view dir_name) {
    return GetMetaTitlePath(user_id, title_id) / fmt::format("sce_bu_{}_icon0.png", dir_name);
}

fs::path GetMetaBackupSfoPath(Libraries::UserService::OrbisUserServiceUserId user_id,
                              std::string_view title_id, std::string_view dir_name) {
    return GetMetaTitlePath(user_id, title_id) / fmt::format("sce_bu_{}.sfo", dir_name);
}

fs::path GetDbPath(Libraries::UserService::OrbisUserServiceUserId user_id) {
    return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "system_data" / "savedata" /
           FormatUidHex(user_id) / "db" / "user" / "savedata.db";
}

std::string UtcTimeToIso8601(s64 epoch_sec) {
    if (epoch_sec < 0) {
        epoch_sec = 0;
    }
    const s64 days = epoch_sec / 86400;
    const s64 rem = epoch_sec % 86400;
    const s64 hour = rem / 3600;
    const s64 min = (rem % 3600) / 60;
    const s64 sec = rem % 60;

    // Howard Hinnant's civil-from-days algorithm
    const s64 z = days + 719468;
    const s64 era = (z >= 0 ? z : z - 146096) / 146097;
    const u64 doe = static_cast<u64>(z - era * 146097);
    const u64 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    s64 year = static_cast<s64>(yoe) + era * 400;
    const u64 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const u64 mp = (5 * doy + 2) / 153;
    const u64 day = doy - (153 * mp + 2) / 5 + 1;
    const u64 month = mp + (mp < 10 ? 3 : static_cast<u64>(-9));
    year += (month <= 2);

    return fmt::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.00Z", year, month, day, hour, min,
                       sec);
}

s64 Iso8601ToUtcTime(std::string_view iso) {
    if (iso.size() < 19 || iso[4] != '-' || iso[7] != '-' || (iso[10] != 'T' && iso[10] != ' ') ||
        iso[13] != ':' || iso[16] != ':') {
        return 0;
    }
    auto parse_u = [&](size_t pos, size_t len, s64& out) -> bool {
        const char* first = iso.data() + pos;
        const char* last = first + len;
        auto [ptr, ec] = std::from_chars(first, last, out);
        return ec == std::errc{} && ptr == last;
    };
    s64 year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    if (!parse_u(0, 4, year) || !parse_u(5, 2, month) || !parse_u(8, 2, day) ||
        !parse_u(11, 2, hour) || !parse_u(14, 2, min) || !parse_u(17, 2, sec)) {
        return 0;
    }
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }
    year -= (month <= 2);
    const s64 era = (year >= 0 ? year : year - 399) / 400;
    const u64 yoe = static_cast<u64>(year - era * 400);
    const u64 m_adj = static_cast<u64>(month) + (month > 2 ? static_cast<u64>(-3) : 9);
    const u64 doy = (153 * m_adj + 2) / 5 + static_cast<u64>(day) - 1;
    const u64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const s64 days = era * 146097 + static_cast<s64>(doe) - 719468;
    return days * 86400 + hour * 3600 + min * 60 + sec;
}

u64 ComputeFreeBlocks(const fs::path& save_dir, u64 total_blocks) {
    std::error_code ec;
    if (!fs::exists(save_dir, ec) || !fs::is_directory(save_dir, ec)) {
        return total_blocks;
    }
    u64 used_bytes = 0;
    for (const auto& entry : fs::recursive_directory_iterator(save_dir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        const auto rel = fs::relative(entry.path(), save_dir, ec);
        if (!rel.empty() && *rel.begin() == "sce_sys") {
            continue;
        }
        used_bytes += entry.file_size(ec);
    }
    const u64 used_blocks = (used_bytes + BlockSizeBytes - 1) / BlockSizeBytes;
    return total_blocks > used_blocks ? total_blocks - used_blocks : 0;
}

void SaveDb::CloseAll() {
    std::scoped_lock lk{g_db_mutex};
    for (auto& [path, db] : g_db_handles) {
        sqlite3_close_v2(db);
    }
    g_db_handles.clear();
}

bool SaveDb::EnsureDatabase(Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lk{g_db_mutex};
    return OpenLocked(user_id) != nullptr;
}

bool SaveDb::UpsertCreating(Libraries::UserService::OrbisUserServiceUserId user_id,
                            const SaveDbEntry& entry) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    return UpsertEntryLocked(db, entry, true);
}

bool SaveDb::Finalize(Libraries::UserService::OrbisUserServiceUserId user_id,
                      const SaveDbEntry& entry) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    return UpsertEntryLocked(db, entry, entry.is_broken);
}

bool SaveDb::MarkBroken(Libraries::UserService::OrbisUserServiceUserId user_id,
                        std::string_view title_id, std::string_view dir_name, bool broken) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    Stmt stmt;
    if (!stmt.Prepare(db,
                      "UPDATE savedata SET is_broken = ? WHERE title_id = ? AND dir_name = ?;")) {
        return false;
    }
    sqlite3_bind_int(stmt.ptr, 1, broken ? 1 : 0);
    BindText(stmt.ptr, 2, title_id);
    BindText(stmt.ptr, 3, dir_name);
    return sqlite3_step(stmt.ptr) == SQLITE_DONE;
}

bool SaveDb::SetFakeBroken(Libraries::UserService::OrbisUserServiceUserId user_id,
                           std::string_view title_id, std::string_view dir_name, bool fake_broken) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    Stmt stmt;
    if (!stmt.Prepare(db,
                      "UPDATE savedata SET fake_broken = ? WHERE title_id = ? AND dir_name = ?;")) {
        return false;
    }
    sqlite3_bind_int(stmt.ptr, 1, fake_broken ? 1 : 0);
    BindText(stmt.ptr, 2, title_id);
    BindText(stmt.ptr, 3, dir_name);
    return sqlite3_step(stmt.ptr) == SQLITE_DONE && sqlite3_changes(db) > 0;
}

bool SaveDb::Remove(Libraries::UserService::OrbisUserServiceUserId user_id,
                    std::string_view title_id, std::string_view dir_name) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    Stmt stmt;
    if (!stmt.Prepare(db, "DELETE FROM savedata WHERE title_id = ? AND dir_name = ?;")) {
        return false;
    }
    BindText(stmt.ptr, 1, title_id);
    BindText(stmt.ptr, 2, dir_name);
    return sqlite3_step(stmt.ptr) == SQLITE_DONE;
}

bool SaveDb::RemoveTitle(Libraries::UserService::OrbisUserServiceUserId user_id,
                         std::string_view title_id) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }
    Stmt stmt;
    if (!stmt.Prepare(db, "DELETE FROM savedata WHERE title_id = ?;")) {
        return false;
    }
    BindText(stmt.ptr, 1, title_id);
    return sqlite3_step(stmt.ptr) == SQLITE_DONE;
}

bool SaveDb::RemoveUser(Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lk{g_db_mutex};
    CloseLockedForUser(user_id);
    std::error_code ec;
    const fs::path db_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "system_data" /
                            "savedata" / FormatUidHex(user_id);
    fs::remove_all(db_dir, ec);
    return !ec;
}

std::optional<SaveDbEntry> SaveDb::GetEntry(Libraries::UserService::OrbisUserServiceUserId user_id,
                                            std::string_view title_id, std::string_view dir_name) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return std::nullopt;
    }
    Stmt stmt;
    const std::string sql =
        fmt::format("{} WHERE title_id = ? AND dir_name = ? LIMIT 1;", kSelectAllColumnsSql);
    if (!stmt.Prepare(db, sql.c_str())) {
        return std::nullopt;
    }
    BindText(stmt.ptr, 1, title_id);
    BindText(stmt.ptr, 2, dir_name);
    if (sqlite3_step(stmt.ptr) == SQLITE_ROW) {
        return ReadEntryFromStmt(stmt.ptr);
    }
    return std::nullopt;
}

bool SaveDb::QueryDirNames(Libraries::UserService::OrbisUserServiceUserId user_id,
                           std::string_view title_id, std::optional<std::string_view> dir_pattern,
                           DbSortKey sort_key, DbSortOrder sort_order, bool include_broken,
                           std::vector<SaveDbEntry>& out_entries) {
    out_entries.clear();
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }

    const char* order_col = "dir_name";
    switch (sort_key) {
    case DbSortKey::DirName:
        order_col = "dir_name";
        break;
    case DbSortKey::UserParam:
        order_col = "user_param";
        break;
    case DbSortKey::Blocks:
        order_col = "blocks";
        break;
    case DbSortKey::Mtime:
        order_col = "mtime";
        break;
    case DbSortKey::FreeBlocks:
        order_col = "free_blocks";
        break;
    default:
        order_col = "dir_name";
        break;
    }
    const char* dir_str = sort_order == DbSortOrder::Desc ? "DESC" : "ASC";

    std::string sql = fmt::format("{} WHERE title_id = ?", kSelectAllColumnsSql);
    if (!include_broken) {
        sql += " AND (is_broken IS NULL OR is_broken = 0)";
    }
    if (dir_pattern.has_value()) {
        sql += " AND dir_name LIKE ? ESCAPE '\\'";
    }
    sql += fmt::format(" ORDER BY {} {}", order_col, dir_str);
    if (sort_key != DbSortKey::DirName) {
        sql += ", dir_name ASC";
    }
    sql.push_back(';');

    Stmt stmt;
    if (!stmt.Prepare(db, sql.c_str())) {
        return false;
    }
    BindText(stmt.ptr, 1, title_id);
    std::string escaped;
    if (dir_pattern.has_value()) {
        escaped = EscapeLikePattern(*dir_pattern);
        BindText(stmt.ptr, 2, escaped);
    }

    while (true) {
        const int rc = sqlite3_step(stmt.ptr);
        if (rc == SQLITE_ROW) {
            out_entries.push_back(ReadEntryFromStmt(stmt.ptr));
        } else if (rc == SQLITE_DONE) {
            return true;
        } else {
            out_entries.clear();
            return false;
        }
    }
}

std::optional<u32> SaveDb::GetCount(Libraries::UserService::OrbisUserServiceUserId user_id,
                                    std::string_view title_id, bool include_broken) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return std::nullopt;
    }
    const char* sql = include_broken
                          ? "SELECT COUNT(*) FROM savedata WHERE title_id = ?;"
                          : "SELECT COUNT(*) FROM savedata WHERE title_id = ? AND (is_broken IS "
                            "NULL OR is_broken = 0);";
    Stmt stmt;
    if (!stmt.Prepare(db, sql)) {
        return std::nullopt;
    }
    BindText(stmt.ptr, 1, title_id);
    if (sqlite3_step(stmt.ptr) == SQLITE_ROW) {
        return static_cast<u32>(std::max<s64>(0, sqlite3_column_int64(stmt.ptr, 0)));
    }
    return std::nullopt;
}

std::optional<u64> SaveDb::GetTotalSizeBytes(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             std::optional<std::string_view> title_id) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return std::nullopt;
    }
    const char* sql = title_id.has_value()
                          ? "SELECT COALESCE(SUM(size_kib), 0) FROM savedata WHERE title_id = ?;"
                          : "SELECT COALESCE(SUM(size_kib), 0) FROM savedata;";
    Stmt stmt;
    if (!stmt.Prepare(db, sql)) {
        return std::nullopt;
    }
    if (title_id.has_value()) {
        BindText(stmt.ptr, 1, *title_id);
    }
    if (sqlite3_step(stmt.ptr) == SQLITE_ROW) {
        const u64 total_kib = static_cast<u64>(std::max<s64>(0, sqlite3_column_int64(stmt.ptr, 0)));
        return total_kib * 1024ULL;
    }
    return std::nullopt;
}

bool SaveDb::RebuildFromDisk(Libraries::UserService::OrbisUserServiceUserId user_id,
                             std::optional<std::string_view> title_id) {
    std::scoped_lock lk{g_db_mutex};
    sqlite3* db = OpenLocked(user_id);
    if (db == nullptr) {
        return false;
    }

    sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    bool ok = true;
    if (title_id.has_value()) {
        ok = RebuildTitleLocked(db, user_id, *title_id);
    } else {
        {
            Stmt clear_all;
            ok = clear_all.Prepare(db, "DELETE FROM savedata;") &&
                 sqlite3_step(clear_all.ptr) == SQLITE_DONE;
        }
        if (ok) {
            const fs::path save_root = GetSaveRootPath(user_id);
            std::error_code ec;
            if (fs::exists(save_root, ec) && fs::is_directory(save_root, ec)) {
                for (const auto& tid_entry : fs::directory_iterator(save_root, ec)) {
                    if (!tid_entry.is_directory(ec)) {
                        continue;
                    }
                    if (!RebuildTitleLocked(db, user_id, tid_entry.path().filename().string())) {
                        ok = false;
                        break;
                    }
                }
            }
        }
    }

    if (ok) {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    return ok;
}

} // namespace Libraries::SaveData
