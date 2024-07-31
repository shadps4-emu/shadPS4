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

constexpr int ORBIS_SAVE_DATA_TITLE_ID_DATA_SIZE = 10;
struct OrbisSaveDataTitleId {
    char data[ORBIS_SAVE_DATA_TITLE_ID_DATA_SIZE];
    char padding[6];
};

constexpr int ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE = 65;
struct OrbisSaveDataFingerprint {
    char data[ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE];
    char padding[15];
};

struct OrbisSaveDataMount {
    s32 user_id;
    s32 pad;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dir_name;
    const OrbisSaveDataFingerprint* fingerprint;
    u64 blocks;
    u32 mount_mode;
    u8 reserved[32];
};

typedef u32 OrbisSaveDataParamType;

constexpr int ORBIS_SAVE_DATA_TITLE_MAXSIZE = 128;
constexpr int ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE = 128;
constexpr int ORBIS_SAVE_DATA_DETAIL_MAXSIZE = 1024;
struct OrbisSaveDataParam {
    char title[ORBIS_SAVE_DATA_TITLE_MAXSIZE];
    char subTitle[ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE];
    char detail[ORBIS_SAVE_DATA_DETAIL_MAXSIZE];
    u32 userParam;
    int : 32;
    time_t mtime;
    u8 reserved[32];
};

struct OrbisSaveDataIcon {
    void* buf;
    size_t bufSize;
    size_t dataSize;
    u8 reserved[32];
};

typedef u32 OrbisSaveDataSaveDataMemoryOption;
#define ORBIS_SAVE_DATA_MEMORY_OPTION_NONE (0x00000000)
#define ORBIS_SAVE_DATA_MEMORY_OPTION_SET_PARAM (0x00000001 << 0)
#define ORBIS_SAVE_DATA_MEMORY_OPTION_DOUBLE_BUFFER (0x00000001 << 1)

struct OrbisSaveDataMemorySetup2 {
    OrbisSaveDataSaveDataMemoryOption option;
    s32 userId;
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

typedef u32 OrbisSaveDataEventType;
#define SCE_SAVE_DATA_EVENT_TYPE_INVALID (0)
#define SCE_SAVE_DATA_EVENT_TYPE_UMOUNT_BACKUP_END (1)
#define SCE_SAVE_DATA_EVENT_TYPE_BACKUP_END (2)
#define SCE_SAVE_DATA_EVENT_TYPE_SAVE_DATA_MEMORY_SYNC_END (3)

struct OrbisSaveDataEvent {
    OrbisSaveDataEventType type;
    s32 errorCode;
    s32 userId;
    u8 padding[4];
    OrbisSaveDataTitleId titleId;
    OrbisSaveDataDirName dirName;
    u8 reserved[40];
};

struct OrbisSaveDataMemoryData {
    void* buf;
    size_t bufSize;
    off_t offset;
    u8 reserved[40];
};

struct OrbisSaveDataMemoryGet2 {
    s32 userId;
    u8 padding[4];
    OrbisSaveDataMemoryData* data;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u32 slotId;
    u8 reserved[28];
};

struct OrbisSaveDataMemorySet2 {
    s32 userId;
    u8 padding[4];
    const OrbisSaveDataMemoryData* data;
    const OrbisSaveDataParam* param;
    const OrbisSaveDataIcon* icon;
    u32 dataNum;
    u8 slotId;
    u8 reserved[24];
};

struct OrbisSaveDataCheckBackupData {
    s32 userId;
    int : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataParam* param;
    OrbisSaveDataIcon* icon;
    u8 reserved[32];
};

struct OrbisSaveDataMountInfo {
    u64 blocks;
    u64 freeBlocks;
    u8 reserved[32];
};

#define ORBIS_SAVE_DATA_BLOCK_SIZE (32768)
#define ORBIS_SAVE_DATA_BLOCKS_MIN2 (96)
#define ORBIS_SAVE_DATA_BLOCKS_MAX (32768)

// savedataMount2 mountModes (ORed values)
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY = 1;
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_RDWR = 2;
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_CREATE = 4;
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF = 8;
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_COPY_ICON = 16;
constexpr int ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2 = 32;
typedef struct _OrbisSaveDataEventParam OrbisSaveDataEventParam;

typedef u32 OrbisSaveDataSortKey;
#define ORBIS_SAVE_DATA_SORT_KEY_DIRNAME (0)
#define ORBIS_SAVE_DATA_SORT_KEY_USER_PARAM (1)
#define ORBIS_SAVE_DATA_SORT_KEY_BLOCKS (2)
#define ORBIS_SAVE_DATA_SORT_KEY_MTIME (3)
#define ORBIS_SAVE_DATA_SORT_KEY_FREE_BLOCKS (5)

typedef u32 OrbisSaveDataSortOrder;
#define ORBIS_SAVE_DATA_SORT_ORDER_ASCENT (0)
#define ORBIS_SAVE_DATA_SORT_ORDER_DESCENT (1)

struct OrbisSaveDataDirNameSearchCond {
    s32 userId;
    int : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataSortKey key;
    OrbisSaveDataSortOrder order;
    u8 reserved[32];
};

struct OrbisSaveDataSearchInfo {
    u64 blocks;
    u64 freeBlocks;
    u8 reserved[32];
};

struct OrbisSaveDataDirNameSearchResult {
    u32 hitNum;
    int : 32;
    OrbisSaveDataDirName* dirNames;
    u32 dirNamesNum;
    u32 setNum;
    OrbisSaveDataParam* params;
    OrbisSaveDataSearchInfo* infos;
    u8 reserved[12];
    int : 32;
};

struct OrbisSaveDataDelete {
    s32 userId;
    int : 32;
    const OrbisSaveDataTitleId* titleId;
    const OrbisSaveDataDirName* dirName;
    u32 unused;
    u8 reserved[32];
    int : 32;
};

typedef u32 OrbisSaveDataMemorySyncOption;

#define SCE_SAVE_DATA_MEMORY_SYNC_OPTION_NONE (0x00000000)
#define SCE_SAVE_DATA_MEMORY_SYNC_OPTION_BLOCKING (0x00000001 << 0)

struct OrbisSaveDataMemorySync {
    s32 userId;
    u32 slotId;
    OrbisSaveDataMemorySyncOption option;
    u8 reserved[28];
};

constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_ALL = 0;
constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_TITLE = 1;
constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_SUB_TITLE = 2;
constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_DETAIL = 3;
constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_USER_PARAM = 4;
constexpr int ORBIS_SAVE_DATA_PARAM_TYPE_MTIME = 5;

int PS4_SYSV_ABI sceSaveDataAbort();
int PS4_SYSV_ABI sceSaveDataBackup();
int PS4_SYSV_ABI sceSaveDataBindPsnAccount();
int PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup();
int PS4_SYSV_ABI sceSaveDataChangeDatabase();
int PS4_SYSV_ABI sceSaveDataChangeInternal();
int PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check);
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
int PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del);
int PS4_SYSV_ABI sceSaveDataDelete5();
int PS4_SYSV_ABI sceSaveDataDeleteAllUser();
int PS4_SYSV_ABI sceSaveDataDeleteCloudData();
int PS4_SYSV_ABI sceSaveDataDeleteUser();
int PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
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
int PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam* eventParam,
                                           OrbisSaveDataEvent* event);
int PS4_SYSV_ABI sceSaveDataGetFormat();
int PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount();
int PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                         OrbisSaveDataMountInfo* info);
int PS4_SYSV_ABI sceSaveDataGetParam(const OrbisSaveDataMountPoint* mountPoint,
                                     const OrbisSaveDataParamType paramType, void* paramBuf,
                                     const size_t paramBufSize, size_t* gotSize);
int PS4_SYSV_ABI sceSaveDataGetProgress();
int PS4_SYSV_ABI sceSaveDataGetSaveDataCount();
int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(const u32 userId, void* buf, const size_t bufSize,
                                              const int64_t offset);
int PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(OrbisSaveDataMemoryGet2* getParam);
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
int PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataIcon* icon);
int PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                  OrbisSaveDataMountResult* mount_result);
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
int PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     const OrbisSaveDataIcon* icon);
int PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting();
int PS4_SYSV_ABI sceSaveDataSetEventInfo();
int PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataParamType paramType, const void* paramBuf,
                                     size_t paramBufSize);
int PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser();
int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(const u32 userId, const void* buf,
                                              const size_t bufSize, const int64_t offset);
int PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam);
int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(u32 userId, size_t memorySize,
                                                OrbisSaveDataParam* param);
int PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                 OrbisSaveDataMemorySetupResult* result);
int PS4_SYSV_ABI sceSaveDataShutdownStart();
int PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus();
int PS4_SYSV_ABI sceSaveDataSyncCloudList();
int PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(OrbisSaveDataMemorySync* syncParam);
int PS4_SYSV_ABI sceSaveDataTerminate();
int PS4_SYSV_ABI sceSaveDataTransferringMount();
int PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint);
int PS4_SYSV_ABI sceSaveDataUmountSys();
int PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint);
int PS4_SYSV_ABI sceSaveDataUnregisterEventCallback();
int PS4_SYSV_ABI sceSaveDataUpload();
int PS4_SYSV_ABI Func_02E4C4D201716422();

void RegisterlibSceSaveData(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SaveData
