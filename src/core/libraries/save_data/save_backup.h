// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "core/file_format/psf.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::SaveData::Backup {

enum class WorkerStatus {
    NotStarted,
    Waiting,
    Running,
    Stopping,
};

enum class OrbisSaveDataEventType : u32 {
    UMOUNT_BACKUP = 1,
    BACKUP = 2,
    SAVE_DATA_MEMORY_SYNC = 3,

    __DO_NOT_SAVE = 1000000, // Internal value for silent backup jobs
};

struct BackupRequest {
    Libraries::UserService::OrbisUserServiceUserId user_id{};
    std::string title_id;
    std::string dir_name;
    OrbisSaveDataEventType origin{};
    std::filesystem::path save_path;
    bool done = false;
    s32 error_code = 0;
};

void StartThread();
void StopThread();

bool NewRequest(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                std::string_view dir_name, OrbisSaveDataEventType origin);

bool Restore(const std::filesystem::path& save_path);
bool Restore(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
             std::string_view dir_name);

std::unique_lock<std::mutex> LockForWrite();

bool DeleteBackup(Libraries::UserService::OrbisUserServiceUserId user_id, std::string_view title_id,
                  std::string_view dir_name);

[[nodiscard]] bool HasBackup(Libraries::UserService::OrbisUserServiceUserId user_id,
                             std::string_view title_id, std::string_view dir_name);

[[nodiscard]] bool CheckBackup(Libraries::UserService::OrbisUserServiceUserId user_id,
                               std::string_view title_id, std::string_view dir_name, PSF& out_sfo,
                               std::filesystem::path& out_icon_path);

[[nodiscard]] WorkerStatus GetWorkerStatus();
[[nodiscard]] bool IsBackupExecutingFor(const std::filesystem::path& save_path);
[[nodiscard]] bool HasPendingEvents();
[[nodiscard]] std::filesystem::path MakeBackupPath(const std::filesystem::path& save_path);
[[nodiscard]] std::optional<BackupRequest> PopLastEvent();
void PushBackupEvent(BackupRequest&& req);

[[nodiscard]] float GetProgress();
void ClearProgress();

} // namespace Libraries::SaveData::Backup
