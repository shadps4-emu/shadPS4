// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>
#include <common/path_util.h>
#include <common/singleton.h>
#include <core/file_format/psf.h>
#include <core/file_sys/fs.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/save_data/savedata.h"
#include "error_codes.h"

namespace Libraries::SaveData {
static std::string g_mount_point = "/savedata0"; // temp mount point (todo)
std::string game_serial;

int PS4_SYSV_ABI sceSaveDataAbort() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataBackup() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string mount_dir = mnt->GetHostFile(check->dirName->data);
    if (!std::filesystem::exists(mount_dir)) {
        return ORBIS_SAVE_DATA_ERROR_NOT_FOUND;
    }
    LOG_INFO(Lib_SaveData, "called = {}", mount_dir);

    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataClearProgress() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del) {
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(1) / game_serial / std::string(del->dirName->data);
    LOG_INFO(Lib_SaveData, "called: dirname = {}, mount_dir = {}", (char*)del->dirName->data,
             mount_dir.string());
    if (std::filesystem::exists(mount_dir) && std::filesystem::is_directory(mount_dir)) {
        std::filesystem::remove_all(mount_dir);
    }
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
                                          OrbisSaveDataDirNameSearchResult* result) {
    if (cond == nullptr)
        return ORBIS_SAVE_DATA_ERROR_PARAMETER;
    LOG_ERROR(Lib_SaveData, "TODO sceSaveDataDirNameSearch: Add params");
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(cond->userId) / game_serial;
    if (!mount_dir.empty() && std::filesystem::exists(mount_dir)) {
        if (cond->dirName == nullptr) { // look for all dirs if no dir is provided.
            for (int i = 0; const auto& entry : std::filesystem::directory_iterator(mount_dir)) {
                if (std::filesystem::is_directory(entry.path())) {
                    i++;
                    result->dirNamesNum = 0; // why is it 1024? is it max?
                    // copy dir name to be used by sceSaveDataMount in read mode.
                    strncpy(result->dirNames[i].data, entry.path().filename().string().c_str(), 32);
                    result->hitNum = i + 1;
                    result->dirNamesNum = i + 1; // to confirm
                    result->setNum = i + 1;      // to confirm
                }
            }
        } else { // Need a game to test.
            strncpy(result->dirNames[0].data, cond->dirName->data, 32);
            result->hitNum = 1;
            result->dirNamesNum = 1; // to confirm
            result->setNum = 1;      // to confirm
        }
    } else {
        result->hitNum = 0;
        result->dirNamesNum = 0;
        result->setNum = 0;
    }
    if (result->infos != nullptr) {
        result->infos->blocks = ORBIS_SAVE_DATA_BLOCK_SIZE;
        result->infos->freeBlocks = ORBIS_SAVE_DATA_BLOCK_SIZE;
    }
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam* eventParam,
                                           OrbisSaveDataEvent* event) {
    // eventParam can be 0/null.
    if (event == nullptr)
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;

    LOG_INFO(Lib_SaveData, "called: Todo.");
    event->userId = 1;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetFormat() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                         OrbisSaveDataMountInfo* info) {
    LOG_INFO(Lib_SaveData, "called");
    info->blocks = ORBIS_SAVE_DATA_BLOCKS_MAX;
    info->freeBlocks = ORBIS_SAVE_DATA_BLOCKS_MAX;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetParam() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetProgress() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataCount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(const u32 userId, void* buf, const size_t bufSize,
                                              const int64_t offset) {
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(userId) / game_serial / "save_mem1.sav";

    Common::FS::IOFile file(mount_dir, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }
    file.Seek(offset);
    size_t nbytes = file.ReadRaw<u8>(buf, bufSize);
    LOG_INFO(Lib_SaveData, "called: bufSize = {}, offset = {}", bufSize, offset, nbytes);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(OrbisSaveDataMemoryGet2* getParam) {
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(getParam->userId) / game_serial;
    if (getParam == nullptr)
        return ORBIS_SAVE_DATA_ERROR_PARAMETER;
    if (getParam->data != nullptr) {
        Common::FS::IOFile file(mount_dir / "save_mem2.sav", Common::FS::FileAccessMode::Read);
        if (!file.IsOpen()) {
            return false;
        }
        file.Seek(getParam->data->offset);
        size_t nbytes = file.ReadRaw<u8>(getParam->data->buf, getParam->data->bufSize);
        LOG_INFO(Lib_SaveData, "called: bufSize = {}, offset = {}", getParam->data->bufSize,
                 getParam->data->offset);
    }

    if (getParam->param != nullptr) {
        Common::FS::IOFile file1(mount_dir / "param.txt", Common::FS::FileAccessMode::Read);
        size_t nbytes = file1.ReadRaw<u8>(getParam->param, sizeof(OrbisSaveDataParam));
    }

    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataInitialize() {
    LOG_INFO(Lib_SaveData, "called");
    static auto* param_sfo = Common::Singleton<PSF>::Instance();
    game_serial = std::string(param_sfo->GetString("CONTENT_ID"), 7, 9);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataInitialize2() {
    LOG_INFO(Lib_SaveData, "called");
    static auto* param_sfo = Common::Singleton<PSF>::Instance();
    game_serial = std::string(param_sfo->GetString("CONTENT_ID"), 7, 9);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataInitialize3() {
    LOG_INFO(Lib_SaveData, "called");
    static auto* param_sfo = Common::Singleton<PSF>::Instance();
    game_serial = std::string(param_sfo->GetString("CONTENT_ID"), 7, 9);
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataIcon* icon) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string mount_dir = mnt->GetHostFile(mountPoint->data);
    LOG_INFO(Lib_SaveData, "called: dir = {}", mount_dir);

    if (icon != nullptr) {
        Common::FS::IOFile file(mount_dir + "/save_data.png", Common::FS::FileAccessMode::Read);
        icon->bufSize = file.GetSize();
        size_t nbytes = file.ReadRaw<u8>(icon->buf, icon->bufSize);
    }
    return ORBIS_OK;
}

s32 saveDataMount(u32 user_id, char* dir_name, u32 mount_mode,
                  OrbisSaveDataMountResult* mount_result) {
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(user_id) / game_serial / dir_name;
    switch (mount_mode) {
    case ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY:
    case ORBIS_SAVE_DATA_MOUNT_MODE_RDWR:
    case ORBIS_SAVE_DATA_MOUNT_MODE_RDWR | ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF:
    case ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY | ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF: {
        if (!std::filesystem::exists(mount_dir)) {
            return ORBIS_SAVE_DATA_ERROR_NOT_FOUND;
        }
        auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        mount_result->mount_status = 0;
        std::strncpy(mount_result->mount_point.data, g_mount_point.c_str(), 16);
        mnt->Mount(mount_dir, mount_result->mount_point.data);
    } break;
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE | ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR |
        ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR |
        ORBIS_SAVE_DATA_MOUNT_MODE_COPY_ICON:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE | ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF |
        ORBIS_SAVE_DATA_MOUNT_MODE_COPY_ICON: {
        auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        if (std::filesystem::exists(mount_dir)) {
            std::strncpy(mount_result->mount_point.data, g_mount_point.c_str(), 16);
            mnt->Mount(mount_dir, mount_result->mount_point.data);
            mount_result->required_blocks = 0;
            mount_result->mount_status = 0;
            return ORBIS_SAVE_DATA_ERROR_EXISTS;
        }
        if (std::filesystem::create_directories(mount_dir)) {
            std::strncpy(mount_result->mount_point.data, g_mount_point.c_str(), 16);
            mnt->Mount(mount_dir, mount_result->mount_point.data);
            mount_result->mount_status = 1;
        }
    } break;
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2 | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR:
    case ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2 | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR |
        ORBIS_SAVE_DATA_MOUNT_MODE_COPY_ICON: {
        if (!std::filesystem::exists(mount_dir)) {
            std::filesystem::create_directories(mount_dir);
        }
        auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        std::strncpy(mount_result->mount_point.data, g_mount_point.c_str(), 16);
        mnt->Mount(mount_dir, mount_result->mount_point.data);
        mount_result->mount_status = 1;
    } break;
    default:
        UNREACHABLE();
    }
    mount_result->required_blocks = 0;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                  OrbisSaveDataMountResult* mount_result) {
    if (mount == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_PARAMETER;
    }
    LOG_INFO(Lib_SaveData, "called: dirName = {}, mode = {}, blocks = {}", mount->dir_name->data,
             mount->mount_mode, mount->blocks);
    return saveDataMount(mount->user_id, (char*)mount->dir_name->data, mount->mount_mode,
                         mount_result);
}

s32 PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                   OrbisSaveDataMountResult* mount_result) {
    if (mount == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_PARAMETER;
    }
    LOG_INFO(Lib_SaveData, "called: dirName = {}, mode = {}, blocks = {}", mount->dir_name->data,
             mount->mount_mode, mount->blocks);
    return saveDataMount(mount->user_id, (char*)mount->dir_name->data, mount->mount_mode,
                         mount_result);
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

int PS4_SYSV_ABI sceSaveDataRestoreBackupData() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataRestoreBackupDataForCdlg() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataRestoreLoadSaveDataMemory() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     const OrbisSaveDataIcon* icon) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string mount_dir = mnt->GetHostFile(mountPoint->data);
    LOG_INFO(Lib_SaveData, "called = {}", mount_dir);

    if (icon != nullptr) {
        Common::FS::IOFile file(mount_dir + "/save_data.png", Common::FS::FileAccessMode::Write);
        file.WriteRaw<u8>((void*)icon->buf, icon->bufSize);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetEventInfo() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataParamType paramType, const void* paramBuf,
                                     size_t paramBufSize) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string mount_dir = mnt->GetHostFile(mountPoint->data);
    LOG_INFO(Lib_SaveData, "called = {}, mountPoint->data = {}", mount_dir, mountPoint->data);

    if (paramBuf != nullptr) {
        Common::FS::IOFile file(mount_dir + "/param.txt", Common::FS::FileAccessMode::Write);
        file.WriteRaw<u8>((void*)paramBuf, paramBufSize);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(const u32 userId, const void* buf,
                                              const size_t bufSize, const int64_t offset) {
    LOG_INFO(Lib_SaveData, "called");
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(userId) / game_serial / "save_mem1.sav";

    Common::FS::IOFile file(mount_dir, Common::FS::FileAccessMode::Write);
    file.Seek(offset);
    file.WriteRaw<u8>((void*)buf, bufSize);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam) {
    LOG_INFO(Lib_SaveData, "called: dataNum = {}, slotId= {}", setParam->dataNum, setParam->slotId);
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(setParam->userId) / game_serial;
    if (setParam->data != nullptr) {
        Common::FS::IOFile file(mount_dir / "save_mem2.sav", Common::FS::FileAccessMode::Write);
        if (!file.IsOpen())
            return -1;
        file.Seek(setParam->data->offset);
        file.WriteRaw<u8>((void*)setParam->data->buf, setParam->data->bufSize);
    }

    if (setParam->param != nullptr) {
        Common::FS::IOFile file(mount_dir / "param.txt", Common::FS::FileAccessMode::Write);
        file.WriteRaw<u8>((void*)setParam->param, sizeof(OrbisSaveDataParam));
    }

    if (setParam->icon != nullptr) {
        Common::FS::IOFile file(mount_dir / "save_icon.png", Common::FS::FileAccessMode::Write);
        file.WriteRaw<u8>((void*)setParam->icon->buf, setParam->icon->bufSize);
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(u32 userId, size_t memorySize,
                                                OrbisSaveDataParam* param) {

    LOG_INFO(Lib_SaveData, "called:userId = {}, memorySize = {}", userId, memorySize);

    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(userId) / game_serial;

    if (std::filesystem::exists(mount_dir)) {
        return ORBIS_SAVE_DATA_ERROR_EXISTS;
    }
    std::filesystem::create_directories(mount_dir);
    std::vector<u8> buf(memorySize);
    Common::FS::IOFile::WriteBytes(mount_dir / "save_mem1.sav", buf);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                 OrbisSaveDataMemorySetupResult* result) {
    if (setupParam == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_PARAMETER;
    }
    LOG_INFO(Lib_SaveData, "called");
    // if (setupParam->option == 1) { // check this later.
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(setupParam->userId) / game_serial;
    if (std::filesystem::exists(mount_dir) &&
        std::filesystem::exists(mount_dir / "save_mem2.sav")) {
        Common::FS::IOFile file(mount_dir / "save_mem2.sav", Common::FS::FileAccessMode::Read);
        if (!file.IsOpen())
            return -1;
        // Bunny - CUSA07988 has a null result, having null result is checked and valid.
        if (result != nullptr)
            result->existedMemorySize = file.GetSize(); // Assign the saved data size.
        //  do not return ORBIS_SAVE_DATA_ERROR_EXISTS, as it will not trigger
        //  sceSaveDataGetSaveDataMemory2.
    } else {
        std::filesystem::create_directories(mount_dir);
        std::vector<u8> buf(setupParam->memorySize); // check if > 0x1000000 (16.77mb) or x2?
        Common::FS::IOFile::WriteBytes(mount_dir / "save_mem2.sav", buf);
        std::vector<u8> paramBuf(sizeof(OrbisSaveDataParam));
        Common::FS::IOFile::WriteBytes(mount_dir / "param.txt", paramBuf);
        std::vector<u8> iconBuf(setupParam->iconMemorySize);
        Common::FS::IOFile::WriteBytes(mount_dir / "save_icon.png", iconBuf);
    }
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(OrbisSaveDataMemorySync* syncParam) {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called: option = {}", syncParam->option);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataTerminate() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataTransferringMount() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint) {
    if (std::string(mountPoint->data).empty()) {
        return ORBIS_SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    const auto& mount_dir = Common::FS::GetUserPath(Common::FS::PathType::SaveDataDir) /
                            std::to_string(1) / game_serial / mountPoint->data;
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    mnt->Unmount(mount_dir, mountPoint->data);
    LOG_INFO(Lib_SaveData, "mountPoint = {}", std::string(mountPoint->data));

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataUmountSys() {
    LOG_ERROR(Lib_SaveData, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint) {
    LOG_ERROR(Lib_SaveData, "called = {}", std::string(mountPoint->data));
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string mount_dir = mnt->GetHostFile(mountPoint->data);
    if (!std::filesystem::exists(mount_dir)) {
        return ORBIS_SAVE_DATA_ERROR_NOT_FOUND;
    } else {
        std::filesystem::path mnt_dir(mount_dir);
        std::filesystem::create_directories(mnt_dir.parent_path() / "backup");

        for (const auto& entry : std::filesystem::recursive_directory_iterator(mnt_dir)) {
            const auto& path = entry.path();
            std::filesystem::path target_path = mnt_dir.parent_path() / "backup";

            if (std::filesystem::is_regular_file(path)) {
                std::filesystem::copy(path, target_path,
                                      std::filesystem::copy_options::overwrite_existing);
            }
        }
        mnt->Unmount(mount_dir, mountPoint->data);
    }
    return ORBIS_OK;
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
