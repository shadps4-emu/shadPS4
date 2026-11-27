// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_backup.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <ranges>
#include <semaphore>
#include <system_error>
#include <vector>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "save_db.h"
#include "save_instance.h"
#include "savedata_error.h"

namespace fs = std::filesystem;

namespace Libraries::SaveData {
void DispatchBackupEvent(const Backup::BackupRequest& request);
}

namespace Libraries::SaveData::Backup {

namespace {

constexpr std::string_view BackupPrefix = "sce_bu_";
constexpr std::string_view BackupPrefixTmp = "sce_bu_tmp_";
constexpr std::string_view BackupPrefixOld = "sce_bu_old_";

Kernel::Thread g_backup_thread;
std::counting_semaphore g_backup_thread_semaphore{0};

std::mutex g_backup_running_mutex;
std::mutex g_backup_queue_mutex;
std::deque<BackupRequest> g_backup_queue;
std::deque<BackupRequest> g_result_queue;

std::atomic_int g_backup_progress = 0;
std::atomic<WorkerStatus> g_backup_status = WorkerStatus::NotStarted;
std::atomic_bool g_backup_stopping = false;

void CopyShareReadWrite(const fs::path& from, const fs::path& to) {
    if (from.extension() == ".tmp") {
        return;
    }
    std::error_code ec;
    if (fs::is_directory(from, ec)) {
        fs::create_directories(to);
        for (const auto& entry : fs::directory_iterator(from)) {
            CopyShareReadWrite(entry.path(), to / entry.path().filename());
        }
        return;
    }

    Common::FS::IOFile src{from, Common::FS::FileAccessMode::Read, Common::FS::FileType::BinaryFile,
                           Common::FS::FileShareFlag::ShareReadWrite};
    if (!src.IsOpen()) {
        LOG_ERROR(Lib_SaveData, "Backup: failed to open source {}", fmt::UTF(from.u8string()));
        throw fs::filesystem_error("backup source open failed", from,
                                   std::make_error_code(std::errc::io_error));
    }
    const u64 size = src.GetSize();
    std::vector<u8> buf(size);
    if (size > 0 && src.ReadRaw<u8>(buf.data(), size) != size) {
        throw fs::filesystem_error("backup source read failed", from,
                                   std::make_error_code(std::errc::io_error));
    }
    src.Close();

    Common::FS::IOFile dst{to, Common::FS::FileAccessMode::Create, Common::FS::FileType::BinaryFile,
                           Common::FS::FileShareFlag::ShareReadWrite};
    if (!dst.IsOpen()) {
        LOG_ERROR(Lib_SaveData, "Backup: failed to open destination {}", fmt::UTF(to.u8string()));
        throw fs::filesystem_error("backup destination open failed", to,
                                   std::make_error_code(std::errc::io_error));
    }
    if (size > 0 && dst.WriteRaw<u8>(buf.data(), size) != size) {
        throw fs::filesystem_error("backup destination write failed", to,
                                   std::make_error_code(std::errc::io_error));
    }
    dst.Close();
}

void SyncBackupMeta(const BackupRequest& req, const fs::path& backup_dir) {
    if (req.title_id.empty() || req.dir_name.empty()) {
        return;
    }
    std::error_code ec;
    const fs::path meta_dir = GetMetaTitlePath(req.user_id, req.title_id);
    fs::create_directories(meta_dir, ec);

    const fs::path src_sfo = SaveInstance::GetParamSFOPath(backup_dir);
    if (fs::exists(src_sfo, ec)) {
        fs::copy_file(src_sfo, GetMetaBackupSfoPath(req.user_id, req.title_id, req.dir_name),
                      fs::copy_options::overwrite_existing, ec);
    }

    const fs::path src_icon = SaveInstance::GetIconPath(backup_dir);
    const fs::path dst_icon = GetMetaBackupIconPath(req.user_id, req.title_id, req.dir_name);
    if (fs::exists(src_icon, ec)) {
        fs::copy_file(src_icon, dst_icon, fs::copy_options::overwrite_existing, ec);
    } else {
        fs::remove(dst_icon, ec);
    }
}

void PerformBackup(const fs::path& save_path) {
    std::unique_lock lk{g_backup_running_mutex};
    if (!fs::exists(save_path)) {
        throw fs::filesystem_error("backup source directory missing", save_path,
                                   std::make_error_code(std::errc::no_such_file_or_directory));
    }

    const auto parent = save_path.parent_path();
    const auto name = save_path.filename().string();
    const auto backup_dir = parent / (std::string(BackupPrefix) + name);
    const auto backup_dir_tmp = parent / (std::string(BackupPrefixTmp) + name);
    const auto backup_dir_old = parent / (std::string(BackupPrefixOld) + name);

    fs::remove_all(backup_dir_tmp);
    fs::remove_all(backup_dir_old);

    std::vector<fs::path> backup_files;
    for (const auto& entry : fs::directory_iterator(save_path)) {
        if (entry.path().extension() == ".tmp") {
            continue;
        }
        backup_files.push_back(entry.path());
    }

    g_backup_progress = 0;

    const int total_count = static_cast<int>(backup_files.size());
    int current_count = 0;

    fs::create_directory(backup_dir_tmp);
    for (const auto& file : backup_files) {
        CopyShareReadWrite(file, backup_dir_tmp / file.filename());
        current_count++;
        g_backup_progress = total_count > 0 ? (current_count * 100 / total_count) : 100;
    }
    if (total_count == 0) {
        g_backup_progress = 100;
    }

    const bool has_existing_backup = fs::exists(backup_dir);
    if (has_existing_backup) {
        fs::rename(backup_dir, backup_dir_old);
    }
    fs::rename(backup_dir_tmp, backup_dir);
    if (has_existing_backup) {
        fs::remove_all(backup_dir_old);
    }
}

void BackupThreadBody() {
    Common::SetCurrentThreadName("shadPS4:SaveData:BackupThread");
    while (!g_backup_stopping) {
        g_backup_status = WorkerStatus::Waiting;

        BackupRequest req;
        bool has_item = false;
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            if (!g_backup_queue.empty()) {
                req = g_backup_queue.front();
                has_item = true;
            }
        }
        if (!has_item) {
            g_backup_thread_semaphore.acquire();
            std::scoped_lock lk{g_backup_queue_mutex};
            if (g_backup_queue.empty()) {
                continue;
            }
            req = g_backup_queue.front();
        }
        if (req.save_path.empty()) {
            break;
        }
        g_backup_status = WorkerStatus::Running;

        LOG_INFO(Lib_SaveData, "Backing up {}", fmt::UTF(req.save_path.u8string()));
        try {
            PerformBackup(req.save_path);
            SyncBackupMeta(req, MakeBackupPath(req.save_path));
        } catch (const std::exception& err) {
            req.error_code = static_cast<s32>(OrbisSaveDataError::INTERNAL);
            LOG_ERROR(Lib_SaveData, "Failed to backup {}: {}", fmt::UTF(req.save_path.u8string()),
                      err.what());
        }
        LOG_INFO(Lib_SaveData, "Backup finished: {}, error=0x{:08x}",
                 fmt::UTF(req.save_path.u8string()), static_cast<u32>(req.error_code));
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            if (!g_backup_queue.empty()) {
                g_backup_queue.front().done = true;
                g_backup_queue.pop_front();
            }
            if (req.origin != OrbisSaveDataEventType::__DO_NOT_SAVE) {
                g_result_queue.push_back(req);
                if (g_result_queue.size() > 20) {
                    g_result_queue.pop_front();
                }
            }
        }
        if (req.origin != OrbisSaveDataEventType::__DO_NOT_SAVE) {
            DispatchBackupEvent(req);
        }
    }
    g_backup_status = WorkerStatus::NotStarted;
}

} // namespace

void StartThread() {
    if (g_backup_status != WorkerStatus::NotStarted) {
        return;
    }
    g_backup_thread.Join();
    while (g_backup_thread_semaphore.try_acquire()) {
    }
    g_backup_stopping = false;
    LOG_DEBUG(Lib_SaveData, "Starting backup thread");
    g_backup_status = WorkerStatus::Waiting;
    g_backup_thread.Run([](std::stop_token) { BackupThreadBody(); });
}

void StopThread() {
    if (g_backup_status == WorkerStatus::NotStarted || g_backup_status == WorkerStatus::Stopping) {
        return;
    }
    LOG_DEBUG(Lib_SaveData, "Stopping backup thread");
    g_backup_stopping = true;
    g_backup_status = WorkerStatus::Stopping;
    {
        std::scoped_lock lk{g_backup_queue_mutex};
        g_backup_queue.emplace_back(BackupRequest{});
    }
    g_backup_thread_semaphore.release();
    if (Kernel::g_curthread != nullptr) {
        g_backup_thread.Join();
    } else {
        for (int i = 0; i < 100 && g_backup_status != WorkerStatus::NotStarted; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    std::scoped_lock lk{g_backup_queue_mutex};
    g_backup_queue.clear();
    g_result_queue.clear();
}

bool NewRequest(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                std::string_view dir_name, OrbisSaveDataEventType origin) {
    auto save_path = GetSaveDirPath(user_id, title_id, dir_name);

    if (g_backup_stopping ||
        (g_backup_status != WorkerStatus::Waiting && g_backup_status != WorkerStatus::Running)) {
        LOG_ERROR(Lib_SaveData, "Called backup while status is {}. Backup request to {} ignored",
                  magic_enum::enum_name(g_backup_status.load()), fmt::UTF(save_path.u8string()));
        return false;
    }
    {
        std::scoped_lock lk{g_backup_queue_mutex};
        for (const auto& it : g_backup_queue) {
            if (it.user_id == user_id && it.title_id == title_id && it.dir_name == dir_name) {
                LOG_TRACE(Lib_SaveData, "Backup request to {} ignored. Already queued", dir_name);
                return false;
            }
        }
        g_backup_queue.push_back(BackupRequest{
            .user_id = user_id,
            .title_id = std::string{title_id},
            .dir_name = std::string{dir_name},
            .origin = origin,
            .save_path = std::move(save_path),
        });
    }
    g_backup_thread_semaphore.release();
    return true;
}

bool Restore(const fs::path& save_path) {
    LOG_INFO(Lib_SaveData, "Restoring backup for {}", fmt::UTF(save_path.u8string()));
    std::unique_lock lk{g_backup_running_mutex};
    const auto backup_dir = MakeBackupPath(save_path);
    std::error_code ec;
    if (!fs::exists(backup_dir, ec) || !fs::exists(SaveInstance::GetParamSFOPath(backup_dir), ec)) {
        return false;
    }

    const fs::path restore_tmp =
        save_path.parent_path() / fmt::format("sce_restore_tmp_{}", save_path.filename().string());
    const fs::path restore_old =
        save_path.parent_path() / fmt::format("sce_restore_old_{}", save_path.filename().string());
    fs::remove_all(restore_tmp, ec);
    fs::remove_all(restore_old, ec);

    try {
        fs::create_directories(restore_tmp);
        for (const auto& entry : fs::directory_iterator(backup_dir)) {
            CopyShareReadWrite(entry.path(), restore_tmp / entry.path().filename());
        }
        const bool had_save = fs::exists(save_path, ec);
        if (had_save) {
            fs::rename(save_path, restore_old);
        }
        fs::rename(restore_tmp, save_path);
        if (had_save) {
            fs::remove_all(restore_old, ec);
        }
    } catch (const std::exception& err) {
        LOG_ERROR(Lib_SaveData, "Restore failed for {}: {}", fmt::UTF(save_path.u8string()),
                  err.what());
        fs::remove_all(restore_tmp, ec);
        return false;
    }
    return true;
}

std::unique_lock<std::mutex> LockForWrite() {
    return std::unique_lock{g_backup_running_mutex};
}

bool Restore(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
             std::string_view dir_name) {
    const fs::path save_path = GetSaveDirPath(user_id, title_id, dir_name);
    if (!Restore(save_path)) {
        return false;
    }

    PSF sfo;
    if (!sfo.Open(SaveInstance::GetParamSFOPath(save_path))) {
        return false;
    }
    SaveDb::Finalize(user_id,
                     SaveInstance::BuildDbEntryFromSfo(sfo, user_id, title_id, title_id, dir_name,
                                                       save_path, 96, false, false));

    std::error_code ec;
    const fs::path live_icon = SaveInstance::GetIconPath(save_path);
    const fs::path meta_icon = GetMetaIconPath(user_id, title_id, dir_name);
    if (fs::exists(live_icon, ec)) {
        fs::create_directories(meta_icon.parent_path(), ec);
        fs::copy_file(live_icon, meta_icon, fs::copy_options::overwrite_existing, ec);
    } else {
        fs::remove(meta_icon, ec);
    }
    return true;
}

bool DeleteBackup(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                  std::string_view dir_name) {
    std::unique_lock lk{g_backup_running_mutex};
    std::error_code ec;
    fs::remove_all(GetBackupDirPath(user_id, title_id, dir_name), ec);
    fs::remove(GetMetaBackupSfoPath(user_id, title_id, dir_name), ec);
    fs::remove(GetMetaBackupIconPath(user_id, title_id, dir_name), ec);
    return !ec;
}

bool HasBackup(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
               std::string_view dir_name) {
    std::error_code ec;
    const fs::path backup_dir = GetBackupDirPath(user_id, title_id, dir_name);
    return fs::exists(backup_dir, ec) && fs::exists(SaveInstance::GetParamSFOPath(backup_dir), ec);
}

bool CheckBackup(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                 std::string_view dir_name, PSF& out_sfo, fs::path& out_icon_path) {
    std::unique_lock lk{g_backup_running_mutex};
    std::error_code ec;
    const fs::path backup_dir = GetBackupDirPath(user_id, title_id, dir_name);
    const fs::path backup_sfo = SaveInstance::GetParamSFOPath(backup_dir);
    if (fs::exists(backup_sfo, ec) && out_sfo.Open(backup_sfo)) {
        const fs::path container_icon = SaveInstance::GetIconPath(backup_dir);
        const fs::path meta_icon = GetMetaBackupIconPath(user_id, title_id, dir_name);
        if (fs::exists(container_icon, ec)) {
            out_icon_path = container_icon;
        } else if (fs::exists(meta_icon, ec)) {
            out_icon_path = meta_icon;
        } else {
            out_icon_path.clear();
        }
        return true;
    }
    const fs::path meta_sfo = GetMetaBackupSfoPath(user_id, title_id, dir_name);
    if (fs::exists(meta_sfo, ec) && out_sfo.Open(meta_sfo)) {
        const fs::path meta_icon = GetMetaBackupIconPath(user_id, title_id, dir_name);
        out_icon_path = fs::exists(meta_icon, ec) ? meta_icon : fs::path{};
        return true;
    }
    return false;
}

WorkerStatus GetWorkerStatus() {
    return g_backup_status;
}

bool IsBackupExecutingFor(const fs::path& save_path) {
    std::scoped_lock lk{g_backup_queue_mutex};
    const auto it =
        std::ranges::find(g_backup_queue, save_path, [](const auto& v) { return v.save_path; });
    return it != g_backup_queue.end() && !it->done;
}

bool HasPendingEvents() {
    std::scoped_lock lk{g_backup_queue_mutex};
    return !g_backup_queue.empty() || g_backup_status == WorkerStatus::Running;
}

fs::path MakeBackupPath(const fs::path& save_path) {
    return save_path.parent_path() / (std::string(BackupPrefix) + save_path.filename().string());
}

std::optional<BackupRequest> PopLastEvent() {
    std::scoped_lock lk{g_backup_queue_mutex};
    if (g_result_queue.empty()) {
        return std::nullopt;
    }
    auto req = std::move(g_result_queue.front());
    g_result_queue.pop_front();
    return req;
}

void PushBackupEvent(BackupRequest&& req) {
    std::scoped_lock lk{g_backup_queue_mutex};
    g_result_queue.push_back(std::move(req));
    if (g_result_queue.size() > 20) {
        g_result_queue.pop_front();
    }
}

float GetProgress() {
    return static_cast<float>(g_backup_progress) / 100.0f;
}

void ClearProgress() {
    g_backup_progress = 0;
}

} // namespace Libraries::SaveData::Backup
