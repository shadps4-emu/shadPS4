// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

class PSF;

namespace Libraries::SaveData {

constexpr size_t ORBIS_SAVE_DATA_TITLE_MAXSIZE = 128;
constexpr size_t ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE = 128;
constexpr size_t ORBIS_SAVE_DATA_DETAIL_MAXSIZE = 1024;
constexpr int ORBIS_SAVE_DATA_TITLE_ID_DATA_SIZE = 10;
constexpr int ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE = 32;
constexpr int ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE = 65;
constexpr int ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE = 16;

constexpr size_t OrbisSaveDataTitleMaxsize = ORBIS_SAVE_DATA_TITLE_MAXSIZE;
constexpr size_t OrbisSaveDataSubtitleMaxsize = ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE;
constexpr size_t OrbisSaveDataDetailMaxsize = ORBIS_SAVE_DATA_DETAIL_MAXSIZE;
constexpr int OrbisSaveDataTitleIdDataSize = ORBIS_SAVE_DATA_TITLE_ID_DATA_SIZE;
constexpr int OrbisSaveDataDirnameDataMaxsize = ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE;
constexpr int OrbisSaveDataFingerprintDataSize = ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE;
constexpr int OrbisSaveDataMountPointDataMaxsize = ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE;

constexpr u64 ORBIS_SAVE_DATA_BLOCK_SIZE = 32768;
constexpr u64 OrbisSaveDataBlockSize = ORBIS_SAVE_DATA_BLOCK_SIZE;
constexpr u64 OrbisSaveDataBlocksMin2 = 96;
constexpr u64 OrbisSaveDataBlocksMax = 32768;

constexpr u32 OrbisSaveDataMaxMounts = 15;
constexpr u32 OrbisSaveDataMemoryMaxTotalSize = 0x2000000; // 32 MiB
constexpr u32 OrbisSaveDataMemorySmallMaxSize = 0x800000;  // 8 MiB
constexpr u32 OrbisSaveDataMemorySmallHalfMax = 0x400000;  // 4 MiB
constexpr u32 OrbisSaveDataMemoryLargeHalfMax = 0x1000000; // 16 MiB
constexpr u32 OrbisSaveDataMountMaxPathDataSize = 1024;
constexpr u32 OrbisSaveDataMemorySlotCount = 4;

enum class OrbisSaveDataMountMode : u32 {
    RDONLY = 0x00000001,
    RDWR = 0x00000002,
    CREATE = 0x00000004,
    DESTRUCTIVE = 0x00000008,
    COPY_ICON = 0x00000010,
    CREATE2 = 0x00000020,
};

enum class OrbisSaveDataMountStatus : u32 {
    EXISTING = 0x00000000,
    CREATED = 0x00000001,
};

enum class OrbisSaveDataParamType : u32 {
    ALL = 0,
    TITLE = 1,
    SUB_TITLE = 2,
    DETAIL = 3,
    USER_PARAM = 4,
    MTIME = 5,
};

struct OrbisSaveDataTitleId {
    char data[ORBIS_SAVE_DATA_TITLE_ID_DATA_SIZE];
    char _pad[6];
};
static_assert(sizeof(OrbisSaveDataTitleId) == 16);

struct OrbisSaveDataDirName {
    char data[ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE];
};
static_assert(sizeof(OrbisSaveDataDirName) == 32);

struct OrbisSaveDataFingerprint {
    char data[ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE];
    char padding[15];
};
static_assert(sizeof(OrbisSaveDataFingerprint) == 80);

struct OrbisSaveDataMountPoint {
    char data[ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE];
};
static_assert(sizeof(OrbisSaveDataMountPoint) == 16);

struct OrbisSaveDataParam {
    char title[ORBIS_SAVE_DATA_TITLE_MAXSIZE];
    char subTitle[ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE];
    char detail[ORBIS_SAVE_DATA_DETAIL_MAXSIZE];
    u32 userParam;
    int : 32;
    time_t mtime;
    u8 reserved[32];
};
static_assert(sizeof(OrbisSaveDataParam) == 0x530);

struct OrbisSaveDataMount {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u64 blocks;
    u32 mountMode;
    u8 reserved[32];
};

struct OrbisSaveDataMount2 {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataDirName* dirName;
    u64 blocks;
    u32 mountMode;
    u8 reserved[32];
};

struct OrbisSaveDataTransferringMount {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u8 reserved[32];
};

struct OrbisSaveDataMount5 {
    const char* path;
    const OrbisSaveDataFingerprint* fingerprint;
    u64 blocks;
    u32 mountMode;
    u8 reserved[32];
};

struct OrbisSaveDataMountInternal {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u64 blocks;
    u32 mountMode;
    u8 reserved[32];
};

struct OrbisSaveDataMountResult {
    OrbisSaveDataMountPoint mountPoint;
    u64 requiredBlocks;
    u32 mountStatus;
    u8 reserved[28];
};

struct OrbisSaveDataMountInfo {
    u64 blocks;
    u64 freeBlocks;
    u8 reserved[32];
};

struct OrbisSaveDataSearchInfo {
    u64 blocks;
    u64 freeBlocks;
    u8 reserved[32];
};

struct OrbisSaveDataDelete {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u8 reserved[32];
};

struct OrbisSaveDataDelete5 {
    const char* path;
    u8 reserved[32];
};

struct OrbisSaveDataDeleteUser {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u8 reserved[32];
};

struct OrbisSaveDataCopy5 {
    const char* srcPath;
    const char* dstPath;
    u8 reserved[32];
};

struct OrbisSaveDataPromote5 {
    const char* srcPath;
    const char* dstPath;
    u8 reserved[32];
};

struct OrbisSaveDataDirNameSearchCond {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 reserved;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u32 key;
    u32 order;
    u8 reserved2[32];
};

struct OrbisSaveDataDirNameSearchResult {
    u32 hitNum;
    u32 reserved;
    OrbisSaveDataDirName* dirNames;
    u32 dirNamesNum;
    u32 setNum;
    OrbisSaveDataParam* params;
    OrbisSaveDataSearchInfo* infos;
    u8 reserved2[12];
};

struct OrbisSaveDataIcon {
    void* buf;
    size_t bufSize;
    size_t dataSize;
    u8 reserved[32];
};

struct OrbisSaveDataBackup {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u8 reserved[32];
};

struct OrbisSaveDataRestoreBackupData {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u32 option;
    u8 reserved[28];
};

struct OrbisSaveDataRestoreBackupDataForCdlg {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u64 blocks;
    u32 mountMode;
    u8 reserved[20];
    u32 option;
    u8 reserved2[4];
};

struct OrbisSaveDataCheckBackupData {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u8 reserved[32];
};

struct OrbisSaveDataCheckBackupDataInternal {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u8 reserved[32];
};

struct OrbisSaveDataCheckBackupDataForCdlg {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u8 reserved[32];
};

struct OrbisSaveDataMemorySetup2 {
    u32 option;
    Libraries::UserService::OrbisUserServiceUserId userId;
    size_t memorySize;
    size_t iconMemorySize;
    const OrbisSaveDataParam* initParam;
    const OrbisSaveDataIcon* initIcon;
    u32 slotId;
    u8 reserved[20];
};

struct OrbisSaveDataMemorySetupResult {
    size_t existedMemorySize;
    u8 reserved[16];
};

struct OrbisSaveDataMemoryData {
    void* buf;
    size_t bufSize;
    s64 offset;
    u8 reserved[40];
};

struct OrbisSaveDataMemoryGet2 {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u8 reserved[4];
    OrbisSaveDataMemoryData* data;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u32 slotId;
    u8 reserved2[28];
};

struct OrbisSaveDataMemorySet2 {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u8 reserved[4];
    const OrbisSaveDataMemoryData* data;
    const OrbisSaveDataParam* param;
    const OrbisSaveDataIcon* icon;
    u32 slotId;
    u8 reserved2[28];
};

struct OrbisSaveDataMemorySync {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 slotId;
    u32 option;
    u8 reserved[28];
};

struct OrbisSaveDataRestoreLoadMemory {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 slotId;
    u32 option;
    u8 reserved[28];
};

struct OrbisSaveDataEvent {
    s32 type;
    s32 errorCode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    u8 reserved[4];
    OrbisSaveDataTitleId titleId;
    u8 pad[7];
    OrbisSaveDataDirName dirName;
    u8 reserved2[40];
};

struct OrbisSaveDataEventParam {
    OrbisSaveDataTitleId titleId;
    u8 pad[7];
    OrbisSaveDataDirName dirName;
    Libraries::UserService::OrbisUserServiceUserId userId;
    s32 type;
    u8 reserved[32];
};

struct OrbisSaveDataEventInfo {
    u32 infoType;
    Libraries::UserService::OrbisUserServiceUserId userId;
    OrbisSaveDataTitleId titleId;
    OrbisSaveDataDirName dirName;
    u8 reserved[64];
};

struct OrbisSaveDataEventResult {
    s32 type;
    s32 errorCode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    u8 pad[4];
    OrbisSaveDataTitleId titleId;
    u8 pad2[7];
    OrbisSaveDataDirName dirName;
    u8 reserved[32];
};

struct OrbisSaveDataInitParams3 {
    u64 size;
    u32 option;
    u32 reserved;
};

struct OrbisSaveDataChangeInternal {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    const OrbisSaveDataFingerprint* fingerprint;
    u32 changeType;
    u8 reserved[28];
};

struct OrbisSaveDataCheckSaveDataBroken {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u8 reserved[32];
};

struct OrbisSaveDataBrokenResult {
    u32 broken;
    u8 reserved[28];
};

struct OrbisSaveDataCheckVersion {
    Libraries::UserService::OrbisUserServiceUserId userId;
    u32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u8 reserved[32];
};

using OrbisSaveDataEventCallback = PS4_SYSV_ABI void (*)(OrbisSaveDataEvent* event, void* userdata);
using OrbisSaveDataEventCallbackFunc = OrbisSaveDataEventCallback;

s32 PS4_SYSV_ABI sceSaveDataInitialize(const void* params);
s32 PS4_SYSV_ABI sceSaveDataInitialize2(const void* params);
s32 PS4_SYSV_ABI sceSaveDataInitialize3(const OrbisSaveDataInitParams3* params);
s32 PS4_SYSV_ABI sceSaveDataInitializeForCdlg(const void* params);
s32 PS4_SYSV_ABI sceSaveDataTerminate();
s32 PS4_SYSV_ABI sceSaveDataShutdownStart();

s32 PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                  OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                   OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataTransferringMount(const OrbisSaveDataTransferringMount* mount,
                                              OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataMount5(const OrbisSaveDataMount5* mount,
                                   OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataMountInternal(const OrbisSaveDataMountInternal* mount,
                                          OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint);
s32 PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint);

s32 PS4_SYSV_ABI sceSaveDataBackup(const OrbisSaveDataBackup* backup);
s32 PS4_SYSV_ABI sceSaveDataRestoreBackupData(const OrbisSaveDataRestoreBackupData* restore);
s32 PS4_SYSV_ABI
sceSaveDataRestoreBackupDataForCdlg(const OrbisSaveDataRestoreBackupDataForCdlg* restore);
s32 PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check);
s32 PS4_SYSV_ABI
sceSaveDataCheckBackupDataInternal(const OrbisSaveDataCheckBackupDataInternal* check);
s32 PS4_SYSV_ABI
sceSaveDataCheckBackupDataForCdlg(const OrbisSaveDataCheckBackupDataForCdlg* check);
s32 PS4_SYSV_ABI sceSaveDataChangeInternal(const OrbisSaveDataChangeInternal* change);

s32 PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                         OrbisSaveDataMountInfo* info);
s32 PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del);
s32 PS4_SYSV_ABI sceSaveDataDelete5(const OrbisSaveDataDelete5* del);
s32 PS4_SYSV_ABI sceSaveDataDeleteUser(const OrbisSaveDataDeleteUser* del);
s32 PS4_SYSV_ABI sceSaveDataDeleteAllUser();
s32 PS4_SYSV_ABI sceSaveDataCopy5(const OrbisSaveDataCopy5* copy);
s32 PS4_SYSV_ABI sceSaveDataPromote5(const OrbisSaveDataPromote5* promote);
s32 PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
                                          OrbisSaveDataDirNameSearchResult* result);
s32 PS4_SYSV_ABI sceSaveDataDirNameSearchInternal(const OrbisSaveDataDirNameSearchCond* cond,
                                                  OrbisSaveDataDirNameSearchResult* result);

s32 PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint, u32 paramType,
                                     const void* paramBuf, size_t paramBufSize);
s32 PS4_SYSV_ABI sceSaveDataGetParam(const OrbisSaveDataMountPoint* mountPoint, u32 paramType,
                                     void* paramBuf, size_t paramBufSize, size_t* gotSize);
s32 PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     const OrbisSaveDataIcon* icon);
s32 PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataIcon* icon);

s32 PS4_SYSV_ABI sceSaveDataRegisterEventCallback(OrbisSaveDataEventCallback cb, void* userdata);
s32 PS4_SYSV_ABI sceSaveDataUnregisterEventCallback();
s32 PS4_SYSV_ABI sceSaveDataRebuildDatabase();
s32 PS4_SYSV_ABI sceSaveDataChangeDatabase(Libraries::UserService::OrbisUserServiceUserId userId,
                                           u32 param);
s32 PS4_SYSV_ABI sceSaveDataAbort();

s32 PS4_SYSV_ABI sceSaveDataMountSys(Libraries::UserService::OrbisUserServiceUserId userId,
                                     s32 mountMode, OrbisSaveDataMountResult* mountResult);
s32 PS4_SYSV_ABI sceSaveDataUmountSys(const OrbisSaveDataMountPoint* mountPoint);
s32 PS4_SYSV_ABI sceSaveDataGetSavePoint();
s32 PS4_SYSV_ABI sceSaveDataGetDataBaseFilePath(s32 savePoint, char* buf, size_t bufSize);
s32 PS4_SYSV_ABI
sceSaveDataGetSaveDataRootPath(s32 savePoint, Libraries::UserService::OrbisUserServiceUserId userId,
                               char* buf, size_t bufSize);
s32 PS4_SYSV_ABI sceSaveDataGetSaveDataRootUsbPath(s32 savePoint, u64 accountId, char* buf,
                                                   size_t bufSize);
s32 PS4_SYSV_ABI
sceSaveDataGetSaveDataRootDir(s32 savePoint, Libraries::UserService::OrbisUserServiceUserId userId,
                              char* buf, size_t bufSize);

s32 PS4_SYSV_ABI sceSaveDataIsMounted(Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount(u32* count);
s32 PS4_SYSV_ABI
sceSaveDataGetUpdatedDataCount(Libraries::UserService::OrbisUserServiceUserId userId, u32* count);
s32 PS4_SYSV_ABI sceSaveDataBindPsnAccount(Libraries::UserService::OrbisUserServiceUserId userId,
                                           u64 accountId);
s32 PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup(
    Libraries::UserService::OrbisUserServiceUserId userId, u64 accountId);
s32 PS4_SYSV_ABI sceSaveDataGetBoundPsnAccountCount(s32* count);
s32 PS4_SYSV_ABI sceSaveDataGetAllSize(u64* size);

s32 PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(
    Libraries::UserService::OrbisUserServiceUserId userId, size_t memorySize,
    const OrbisSaveDataParam* param, const OrbisSaveDataIcon* icon, u32 value);
s32 PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                 OrbisSaveDataMemorySetupResult* result);
s32 PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(Libraries::UserService::OrbisUserServiceUserId userId,
                                              void* buf, size_t bufSize, s64 offset);
s32 PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(const OrbisSaveDataMemoryGet2* getParam);
s32 PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(Libraries::UserService::OrbisUserServiceUserId userId,
                                              const void* buf, size_t bufSize, s64 offset);
s32 PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam);
s32 PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(const OrbisSaveDataMemorySync* syncParam);
s32 PS4_SYSV_ABI
sceSaveDataRestoreLoadSaveDataMemory(const OrbisSaveDataRestoreLoadMemory* restoreParam);

s32 PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus(s32 flag);
s32 PS4_SYSV_ABI sceSaveDataGetFormat();
s32 PS4_SYSV_ABI
sceSaveDataSetSaveDataLibraryUser(Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceSaveDataGetSaveDataCount(Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceSaveDataGetProgress(float* progress);
s32 PS4_SYSV_ABI sceSaveDataClearProgress();
s32 PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam* eventParam,
                                           OrbisSaveDataEventResult* eventResult);
s32 PS4_SYSV_ABI sceSaveDataSetEventInfo(const OrbisSaveDataEventInfo* info);
s32 PS4_SYSV_ABI sceSaveDataGetEventInfo(u32 infoType, OrbisSaveDataEventInfo* info);
s32 PS4_SYSV_ABI
sceSaveDataGetAppLaunchedUser(Libraries::UserService::OrbisUserServiceUserId userId,
                              Libraries::UserService::OrbisUserServiceUserId* launchedUser);
s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataBroken(const OrbisSaveDataCheckSaveDataBroken* check,
                                                OrbisSaveDataBrokenResult* result);
s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataVersion(const OrbisSaveDataCheckVersion* check);
s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataVersionLatest(const OrbisSaveDataCheckVersion* check);

s32 PS4_SYSV_ABI sceSaveDataCreateUploadData(const void*);
s32 PS4_SYSV_ABI sceSaveDataSyncCloudList(const void*);
s32 PS4_SYSV_ABI sceSaveDataGetCloudQuotaInfo(const void*, void*);
s32 PS4_SYSV_ABI sceSaveDataUpload(const void*);
s32 PS4_SYSV_ABI sceSaveDataDownload(const void*);
s32 PS4_SYSV_ABI sceSaveDataDeleteCloudData(const void*);
s32 PS4_SYSV_ABI sceSaveDataCheckCloudData(const void*, void*);
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadSetting(const void*, void*);
s32 PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting(const void*);
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadRequestInfo(const void*, void*);
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadConditions(const void*, void*);

s32 PS4_SYSV_ABI Func_02E4C4D201716422();
s32 PS4_SYSV_ABI sceSaveDataDebugCleanMount();
s32 PS4_SYSV_ABI sceSaveDataDebugCreateSaveDataRoot();
s32 PS4_SYSV_ABI sceSaveDataDebugRemoveSaveDataRoot();
s32 PS4_SYSV_ABI sceSaveDataDebugTarget();
s32 PS4_SYSV_ABI sceSaveDataDebugCompiledSdkVersion();
s32 PS4_SYSV_ABI sceSaveDataGetClientThreadPriority(u32* priority);
s32 PS4_SYSV_ABI sceSaveDataDebugGetThreadId();
s32 PS4_SYSV_ABI sceSaveDataDebug();
s32 PS4_SYSV_ABI sceSaveDataCheckIpmiIfSize();
s32 PS4_SYSV_ABI sceSaveDataIsDeletingUsbDb();
s32 PS4_SYSV_ABI Func_C09EA88AEF93A8AF(u64* requiredBlocks);

// Test hooks for isolated unit testing without booting a full ELF.
void SetTestEnvironment(std::string_view title_id, u32 sdk_ver);
void ResetForTest();

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::SaveData
