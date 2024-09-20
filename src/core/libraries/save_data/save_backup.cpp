// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <mutex>
#include <semaphore>

#include <magic_enum.hpp>

#include "save_backup.h"
#include "save_instance.h"

#include "common/logging/log.h"
#include "common/logging/log_entry.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"

constexpr std::string_view sce_sys = "sce_sys";               // system folder inside save
constexpr std::string_view backup_dir = "sce_backup";         // backup folder
constexpr std::string_view backup_dir_tmp = "sce_backup_tmp"; // in-progress backup folder

namespace fs = std::filesystem;

namespace Libraries::SaveData::Backup {

static std::jthread g_backup_thread;
static std::counting_semaphore g_backup_thread_semaphore{0};

static std::mutex g_backup_queue_mutex;
static std::deque<BackupRequest> g_backup_queue;
static std::deque<BackupRequest> g_result_queue;

static std::atomic_int g_backup_progress = 0;
static std::atomic g_backup_status = WorkerStatus::NotStarted;

static void backup(const std::filesystem::path& dir_name) {
    if (!fs::exists(dir_name)) {
        return;
    }
    std::vector<std::filesystem::path> backup_files;
    for (const auto& entry : fs::directory_iterator(dir_name)) {
        const auto filename = entry.path().filename();
        if (filename != backup_dir && filename != backup_dir_tmp) {
            backup_files.push_back(entry.path());
        }
    }
    const auto backup_dir = dir_name / ::backup_dir;
    const auto backup_dir_tmp = dir_name / ::backup_dir_tmp;

    g_backup_progress = 0;

    int total_count = static_cast<int>(backup_files.size());
    int current_count = 0;

    fs::remove_all(backup_dir_tmp);
    fs::create_directory(backup_dir_tmp);
    for (const auto& file : backup_files) {
        fs::copy(file, backup_dir_tmp / file.filename(), fs::copy_options::recursive);
        current_count++;
        g_backup_progress = current_count * 100 / total_count;
    }
    bool has_existing = fs::exists(backup_dir);
    if (has_existing) {
        fs::rename(backup_dir, dir_name / "sce_backup_old");
    }
    fs::rename(backup_dir_tmp, backup_dir);
    if (has_existing) {
        fs::remove_all(dir_name / "sce_backup_old");
    }
}

static void BackupThreadBody() {
    Common::SetCurrentThreadName("SaveData_BackupThread");
    while (true) {
        g_backup_status = WorkerStatus::Waiting;
        g_backup_thread_semaphore.acquire();
        BackupRequest req;
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            req = g_backup_queue.front();
        }
        if (req.save_path.empty()) {
            break;
        }
        g_backup_status = WorkerStatus::Running;
        LOG_INFO(Lib_SaveData, "Backing up the following directory: {}", req.save_path.string());
        backup(req.save_path);
        LOG_DEBUG(Lib_SaveData, "Backing up the following directory: {} finished",
                  req.save_path.string());
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            g_backup_queue.pop_front();
            g_result_queue.push_back(std::move(req));
            if (g_result_queue.size() > 20) {
                g_result_queue.pop_front();
            }
        }
    }
    g_backup_status = WorkerStatus::NotStarted;
}

void StartThread() {
    if (g_backup_status != WorkerStatus::NotStarted) {
        return;
    }
    LOG_DEBUG(Lib_SaveData, "Starting backup thread");
    g_backup_thread = std::jthread{BackupThreadBody};
    g_backup_status = WorkerStatus::Waiting;
}

void StopThread() {
    if (g_backup_status == WorkerStatus::NotStarted || g_backup_status == WorkerStatus::Stopping) {
        return;
    }
    LOG_DEBUG(Lib_SaveData, "Stopping backup thread");
    {
        std::scoped_lock lk{g_backup_queue_mutex};
        g_backup_queue.emplace_back(BackupRequest{});
    }
    g_backup_thread_semaphore.release();
    g_backup_status = WorkerStatus::Stopping;
}

bool NewRequest(OrbisUserServiceUserId user_id, std::string_view title_id,
                std::string_view dir_name, OrbisSaveDataEventType origin) {
    auto save_path = SaveInstance::MakeDirSavePath(user_id, title_id, dir_name);

    if (g_backup_status != WorkerStatus::Waiting && g_backup_status != WorkerStatus::Running) {
        LOG_ERROR(Lib_SaveData, "Called backup while status is {}. Backup request to {} ignored",
                  magic_enum::enum_name(g_backup_status.load()), save_path.string());
        return false;
    }
    {
        std::scoped_lock lk{g_backup_queue_mutex};
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

bool Restore(const std::filesystem::path& save_path) {
    LOG_INFO(Lib_SaveData, "Restoring backup for {}", save_path.string());
    if (!fs::exists(save_path) || !fs::exists(save_path / backup_dir)) {
        return false;
    }
    for (const auto& entry : fs::directory_iterator(save_path)) {
        const auto filename = entry.path().filename();
        if (filename != backup_dir) {
            fs::remove_all(entry.path());
        }
    }

    for (const auto& entry : fs::directory_iterator(save_path / backup_dir)) {
        const auto filename = entry.path().filename();
        fs::copy(entry.path(), save_path / filename, fs::copy_options::recursive);
    }

    return true;
}

WorkerStatus GetWorkerStatus() {
    return g_backup_status;
}

bool IsBackupExecutingFor(const std::filesystem::path& save_path) {
    std::scoped_lock lk{g_backup_queue_mutex};
    return std::ranges::find(g_backup_queue, save_path,
                             [](const auto& v) { return v.save_path; }) != g_backup_queue.end();
}

std::filesystem::path MakeBackupPath(const std::filesystem::path& save_path) {
    return save_path / backup_dir;
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