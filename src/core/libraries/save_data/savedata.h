// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/cstring.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

class PSF;

namespace Libraries::SaveData {

constexpr size_t OrbisSaveDataTitleMaxsize = 128;    // Maximum title name size
constexpr size_t OrbisSaveDataSubtitleMaxsize = 128; // Maximum subtitle name size
constexpr size_t OrbisSaveDataDetailMaxsize = 1024;  // Maximum detail name size

enum class Error : u32;
enum class OrbisSaveDataParamType : u32;

using OrbisUserServiceUserId = s32;

// Maximum size for a title ID (4 uppercase letters + 5 digits)
constexpr int OrbisSaveDataTitleIdDataSize = 10;
// Maximum save directory name size
constexpr int OrbisSaveDataDirnameDataMaxsize = 32;

struct OrbisSaveDataTitleId {
    Common::CString<OrbisSaveDataTitleIdDataSize> data;
    std::array<char, 6> _pad;
};

struct OrbisSaveDataDirName {
    Common::CString<OrbisSaveDataDirnameDataMaxsize> data;
};

struct OrbisSaveDataParam {
    Common::CString<OrbisSaveDataTitleMaxsize> title;
    Common::CString<OrbisSaveDataSubtitleMaxsize> subTitle;
    Common::CString<OrbisSaveDataDetailMaxsize> detail;
    u32 userParam;
    int : 32;
    time_t mtime;
    std::array<u8, 32> _reserved;

    void FromSFO(const PSF& sfo);

    void ToSFO(PSF& sfo) const;
};

struct OrbisSaveDataBackup;
struct OrbisSaveDataCheckBackupData;
struct OrbisSaveDataDelete;
struct OrbisSaveDataDirNameSearchCond;
struct OrbisSaveDataDirNameSearchResult;
struct OrbisSaveDataEvent;
struct OrbisSaveDataEventParam;
struct OrbisSaveDataIcon;
struct OrbisSaveDataMemoryGet2;
struct OrbisSaveDataMemorySet2;
struct OrbisSaveDataMemorySetup2;
struct OrbisSaveDataMemorySetupResult;
struct OrbisSaveDataMemorySync;
struct OrbisSaveDataMount2;
struct OrbisSaveDataMount;
struct OrbisSaveDataMountInfo;
struct OrbisSaveDataMountPoint;
struct OrbisSaveDataMountResult;
struct OrbisSaveDataRestoreBackupData;
struct OrbisSaveDataTransferringMount;

int PS4_SYSV_ABI sceSaveDataAbort();
Error PS4_SYSV_ABI sceSaveDataBackup(const OrbisSaveDataBackup* backup);
int PS4_SYSV_ABI sceSaveDataBindPsnAccount();
int PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup();
int PS4_SYSV_ABI sceSaveDataChangeDatabase();
int PS4_SYSV_ABI sceSaveDataChangeInternal();
Error PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check);
int PS4_SYSV_ABI sceSaveDataCheckBackupDataForCdlg();
int PS4_SYSV_ABI sceSaveDataCheckBackupDataInternal();
int PS4_SYSV_ABI sceSaveDataCheckCloudData();
int PS4_SYSV_ABI sceSaveDataCheckIpmiIfSize();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataBroken();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersion();
int PS4_SYSV_ABI sceSaveDataCheckSaveDataVersionLatest();
Error PS4_SYSV_ABI sceSaveDataClearProgress();
int PS4_SYSV_ABI sceSaveDataCopy5();
int PS4_SYSV_ABI sceSaveDataCreateUploadData();
int PS4_SYSV_ABI sceSaveDataDebug();
int PS4_SYSV_ABI sceSaveDataDebugCleanMount();
int PS4_SYSV_ABI sceSaveDataDebugCompiledSdkVersion();
int PS4_SYSV_ABI sceSaveDataDebugCreateSaveDataRoot();
int PS4_SYSV_ABI sceSaveDataDebugGetThreadId();
int PS4_SYSV_ABI sceSaveDataDebugRemoveSaveDataRoot();
int PS4_SYSV_ABI sceSaveDataDebugTarget();
Error PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del);
int PS4_SYSV_ABI sceSaveDataDelete5();
int PS4_SYSV_ABI sceSaveDataDeleteAllUser();
int PS4_SYSV_ABI sceSaveDataDeleteCloudData();
int PS4_SYSV_ABI sceSaveDataDeleteUser();
Error PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
                                            OrbisSaveDataDirNameSearchResult* result);
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
Error PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam* eventParam,
                                             OrbisSaveDataEvent* event);
int PS4_SYSV_ABI sceSaveDataGetFormat();
int PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount();
Error PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                           OrbisSaveDataMountInfo* info);
Error PS4_SYSV_ABI sceSaveDataGetParam(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataParamType paramType, void* paramBuf,
                                       size_t paramBufSize, size_t* gotSize);
Error PS4_SYSV_ABI sceSaveDataGetProgress(float* progress);
int PS4_SYSV_ABI sceSaveDataGetSaveDataCount();
Error PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(OrbisUserServiceUserId userId, void* buf,
                                                size_t bufSize, int64_t offset);
Error PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(OrbisSaveDataMemoryGet2* getParam);
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootDir();
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootPath();
int PS4_SYSV_ABI sceSaveDataGetSaveDataRootUsbPath();
int PS4_SYSV_ABI sceSaveDataGetSavePoint();
int PS4_SYSV_ABI sceSaveDataGetUpdatedDataCount();
Error PS4_SYSV_ABI sceSaveDataInitialize(void*);
Error PS4_SYSV_ABI sceSaveDataInitialize2(void*);
Error PS4_SYSV_ABI sceSaveDataInitialize3(void*);
int PS4_SYSV_ABI sceSaveDataInitializeForCdlg();
int PS4_SYSV_ABI sceSaveDataIsDeletingUsbDb();
int PS4_SYSV_ABI sceSaveDataIsMounted();
Error PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataIcon* icon);
Error PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                    OrbisSaveDataMountResult* mount_result);
Error PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                     OrbisSaveDataMountResult* mount_result);
int PS4_SYSV_ABI sceSaveDataMount5();
int PS4_SYSV_ABI sceSaveDataMountInternal();
int PS4_SYSV_ABI sceSaveDataMountSys();
int PS4_SYSV_ABI sceSaveDataPromote5();
int PS4_SYSV_ABI sceSaveDataRebuildDatabase();
int PS4_SYSV_ABI sceSaveDataRegisterEventCallback();
Error PS4_SYSV_ABI sceSaveDataRestoreBackupData(const OrbisSaveDataRestoreBackupData* restore);
int PS4_SYSV_ABI sceSaveDataRestoreBackupDataForCdlg();
int PS4_SYSV_ABI sceSaveDataRestoreLoadSaveDataMemory();
Error PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                       const OrbisSaveDataIcon* icon);
int PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting();
int PS4_SYSV_ABI sceSaveDataSetEventInfo();
Error PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint,
                                       OrbisSaveDataParamType paramType, const void* paramBuf,
                                       size_t paramBufSize);
int PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser();
Error PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(OrbisUserServiceUserId userId, void* buf,
                                                size_t bufSize, int64_t offset);
Error PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam);
Error PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(OrbisUserServiceUserId userId, size_t memorySize,
                                                  OrbisSaveDataParam* param);
Error PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                   OrbisSaveDataMemorySetupResult* result);
int PS4_SYSV_ABI sceSaveDataShutdownStart();
int PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus();
int PS4_SYSV_ABI sceSaveDataSyncCloudList();
Error PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(OrbisSaveDataMemorySync* syncParam);
Error PS4_SYSV_ABI sceSaveDataTerminate();
Error PS4_SYSV_ABI sceSaveDataTransferringMount(const OrbisSaveDataTransferringMount* mount,
                                                OrbisSaveDataMountResult* mountResult);
Error PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint);
int PS4_SYSV_ABI sceSaveDataUmountSys();
Error PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint);
int PS4_SYSV_ABI sceSaveDataUnregisterEventCallback();
int PS4_SYSV_ABI sceSaveDataUpload();
int PS4_SYSV_ABI Func_02E4C4D201716422();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SaveData
