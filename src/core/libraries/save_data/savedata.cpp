// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>

#include <core/libraries/system/msgdialog_ui.h>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/cstring.h"
#include "common/elf_info.h"
#include "common/enum.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/system/msgdialog.h"
#include "save_backup.h"
#include "save_instance.h"
#include "save_memory.h"

namespace fs = std::filesystem;
namespace chrono = std::chrono;

using Common::CString;
using Common::ElfInfo;

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

enum class OrbisSaveDataSaveDataMemoryOption : u32 {
    NONE = 0,
    SET_PARAM = 1 << 0,
    DOUBLE_BUFFER = 1 << 1,
    UNLOCK_LIMITATIONS = 1 << 2,
};

using OrbisUserServiceUserId = s32;
using OrbisSaveDataBlocks = u64;

constexpr u32 OrbisSaveDataBlockSize = 32768; // 32 KiB
constexpr u32 OrbisSaveDataBlocksMin2 = 96;   // 3MiB
constexpr u32 OrbisSaveDataBlocksMax = 32768; // 1 GiB

// Maximum size for a mount point "/savedataN", where N is a number
constexpr size_t OrbisSaveDataMountPointDataMaxsize = 16;

constexpr size_t OrbisSaveDataFingerprintDataSize = 65; // Maximum fingerprint size

enum class OrbisSaveDataMountMode : u32 {
    RDONLY = 1 << 0,
    RDWR = 1 << 1,
    CREATE = 1 << 2,
    DESTRUCT_OFF = 1 << 3,
    COPY_ICON = 1 << 4,
    CREATE2 = 1 << 5,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisSaveDataMountMode);

enum class OrbisSaveDataMountStatus : u32 {
    NOTHING = 0,
    CREATED = 1,
};

enum class OrbisSaveDataParamType : u32 {
    ALL = 0,
    TITLE = 1,
    SUB_TITLE = 2,
    DETAIL = 3,
    USER_PARAM = 4,
    MTIME = 5,
};

enum class OrbisSaveDataSortKey : u32 {
    DIRNAME = 0,
    USER_PARAM = 1,
    BLOCKS = 2,
    MTIME = 3,
    FREE_BLOCKS = 5,
};

enum class OrbisSaveDataSortOrder : u32 {
    ASCENT = 0,
    DESCENT = 1,
};

struct OrbisSaveDataFingerprint {
    CString<OrbisSaveDataFingerprintDataSize> data;
    std::array<u8, 15> _pad;
};

struct OrbisSaveDataBackup {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* param;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataCheckBackupData {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataDelete {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u32 _unused;
    std::array<u8, 32> _reserved;
    s32 : 32;
};

struct OrbisSaveDataIcon {
    void* buf;
    size_t bufSize;
    size_t dataSize;
    std::array<u8, 32> _reserved;

    Error LoadIcon(const std::filesystem::path& icon_path) {
        try {
            const Common::FS::IOFile file(icon_path, Common::FS::FileAccessMode::Read);
            dataSize = file.GetSize();
            file.Seek(0);
            file.ReadRaw<u8>(buf, std::min(bufSize, dataSize));
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR(Lib_SaveData, "Failed to load icon: {}", e.what());
            return Error::INTERNAL;
        }
        return Error::OK;
    }
};

struct OrbisSaveDataMemoryData {
    void* buf;
    size_t bufSize;
    s64 offset;
    u8 _reserved[40];
};

struct OrbisSaveDataMemoryGet2 {
    OrbisUserServiceUserId userId;
    std::array<u8, 4> _pad;
    OrbisSaveDataMemoryData* data;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataMemorySet2 {
    OrbisUserServiceUserId userId;
    std::array<u8, 4> _pad;
    const OrbisSaveDataMemoryData* data;
    const OrbisSaveDataParam* param;
    const OrbisSaveDataIcon* icon;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataMemorySetup2 {
    OrbisSaveDataSaveDataMemoryOption option;
    OrbisUserServiceUserId userId;
    size_t memorySize;
    size_t iconMemorySize;
    // +4.5
    const OrbisSaveDataParam* initParam;
    // +4.5
    const OrbisSaveDataIcon* initIcon;
    std::array<u8, 24> _reserved;
};

struct OrbisSaveDataMemorySetupResult {
    size_t existedMemorySize;
    std::array<u8, 16> _reserved;
};

struct OrbisSaveDataMemorySync {
    OrbisUserServiceUserId userId;
    std::array<u8, 36> _reserved;
};

struct OrbisSaveDataMount2 {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataBlocks blocks;
    OrbisSaveDataMountMode mountMode;
    std::array<u8, 32> _reserved;
    s32 : 32;
};

struct OrbisSaveDataMount {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    OrbisSaveDataBlocks blocks;
    OrbisSaveDataMountMode mountMode;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataMountInfo {
    OrbisSaveDataBlocks blocks;
    OrbisSaveDataBlocks freeBlocks;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataMountPoint {
    CString<OrbisSaveDataMountPointDataMaxsize> data;
};

struct OrbisSaveDataMountResult {
    OrbisSaveDataMountPoint mount_point;
    OrbisSaveDataBlocks required_blocks;
    u32 _unused;
    // +4.5
    OrbisSaveDataMountStatus mount_status;
    std::array<u8, 28> _reserved;
    s32 : 32;
};

struct OrbisSaveDataRestoreBackupData {
    OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u32 _unused;
    std::array<u8, 32> _reserved;
    s32 : 32;
};

struct OrbisSaveDataTransferringMount {
    OrbisUserServiceUserId userId;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataDirNameSearchCond {
    OrbisUserServiceUserId userId;
    int : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataSortKey key;
    OrbisSaveDataSortOrder order;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataSearchInfo {
    u64 blocks;
    u64 freeBlocks;
    std::array<u8, 32> _reserved;
};

struct OrbisSaveDataDirNameSearchResult {
    u32 hitNum;
    int : 32;
    OrbisSaveDataDirName* dirNames;
    u32 dirNamesNum;
    // +1.7
    u32 setNum;
    // +1.7
    OrbisSaveDataParam* params;
    // +2.5
    OrbisSaveDataSearchInfo* infos;
    std::array<u8, 12> _reserved;
    int : 32;
};

struct OrbisSaveDataEventParam { // dummy structure
    OrbisSaveDataEventParam() = delete;
};

using OrbisSaveDataEventType = Backup::OrbisSaveDataEventType;

struct OrbisSaveDataEvent {
    OrbisSaveDataEventType type;
    s32 errorCode;
    OrbisUserServiceUserId userId;
    std::array<u8, 4> _pad;
    OrbisSaveDataTitleId titleId;
    OrbisSaveDataDirName dirName;
    std::array<u8, 40> _reserved;
};

static bool g_initialized = false;
static std::string g_game_serial;
static u32 g_fw_ver;
static std::array<std::optional<SaveInstance>, 16> g_mount_slots;

static void initialize() {
    g_initialized = true;
    g_game_serial = ElfInfo::Instance().GameSerial();
    g_fw_ver = ElfInfo::Instance().FirmwareVer();
}

// game_00other | game*other

static bool match(std::string_view str, std::string_view pattern) {
    auto str_it = str.begin();
    auto pat_it = pattern.begin();
    while (str_it != str.end() && pat_it != pattern.end()) {
        if (*pat_it == '%') { // 0 or more wildcard
            for (auto str_wild_it = str_it; str_wild_it <= str.end(); ++str_wild_it) {
                if (match({str_wild_it, str.end()}, {pat_it + 1, pattern.end()})) {
                    return true;
                }
            }
            return false;
        }
        if (*pat_it == '_') { // 1 character wildcard
            ++str_it;
            ++pat_it;
        } else if (*pat_it != *str_it) {
            return false;
        }
        ++str_it;
        ++pat_it;
    }
    return str_it == str.end() && pat_it == pattern.end();
}

static Error setNotInitializedError() {
    if (g_fw_ver < ElfInfo::FW_20) {
        return Error::INTERNAL;
    }
    if (g_fw_ver < ElfInfo::FW_25) {
        return Error::USER_SERVICE_NOT_INITIALIZED;
    }
    return Error::NOT_INITIALIZED;
}

static Error saveDataMount(const OrbisSaveDataMount2* mount_info,
                           OrbisSaveDataMountResult* mount_result,
                           std::string_view title_id = g_game_serial) {

    if (mount_info->userId < 0) {
        return Error::INVALID_LOGIN_USER;
    }
    if (mount_info->dirName == nullptr) {
        LOG_INFO(Lib_SaveData, "called without dirName");
        return Error::PARAMETER;
    }

    // check backup status
    {
        const auto save_path =
            SaveInstance::MakeDirSavePath(mount_info->userId, title_id, mount_info->dirName->data);
        if (Backup::IsBackupExecutingFor(save_path) && g_fw_ver) {
            return Error::BACKUP_BUSY;
        }
    }

    auto mount_mode = mount_info->mountMode;
    const bool is_ro = True(mount_mode & OrbisSaveDataMountMode::RDONLY);

    const bool create = True(mount_mode & OrbisSaveDataMountMode::CREATE);
    const bool create_if_not_exist =
        True(mount_mode & OrbisSaveDataMountMode::CREATE2) && g_fw_ver >= ElfInfo::FW_45;
    ASSERT(!create || !create_if_not_exist); // Can't have both

    const bool copy_icon = True(mount_mode & OrbisSaveDataMountMode::COPY_ICON);

    const bool ignore_corrupt =
        True(mount_mode & OrbisSaveDataMountMode::DESTRUCT_OFF) || g_fw_ver < ElfInfo::FW_16;

    const std::string_view dir_name{mount_info->dirName->data};

    // find available mount point
    int slot_num = -1;
    for (size_t i = 0; i < g_mount_slots.size(); i++) {
        const auto& instance = g_mount_slots[i];
        if (instance.has_value()) {
            const auto& slot_name = instance->GetDirName();
            if (slot_name == dir_name) {
                return Error::BUSY;
            }
        } else {
            slot_num = static_cast<int>(i);
            break;
        }
    }
    if (slot_num == -1) {
        return Error::MOUNT_FULL;
    }

    SaveInstance save_instance{slot_num, mount_info->userId, std::string{title_id}, dir_name,
                               (int)mount_info->blocks};

    if (save_instance.Mounted()) {
        UNREACHABLE_MSG("Save instance should not be mounted");
    }

    if (!create && !create_if_not_exist && !save_instance.Exists()) {
        return Error::NOT_FOUND;
    }
    if (create && save_instance.Exists()) {
        return Error::EXISTS;
    }

    bool to_be_created = !save_instance.Exists();

    if (to_be_created) { // Check size

        if (mount_info->blocks < OrbisSaveDataBlocksMin2 ||
            mount_info->blocks > OrbisSaveDataBlocksMax) {
            LOG_INFO(Lib_SaveData, "called with invalid block size");
        }

        const auto root_save = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir);
        fs::create_directories(root_save);
        const auto available = fs::space(root_save).available;

        auto requested_size = save_instance.GetMaxBlocks() * OrbisSaveDataBlockSize;
        if (requested_size > available) {
            mount_result->required_blocks = (requested_size - available) / OrbisSaveDataBlockSize;
            return Error::NO_SPACE_FS;
        }
    }

    try {
        save_instance.SetupAndMount(is_ro, copy_icon, ignore_corrupt);
    } catch (const fs::filesystem_error& e) {
        if (e.code() == std::errc::illegal_byte_sequence) {
            LOG_ERROR(Lib_SaveData, "Corrupted save data");
            return Error::BROKEN;
        }
        if (e.code() == std::errc::no_space_on_device) {
            return Error::NO_SPACE_FS;
        }
        LOG_ERROR(Lib_SaveData, "Failed to mount save data: {}", e.what());
        return Error::INTERNAL;
    }

    mount_result->mount_point.data.FromString(save_instance.GetMountPoint());

    if (g_fw_ver >= ElfInfo::FW_45) {
        mount_result->mount_status = create_if_not_exist && to_be_created
                                         ? OrbisSaveDataMountStatus::CREATED
                                         : OrbisSaveDataMountStatus::NOTHING;
    }

    g_mount_slots[slot_num].emplace(std::move(save_instance));

    return Error::OK;
}

static Error Umount(const OrbisSaveDataMountPoint* mountPoint, bool call_backup = false) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mountPoint == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "Umount mountPoint:{}", mountPoint->data.to_view());
    const std::string_view mount_point_str{mountPoint->data};
    for (auto& instance : g_mount_slots) {
        if (instance.has_value()) {
            const auto& slot_name = instance->GetMountPoint();
            if (slot_name == mount_point_str) {
                // TODO: check if is busy
                instance->Umount();
                if (call_backup) {
                    Backup::StartThread();
                    Backup::NewRequest(instance->GetUserId(), instance->GetTitleId(),
                                       instance->GetDirName(),
                                       OrbisSaveDataEventType::UMOUNT_BACKUP);
                }
                instance.reset();
                return Error::OK;
            }
        }
    }
    return Error::NOT_FOUND;
}

void OrbisSaveDataParam::FromSFO(const PSF& sfo) {
    memset(this, 0, sizeof(OrbisSaveDataParam));
    title.FromString(sfo.GetString(SaveParams::MAINTITLE).value_or("Unknown"));
    subTitle.FromString(sfo.GetString(SaveParams::SUBTITLE).value_or(""));
    detail.FromString(sfo.GetString(SaveParams::DETAIL).value_or(""));
    userParam = sfo.GetInteger(SaveParams::SAVEDATA_LIST_PARAM).value_or(0);
    const auto time_since_epoch = sfo.GetLastWrite().time_since_epoch();
    mtime = chrono::duration_cast<chrono::seconds>(time_since_epoch).count();
}

void OrbisSaveDataParam::ToSFO(PSF& sfo) const {
    sfo.AddString(std::string{SaveParams::MAINTITLE}, std::string{title}, true);
    sfo.AddString(std::string{SaveParams::SUBTITLE}, std::string{subTitle}, true);
    sfo.AddString(std::string{SaveParams::DETAIL}, std::string{detail}, true);
    sfo.AddInteger(std::string{SaveParams::SAVEDATA_LIST_PARAM}, static_cast<s32>(userParam), true);
}

int PS4_SYSV_ABI sceSaveDataAbort() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataBackup(const OrbisSaveDataBackup* backup) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (backup == nullptr || backup->dirName == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }

    const std::string_view dir_name{backup->dirName->data};
    LOG_DEBUG(Lib_SaveData, "called dirName: {}", dir_name);

    std::string_view title{backup->titleId != nullptr ? std::string_view{backup->titleId->data}
                                                      : std::string_view{g_game_serial}};

    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetUserId() == backup->userId &&
            instance->GetTitleId() == title && instance->GetDirName() == dir_name) {
            return Error::BUSY;
        }
    }

    Backup::StartThread();
    Backup::NewRequest(backup->userId, title, dir_name, OrbisSaveDataEventType::BACKUP);

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataBindPsnAccount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataChangeDatabase() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataChangeInternal() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (check == nullptr || check->dirName == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }

    const std::string_view title{check->titleId != nullptr ? std::string_view{check->titleId->data}
                                                           : std::string_view{g_game_serial}};
    LOG_DEBUG(Lib_SaveData, "called with titleId={}", title);

    const auto save_path =
        SaveInstance::MakeDirSavePath(check->userId, title, check->dirName->data);

    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetSavePath() == save_path) {
            return Error::BUSY;
        }
    }

    if (Backup::IsBackupExecutingFor(save_path)) {
        return Error::BACKUP_BUSY;
    }

    const auto backup_path = Backup::MakeBackupPath(save_path);
    if (!fs::exists(backup_path)) {
        return Error::NOT_FOUND;
    }

    if (check->param != nullptr) {
        PSF sfo;
        if (!sfo.Open(backup_path / "sce_sys" / "param.sfo")) {
            LOG_ERROR(Lib_SaveData, "Failed to read SFO at {}", fmt::UTF(backup_path.u8string()));
            return Error::INTERNAL;
        }
        check->param->FromSFO(sfo);
    }

    if (check->icon != nullptr) {
        const auto icon_path = backup_path / "sce_sys" / "icon0.png";
        if (fs::exists(icon_path) && check->icon->LoadIcon(icon_path) != Error::OK) {
            return Error::INTERNAL;
        }
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataCheckBackupDataForCdlg() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckBackupDataInternal() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckCloudData() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckIpmiIfSize() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckSaveDataBroken() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersion() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersionLatest() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataClearProgress() {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    LOG_DEBUG(Lib_SaveData, "called");
    Backup::ClearProgress();
    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataCopy5() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataCreateUploadData() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebug() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugCleanMount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugCompiledSdkVersion() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugCreateSaveDataRoot() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugGetThreadId() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugRemoveSaveDataRoot() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDebugTarget() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (del == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    const std::string_view dirName{del->dirName->data};
    LOG_DEBUG(Lib_SaveData, "called dirName: {}", dirName);
    if (dirName.empty()) {
        return Error::PARAMETER;
    }
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetDirName() == dirName) {
            return Error::BUSY;
        }
    }
    const auto save_path = SaveInstance::MakeDirSavePath(del->userId, g_game_serial, dirName);
    try {
        if (fs::exists(save_path)) {
            fs::remove_all(save_path);
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR(Lib_SaveData, "Failed to delete save data: {}", e.what());
        return Error::INTERNAL;
    }
    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataDelete5() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDeleteAllUser() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDeleteCloudData() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDeleteUser() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
                                            OrbisSaveDataDirNameSearchResult* result) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (cond == nullptr || result == nullptr || cond->key > OrbisSaveDataSortKey::FREE_BLOCKS ||
        cond->order > OrbisSaveDataSortOrder::DESCENT) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    const std::string_view title_id{cond->titleId == nullptr
                                        ? std::string_view{g_game_serial}
                                        : std::string_view{cond->titleId->data}};
    const auto save_path = SaveInstance::MakeTitleSavePath(cond->userId, title_id);

    if (!fs::exists(save_path)) {
        result->hitNum = 0;
        if (g_fw_ver >= ElfInfo::FW_17) {
            result->setNum = 0;
        }
        return Error::OK;
    }

    std::vector<std::string> dir_list;

    for (const auto& path : fs::directory_iterator{save_path}) {
        auto dir_name = path.path().filename().string();
        // skip non-directories, sce_* and directories without param.sfo
        if (fs::is_directory(path) && !dir_name.starts_with("sce_") &&
            fs::exists(SaveInstance::GetParamSFOPath(path))) {
            dir_list.push_back(dir_name);
        }
    }
    if (cond->dirName != nullptr) {
        // Filter names
        const auto pat = Common::ToLower(std::string_view{cond->dirName->data});
        if (!pat.empty()) {
            std::erase_if(dir_list, [&](const std::string& dir_name) {
                return !match(Common::ToLower(dir_name), pat);
            });
        }
    }

    std::unordered_map<std::string, PSF> map_dir_sfo;
    std::unordered_map<std::string, int> map_max_blocks;
    std::unordered_map<std::string, OrbisSaveDataBlocks> map_free_size;

    for (const auto& dir_name : dir_list) {
        const auto dir_path = SaveInstance::MakeDirSavePath(cond->userId, title_id, dir_name);
        const auto sfo_path = SaveInstance::GetParamSFOPath(dir_path);
        PSF sfo;
        if (!sfo.Open(sfo_path)) {
            LOG_ERROR(Lib_SaveData, "Failed to read SFO: {}", fmt::UTF(sfo_path.u8string()));
            ASSERT_MSG(false, "Failed to read SFO");
        }

        size_t size = Common::FS::GetDirectorySize(dir_path);
        size_t total = SaveInstance::GetMaxBlockFromSFO(sfo);

        map_dir_sfo.emplace(dir_name, std::move(sfo));
        map_free_size.emplace(dir_name, total - size / OrbisSaveDataBlockSize);
        map_max_blocks.emplace(dir_name, total);
    }

#define PROJ(x) [&](const auto& v) { return x; }
    switch (cond->key) {
    case OrbisSaveDataSortKey::DIRNAME:
        std::ranges::stable_sort(dir_list);
        break;
    case OrbisSaveDataSortKey::USER_PARAM:
        std::ranges::stable_sort(
            dir_list, {},
            PROJ(map_dir_sfo.at(v).GetInteger(SaveParams::SAVEDATA_LIST_PARAM).value_or(0)));
        break;
    case OrbisSaveDataSortKey::BLOCKS:
        std::ranges::stable_sort(dir_list, {}, PROJ(map_max_blocks.at(v)));
        break;
    case OrbisSaveDataSortKey::FREE_BLOCKS:
        std::ranges::stable_sort(dir_list, {}, PROJ(map_free_size.at(v)));
        break;
    case OrbisSaveDataSortKey::MTIME:
        std::ranges::stable_sort(dir_list, {}, PROJ(map_dir_sfo.at(v).GetLastWrite()));
        break;
    }
#undef PROJ

    if (cond->order == OrbisSaveDataSortOrder::DESCENT) {
        std::ranges::reverse(dir_list);
    }

    size_t max_count = std::min(static_cast<size_t>(result->dirNamesNum), dir_list.size());
    if (g_fw_ver >= ElfInfo::FW_17) {
        result->hitNum = dir_list.size();
        result->setNum = max_count;
    } else {
        result->hitNum = max_count;
    }

    for (size_t i = 0; i < max_count; i++) {
        auto& name_data = result->dirNames[i].data;
        name_data.FromString(dir_list[i]);

        if (g_fw_ver >= ElfInfo::FW_17 && result->params != nullptr) {
            auto& sfo = map_dir_sfo.at(dir_list[i]);
            auto& param_data = result->params[i];
            param_data.FromSFO(sfo);
        }

        if (g_fw_ver >= ElfInfo::FW_25 && result->infos != nullptr) {
            auto& info = result->infos[i];
            info.blocks = map_max_blocks.at(dir_list[i]);
            info.freeBlocks = map_free_size.at(dir_list[i]);
        }
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataDirNameSearchInternal() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDownload() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetAllSize() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetAppLaunchedUser() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetAutoUploadConditions() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetAutoUploadRequestInfo() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetAutoUploadSetting() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetBoundPsnAccountCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetClientThreadPriority() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetCloudQuotaInfo() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetDataBaseFilePath() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetEventInfo() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam*,
                                             OrbisSaveDataEvent* event) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (event == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_TRACE(Lib_SaveData, "called");

    auto last_event = Backup::PopLastEvent();
    if (!last_event.has_value()) {
        return Error::NOT_FOUND;
    }

    event->type = last_event->origin;
    event->errorCode = 0;
    event->userId = last_event->user_id;
    event->titleId.data.FromString(last_event->title_id);
    event->dirName.data.FromString(last_event->dir_name);

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataGetFormat() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                           OrbisSaveDataMountInfo* info) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mountPoint == nullptr || info == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    const std::string_view mount_point_str{mountPoint->data};
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetMountPoint() == mount_point_str) {
            const u32 blocks = instance->GetMaxBlocks();
            const u64 size = Common::FS::GetDirectorySize(instance->GetSavePath());
            info->blocks = blocks;
            info->freeBlocks = blocks - size / OrbisSaveDataBlockSize;
            return Error::OK;
        }
    }
    return Error::NOT_MOUNTED;
}

Error PS4_SYSV_ABI sceSaveDataGetParam(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataParamType paramType, void* paramBuf,
                                       size_t paramBufSize, size_t* gotSize) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (paramType > OrbisSaveDataParamType::MTIME || paramBuf == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called: paramType = {}", magic_enum::enum_name(paramType));
    const PSF* param_sfo = nullptr;

    const std::string_view mount_point_str{mountPoint->data};
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetMountPoint() == mount_point_str) {
            param_sfo = &instance->GetParamSFO();
            break;
        }
    }
    if (param_sfo == nullptr) {
        return Error::NOT_MOUNTED;
    }

    switch (paramType) {
    case OrbisSaveDataParamType::ALL: {
        const auto param = static_cast<OrbisSaveDataParam*>(paramBuf);
        ASSERT(paramBufSize == sizeof(OrbisSaveDataParam));
        param->FromSFO(*param_sfo);
        if (gotSize != nullptr) {
            *gotSize = sizeof(OrbisSaveDataParam);
        }
        break;
    }
    case OrbisSaveDataParamType::TITLE:
    case OrbisSaveDataParamType::SUB_TITLE:
    case OrbisSaveDataParamType::DETAIL: {
        const auto param = static_cast<char*>(paramBuf);
        std::string_view key;
        if (paramType == OrbisSaveDataParamType::TITLE) {
            key = SaveParams::MAINTITLE;
        } else if (paramType == OrbisSaveDataParamType::SUB_TITLE) {
            key = SaveParams::SUBTITLE;
        } else if (paramType == OrbisSaveDataParamType::DETAIL) {
            key = SaveParams::DETAIL;
        } else {
            UNREACHABLE();
        }
        const size_t s = param_sfo->GetString(key).value_or("").copy(param, paramBufSize - 1);
        param[s] = '\0'; // null terminate
        if (gotSize != nullptr) {
            *gotSize = s + 1;
        }
    } break;
    case OrbisSaveDataParamType::USER_PARAM: {
        const auto param = static_cast<u32*>(paramBuf);
        *param = param_sfo->GetInteger(SaveParams::SAVEDATA_LIST_PARAM).value_or(0);
        if (gotSize != nullptr) {
            *gotSize = sizeof(u32);
        }
    } break;
    case OrbisSaveDataParamType::MTIME: {
        const auto param = static_cast<time_t*>(paramBuf);
        const auto last_write = param_sfo->GetLastWrite().time_since_epoch();
        *param = chrono::duration_cast<chrono::seconds>(last_write).count();
        if (gotSize != nullptr) {
            *gotSize = sizeof(time_t);
        }
    } break;
    default:
        UNREACHABLE();
    }

    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataGetProgress(float* progress) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (progress == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    *progress = Backup::GetProgress();
    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(const OrbisUserServiceUserId userId, void* buf,
                                                const size_t bufSize, const int64_t offset) {
    LOG_DEBUG(Lib_SaveData, "Redirecting to sceSaveDataGetSaveDataMemory2");
    OrbisSaveDataMemoryData data{};
    data.buf = buf;
    data.bufSize = bufSize;
    data.offset = offset;
    OrbisSaveDataMemoryGet2 param{};
    param.userId = userId;
    param.data = &data;
    param.param = nullptr;
    param.icon = nullptr;
    return sceSaveDataGetSaveDataMemory2(&param);
}

Error PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(OrbisSaveDataMemoryGet2* getParam) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (getParam == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    if (!SaveMemory::IsSaveMemoryInitialized()) {
        LOG_INFO(Lib_SaveData, "called without save memory initialized");
        return Error::MEMORY_NOT_READY;
    }
    if (SaveMemory::IsSaving()) {
        LOG_TRACE(Lib_SaveData, "called while saving");
        return Error::BUSY_FOR_SAVING;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    auto data = getParam->data;
    if (data != nullptr) {
        SaveMemory::ReadMemory(data->buf, data->bufSize, data->offset);
    }
    auto param = getParam->param;
    if (param != nullptr) {
        param->FromSFO(SaveMemory::GetParamSFO());
    }
    auto icon = getParam->icon;
    if (icon != nullptr) {
        auto icon_mem = SaveMemory::GetIcon();
        size_t total = std::min(icon->bufSize, icon_mem.size());
        std::memcpy(icon->buf, icon_mem.data(), total);
        icon->dataSize = total;
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataRootDir() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataRootPath() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataRootUsbPath() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSavePoint() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetUpdatedDataCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataInitialize(void*) {
    LOG_DEBUG(Lib_SaveData, "called");
    initialize();
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataInitialize2(void*) {
    LOG_DEBUG(Lib_SaveData, "called");
    initialize();
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataInitialize3(void*) {
    LOG_DEBUG(Lib_SaveData, "called");
    initialize();
    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataInitializeForCdlg() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataIsDeletingUsbDb() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataIsMounted() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataIcon* icon) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mountPoint == nullptr || icon == nullptr || icon->buf == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    std::filesystem::path path;
    const std::string_view mount_point_str{mountPoint->data};
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetMountPoint() == mount_point_str) {
            path = instance->GetIconPath();
            break;
        }
    }
    if (path.empty()) {
        return Error::NOT_MOUNTED;
    }
    if (!fs::exists(path)) {
        return Error::NOT_FOUND;
    }

    return icon->LoadIcon(path);
}

Error PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                    OrbisSaveDataMountResult* mount_result) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mount == nullptr && mount->dirName != nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called dirName: {}, mode: {:0b}, blocks: {}",
              mount->dirName->data.to_view(), (int)mount->mountMode, mount->blocks);

    OrbisSaveDataMount2 mount_info{};
    mount_info.userId = mount->userId;
    mount_info.dirName = mount->dirName;
    mount_info.mountMode = mount->mountMode;
    mount_info.blocks = mount->blocks;
    return saveDataMount(&mount_info, mount_result);
}

Error PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                     OrbisSaveDataMountResult* mount_result) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mount == nullptr && mount->dirName != nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called dirName: {}, mode: {:0b}, blocks: {}",
              mount->dirName->data.to_view(), (int)mount->mountMode, mount->blocks);
    return saveDataMount(mount, mount_result);
}

int PS4_SYSV_ABI sceSaveDataMount5() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataMountInternal() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataMountSys() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataPromote5() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataRebuildDatabase() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataRegisterEventCallback() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataRestoreBackupData(const OrbisSaveDataRestoreBackupData* restore) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (restore == nullptr || restore->dirName == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }

    const std::string_view dir_name{restore->dirName->data};
    LOG_DEBUG(Lib_SaveData, "called dirName: {}", dir_name);

    std::string_view title{restore->titleId != nullptr ? std::string_view{restore->titleId->data}
                                                       : std::string_view{g_game_serial}};

    const auto save_path = SaveInstance::MakeDirSavePath(restore->userId, title, dir_name);

    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetSavePath() == save_path) {
            return Error::BUSY;
        }
    }
    if (Backup::IsBackupExecutingFor(save_path)) {
        return Error::BACKUP_BUSY;
    }

    const auto backup_path = Backup::MakeBackupPath(save_path);
    if (!fs::exists(backup_path)) {
        return Error::NOT_FOUND;
    }

    const bool ok = Backup::Restore(save_path);
    if (!ok) {
        return Error::INTERNAL;
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataRestoreBackupDataForCdlg() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataRestoreLoadSaveDataMemory() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                       const OrbisSaveDataIcon* icon) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mountPoint == nullptr || icon == nullptr || icon->buf == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    std::filesystem::path path;
    const std::string_view mount_point_str{mountPoint->data};
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetMountPoint() == mount_point_str) {
            if (instance->IsReadOnly()) {
                return Error::BAD_MOUNTED;
            }
            path = instance->GetIconPath();
            break;
        }
    }
    if (path.empty()) {
        return Error::NOT_MOUNTED;
    }

    try {
        const Common::FS::IOFile file(path, Common::FS::FileAccessMode::Write);
        file.WriteRaw<u8>(icon->buf, std::min(icon->bufSize, icon->dataSize));
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR(Lib_SaveData, "Failed to load icon: {}", e.what());
        return Error::INTERNAL;
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetEventInfo() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataParamType paramType, const void* paramBuf,
                                       size_t paramBufSize) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (paramType > OrbisSaveDataParamType::USER_PARAM || mountPoint == nullptr ||
        paramBuf == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called: paramType = {}", magic_enum::enum_name(paramType));
    PSF* param_sfo = nullptr;
    const std::string_view mount_point_str{mountPoint->data};
    for (auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetMountPoint() == mount_point_str) {
            param_sfo = &instance->GetParamSFO();
            break;
        }
    }
    if (param_sfo == nullptr) {
        return Error::NOT_MOUNTED;
    }

    switch (paramType) {
    case OrbisSaveDataParamType::ALL: {
        const auto param = static_cast<const OrbisSaveDataParam*>(paramBuf);
        ASSERT(paramBufSize == sizeof(OrbisSaveDataParam));
        param->ToSFO(*param_sfo);
        return Error::OK;
    } break;
    case OrbisSaveDataParamType::TITLE: {
        const auto value = static_cast<const char*>(paramBuf);
        param_sfo->AddString(std::string{SaveParams::MAINTITLE}, {value}, true);
    } break;
    case OrbisSaveDataParamType::SUB_TITLE: {
        const auto value = static_cast<const char*>(paramBuf);
        param_sfo->AddString(std::string{SaveParams::SUBTITLE}, {value}, true);
    } break;
    case OrbisSaveDataParamType::DETAIL: {
        const auto value = static_cast<const char*>(paramBuf);
        param_sfo->AddString(std::string{SaveParams::DETAIL}, {value}, true);
    } break;
    case OrbisSaveDataParamType::USER_PARAM: {
        const auto value = static_cast<const s32*>(paramBuf);
        param_sfo->AddInteger(std::string{SaveParams::SAVEDATA_LIST_PARAM}, *value, true);
    } break;
    default:
        UNREACHABLE();
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(OrbisUserServiceUserId userId, void* buf,
                                                size_t bufSize, int64_t offset) {
    LOG_DEBUG(Lib_SaveData, "Redirecting to sceSaveDataSetSaveDataMemory2");
    OrbisSaveDataMemoryData data{};
    data.buf = buf;
    data.bufSize = bufSize;
    data.offset = offset;
    OrbisSaveDataMemorySet2 setParam{};
    setParam.userId = userId;
    setParam.data = &data;
    setParam.param = nullptr;
    setParam.icon = nullptr;
    return sceSaveDataSetSaveDataMemory2(&setParam);
}

Error PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (setParam == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    if (!SaveMemory::IsSaveMemoryInitialized()) {
        LOG_INFO(Lib_SaveData, "called without save memory initialized");
        return Error::MEMORY_NOT_READY;
    }
    if (SaveMemory::IsSaving()) {
        LOG_TRACE(Lib_SaveData, "called while saving");
        return Error::BUSY_FOR_SAVING;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    auto data = setParam->data;
    if (data != nullptr) {
        SaveMemory::WriteMemory(data->buf, data->bufSize, data->offset);
    }
    auto param = setParam->param;
    if (param != nullptr) {
        param->ToSFO(SaveMemory::GetParamSFO());
        SaveMemory::SaveSFO();
    }
    auto icon = setParam->icon;
    if (icon != nullptr) {
        SaveMemory::WriteIcon(icon->buf, icon->bufSize);
    }

    SaveMemory::TriggerSaveWithoutEvent();
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(OrbisUserServiceUserId userId, size_t memorySize,
                                                  OrbisSaveDataParam* param) {
    LOG_DEBUG(Lib_SaveData, "called: userId = {}, memorySize = {}", userId, memorySize);
    OrbisSaveDataMemorySetup2 setupParam{};
    setupParam.userId = userId;
    setupParam.memorySize = memorySize;
    setupParam.initParam = nullptr;
    setupParam.initIcon = nullptr;
    OrbisSaveDataMemorySetupResult result{};
    const auto res = sceSaveDataSetupSaveDataMemory2(&setupParam, &result);
    if (res != Error::OK) {
        return res;
    }
    if (param != nullptr) {
        OrbisSaveDataMemorySet2 setParam{};
        setParam.userId = userId;
        setParam.data = nullptr;
        setParam.param = param;
        setParam.icon = nullptr;
        sceSaveDataSetSaveDataMemory2(&setParam);
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                   OrbisSaveDataMemorySetupResult* result) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (setupParam == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called");

    SaveMemory::SetDirectories(setupParam->userId, g_game_serial);

    const auto& save_path = SaveMemory::GetSavePath();
    for (const auto& instance : g_mount_slots) {
        if (instance.has_value() && instance->GetSavePath() == save_path) {
            return Error::BUSY;
        }
    }

    try {
        size_t existed_size = SaveMemory::CreateSaveMemory(setupParam->memorySize);
        if (existed_size == 0) { // Just created
            if (g_fw_ver >= ElfInfo::FW_45 && setupParam->initParam != nullptr) {
                auto& sfo = SaveMemory::GetParamSFO();
                setupParam->initParam->ToSFO(sfo);
            }
            SaveMemory::SaveSFO();

            auto init_icon = setupParam->initIcon;
            if (g_fw_ver >= ElfInfo::FW_45 && init_icon != nullptr) {
                SaveMemory::SetIcon(init_icon->buf, init_icon->bufSize);
            } else {
                SaveMemory::SetIcon(nullptr, 0);
            }
        }
        SaveMemory::TriggerSaveWithoutEvent();
        if (g_fw_ver >= ElfInfo::FW_45 && result != nullptr) {
            result->existedMemorySize = existed_size;
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR(Lib_SaveData, "Failed to create/load save memory: {}", e.what());

        const MsgDialog::MsgDialogState dialog{MsgDialog::MsgDialogState::UserState{
            .type = MsgDialog::ButtonType::OK,
            .msg = "Failed to create or load save memory:\n" + std::string{e.what()},
        }};
        MsgDialog::ShowMsgDialog(dialog);

        return Error::INTERNAL;
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceSaveDataShutdownStart() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSyncCloudList() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(OrbisSaveDataMemorySync* syncParam) {
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (syncParam == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    if (!SaveMemory::IsSaveMemoryInitialized()) {
        LOG_INFO(Lib_SaveData, "called without save memory initialized");
        return Error::MEMORY_NOT_READY;
    }
    LOG_DEBUG(Lib_SaveData, "called");
    bool ok = SaveMemory::TriggerSave();
    if (!ok) {
        return Error::BUSY_FOR_SAVING;
    }
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataTerminate() {
    LOG_DEBUG(Lib_SaveData, "called");
    if (!g_initialized) {
        return setNotInitializedError();
    }
    for (auto& instance : g_mount_slots) {
        if (instance.has_value()) {
            if (g_fw_ver >= ElfInfo::FW_40) {
                return Error::BUSY;
            }
            instance->Umount();
            instance.reset();
        }
    }
    g_initialized = false;
    Backup::StopThread();
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataTransferringMount(const OrbisSaveDataTransferringMount* mount,
                                                OrbisSaveDataMountResult* mountResult) {
    LOG_DEBUG(Lib_SaveData, "called");
    if (!g_initialized) {
        LOG_INFO(Lib_SaveData, "called without initialize");
        return setNotInitializedError();
    }
    if (mount == nullptr || mount->titleId == nullptr || mount->dirName == nullptr) {
        LOG_INFO(Lib_SaveData, "called with invalid parameter");
        return Error::PARAMETER;
    }
    LOG_DEBUG(Lib_SaveData, "called titleId: {}, dirName: {}", mount->titleId->data.to_view(),
              mount->dirName->data.to_view());
    OrbisSaveDataMount2 mount_info{};
    mount_info.userId = mount->userId;
    mount_info.dirName = mount->dirName;
    mount_info.mountMode = OrbisSaveDataMountMode::RDONLY;
    return saveDataMount(&mount_info, mountResult, mount->titleId->data.to_string());
}

Error PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint) {
    LOG_DEBUG(Lib_SaveData, "called");
    return Umount(mountPoint);
}

int PS4_SYSV_ABI sceSaveDataUmountSys() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint) {
    LOG_DEBUG(Lib_SaveData, "called");
    return Umount(mountPoint, true);
}

int PS4_SYSV_ABI sceSaveDataUnregisterEventCallback() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataUpload() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_02E4C4D201716422() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceSaveData(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("dQ2GohUHXzk", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataAbort);
    LIB_FUNCTION("z1JA8-iJt3k", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataBackup);
    LIB_FUNCTION("kLJQ3XioYiU", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataBindPsnAccount);
    LIB_FUNCTION("hHHCPRqA3+g", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataBindPsnAccountForSystemBackup);
    LIB_FUNCTION("ykwIZfVD08s", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataChangeDatabase);
    LIB_FUNCTION("G0hFeOdRCUs", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataChangeInternal);
    LIB_FUNCTION("RQOqDbk3bSU", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckBackupData);
    LIB_FUNCTION("rYvLW1z2poM", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckBackupDataForCdlg);
    LIB_FUNCTION("v1TrX+3ZB10", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckBackupDataInternal);
    LIB_FUNCTION("-eczr5e4dsI", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckCloudData);
    LIB_FUNCTION("4OPOZxfVkHA", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckIpmiIfSize);
    LIB_FUNCTION("1i0rfc+mfa8", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckSaveDataBroken);
    LIB_FUNCTION("p6A1adyQi3E", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckSaveDataVersion);
    LIB_FUNCTION("S49B+I96kpk", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCheckSaveDataVersionLatest);
    LIB_FUNCTION("Wz-4JZfeO9g", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataClearProgress);
    LIB_FUNCTION("YbCO38BOOl4", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataCopy5);
    LIB_FUNCTION("kbIIP9aXK9A", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataCreateUploadData);
    LIB_FUNCTION("gW6G4HxBBXA", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataDebug);
    LIB_FUNCTION("bYCnxLexU7M", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugCleanMount);
    LIB_FUNCTION("hVDqYB8+jkk", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugCompiledSdkVersion);
    LIB_FUNCTION("K9gXXlrVLNI", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugCreateSaveDataRoot);
    LIB_FUNCTION("5yHFvMwZX2o", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugGetThreadId);
    LIB_FUNCTION("UGTldPVEdB4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugRemoveSaveDataRoot);
    LIB_FUNCTION("AYBQmnRplrg", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDebugTarget);
    LIB_FUNCTION("S1GkePI17zQ", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataDelete);
    LIB_FUNCTION("SQWusLoK8Pw", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataDelete5);
    LIB_FUNCTION("pJrlpCgR8h4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDeleteAllUser);
    LIB_FUNCTION("fU43mJUgKcM", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDeleteCloudData);
    LIB_FUNCTION("uZqc4JpFdeY", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataDeleteUser);
    LIB_FUNCTION("dyIhnXq-0SM", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDirNameSearch);
    LIB_FUNCTION("xJ5NFWC3m+k", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataDirNameSearchInternal);
    LIB_FUNCTION("h1nP9EYv3uc", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataDownload);
    LIB_FUNCTION("A1ThglSGUwA", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataGetAllSize);
    LIB_FUNCTION("KuXcrMAQIMQ", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetAppLaunchedUser);
    LIB_FUNCTION("itZ46iH14Vs", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetAutoUploadConditions);
    LIB_FUNCTION("PL20kjAXZZ4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetAutoUploadRequestInfo);
    LIB_FUNCTION("G12foE0S77E", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetAutoUploadSetting);
    LIB_FUNCTION("PzDtD6eBXIM", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetBoundPsnAccountCount);
    LIB_FUNCTION("tu0SDPl+h88", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetClientThreadPriority);
    LIB_FUNCTION("6lZYZqQPfkY", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetCloudQuotaInfo);
    LIB_FUNCTION("CWlBd2Ay1M4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetDataBaseFilePath);
    LIB_FUNCTION("eBSSNIG6hMk", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetEventInfo);
    LIB_FUNCTION("j8xKtiFj0SY", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetEventResult);
    LIB_FUNCTION("UMpxor4AlKQ", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataGetFormat);
    LIB_FUNCTION("pc4guaUPVqA", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetMountedSaveDataCount);
    LIB_FUNCTION("65VH0Qaaz6s", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetMountInfo);
    LIB_FUNCTION("XgvSuIdnMlw", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataGetParam);
    LIB_FUNCTION("ANmSWUiyyGQ", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetProgress);
    LIB_FUNCTION("SN7rTPHS+Cg", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataCount);
    LIB_FUNCTION("7Bt5pBC-Aco", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataMemory);
    LIB_FUNCTION("QwOO7vegnV8", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataMemory2);
    LIB_FUNCTION("+bRDRotfj0Y", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataRootDir);
    LIB_FUNCTION("3luF0xq0DkQ", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataRootPath);
    LIB_FUNCTION("DwAvlQGvf1o", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSaveDataRootUsbPath);
    LIB_FUNCTION("kb24-4DLyNo", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetSavePoint);
    LIB_FUNCTION("OYmnApJ9q+U", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataGetUpdatedDataCount);
    LIB_FUNCTION("ZkZhskCPXFw", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataInitialize);
    LIB_FUNCTION("l1NmDeDpNGU", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataInitialize2);
    LIB_FUNCTION("TywrFKCoLGY", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataInitialize3);
    LIB_FUNCTION("g9uwUI3BlQ8", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataInitializeForCdlg);
    LIB_FUNCTION("voAQW45oKuo", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataIsDeletingUsbDb);
    LIB_FUNCTION("ieP6jP138Qo", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataIsMounted);
    LIB_FUNCTION("cGjO3wM3V28", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataLoadIcon);
    LIB_FUNCTION("32HQAQdwM2o", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMount);
    LIB_FUNCTION("0z45PIH+SNI", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMount2);
    LIB_FUNCTION("xz0YMi6BfNk", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMount5);
    LIB_FUNCTION("msCER7Iibm8", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataMountInternal);
    LIB_FUNCTION("-XYmdxjOqyA", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataMountSys);
    LIB_FUNCTION("uNu7j3pL2mQ", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataPromote5);
    LIB_FUNCTION("SgIY-XYA2Xg", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataRebuildDatabase);
    LIB_FUNCTION("hsKd5c21sQc", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataRegisterEventCallback);
    LIB_FUNCTION("lU9YRFsgwSU", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataRestoreBackupData);
    LIB_FUNCTION("HuToUt1GQ8w", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataRestoreBackupDataForCdlg);
    LIB_FUNCTION("aoZKKNjlq3Y", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataRestoreLoadSaveDataMemory);
    LIB_FUNCTION("c88Yy54Mx0w", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataSaveIcon);
    LIB_FUNCTION("0VFHv-Fa4w8", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetAutoUploadSetting);
    LIB_FUNCTION("52pL2GKkdjA", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetEventInfo);
    LIB_FUNCTION("85zul--eGXs", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataSetParam);
    LIB_FUNCTION("v3vg2+cooYw", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetSaveDataLibraryUser);
    LIB_FUNCTION("h3YURzXGSVQ", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetSaveDataMemory);
    LIB_FUNCTION("cduy9v4YmT4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetSaveDataMemory2);
    LIB_FUNCTION("v7AAAMo0Lz4", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetupSaveDataMemory);
    LIB_FUNCTION("oQySEUfgXRA", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSetupSaveDataMemory2);
    LIB_FUNCTION("zMgXM79jRhw", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataShutdownStart);
    LIB_FUNCTION("+orZm32HB1s", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSupportedFakeBrokenStatus);
    LIB_FUNCTION("LMSQUTxmGVg", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSyncCloudList);
    LIB_FUNCTION("wiT9jeC7xPw", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataSyncSaveDataMemory);
    LIB_FUNCTION("yKDy8S5yLA0", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataTerminate);
    LIB_FUNCTION("WAzWTZm1H+I", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataTransferringMount);
    LIB_FUNCTION("BMR4F-Uek3E", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataUmount);
    LIB_FUNCTION("2-8NWLS8QSA", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataUmountSys);
    LIB_FUNCTION("VwadwBBBJ80", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataUmountWithBackup);
    LIB_FUNCTION("v-AK1AxQhS0", "libSceSaveData", 1, "libSceSaveData", 1, 1,
                 sceSaveDataUnregisterEventCallback);
    LIB_FUNCTION("COwz3WBj+5s", "libSceSaveData", 1, "libSceSaveData", 1, 1, sceSaveDataUpload);
    LIB_FUNCTION("AuTE0gFxZCI", "libSceSaveData", 1, "libSceSaveData", 1, 1, Func_02E4C4D201716422);
};

} // namespace Libraries::SaveData
