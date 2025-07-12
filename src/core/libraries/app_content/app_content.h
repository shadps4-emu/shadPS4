// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AppContent {

constexpr int ORBIS_APP_CONTENT_APPPARAM_ID_SKU_FLAG = 0;
constexpr int ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_1 = 1;
constexpr int ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_2 = 2;
constexpr int ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_3 = 3;
constexpr int ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_4 = 4;

constexpr int ORBIS_APP_CONTENT_APPPARAM_SKU_FLAG_TRIAL = 1;
constexpr int ORBIS_APP_CONTENT_APPPARAM_SKU_FLAG_FULL = 3;

struct OrbisAppContentInitParam {
    char reserved[32];
};

struct OrbisAppContentBootParam {
    char reserved1[4];
    u32 attr;
    char reserved2[32];
};

using OrbisAppContentTemporaryDataOption = u32;

constexpr int ORBIS_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE = 16;

struct OrbisAppContentMountPoint {
    char data[ORBIS_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE];
};

constexpr int ORBIS_APP_CONTENT_TEMPORARY_DATA_OPTION_NONE = 0;
constexpr int ORBIS_APP_CONTENT_TEMPORARY_DATA_OPTION_FORMAT = (1 << 0);
constexpr int ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE = 17;
constexpr int ORBIS_APP_CONTENT_ENTITLEMENT_KEY_SIZE = 16;
constexpr int ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE = 2500;

enum class OrbisAppContentAddcontDownloadStatus : u32 {
    NoExtraData = 0,
    NoInQueue = 1,
    Downloading = 2,
    DownloadSuspended = 3,
    Installed = 4
};

struct OrbisNpUnifiedEntitlementLabel {
    char data[ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE];
    char padding[3];
};

using OrbisAppContentAppParamId = u32;

struct OrbisAppContentAddcontInfo {
    OrbisNpUnifiedEntitlementLabel entitlement_label;
    OrbisAppContentAddcontDownloadStatus status;
};

struct OrbisAppContentGetEntitlementKey {
    char data[ORBIS_APP_CONTENT_ENTITLEMENT_KEY_SIZE];
};

int PS4_SYSV_ABI _Z5dummyv();
int PS4_SYSV_ABI sceAppContentAddcontDelete();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownload();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadSp();
int PS4_SYSV_ABI sceAppContentAddcontMount(u32 service_label,
                                           const OrbisNpUnifiedEntitlementLabel* entitlement_label,
                                           OrbisAppContentMountPoint* mount_point);
int PS4_SYSV_ABI sceAppContentAddcontShrink();
int PS4_SYSV_ABI sceAppContentAddcontUnmount();
int PS4_SYSV_ABI sceAppContentAppParamGetInt(OrbisAppContentAppParamId paramId, s32* value);
int PS4_SYSV_ABI sceAppContentAppParamGetString();
int PS4_SYSV_ABI sceAppContentDownload0Expand();
int PS4_SYSV_ABI sceAppContentDownload0Shrink();
int PS4_SYSV_ABI sceAppContentDownload1Expand();
int PS4_SYSV_ABI sceAppContentDownload1Shrink();
int PS4_SYSV_ABI sceAppContentDownloadDataFormat();
int PS4_SYSV_ABI sceAppContentDownloadDataGetAvailableSpaceKb(OrbisAppContentMountPoint* mountPoint,
                                                              u64* availableSpaceKb);
int PS4_SYSV_ABI sceAppContentGetAddcontDownloadProgress();
int PS4_SYSV_ABI sceAppContentGetAddcontInfo(u32 service_label,
                                             const OrbisNpUnifiedEntitlementLabel* entitlementLabel,
                                             OrbisAppContentAddcontInfo* info);
int PS4_SYSV_ABI sceAppContentGetAddcontInfoList(u32 service_label,
                                                 OrbisAppContentAddcontInfo* list, u32 list_num,
                                                 u32* hit_num);
int PS4_SYSV_ABI sceAppContentGetEntitlementKey(
    u32 service_label, const OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisAppContentGetEntitlementKey* key);
int PS4_SYSV_ABI sceAppContentGetRegion();
int PS4_SYSV_ABI sceAppContentInitialize(const OrbisAppContentInitParam* initParam,
                                         OrbisAppContentBootParam* bootParam);
int PS4_SYSV_ABI sceAppContentRequestPatchInstall();
int PS4_SYSV_ABI sceAppContentSmallSharedDataFormat();
int PS4_SYSV_ABI sceAppContentSmallSharedDataGetAvailableSpaceKb();
int PS4_SYSV_ABI sceAppContentSmallSharedDataMount();
int PS4_SYSV_ABI sceAppContentSmallSharedDataUnmount();
int PS4_SYSV_ABI sceAppContentTemporaryDataFormat();
int PS4_SYSV_ABI sceAppContentTemporaryDataGetAvailableSpaceKb(
    const OrbisAppContentMountPoint* mountPoint, u64* availableSpaceKb);
int PS4_SYSV_ABI sceAppContentTemporaryDataMount();
int PS4_SYSV_ABI sceAppContentTemporaryDataMount2(OrbisAppContentTemporaryDataOption option,
                                                  OrbisAppContentMountPoint* mountPoint);
int PS4_SYSV_ABI sceAppContentTemporaryDataUnmount();
int PS4_SYSV_ABI sceAppContentGetPftFlag();
int PS4_SYSV_ABI Func_C59A36FF8D7C59DA();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadByEntitlementId();
int PS4_SYSV_ABI sceAppContentAddcontMountByEntitlementId();
int PS4_SYSV_ABI sceAppContentGetAddcontInfoByEntitlementId();
int PS4_SYSV_ABI sceAppContentGetAddcontInfoListByIroTag();
int PS4_SYSV_ABI sceAppContentGetDownloadedStoreCountry();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AppContent
