// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <mutex>
#include <semaphore>

#include <magic_enum/magic_enum.hpp>

#include "save_backup.h"
#include "save_instance.h"

#include "common/logging/log.h"
#include "common/logging/log_entry.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"

constexpr std::string_view sce_sys = "sce_sys";               // system folder inside save
constexpr std::string_view backup_dir = "sce_backup";         // backup folder
constexpr std::string_view backup_dir_tmp = "sce_backup_tmp"; // in-progress backup folder
constexpr std::string_view backup_dir_old = "sce_backup_old"; // previous backup folder

namespace fs = std::filesystem;

namespace Libraries::SaveData::Backup {

static std::jthread g_backup_thread;
static std::counting_semaphore g_backup_thread_semaphore{0};

static std::mutex g_backup_running_mutex;

static std::mutex g_backup_queue_mutex;
static std::deque<BackupRequest> g_backup_queue;
static std::deque<BackupRequest> g_result_queue;

static std::atomic_int g_backup_progress = 0;
static std::atomic g_backup_status = WorkerStatus::NotStarted;

static void backup(const std::filesystem::path& dir_name) {
    std::unique_lock lk{g_backup_running_mutex};
    if (!fs::exists(dir_name)) {
        return;
    }

    const auto backup_dir = dir_name / ::backup_dir;
    const auto backup_dir_tmp = dir_name / ::backup_dir_tmp;
    const auto backup_dir_old = dir_name / ::backup_dir_old;

    fs::remove_all(backup_dir_tmp);
    fs::remove_all(backup_dir_old);

    std::vector<std::filesystem::path> backup_files;
    for (const auto& entry : fs::directory_iterator(dir_name)) {
        const auto filename = entry.path().filename();
        if (filename != ::backup_dir) {
            backup_files.push_back(entry.path());
        }
    }

    g_backup_progress = 0;

    int total_count = static_cast<int>(backup_files.size());
    int current_count = 0;

    fs::create_directory(backup_dir_tmp);
    for (const auto& file : backup_files) {
        fs::copy(file, backup_dir_tmp / file.filename(), fs::copy_options::recursive);
        current_count++;
        g_backup_progress = current_count * 100 / total_count;
    }
    bool has_existing_backup = fs::exists(backup_dir);
    if (has_existing_backup) {
        fs::rename(backup_dir, backup_dir_old);
    }
    fs::rename(backup_dir_tmp, backup_dir);
    if (has_existing_backup) {
        fs::remove_all(backup_dir_old);
    }
}

static void BackupThreadBody() {
    Common::SetCurrentThreadName("shadPS4:SaveData:BackupThread");
    while (g_backup_status != WorkerStatus::Stopping) {
        g_backup_status = WorkerStatus::Waiting;

        bool wait;
        BackupRequest req;
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            wait = g_backup_queue.empty();
            if (!wait) {
                req = g_backup_queue.front();
            }
        }
        if (wait) {
            g_backup_thread_semaphore.acquire();
            {
                std::scoped_lock lk{g_backup_queue_mutex};
                if (g_backup_queue.empty()) {
                    continue;
                }
                req = g_backup_queue.front();
            }
        }
        if (req.save_path.empty()) {
            break;
        }
        g_backup_status = WorkerStatus::Running;

        LOG_INFO(Lib_SaveData, "Backing up the following directory: {}",
                 fmt::UTF(req.save_path.u8string()));
        try {
            backup(req.save_path);
        } catch (const std::filesystem::filesystem_error& err) {
            LOG_ERROR(Lib_SaveData, "Failed to backup {}: {}", fmt::UTF(req.save_path.u8string()),
                      err.what());
        }
        LOG_DEBUG(Lib_SaveData, "Backing up the following directory: {} finished",
                  fmt::UTF(req.save_path.u8string()));
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            g_backup_queue.front().done = true;
        }
        {
            std::scoped_lock lk{g_backup_queue_mutex};
            g_backup_queue.pop_front();
            if (req.origin != OrbisSaveDataEventType::__DO_NOT_SAVE) {
                g_result_queue.push_back(std::move(req));
                if (g_result_queue.size() > 20) {
                    g_result_queue.pop_front();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Don't backup too often
    }
    g_backup_status = WorkerStatus::NotStarted;
}

void StartThread() {
    if (g_backup_status != WorkerStatus::NotStarted) {
        return;
    }
    LOG_DEBUG(Lib_SaveData, "Starting backup thread");
    g_backup_status = WorkerStatus::Waiting;
    g_backup_thread = std::jthread{BackupThreadBody};
    static std::once_flag flag;
    std::call_once(flag, [] {
        std::at_quick_exit([] {
            StopThread();
            while (GetWorkerStatus() != WorkerStatus::NotStarted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    });
}

void StopThread() {
    if (g_backup_status == WorkerStatus::NotStarted || g_backup_status == WorkerStatus::Stopping) {
        return;
    }
    LOG_DEBUG(Lib_SaveData, "Stopping backup thread");
    g_backup_status = WorkerStatus::Stopping;
    {
        std::scoped_lock lk{g_backup_queue_mutex};
        g_backup_queue.emplace_back(BackupRequest{});
    }
    g_backup_thread_semaphore.release();
}

bool NewRequest(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                std::string_view dir_name, OrbisSaveDataEventType origin) {
    auto save_path = SaveInstance::MakeDirSavePath(user_id, title_id, dir_name);

    if (g_backup_status != WorkerStatus::Waiting && g_backup_status != WorkerStatus::Running) {
        LOG_ERROR(Lib_SaveData, "Called backup while status is {}. Backup request to {} ignored",
                  magic_enum::enum_name(g_backup_status.load()), fmt::UTF(save_path.u8string()));
        return false;
    }
    {
        std::scoped_lock lk{g_backup_queue_mutex};
        for (const auto& it : g_backup_queue) {
            if (it.dir_name == dir_name) {
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

bool Restore(const std::filesystem::path& save_path) {
    LOG_INFO(Lib_SaveData, "Restoring backup for {}", fmt::UTF(save_path.u8string()));
    std::unique_lock lk{g_backup_running_mutex};
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
    const auto& it =
        std::ranges::find(g_backup_queue, save_path, [](const auto& v) { return v.save_path; });
    return it != g_backup_queue.end() && !it->done;
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