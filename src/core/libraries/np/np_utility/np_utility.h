// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpUtility {

s32 PS4_SYSV_ABI sceNpAppInfoIntAbortRequest();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckAvailability();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckAvailabilityA();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckAvailabilityAll();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckAvailabilityAllA();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckServiceAvailability();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckServiceAvailabilityA();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckServiceAvailabilityAll();
s32 PS4_SYSV_ABI sceNpAppInfoIntCheckServiceAvailabilityAllA();
s32 PS4_SYSV_ABI sceNpAppInfoIntCreateRequest();
s32 PS4_SYSV_ABI sceNpAppInfoIntDestroyRequest();
s32 PS4_SYSV_ABI sceNpAppInfoIntFinalize();
s32 PS4_SYSV_ABI sceNpAppInfoIntInitialize();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntAbortRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntCreateRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntDestroyRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntFinalize();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntGetCompatibleTitleIdList();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntGetCompatibleTitleIdNum();
s32 PS4_SYSV_ABI sceNpAppLaunchLink2IntInitialize();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntAbortRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntCreateRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntDestroyRequest();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntFinalize();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntGetCompatibleTitleIdList();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntGetCompatibleTitleIdNum();
s32 PS4_SYSV_ABI sceNpAppLaunchLinkIntInitialize();
s32 PS4_SYSV_ABI sceNpBandwidthTestAbort();
s32 PS4_SYSV_ABI sceNpBandwidthTestDownloadOnlyInitStart();
s32 PS4_SYSV_ABI sceNpBandwidthTestGetStatus();
s32 PS4_SYSV_ABI sceNpBandwidthTestInitStart();
s32 PS4_SYSV_ABI sceNpBandwidthTestInitStartDownload();
s32 PS4_SYSV_ABI sceNpBandwidthTestInitStartUpload();
s32 PS4_SYSV_ABI sceNpBandwidthTestShutdown();
s32 PS4_SYSV_ABI sceNpBandwidthTestShutdownWithDetailedInfo();
s32 PS4_SYSV_ABI sceNpBandwidthTestUploadOnlyInitStart();
s32 PS4_SYSV_ABI sceNpLookupAbortRequest();
s32 PS4_SYSV_ABI sceNpLookupCreateAsyncRequest();
s32 PS4_SYSV_ABI sceNpLookupCreateRequest();
s32 PS4_SYSV_ABI sceNpLookupCreateTitleCtx();
s32 PS4_SYSV_ABI sceNpLookupCreateTitleCtxA();
s32 PS4_SYSV_ABI sceNpLookupDeleteRequest();
s32 PS4_SYSV_ABI sceNpLookupDeleteTitleCtx();
s32 PS4_SYSV_ABI sceNpLookupNetAbortRequest();
s32 PS4_SYSV_ABI sceNpLookupNetCensorComment();
s32 PS4_SYSV_ABI sceNpLookupNetConvertJidToNpId();
s32 PS4_SYSV_ABI sceNpLookupNetConvertNpIdToJid();
s32 PS4_SYSV_ABI sceNpLookupNetCreateRequest();
s32 PS4_SYSV_ABI sceNpLookupNetCreateTitleCtx();
s32 PS4_SYSV_ABI sceNpLookupNetDeleteRequest();
s32 PS4_SYSV_ABI sceNpLookupNetDeleteTitleCtx();
s32 PS4_SYSV_ABI sceNpLookupNetInit();
s32 PS4_SYSV_ABI sceNpLookupNetInitWithFunctionPointer();
s32 PS4_SYSV_ABI sceNpLookupNetInitWithMemoryPool();
s32 PS4_SYSV_ABI sceNpLookupNetIsInit();
s32 PS4_SYSV_ABI sceNpLookupNetNpId();
s32 PS4_SYSV_ABI sceNpLookupNetSanitizeComment();
s32 PS4_SYSV_ABI sceNpLookupNetSetTimeout();
s32 PS4_SYSV_ABI sceNpLookupNetTerm();
s32 PS4_SYSV_ABI sceNpLookupNpId();
s32 PS4_SYSV_ABI sceNpLookupPollAsync();
s32 PS4_SYSV_ABI sceNpLookupSetTimeout();
s32 PS4_SYSV_ABI sceNpLookupWaitAsync();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntAbortRequest();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntCreateRequest();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntDestroyRequest();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntFinalize();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntGetServiceAvailability();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntGetServiceAvailabilityA();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntInitialize();
s32 PS4_SYSV_ABI sceNpServiceChecker2IntIsSetServiceType();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntAbortRequest();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntCreateRequest();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntDestroyRequest();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntFinalize();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntGetAvailability();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntGetAvailabilityList();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntInitialize();
s32 PS4_SYSV_ABI sceNpServiceCheckerIntIsCached();
s32 PS4_SYSV_ABI sceNpTitleMetadataIntAbortRequest();
s32 PS4_SYSV_ABI sceNpTitleMetadataIntCreateRequest();
s32 PS4_SYSV_ABI sceNpTitleMetadataIntDeleteRequest();
s32 PS4_SYSV_ABI sceNpTitleMetadataIntGetInfo();
s32 PS4_SYSV_ABI sceNpTitleMetadataIntGetNpTitleId();
s32 PS4_SYSV_ABI sceNpUtilityInit();
s32 PS4_SYSV_ABI sceNpUtilityTerm();
s32 PS4_SYSV_ABI sceNpWordFilterAbortRequest();
s32 PS4_SYSV_ABI sceNpWordFilterCensorComment();
s32 PS4_SYSV_ABI sceNpWordFilterCreateAsyncRequest();
s32 PS4_SYSV_ABI sceNpWordFilterCreateRequest();
s32 PS4_SYSV_ABI sceNpWordFilterCreateTitleCtx();
s32 PS4_SYSV_ABI sceNpWordFilterCreateTitleCtxA();
s32 PS4_SYSV_ABI sceNpWordFilterDeleteRequest();
s32 PS4_SYSV_ABI sceNpWordFilterDeleteTitleCtx();
s32 PS4_SYSV_ABI sceNpWordFilterPollAsync();
s32 PS4_SYSV_ABI sceNpWordFilterSanitizeComment();
s32 PS4_SYSV_ABI sceNpWordFilterSetTimeout();
s32 PS4_SYSV_ABI sceNpWordFilterWaitAsync();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpUtility