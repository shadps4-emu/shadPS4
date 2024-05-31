// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AppContent {

struct OrbisAppContentInitParam {
    char reserved[32];
};

struct OrbisAppContentBootParam {
    char reserved1[4];
    u32 attr;
    char reserved2[32];
};

int PS4_SYSV_ABI _Z5dummyv();
int PS4_SYSV_ABI sceAppContentAddcontDelete();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownload();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadSp();
int PS4_SYSV_ABI sceAppContentAddcontMount();
int PS4_SYSV_ABI sceAppContentAddcontShrink();
int PS4_SYSV_ABI sceAppContentAddcontUnmount();
int PS4_SYSV_ABI sceAppContentAppParamGetInt();
int PS4_SYSV_ABI sceAppContentAppParamGetString();
int PS4_SYSV_ABI sceAppContentDownload0Expand();
int PS4_SYSV_ABI sceAppContentDownload0Shrink();
int PS4_SYSV_ABI sceAppContentDownload1Expand();
int PS4_SYSV_ABI sceAppContentDownload1Shrink();
int PS4_SYSV_ABI sceAppContentDownloadDataFormat();
int PS4_SYSV_ABI sceAppContentDownloadDataGetAvailableSpaceKb();
int PS4_SYSV_ABI sceAppContentGetAddcontDownloadProgress();
int PS4_SYSV_ABI sceAppContentGetAddcontInfo();
int PS4_SYSV_ABI sceAppContentGetAddcontInfoList();
int PS4_SYSV_ABI sceAppContentGetEntitlementKey();
int PS4_SYSV_ABI sceAppContentGetRegion();
int PS4_SYSV_ABI sceAppContentInitialize(const OrbisAppContentInitParam* initParam,
                                         OrbisAppContentBootParam* bootParam);
int PS4_SYSV_ABI sceAppContentRequestPatchInstall();
int PS4_SYSV_ABI sceAppContentSmallSharedDataFormat();
int PS4_SYSV_ABI sceAppContentSmallSharedDataGetAvailableSpaceKb();
int PS4_SYSV_ABI sceAppContentSmallSharedDataMount();
int PS4_SYSV_ABI sceAppContentSmallSharedDataUnmount();
int PS4_SYSV_ABI sceAppContentTemporaryDataFormat();
int PS4_SYSV_ABI sceAppContentTemporaryDataGetAvailableSpaceKb();
int PS4_SYSV_ABI sceAppContentTemporaryDataMount();
int PS4_SYSV_ABI sceAppContentTemporaryDataMount2();
int PS4_SYSV_ABI sceAppContentTemporaryDataUnmount();
int PS4_SYSV_ABI sceAppContentGetPftFlag();
int PS4_SYSV_ABI Func_C59A36FF8D7C59DA();
int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadByEntitlemetId();
int PS4_SYSV_ABI sceAppContentAddcontMountByEntitlemetId();
int PS4_SYSV_ABI sceAppContentGetAddcontInfoByEntitlementId();
int PS4_SYSV_ABI sceAppContentGetAddcontInfoListByIroTag();
int PS4_SYSV_ABI sceAppContentGetDownloadedStoreCountry();

void RegisterlibSceAppContent(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::AppContent