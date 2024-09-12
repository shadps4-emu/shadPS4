// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>

#include "common/types.h"

namespace Libraries::SaveData {

using OrbisUserServiceUserId = s32;

namespace Backup {

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
};

struct BackupRequest {
    OrbisUserServiceUserId user_id{};
    std::string title_id{};
    std::string dir_name{};
    OrbisSaveDataEventType origin{};

    std::filesystem::path save_path{};
};

// No problem calling this function if the backup thread is already running
void StartThread();

void StopThread();

bool NewRequest(OrbisUserServiceUserId user_id, std::string_view title_id,
                std::string_view dir_name, OrbisSaveDataEventType origin);

bool Restore(const std::filesystem::path& save_path);

WorkerStatus GetWorkerStatus();

bool IsBackupExecutingFor(const std::filesystem::path& save_path);

std::filesystem::path MakeBackupPath(const std::filesystem::path& save_path);

std::optional<BackupRequest> PopLastEvent();

void PushBackupEvent(BackupRequest&& req);

float GetProgress();

void ClearProgress();

} // namespace Backup

} // namespace Libraries::SaveData
