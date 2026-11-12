// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::SaveData {

enum class OrbisSaveDataError : u32 {
    OK = 0,
    PARAMETER = 0x809F0000,
    NOT_INITIALIZED = 0x809F0001,
    OUT_OF_MEMORY = 0x809F0002,
    BUSY = 0x809F0003,
    NOT_MOUNTED = 0x809F0004,
    NO_PERMISSION = 0x809F0005,
    FINGERPRINT_MISMATCH = 0x809F0006,
    EXISTS = 0x809F0007,
    NOT_FOUND = 0x809F0008,
    NO_SPACE_FS = 0x809F000A,
    INTERNAL = 0x809F000B,
    MOUNT_FULL = 0x809F000C,
    BAD_MOUNTED = 0x809F000D,
    FILE_NOT_FOUND = 0x809F000E,
    BROKEN = 0x809F000F,
    INVALID_LOGIN_USER = 0x809F0011,
    MEMORY_NOT_READY = 0x809F0012,
    BACKUP_BUSY = 0x809F0013,
    NO_SPACE = 0x809F0015,
    BUSY_BACKUP = 0x809F0016,
    NOT_LOGGED_IN = 0x809F0017,
    EVENT_BUSY = 0x809F0018,
    TOO_MANY_FILES = 0x809F0019,

    // Aliases matching Orbis SDK names
    INVALID_ARGUMENT = PARAMETER,
    BUF_NO_SPACE = NO_SPACE,
    ABORTED = 0x809FA000,
    IS_MOUNTED = 1,
    IS_NOT_MOUNTED = 0,

    // Extended OrbisSaveData error codes (0x809FA0xx) observed in libSceSaveData.sprx
    NO_SPACE_APP = 0x809FA000,
    PSN_NOT_LINKED = 0x809FA001,
    NETWORK_DISABLED = 0x809FA002,
    SERVER_ERROR = 0x809FA003,
    DB_CORRUPT = 0x809FA004,
    CLOUD_QUOTA_FULL = 0x809FA005,
    CLOUD_DATA_NOT_FOUND = 0x809FA006,
    CLOUD_CONFLICT = 0x809FA007,
    DOWNLOAD_FAILED = 0x809FA008,
    NOT_SUPPORTED_FUNCTION = 0x809FA009,
    INVALID_APP = 0x809FA00B,
    CLOUD_LOCKED = 0x809FA00C,
    CLOUD_SYNC_CONFLICT = 0x809FA00D,
    SERVER_MAINTENANCE = 0x809FA00E,
    NO_PS_PLUS = 0x809FA00F,
    FORMAT_VERSION_UNSUPPORTED = 0x809FA010,
    USER_NOT_SIGNED_IN = 0x809FA011,
    UPDATED_COUNT_INVALID = 0x809FA012,
    TRANSFER_NOT_READY = 0x809FA013,
    BACKUP_IN_PROGRESS = 0x809FA014,
    USB_NOT_CONNECTED = 0x809FA015,
    USB_NO_SPACE = 0x809FA016,
    DIR_NAME_SEARCH_FAILED = 0x809FA018,
    BACKUP_FAILED = 0x809FA019,
    USB_ERROR = 0x809FA01A,

    USER_SERVICE_NOT_INITIALIZED = 0x80960002,
};

#ifdef DELETE
#undef DELETE
#endif

// Command IDs used in libSceSaveData.sprx error translation tables
enum class OrbisSaveDataCommand : u32 {
    None = 0x00,
    Mount = 0x01,
    Umount = 0x02,
    Delete = 0x04,
    SaveIcon = 0x07,
    LoadIcon = 0x08,
    SetParam = 0x09,
    GetParam = 0x0A,
    MountInternal = 0x0F,
    MountSys = 0x0C,
    UmountSys = 0x0D,
    Promote5 = 0x10,
    Copy5 = 0x11,
    Delete5 = 0x13,
    IsMounted = 0x14,
    GetAllSize = 0x15,
    RebuildDatabase = 0x1C,
    DeleteUser = 0x20,
    GetFormat = 0x27,
    GetSaveDataCount = 0x2D,
    Mount5 = 0x3C,
    DirNameSearch = 0x06,

    // Aliases / extended command IDs used across libSceSaveData entry points
    NONE = None,
    MOUNT = Mount,
    MOUNT2 = Mount,
    MOUNT5 = Mount5,
    MOUNT_INTERNAL = MountInternal,
    MOUNT_SYS = MountSys,
    TRANSFERRING_MOUNT = Mount,
    UMOUNT = Umount,
    UMOUNT_SYS = UmountSys,
    UMOUNT_WITH_BACKUP = Umount,
    BACKUP = 0x03,
    RESTORE_BACKUP_DATA = 0x05,
    RESTORE_BACKUP_DATA_FOR_CDLG = 0x29,
    CHECK_BACKUP_DATA = 0x22,
    CHECK_BACKUP_DATA_INTERNAL = 0x2A,
    CHECK_BACKUP_DATA_FOR_CDLG = 0x2B,
    CHANGE_INTERNAL = 0x2C,
    GET_MOUNT_INFO = 0x0E,
    DELETE5 = Delete5,
    DELETE_USER = DeleteUser,
    DELETE_ALL_USER = 0x21,
    COPY5 = Copy5,
    PROMOTE5 = Promote5,
    DIR_NAME_SEARCH = DirNameSearch,
    DIR_NAME_SEARCH_INTERNAL = DirNameSearch,
    SET_PARAM = SetParam,
    GET_PARAM = GetParam,
    SAVE_ICON = SaveIcon,
    LOAD_ICON = LoadIcon,
    REBUILD_DATABASE = RebuildDatabase,
    CHANGE_DATABASE = 0x1D,
    GET_DATA_BASE_FILE_PATH = 0x1E,
    GET_SAVE_DATA_ROOT_PATH = 0x1F,
    GET_SAVE_DATA_ROOT_USB_PATH = 0x23,
    IS_MOUNTED = IsMounted,
    GET_UPDATED_DATA_COUNT = 0x24,
    BIND_PSN_ACCOUNT = 0x25,
    GET_ALL_SIZE = GetAllSize,
    SETUP_SAVE_DATA_MEMORY = 0x2E,
    SETUP_SAVE_DATA_MEMORY2 = 0x2F,
    GET_SAVE_DATA_MEMORY = 0x30,
    GET_SAVE_DATA_MEMORY2 = 0x31,
    SET_SAVE_DATA_MEMORY = 0x32,
    SET_SAVE_DATA_MEMORY2 = 0x33,
    SYNC_SAVE_DATA_MEMORY = 0x34,
    RESTORE_LOAD_SAVE_DATA_MEMORY = 0x35,
    SET_LIBRARY_USER = 0x36,
    GET_SAVE_DATA_COUNT = GetSaveDataCount,
    CHECK_SAVE_DATA_BROKEN = 0x38,
    CHECK_SAVE_DATA_VERSION = 0x39,
};

constexpr s32 ORBIS_SAVE_DATA_ERROR_OK = static_cast<s32>(OrbisSaveDataError::OK);
constexpr s32 ORBIS_SAVE_DATA_ERROR_PARAMETER = static_cast<s32>(OrbisSaveDataError::PARAMETER);
constexpr s32 ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT =
    static_cast<s32>(OrbisSaveDataError::PARAMETER);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED =
    static_cast<s32>(OrbisSaveDataError::NOT_INITIALIZED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_OUT_OF_MEMORY =
    static_cast<s32>(OrbisSaveDataError::OUT_OF_MEMORY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BUSY = static_cast<s32>(OrbisSaveDataError::BUSY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_MOUNTED = static_cast<s32>(OrbisSaveDataError::NOT_MOUNTED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NO_PERMISSION =
    static_cast<s32>(OrbisSaveDataError::NO_PERMISSION);
constexpr s32 ORBIS_SAVE_DATA_ERROR_FINGERPRINT_MISMATCH =
    static_cast<s32>(OrbisSaveDataError::FINGERPRINT_MISMATCH);
constexpr s32 ORBIS_SAVE_DATA_ERROR_EXISTS = static_cast<s32>(OrbisSaveDataError::EXISTS);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_FOUND = static_cast<s32>(OrbisSaveDataError::NOT_FOUND);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NO_SPACE_FS = static_cast<s32>(OrbisSaveDataError::NO_SPACE_FS);
constexpr s32 ORBIS_SAVE_DATA_ERROR_INTERNAL = static_cast<s32>(OrbisSaveDataError::INTERNAL);
constexpr s32 ORBIS_SAVE_DATA_ERROR_MOUNT_FULL = static_cast<s32>(OrbisSaveDataError::MOUNT_FULL);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BAD_MOUNTED = static_cast<s32>(OrbisSaveDataError::BAD_MOUNTED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_FILE_NOT_FOUND =
    static_cast<s32>(OrbisSaveDataError::FILE_NOT_FOUND);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BROKEN = static_cast<s32>(OrbisSaveDataError::BROKEN);
constexpr s32 ORBIS_SAVE_DATA_ERROR_INVALID_LOGIN_USER =
    static_cast<s32>(OrbisSaveDataError::INVALID_LOGIN_USER);
constexpr s32 ORBIS_SAVE_DATA_ERROR_MEMORY_NOT_READY =
    static_cast<s32>(OrbisSaveDataError::MEMORY_NOT_READY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BACKUP_BUSY = static_cast<s32>(OrbisSaveDataError::BACKUP_BUSY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NO_SPACE = static_cast<s32>(OrbisSaveDataError::NO_SPACE);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BUF_NO_SPACE = static_cast<s32>(OrbisSaveDataError::NO_SPACE);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BUSY_BACKUP = static_cast<s32>(OrbisSaveDataError::BUSY_BACKUP);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_LOGGED_IN =
    static_cast<s32>(OrbisSaveDataError::NOT_LOGGED_IN);
constexpr s32 ORBIS_SAVE_DATA_ERROR_EVENT_BUSY = static_cast<s32>(OrbisSaveDataError::EVENT_BUSY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_TOO_MANY_FILES =
    static_cast<s32>(OrbisSaveDataError::TOO_MANY_FILES);
constexpr s32 ORBIS_SAVE_DATA_ERROR_ABORTED = static_cast<s32>(OrbisSaveDataError::ABORTED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED =
    static_cast<s32>(OrbisSaveDataError::NOT_SUPPORTED_FUNCTION);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED_FUNCTION =
    static_cast<s32>(OrbisSaveDataError::NOT_SUPPORTED_FUNCTION);

constexpr s32 ORBIS_SAVE_DATA_ERROR_NO_SPACE_APP =
    static_cast<s32>(OrbisSaveDataError::NO_SPACE_APP);
constexpr s32 ORBIS_SAVE_DATA_ERROR_PSN_NOT_LINKED =
    static_cast<s32>(OrbisSaveDataError::PSN_NOT_LINKED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NETWORK_DISABLED =
    static_cast<s32>(OrbisSaveDataError::NETWORK_DISABLED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_SERVER_ERROR =
    static_cast<s32>(OrbisSaveDataError::SERVER_ERROR);
constexpr s32 ORBIS_SAVE_DATA_ERROR_DB_CORRUPT = static_cast<s32>(OrbisSaveDataError::DB_CORRUPT);
constexpr s32 ORBIS_SAVE_DATA_ERROR_CLOUD_QUOTA_FULL =
    static_cast<s32>(OrbisSaveDataError::CLOUD_QUOTA_FULL);
constexpr s32 ORBIS_SAVE_DATA_ERROR_CLOUD_DATA_NOT_FOUND =
    static_cast<s32>(OrbisSaveDataError::CLOUD_DATA_NOT_FOUND);
constexpr s32 ORBIS_SAVE_DATA_ERROR_CLOUD_CONFLICT =
    static_cast<s32>(OrbisSaveDataError::CLOUD_CONFLICT);
constexpr s32 ORBIS_SAVE_DATA_ERROR_DOWNLOAD_FAILED =
    static_cast<s32>(OrbisSaveDataError::DOWNLOAD_FAILED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_INVALID_APP = static_cast<s32>(OrbisSaveDataError::INVALID_APP);
constexpr s32 ORBIS_SAVE_DATA_ERROR_CLOUD_LOCKED =
    static_cast<s32>(OrbisSaveDataError::CLOUD_LOCKED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_CLOUD_SYNC_CONFLICT =
    static_cast<s32>(OrbisSaveDataError::CLOUD_SYNC_CONFLICT);
constexpr s32 ORBIS_SAVE_DATA_ERROR_SERVER_MAINTENANCE =
    static_cast<s32>(OrbisSaveDataError::SERVER_MAINTENANCE);
constexpr s32 ORBIS_SAVE_DATA_ERROR_NO_PS_PLUS = static_cast<s32>(OrbisSaveDataError::NO_PS_PLUS);
constexpr s32 ORBIS_SAVE_DATA_ERROR_FORMAT_VERSION_UNSUPPORTED =
    static_cast<s32>(OrbisSaveDataError::FORMAT_VERSION_UNSUPPORTED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_USER_NOT_SIGNED_IN =
    static_cast<s32>(OrbisSaveDataError::USER_NOT_SIGNED_IN);
constexpr s32 ORBIS_SAVE_DATA_ERROR_UPDATED_COUNT_INVALID =
    static_cast<s32>(OrbisSaveDataError::UPDATED_COUNT_INVALID);
constexpr s32 ORBIS_SAVE_DATA_ERROR_TRANSFER_NOT_READY =
    static_cast<s32>(OrbisSaveDataError::TRANSFER_NOT_READY);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BACKUP_IN_PROGRESS =
    static_cast<s32>(OrbisSaveDataError::BACKUP_IN_PROGRESS);
constexpr s32 ORBIS_SAVE_DATA_ERROR_USB_NOT_CONNECTED =
    static_cast<s32>(OrbisSaveDataError::USB_NOT_CONNECTED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_USB_NO_SPACE =
    static_cast<s32>(OrbisSaveDataError::USB_NO_SPACE);
constexpr s32 ORBIS_SAVE_DATA_ERROR_DIR_NAME_SEARCH_FAILED =
    static_cast<s32>(OrbisSaveDataError::DIR_NAME_SEARCH_FAILED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_BACKUP_FAILED =
    static_cast<s32>(OrbisSaveDataError::BACKUP_FAILED);
constexpr s32 ORBIS_SAVE_DATA_ERROR_USB_ERROR = static_cast<s32>(OrbisSaveDataError::USB_ERROR);
constexpr s32 ORBIS_SAVE_DATA_ERROR_USER_SERVICE_NOT_INITIALIZED =
    static_cast<s32>(OrbisSaveDataError::USER_SERVICE_NOT_INITIALIZED);

// Translates an internal libSceSaveData/SceShellCore 0x809F8xxx error (or kernel 0x8002xxxx /
// 0x8096xxxx error) into the SDK-version-specific public OrbisSaveData return code.
[[nodiscard]] inline OrbisSaveDataError TranslateInternalError(OrbisSaveDataCommand cmd,
                                                               u32 sdk_version, u32 code) {
    if (static_cast<s32>(code) >= 0 || (code & 0xFFFF8000u) == 0x809F0000u) {
        return static_cast<OrbisSaveDataError>(code);
    }

    // Kernel / UserService passthrough and global internal->public mappings (FUN_00013d80)
    switch (code) {
    case 0x80960002u:
        return sdk_version >= 0x02000000u ? OrbisSaveDataError::USER_SERVICE_NOT_INITIALIZED
                                          : OrbisSaveDataError::INTERNAL;
    case 0x809F8002u:
        return OrbisSaveDataError::NOT_INITIALIZED;
    case 0x809F8003u:
        return OrbisSaveDataError::OUT_OF_MEMORY;
    case 0x809F8004u:
    case 0x809F8007u:
        return OrbisSaveDataError::PARAMETER;
    case 0x809F800Du:
        return OrbisSaveDataError::NO_PERMISSION;
    case 0x809F805Eu:
    case 0x809F8709u:
    case 0x809F870Bu:
    case 0x809F870Du:
        return OrbisSaveDataError::BROKEN;
    case 0x809F8060u:
        return OrbisSaveDataError::NO_SPACE;
    case 0x80020010u: // ORBIS_KERNEL_ERROR_EBUSY
        return OrbisSaveDataError::BUSY;
    case 0x80020060u: // ORBIS_KERNEL_ERROR_ESTALE / corrupt FS
        return OrbisSaveDataError::BROKEN;
    case 0x80020002u: // ORBIS_KERNEL_ERROR_ENOENT
        return (cmd == OrbisSaveDataCommand::Umount || cmd == OrbisSaveDataCommand::LoadIcon)
                   ? OrbisSaveDataError::NOT_FOUND
                   : OrbisSaveDataError::BUSY;
    default:
        break;
    }

    // Command-specific tables observed at 0x35500..0x35ba0 in libSceSaveData.sprx
    switch (cmd) {
    case OrbisSaveDataCommand::Mount:
    case OrbisSaveDataCommand::MountInternal:
    case OrbisSaveDataCommand::Mount5:
        switch (code) {
        case 0x809F8032u:
            return OrbisSaveDataError::BUSY;
        case 0x809F8603u:
            return OrbisSaveDataError::BACKUP_BUSY;
        case 0x809F8037u:
        case 0x809F803Au:
            return OrbisSaveDataError::NO_SPACE_FS;
        case 0x809F8024u:
            return OrbisSaveDataError::EXISTS;
        case 0x809F8022u:
            return OrbisSaveDataError::NOT_FOUND;
        case 0x809F8030u:
            return OrbisSaveDataError::MOUNT_FULL;
        case 0x809F8009u:
            return OrbisSaveDataError::INVALID_LOGIN_USER;
        case 0x809F805Cu:
            return OrbisSaveDataError::FINGERPRINT_MISMATCH;
        case 0x809F8061u:
            return OrbisSaveDataError::TOO_MANY_FILES;
        case 0x809F8050u:
        case 0x809F8051u:
        case 0x809F8052u:
        case 0x809F8053u:
        case 0x809F8054u:
        case 0x809F8057u:
        case 0x809F8058u:
        case 0x809F805Au:
        case 0x809F805Du:
        case 0x809F805Fu:
            return OrbisSaveDataError::BROKEN;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::Umount:
    case OrbisSaveDataCommand::UmountSys:
        switch (code) {
        case 0x809F8033u:
            return OrbisSaveDataError::NOT_MOUNTED;
        case 0x809F8057u:
            return OrbisSaveDataError::BROKEN;
        case 0x80020010u:
            return OrbisSaveDataError::BUSY;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::Delete:
    case OrbisSaveDataCommand::Delete5:
        switch (code) {
        case 0x809F8032u:
            return OrbisSaveDataError::BUSY;
        case 0x809F8603u:
            return OrbisSaveDataError::BACKUP_BUSY;
        case 0x809F8009u:
            return OrbisSaveDataError::INVALID_LOGIN_USER;
        case 0x809F8022u:
            return OrbisSaveDataError::NOT_FOUND;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::SaveIcon:
        switch (code) {
        case 0x809F8033u:
            return OrbisSaveDataError::NOT_MOUNTED;
        case 0x809F8034u:
            return OrbisSaveDataError::BAD_MOUNTED;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::LoadIcon:
        switch (code) {
        case 0x809F8033u:
            return OrbisSaveDataError::NOT_MOUNTED;
        case 0x809F8023u:
            return OrbisSaveDataError::FILE_NOT_FOUND;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::SetParam:
        switch (code) {
        case 0x809F8033u:
            return OrbisSaveDataError::NOT_MOUNTED;
        case 0x809F8034u:
            return OrbisSaveDataError::BAD_MOUNTED;
        case 0x809F8050u:
            return OrbisSaveDataError::BROKEN;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::GetParam:
        return code == 0x809F8033u ? OrbisSaveDataError::NOT_MOUNTED : OrbisSaveDataError::INTERNAL;

    case OrbisSaveDataCommand::GetFormat:
        if (sdk_version >= 0x03500000u && code == 0x809F8009u) {
            return OrbisSaveDataError::INVALID_LOGIN_USER;
        }
        switch (code) {
        case 0x809F8600u:
            return OrbisSaveDataError::MEMORY_NOT_READY;
        case 0x809F8032u:
            return OrbisSaveDataError::BUSY;
        case 0x809F8602u:
            return OrbisSaveDataError::BUSY_BACKUP;
        case 0x809F8037u:
            return OrbisSaveDataError::NO_SPACE_FS;
        default:
            return OrbisSaveDataError::INTERNAL;
        }

    case OrbisSaveDataCommand::DirNameSearch:
        if (code == 0x809F8009u) {
            return OrbisSaveDataError::INVALID_LOGIN_USER;
        }
        return OrbisSaveDataError::DIR_NAME_SEARCH_FAILED;

    default:
        break;
    }

    return OrbisSaveDataError::INTERNAL;
}

} // namespace Libraries::SaveData
