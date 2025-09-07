// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Libraries::SaveData {
enum class Error : u32 {
    OK = 0,
    USER_SERVICE_NOT_INITIALIZED = 0x80960002,
    PARAMETER = 0x809F0000,
    NOT_INITIALIZED = 0x809F0001,
    OUT_OF_MEMORY = 0x809F0002,
    BUSY = 0x809F0003,
    NOT_MOUNTED = 0x809F0004,
    EXISTS = 0x809F0007,
    NOT_FOUND = 0x809F0008,
    NO_SPACE_FS = 0x809F000A,
    INTERNAL = 0x809F000B,
    MOUNT_FULL = 0x809F000C,
    BAD_MOUNTED = 0x809F000D,
    BROKEN = 0x809F000F,
    INVALID_LOGIN_USER = 0x809F0011,
    MEMORY_NOT_READY = 0x809F0012,
    BACKUP_BUSY = 0x809F0013,
    BUSY_FOR_SAVING = 0x809F0016,
};
} // namespace Libraries::SaveData
