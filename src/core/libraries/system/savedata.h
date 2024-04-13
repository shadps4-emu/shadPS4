// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SaveData {

constexpr int ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE =
    32; // Maximum size for a save data directory name
constexpr int ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE = 16; // Maximum size for a mount point name

struct OrbisSaveDataDirName {
    char data[ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE];
};

struct OrbisSaveDataMount2 {
    s32 user_id;
    s32 unk1;
    const OrbisSaveDataDirName* dir_name;
    u64 blocks;
    u32 mount_mode;
    u8 reserved[32];
    s32 unk2;
};

struct OrbisSaveDataMountPoint {
    char data[ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE];
};

struct OrbisSaveDataMountResult {
    OrbisSaveDataMountPoint mount_point;
    u64 required_blocks;
    u32 unused;
    u32 mount_status;
    u8 reserved[28];
    s32 unk1;
};

int PS4_SYSV_ABI sceSaveDataAbort();
int PS4_SYSV_ABI sceSaveDataBackup();
int PS4_SYSV_ABI sceSaveDataBindPsnAccount();
int PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup();
int PS4_SYSV_ABI sceSaveDataChangeDatabase();
int PS4_SYSV_ABI sceSaveDataChangeInternal();
int PS4_SYSV_ABI sceSaveDataCheckBackupData();
int PS4_SYSV_ABI sceSaveDataCheckBackupDataForCdlg();
int PS4_SYSV_ABI sceSaveDataCheckBackupDataInternal();
int PS4_SYSV_ABI sceSaveDataCheckCloudData();
int PS4_SYSV_ABI sceSaveDataCheckIpmiIfSize();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataBroken();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersion();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersionLatest();
int PS4_SYSV_ABI sceSaveDataClearProgress();
int PS4_SYSV_ABI sceSaveDataCopy5();
int PS4_SYSV_ABI sceSaveDataCreateUploadData();
int PS4_SYSV_ABI sceSaveDataDebug();
int PS4_SYSV_ABI sceSaveDataDebugCleanMount();
int PS4_SYSV_ABI sceSaveDataDebugCompiledSdkVersion();
int PS4_SYSV_ABI sceSaveDataDebugCreateSaveDataRoot();
int PS4_SYSV_ABI sceSaveDataDebugGetThreadId();
int PS4_SYSV_ABI sceSaveDataDebugRemoveSaveDataRoot();
int PS4_SYSV_ABI sceSaveDataDebugTarget();
int PS4_SYSV_ABI sceSaveDataDelete();
int PS4_SYSV_ABI sceSaveDataDelete5();
int PS4_SYSV_ABI sceSaveDataDeleteAllUser();
int PS4_SYSV_ABI sceSaveDataDeleteCloudData();
int PS4_SYSV_ABI sceSaveDataDeleteUser();
int PS4_SYSV_ABI sceSaveDataDirNameSearch();
int PS4_SYSV_ABI sceSaveDataDirNameSearchInternal();
int PS4_SYSV_ABI sceSaveDataDownload();
int PS4_SYSV_ABI sceSaveDataGetAllSize();
int PS4_SYSV_ABI sceSaveDataGetAppLaunchedUser();
int PS4_SYSV_ABI sceSaveDataGetAutoUploadConditions();
int PS4_SYSV_ABI sceSaveDataGetAutoUploadRequestInfo();
int PS4_SYSV_ABI sceSaveDataGetAutoUploadSetting();
int PS4_SYSV_ABI sceSaveDataGetBoundPsnAccountCount();
int PS4_SYSV_ABI sceSaveDataGetClientThreadPriority();
int PS4_SYSV_ABI sceSaveDataGetCloudQuotaInfo();
int PS4_SYSV_ABI sceSaveDataGetDataBaseFilePath();
int PS4_SYSV_ABI sceSaveDataGetEventInfo();
int PS4_SYSV_ABI sceSaveDataGetEventResult();
int PS4_SYSV_ABI sceSaveDataGetFormat();
int PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount();
int PS4_SYSV_ABI sceSaveDataGetMountInfo();
int PS4_SYSV_ABI sceSaveDataGetParam();
int PS4_SYSV_ABI sceSaveDataGetProgress();
int PS4_SYSV_ABI sceSaveDataGetSaveDataCount();
int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory();
int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2();
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootDir();
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootPath();
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootUsbPath();
int PS4_SYSV_ABI sceSaveDataGetSavePoint();
int PS4_SYSV_ABI sceSaveDataGetUpdatedDataCount();
int PS4_SYSV_ABI sceSaveDataInitialize();
int PS4_SYSV_ABI sceSaveDataInitialize2();
int PS4_SYSV_ABI sceSaveDataInitialize3();
int PS4_SYSV_ABI sceSaveDataInitializeForCdlg();
int PS4_SYSV_ABI sceSaveDataIsDeletingUsbDb();
int PS4_SYSV_ABI sceSaveDataIsMounted();
int PS4_SYSV_ABI sceSaveDataLoadIcon();
int PS4_SYSV_ABI sceSaveDataMount();
s32 PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                   OrbisSaveDataMountResult* mount_result);
int PS4_SYSV_ABI sceSaveDataMount5();
int PS4_SYSV_ABI sceSaveDataMountInternal();
int PS4_SYSV_ABI sceSaveDataMountSys();
int PS4_SYSV_ABI sceSaveDataPromote5();
int PS4_SYSV_ABI sceSaveDataRebuildDatabase();
int PS4_SYSV_ABI sceSaveDataRegisterEventCallback();
int PS4_SYSV_ABI sceSaveDataRestoreBackupData();
int PS4_SYSV_ABI sceSaveDataRestoreBackupDataForCdlg();
int PS4_SYSV_ABI sceSaveDataRestoreLoadSaveDataMemory();
int PS4_SYSV_ABI sceSaveDataSaveIcon();
int PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting();
int PS4_SYSV_ABI sceSaveDataSetEventInfo();
int PS4_SYSV_ABI sceSaveDataSetParam();
int PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser();
int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory();
int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2();
int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory();
int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2();
int PS4_SYSV_ABI sceSaveDataShutdownStart();
int PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus();
int PS4_SYSV_ABI sceSaveDataSyncCloudList();
int PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory();
int PS4_SYSV_ABI sceSaveDataTerminate();
int PS4_SYSV_ABI sceSaveDataTransferringMount();
int PS4_SYSV_ABI sceSaveDataUmount();
int PS4_SYSV_ABI sceSaveDataUmountSys();
int PS4_SYSV_ABI sceSaveDataUmountWithBackup();
int PS4_SYSV_ABI sceSaveDataUnregisterEventCallback();
int PS4_SYSV_ABI sceSaveDataUpload();
int PS4_SYSV_ABI Func_02E4C4D201716422();

void RegisterlibSceSaveData(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SaveData
