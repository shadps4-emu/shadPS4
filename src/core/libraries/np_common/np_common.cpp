// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np_common/np_common.h"
#include "core/libraries/np_common/np_common_error.h"

namespace Libraries::NpCommon {

int PS4_SYSV_ABI sceNpCmpNpId(OrbisNpId* np_id1, OrbisNpId* np_id2) {
    if (np_id1 == nullptr || np_id2 == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    // Compare data
    if (std::strncmp(np_id1->handle.data, np_id2->handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH) != 0) {
        return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
    }

    // Compare opt
    for (u32 i = 0; i < 8; i++) {
        if (np_id1->opt[i] != np_id2->opt[i]) {
            return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
        }
    }

    // Compare reserved
    for (u32 i = 0; i < 8; i++) {
        if (np_id1->reserved[i] != np_id2->reserved[i]) {
            return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
        }
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCmpNpIdInOrder(OrbisNpId* np_id1, OrbisNpId* np_id2, u32* out_result) {
    if (np_id1 == nullptr || np_id2 == nullptr || out_result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    // Compare data
    u32 compare =
        std::strncmp(np_id1->handle.data, np_id2->handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    if (compare < 0) {
        *out_result = -1;
        return ORBIS_OK;
    } else if (compare > 0) {
        *out_result = 1;
        return ORBIS_OK;
    }

    // Compare opt
    for (u32 i = 0; i < 8; i++) {
        if (np_id1->opt[i] < np_id2->opt[i]) {
            *out_result = -1;
            return ORBIS_OK;
        } else if (np_id1->opt[i] > np_id2->opt[i]) {
            *out_result = 1;
            return ORBIS_OK;
        }
    }

    // Compare reserved
    for (u32 i = 0; i < 8; i++) {
        if (np_id1->reserved[i] < np_id2->reserved[i]) {
            *out_result = -1;
            return ORBIS_OK;
        } else if (np_id1->reserved[i] > np_id2->reserved[i]) {
            *out_result = 1;
            return ORBIS_OK;
        }
    }

    *out_result = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCmpOnlineId(OrbisNpOnlineId* online_id1, OrbisNpOnlineId* online_id2) {
    if (online_id1 == nullptr || online_id2 == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (std::strncmp(online_id1->data, online_id2->data, ORBIS_NP_ONLINEID_MAX_LENGTH) != 0) {
        return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExConvertAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExFree() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExMalloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExRealloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExStrdup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorExStrndup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorFree() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorMalloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorRealloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorStrdup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpAllocatorStrndup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpFree() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpHeapFree() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpHeapMalloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpHeapRealloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpHeapStrdup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpHeapStrndup() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpMalloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _sceNpRealloc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable10IsCanceledEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable10LockCancelEPKciS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable11CheckCancelEPKciS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable12UnlockCancelEPKciS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable13SetCancelableEb() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable14SetupSubCancelEPS1_PKciS4_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable16CleanupSubCancelEPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable4InitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable6CancelEij() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10Cancelable7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelableC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelableD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelableD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelableD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLock3EndEPKciS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLock5BeginEPNS0_6HandleEPKciS5_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLockC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLockC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLockD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10CancelLockD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue10ClearAbortEt() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue10TryDequeueEPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue4InitEPKcmm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue5AbortEt() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue7DequeueEPvmj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueue7EnqueueEPKvmj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueueC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueueD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueueD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10EventQueueD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber6SetNumEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber6SetNumEj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber6SetNumEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber6SetNumEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonNumber6SetNumEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonObject16DeleteFieldValueEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonObject5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParser4InitEPK7JsonDefPNS1_12EventHandlerE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParser5ParseEPKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParserC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParserD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParserD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonParserD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonString5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10JsonString6SetStrEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFile4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFile4SyncEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFile5CloseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFile5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFile8TruncateEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFileC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np10MemoryFileD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI
_ZN3sce2np12HttpTemplate19SetAuthInfoCallbackEPFii15SceHttpAuthTypePKcPcS5_iPPhPmPiPvESA_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplate4InitEiPKcib() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplate7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplateC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplateC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplateD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplateD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12HttpTemplateD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamBufferixEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader4ReadEPNS0_6HandleEPNS0_9StreamCtxEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader7ReadAllEPNS0_6HandleEPNS0_9StreamCtxEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader7ReadAllEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader8ReadDataEPNS0_6HandleEPNS0_9StreamCtxEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader8ReadDataEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader8SkipDataEPNS0_6HandleElPl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamReader8SkipDataEPNS0_6HandleEPNS0_9StreamCtxElPl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamWriter15WriteFilledDataEPNS0_6HandleEcl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamWriter15WriteFilledDataEPNS0_6HandleEPNS0_9StreamCtxEcl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamWriter5WriteEPNS0_6HandleEPNS0_9StreamCtxEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamWriter9WriteDataEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12StreamWriter9WriteDataEPNS0_6HandleEPNS0_9StreamCtxEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThread10ThreadMainEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThreadC1EPNS0_9WorkQueueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThreadC2EPNS0_9WorkQueueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThreadD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThreadD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np12WorkerThreadD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParser5ParseEPKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParser9GetResultEPPNS0_10JsonObjectE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParser9GetResultEPPNS0_9JsonValueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParserC2EP16SceNpAllocatorExPK7JsonDef() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParserD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParserD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13JsonDocParserD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecret5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC1EPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC1ERK16SceNpTitleSecret() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC1ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC2EPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC2ERK16SceNpTitleSecret() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC2ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13NpTitleSecretD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory4InitEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory6ExpandEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemory7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemoryC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemoryD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemoryD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np13RingBufMemoryD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContext4InitEPKcimm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContext4InitEPKNS1_5ParamE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContext7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContextC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContextC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContextD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContextD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14CalloutContextD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilder12BuildBufSizeEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilder16EscapeJsonStringEPKcPcmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilder23EscapeJsonStringBufSizeEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilder5BuildEPcmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilderC1ERKNS0_9JsonValueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilderC2ERKNS0_9JsonValueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilderD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilderD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np14JsonDocBuilderD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScope3EndEiPKciS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScope5BeginEPNS0_6HandleEPKciS5_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScopeC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScopeD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScopeD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np15CancelableScopeD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np16StreamReadBufferC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np16StreamReadBufferD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np16StreamReadBufferD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPool13InvalidateAllEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPool4InitEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPool7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPoolC1EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPoolC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPoolD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPoolD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18HttpConnectionPoolD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReader4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReaderC1EPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReaderC2EPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReaderD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReaderD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamReaderD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriter5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriterC1EPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriterC2EPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriterD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriterD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np18MemoryStreamWriterD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReader4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReader5CloseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReaderC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReaderD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReaderD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np20BufferedStreamReaderD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient10DisconnectEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient11IsConnectedEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient16invokeSyncMethodEjPKvmPvPmm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient4InitEPKNS2_6ConfigE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient7ConnectEPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClient7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClientC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClientC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClientD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClientD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc10IpmiClientD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI
_ZN3sce2np3ipc13ServiceClientC1EPNS1_17ServiceIpmiClientEPKNS1_17ServiceClientInfoE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI
_ZN3sce2np3ipc13ServiceClientC2EPNS1_17ServiceIpmiClientEPKNS1_17ServiceClientInfoE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient10DisconnectEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient10EndRequestEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient11findServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient11InitServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient11TermServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient11WaitRequestEiij() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient12AbortRequestEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient12BeginRequestEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient13CreateRequestEPiiPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient13DeleteRequestEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient13PollEventFlagEijmjPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient13WaitEventFlagEijmjPmj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient14PollEventQueueEiPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient15CancelEventFlagEijm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient15RegisterServiceEPKNS1_17ServiceClientInfoE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient16RegisterServicesEPKNS1_17ServiceClientInfoE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient17invokeInitServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient17invokeTermServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient17UnregisterServiceEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient18EndRequestForAsyncEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient19WaitRequestForAsyncEiij() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient20AbortRequestForAsyncEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI
_ZN3sce2np3ipc17ServiceIpmiClient20BeginRequestForAsyncEiiPN4IPMI6Client12EventNotifeeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient21CreateRequestForAsyncEPiiPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient21DeleteRequestForAsyncEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient4InitEPNS2_6ConfigE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient7ConnectEPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClient7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClientC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClientC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClientD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClientD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np3ipc17ServiceIpmiClientD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond4InitEPKcPNS0_5MutexE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond4WaitEj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond6SignalEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Cond9SignalAllEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4CondC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4CondC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4CondD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4CondD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4CondD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Path11BuildAppendEPcmcPKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Path12AddDelimiterEPcmc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Path5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Path6SetStrEPKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4PathD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4PathD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4PathD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time10AddMinutesEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time10AddSecondsEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time12GetUserClockEPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time15AddMicroSecondsEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time15GetNetworkClockEPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time20GetDebugNetworkClockEPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time7AddDaysEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4Time8AddHoursEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4TimeplERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np4TimeplERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex4InitEPKcj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex4LockEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex6UnlockEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5Mutex7TryLockEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5MutexC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5MutexC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5MutexD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5MutexD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5MutexD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np5NpEnv8GetNpEnvEPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Handle10CancelImplEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Handle4InitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Handle7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6HandleC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6HandleC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6HandleD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6HandleD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6HandleD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdaEPv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdaEPvR14SceNpAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdaEPvR16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdlEPv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdlEPvR14SceNpAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectdlEPvR16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectnaEmR14SceNpAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectnaEmR16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectnwEmR14SceNpAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ObjectnwEmR16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread12DoThreadMainEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread4InitEPKcimm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread4InitEPKNS1_5ParamE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread4JoinEPi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread5StartEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread9EntryFuncEPv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread9GetResultEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6Thread9IsRunningEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ThreadC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ThreadD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ThreadD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np6ThreadD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout10IsTimedoutEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout11CalloutFuncEPv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout4StopEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout5StartEjPNS1_7HandlerE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout5StartEmPNS1_7HandlerE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7Callout9IsStartedEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7CalloutC1EPNS0_14CalloutContextE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7CalloutC2EPNS0_14CalloutContextE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7CalloutD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7CalloutD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7CalloutD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUri5BuildEPKS1_PcmPmj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUri5ParseEPS1_PKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUriC1EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUriC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUriD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUriD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7HttpUriD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf14CheckinForReadEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf15CheckinForWriteEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf15CheckoutForReadEPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf16CheckoutForWriteEPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf4InitEPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf4PeekEmPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf4ReadEPvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf5WriteEPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBuf7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBufC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBufC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBufD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBufD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np7RingBufD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFile4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFile5CloseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFileC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8HttpFileD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonBool5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonBool7SetBoolEb() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonFile5CloseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonFileD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8JsonNull5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommId5BuildERKS1_Pcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommId5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommId5ParseEPS1_PKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommId5ParseEPS1_PKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC1ERK20SceNpCommunicationId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC1ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC2ERK20SceNpCommunicationId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC2ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8NpCommIdD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8Selector4InitEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8SelectorD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8SelectorD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8SelectorD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem10SetPendingEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem10SetRunningEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem11SetFinishedEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem14FinishCallbackEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem15RemoveFromQueueEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem6CancelEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItem9BindQueueEPNS0_9WorkQueueEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItemC2EPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItemD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItemD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np8WorkItemD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag3SetEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag4OpenEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag4PollEmjPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag4WaitEmjPmj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag5ClearEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag6CancelEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag6CreateEPKcj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlag7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlagC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlagC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlagD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlagD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9EventFlagD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans10SetTimeoutEPKNS1_12TimeoutParamE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans11SendRequestEPNS0_6HandleEPKvm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans12RecvResponseEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans12SkipResponseEPNS0_6HandleE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans16AddRequestHeaderEPKcS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans16SetRequestHeaderEPKcS3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans21GetResponseStatusCodeEPNS0_6HandleEPi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans21SetRequestContentTypeEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans23SetRequestContentLengthEm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans24GetResponseContentLengthEPNS0_6HandleEPbPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans4InitERKNS0_12HttpTemplateEPNS0_18HttpConnectionPoolEiPKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI
_ZN3sce2np9HttpTrans4InitERKNS0_12HttpTemplateEPNS0_18HttpConnectionPoolEiPKcS8_tS8_m() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTrans7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTransC1EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTransC2EP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTransD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTransD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9HttpTransD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonArray12AddItemArrayEPPS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonArray5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValue12GetItemValueEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValue13GetFieldValueEiPPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValue13GetFieldValueEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValueD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValueD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9JsonValueD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile4SeekEliPl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile4SyncEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile5CloseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile6RemoveEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFile8TruncateEl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFileC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFileC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9LocalFileD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleId5BuildERKS1_Pcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleId5ClearEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleId5ParseEPS1_PKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleId5ParseEPS1_PKcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC1ERK12SceNpTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC1ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC2ERK12SceNpTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC2ERKS1_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9NpTitleIdD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObject6AddRefEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObject7ReleaseEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObjectC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObjectC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObjectD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObjectD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9RefObjectD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9Semaphore4OpenEPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9Semaphore4WaitEj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9Semaphore6CreateEiiPKc() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9Semaphore6SignalEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9Semaphore7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9SemaphoreC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9SemaphoreC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9SemaphoreD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9SemaphoreD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9SemaphoreD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue11GetItemByIdEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue15GetFinishedItemENS0_14WorkItemStatusE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue16WorkItemFinishedEPNS0_8WorkItemEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue17ProcFinishedItemsENS0_14WorkItemStatusE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue18RemoveFinishedItemEPNS0_8WorkItemE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue18WaitForPendingItemEPPNS0_8WorkItemEPb() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue4ctorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue4dtorEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue4InitEPKcimm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue4InitEPKNS0_6Thread5ParamE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue4StopEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue5StartEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue6CancelEii() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue7DestroyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue7EnqueueEiPNS0_8WorkItemE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue9CancelAllEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueue9IsRunningEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueueC1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueueC2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueueD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueueD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2np9WorkQueueD2Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERK12SceNpTitleIdRKNS0_9NpTitleIdE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERK16SceNpTitleSecretRKNS0_13NpTitleSecretE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERK20SceNpCommunicationIdRKNS0_8NpCommIdE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_13NpTitleSecretERK16SceNpTitleSecret() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_13NpTitleSecretES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_8NpCommIdERK20SceNpCommunicationId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_8NpCommIdES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_9NpTitleIdERK12SceNpTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npeqERKNS0_9NpTitleIdES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgeERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgeERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgeERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgtERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgtERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npgtERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npleERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npleERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npleERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npltERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npltERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npltERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERK10SceRtcTickRKNS0_4TimeE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERK12SceNpTitleIdRKNS0_9NpTitleIdE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERK16SceNpTitleSecretRKNS0_13NpTitleSecretE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERK20SceNpCommunicationIdRKNS0_8NpCommIdE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_13NpTitleSecretERK16SceNpTitleSecret() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_13NpTitleSecretES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_4TimeERK10SceRtcTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_4TimeES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_8NpCommIdERK20SceNpCommunicationId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_8NpCommIdES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_9NpTitleIdERK12SceNpTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce2npneERKNS0_9NpTitleIdES3_() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10Cancelable6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10EventQueue6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10EventQueue7IsEmptyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber6GetNumEPcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber6GetNumEPi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber6GetNumEPj() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber6GetNumEPl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber6GetNumEPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonNumber9GetNumStrEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonObject5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonString5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonString6GetStrEPcm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonString6GetStrEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np10JsonString9GetLengthEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np12HttpTemplate6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np18HttpConnectionPool6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np3ipc10IpmiClient6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np3ipc17ServiceIpmiClient6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4Cond6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np4Time18ConvertToPosixTimeEPl() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np5Mutex6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np6Handle6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np6Thread6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np7RingBuf11GetDataSizeEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np7RingBuf11GetFreeSizeEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np7RingBuf6IsFullEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np7RingBuf7IsEmptyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8JsonBool5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8JsonBool7GetBoolEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8JsonNull5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np8NpCommId7IsEmptyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9EventFlag6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9HttpTrans6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9JsonArray5CloneEP16SceNpAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9JsonValue12GetItemValueEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9NpTitleId7IsEmptyEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce2np9Semaphore6IsInitEv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np10MemoryFile5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np10MemoryFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np10MemoryFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9HttpTrans5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9HttpTransD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9HttpTransD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9LocalFile5WriteEPNS0_6HandleEPKvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9LocalFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn16_N3sce2np9LocalFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np10MemoryFile4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np10MemoryFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np10MemoryFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np6Handle10CancelImplEi() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np6HandleD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np6HandleD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9HttpTrans4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9HttpTransD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9HttpTransD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9LocalFile4ReadEPNS0_6HandleEPvmPm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9LocalFileD0Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZThn8_N3sce2np9LocalFileD1Ev() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np10JsonNumberE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np10JsonObjectE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np10JsonStringE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np8JsonBoolE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np8JsonNullE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np8SelectorE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np9JsonArrayE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce2np9JsonValueE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAllocateKernelMemoryNoAlignment() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAllocateKernelMemoryWithAlignment() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpArchInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpArchTerm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAtomicCas32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAtomicDec32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpAtomicInc32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64Decoder() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64Encoder() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64GetDecodeSize() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64UrlDecoder() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64UrlEncoder() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpBase64UrlGetDecodeSize() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCalloutInitCtx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCalloutStartOnCtx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCalloutStartOnCtx64() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCalloutStopOnCtx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCalloutTermCtx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCancelEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpClearEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCloseEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCloseSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondSignal() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondSignalAll() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondSignalTo() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondTimedwait() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCondWait() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCreateEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCreateSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpCreateThread() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpDbgAssignDebugId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpDbgDumpBinary() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpDbgDumpText() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpDeleteEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpDeleteSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEventGetCurrentNetworkTick() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpFreeKernelMemory() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetNavSdkVersion() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetPlatformType() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetProcessId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetRandom() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetSdkVersion() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetSdkVersionUInt() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGetSystemClockUsec() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGlobalHeapGetAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGlobalHeapGetAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGlobalHeapGetAllocatorExPtr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpGlobalHeapGetAllocatorPtr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHeapDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHeapGetAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHeapGetStat() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHeapInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHeapShowStat() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpHexToInt() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInt32ToStr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpInt64ToStr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntGetPlatformType() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntIsOnlineIdString() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntIsValidOnlineId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntSetPlatformType() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIntToHex() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIpc2ClientInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpIpc2ClientTerm() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJoinThread() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParse() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParseBuf() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParseBufInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParseEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParseExInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpJsonParseInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondSignal() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondSignalAll() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondSignalTo() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwCondWait() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwMutexDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwMutexInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwMutexLock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwMutexTryLock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpLwMutexUnlock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMemoryHeapDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMemoryHeapGetAllocator() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMemoryHeapGetAllocatorEx() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMemoryHeapInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMutexDestroy() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMutexInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMutexLock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMutexTryLock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMutexUnlock() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpOpenEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpOpenSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPanic() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPollEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpPollSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRtcConvertToPosixTime() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRtcFormatRFC3339() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpRtcParseRFC3339() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServerErrorJsonGetErrorCode() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServerErrorJsonMultiGetErrorCode() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServerErrorJsonParse() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServerErrorJsonParseInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpServerErrorJsonParseMultiInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSetPlatformType() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSignalSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrBuildHex() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrcpyToBuf() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrncpyToBuf() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrnParseHex() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrParseHex() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrToInt32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrToInt64() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrToUInt32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpStrToUInt64() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpThreadGetId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUInt32ToStr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUInt64ToStr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUserGetUserIdList() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilBuildTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilCanonicalizeNpIdForPs4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilCanonicalizeNpIdForPsp2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilCmpAccountId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetDateSetAuto() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetDbgCommerce() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetEnv() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetFakeDisplayNameMode() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetFakeRateLimit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetIgnoreNpTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpDebug() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpLanguageCode() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpLanguageCode2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpLanguageCode2Str() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpLanguageCodeStr() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNpTestPatch() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetNthChar() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetShareTitleCheck() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetSystemLanguage() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetTrcNotify() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetWebApi2FakeRateLimit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetWebApi2FakeRateLimitTarget() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilGetWebTraceSetting() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilHttpUrlEncode() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilJidToNpId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilJsonEscape() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilJsonGetOneChar() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilJsonUnescape() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilNpIdToJid() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilNumChars() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilParseJid() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilParseTitleId() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilSerializeJid() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilXmlEscape() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilXmlGetOneChar() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpUtilXmlUnescape() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpWaitEventFlag() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpWaitSema() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpXmlParse() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpXmlParseInit() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_00FD578C2DD966DF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0131A2EA80689F4C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_01443C54863BDD20() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_01BC55BDC5C0ADAD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_01D1ECF5750F40E8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_020A479A74F5FBAC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_024AF5E1D9472AB5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_027C5D488713A6B3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_02FE9D94C6858355() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_041F34F1C70D15C1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0530B1D276114248() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_065DAA14E9C73AD9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_06AFF4E5D042BC3E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_06EE369299F73997() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_07C92D9F8D76B617() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_07E9117498F1E4BF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_08F3E0AF3664F275() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0A9937C01EF21375() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0ACBE6ACCBA3876D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0AE07D3354510CE6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0AEC3C342AE67B7C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0B318420C11E7C23() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0BB6C37B03F35D89() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0BBE8A9ACDD90FDF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0C7B62905E224E9C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0D35913117241AF9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0D5EE95CEED879A7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0D6FB24B27AB1DA2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0DE8032D534AC41C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0DF4CCA9DCA9E742() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0E7449B1D3D98C01() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0E77094B7750CB37() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0ECAB397B6D50603() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0F1DE1D1EADA2948() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0F8AFEFA1D26BF1A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_11881710562A6BAD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_11AFD88BBD0C70DB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_11E704A30A4B8877() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_125014842452F94B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_126F0071E11CAC46() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_12926DCF35994B01() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_12CC7ABFBF31618F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_13C4E51F44592AA2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_15330E7C56338254() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1566B358CABF2612() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1625818F268F45EF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_16D32B40D28A9AC2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_183F4483BDBD25CD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1887E9E95AF62F3D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_18A3CE95FD893D3A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_18B3665E4854E7E9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1923B003948AF47E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_19B533DA4C59A532() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1BB399772DB68E08() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1C0AC612D3A2971B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1C5599B779990A43() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1CCBB296B04317BE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1CD045542FB93002() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1DECECA673AB77B7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1E03E024E26C1A7F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1F101732BB0D7E21() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1F4D153EC3DD47BB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1F7C47F63FAF0CBE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1FBE2EE68C0F31B6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2038C1628914B9C9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_203FCB56FDB86A74() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_20569C107C6CB08C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_20AB2D734EDE55F0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_22B1281180FB0A5E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_22F1AADA66A449AE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_238B215EFFDF3D30() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_24E8EC51D149FA15() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_25728E78A3962C02() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_25E649A1C6891C05() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_264B8A38B577705D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_266ED08DC1C82A0E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_27BB4DE62AB58BAD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_283AA96A196EA2EA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_285315A390A85A94() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_29049DBB1EF3194E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_29F7BA9C3732CB47() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2A732DF331ACCB37() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2AA01660EC75B6FB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2B37CBCE941C1681() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2CAA3B64D0544E55() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2CCD79617EC10A75() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2CD8B69716AC0667() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2D74F7C0FF9B5E9C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2DCA5A8080544E95() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2E69F2743CE7CE57() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2EAF1F3BAFF0527D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_31493E55BB4E8F66() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_317EDCAD00FB5F5E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_31E01CFA8A18CDA2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_32AFD782A061B526() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_32B5CDEB093B8189() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_34155152513C93AE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_34E4EFFF8EF6C9FE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3572FA0D5C54563B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_367C479B264E0DB9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_36884FBC964B29CC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3860081BB7559949() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_39314F7E674AB132() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3A02E780FCC556A5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3A17B885BA4849B6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3A38EACAEA5E23A4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3B34A5E07F0DBC1F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3B4E8FFC00FC7EA4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3BAB18FDA235107A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3BDF9996A0A33F11() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3C1952F1A45CC37A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3CA37906CDB05F3B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3CDB2908ACEE3A6F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3D3ED165F2BDCD33() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3DA4D7D1575FCDCE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3DDFB612CD0BC769() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3E0415E167DEADC7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3E7E9F0F1581C1E6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3ED389DB8280ED65() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3F0C7F6C0C35487D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3FDA7200389EF0D2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_3FF3C258BA516E58() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4029453F628A3C5D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_405826DDB4AE538E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_405A926759F25865() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_406608FDEE7AE88A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_40DDA5558C17DDCF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_419D12E52FF60664() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4296E539474BE77F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_42F41FC563CC3654() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_43CCC86F4C93026A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4409F60BDABC65E1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4563C70AEC675382() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_45E66370219BD05E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_466A54F072785696() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_46CD2536976F209A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4863717BD2FDD157() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4902EBD19A263149() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4904F7FE8D83F40C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4A5E13F784ABFCE7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4B65EEB135C12781() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4C19D49978DA85E2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4DE5D620FF66F136() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4E170C12B57A8F9E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4E2F3FA405C3260C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4EA9350577513B4D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_4F78EB6FC4B5F21F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_50348BE4331117B7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_508C7E8CDD281CAA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_521C1D2C028F5A7E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_522FF24A35E67291() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5470FE90C25CDD4C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_557F260F9A4ACD18() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5586F97209F391EB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_55B2C9B7ADA95C3C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_55B488A3A540B936() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5642DFE82AF43143() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_574E046F294AE187() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_578926EBF8AA6CBF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_585DA5FC650896BC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_58D6EB27349EC276() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5906B7317949872D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5910B5614335BE70() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_593D7DA8911F08C9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_59757FE6A93B0D53() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_598E60F862B1141E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5A45351666680DAF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5AABE9EA702E6A7F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5AEA4AE472355B80() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5B20E53CDE598741() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5B480B59FAE947E0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5B5EEC23690AB9BD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5C0AC5B0AF3EDAE0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5D2E999BEA0762D4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5D55BBFD45110E16() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_5DEE15403D2BB5FD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6020C708CA74B130() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_606E1415503C34D2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_612140E8EE9A693E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_61F13F551DAF61DF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6206D39131752328() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_621D4543EF0344DE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6259A9A8E56D0273() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_625F9C7016346F4E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_62EF8DF746CD8C4A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_636D2A99FD1E6B2B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_68013EDF66FE7425() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6971F7067DD639D1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_69896ADB3AB410B2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6A1389AA6E561387() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6A5560D89F12B2E7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6ABF99CF854ABCF1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6B4FDDC6500D8DCB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6CA11D5B49D1928A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6D6C0FB61E6D0715() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6D750745FE1348F5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6E1AF3F9D09914BE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6E53ED4C08B2A521() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6EF43ACA1ED6B968() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6F6FA09F3E1B6A60() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7035C340C7195901() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7038E21CB5CF641B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_706345DCDA5BA44D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7120714EBF10BF1F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_713D28A91BC803DD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7153BD76A53AA012() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_715C625CC7041B6B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_71E467BDB18711D0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_720D17965C1F4E3F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_734380C9BCF65B9A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_73F4C08CCD4BBCCF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_74403101B7B29D46() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7525B081ACD66FF4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_75BF4477C13A05CA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7609793F5987C6F7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7616ED01B04769AA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_764F873D91A124D8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7706F1E123059565() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_77F2D07EB6D806E6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_79C3704CDCD59E57() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_79DA0BBA21351545() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_79FA2447B5F3F0C4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7A4D6F65FF6195A5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7B3195CD114DECE7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7B3238F2301AD36D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7C77FC70750A3266() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7D23A9DC459D6D18() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7D5988C748D0A05F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7D9597147A99F4F4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7E2953F407DD8346() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7EE34E5099709B32() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_80470E5511D5CA00() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_807179701C08F069() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8096E81FFAF24E46() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_80B764F4F1B87042() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_80BF691438AD008B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_80CF6CFC96012442() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_80EA772F8C0519FD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_81D0AFD0084D327A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_821EB8A72176FD67() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_82D2FAB54127273F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_836AE669C42A59E9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8559A25BFEC3518C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_85C1F66C767A49D2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8689ED1383F87BA7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8796CD9E5355D3A6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_87D37EB6DDC19D99() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_880AA48F70F84FDD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_897B07562093665B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8ACAF55F16368087() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8AE8A5589B30D4E0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8AE997909831B331() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8B2D640BE0D0FB99() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8B3D9AB4668DAECB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8B5EFAAAACE0B46C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8C27943F40A988DB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8C54096C75F5F2D0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8D7663A0A5168814() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8E618F509994FAD7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8F19E6CC064E2B98() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_8F6A8AEAEE922FF5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9010E1AD8EBBFBCA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_90A955A0E7001AE9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_90F9D6067FEECC05() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9348F3D19546A1DA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_93D3C011DB19388A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_956E7A4FD9F89103() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_95F699E042C3E40F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_96877B39AA0E8735() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_96CE07C49ED234EA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_976BB178235B5681() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_978C0B25E588C4D6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_98BA2612BEF238D6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_995BDD4931AF9137() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9966E39A926B7250() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_99C2306F18963464() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_99C92C613B776BA7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9A4E4B938CC8AD39() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9B23F7B4B7F72081() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9C0EAEEAE705A8DB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9D47AC59545DE9E8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A13052D8B1B2ACFA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A1AA43E3A78F6F62() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A1E48CDF54649DC9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A2E7DEE5B0AF5D14() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A2F5C7FD9FF113F5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A36296E2269D46BC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A3EE2A7B9F0D88AF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A4471F9F7E0BFA82() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A449BBA521EA34E1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A48E666C334E726C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A49B7449B4DDE69C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A5748451125C9EA4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A690A28D648CC176() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A6A86DE1B1CBB1D9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A8F2BB7B815740A1() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A93F64C06A6F7397() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AB35925FC97D6AA3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AC014AA2C991FA29() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AC06E10901404AEB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AC75C68813523505() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AD441BC497082C3E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AD4F25F021D354C3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ADFA04A85541A4FE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AE9610A6B5217A23() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AF201923826F0A58() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AFC021B4389CA3FA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B015E999A3373D8F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B0384B86107FC652() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B0C630653B316563() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B100DCCD88D5C73D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B11A3FEA5E4D9EA4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B2E7F8DC199C0B93() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B3AB61A296F6DDC8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B3F32F6AE619EC82() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B4227AB213BF8CF5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B4652BF42B604360() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B536C1F13BFE97CB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B645CC264184BC89() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B67E17B1582C6FBD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B6D047C5D7695A4D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B75ED8E1EA62EFC7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B7A9A944DBD7E100() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B7C4E75BE94F31F3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B888B1F92C464121() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B8DEC22564AA057B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B9BADD1CBBBAE4F8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BAA9F7169C85E59F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BAEE5C38908D62DB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BCC855EB25183F84() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BD01F637029C7364() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BDD29F5AC7077E53() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BED83DD33ECAD50D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_BEE7D5D098ABF728() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C0DB15CCF59AE62C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C1C229FEE0FD60FA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C228B9AD68298E98() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C298525CEF6FB283() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C350F09351F6D6B5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C3742E80FA580319() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C3C9853D5D4D45D4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C3F5DAD4FB9FC340() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C45FB0E4CCE9AED6() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C4979CB948B7E3C7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C49B25BA16CF0B8C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C551345D9631201E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C57A294421368298() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C5DC91CAD721D628() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C6DECEE589135357() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C81F8B20D67AC78D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C820FA56FAC87BEA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C878EA9114C5E490() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C8A813EBFF477509() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C966A663D5A35482() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C97C4C67FD3674D3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C990550F15848B07() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CA59737A8EC1BBBE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CAC5FDE8F80D7B65() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB135B30D0639B83() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB8A1AAA61F64C3A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB9E674672580757() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CC2B9D25EAEAAB1D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CD1B252BBEDF5B53() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CF003BE90CBE1A27() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CF008E34884AC1E2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D0B8F4B3A3687AB2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D0EE19B8E91F60F5() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D12B9294BD0E0F56() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D1CC8626D8FA328B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D2FA2BB9EB8B63AC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D32197880CF93CEB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D326F5C26CC81B8E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D4FA06B95A321B7A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D52A37A901E04B21() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D5504DFC399AB400() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D56105CB27F8F5DC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D568AB19235ECB19() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D6DF7BF6639FE611() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D8608A903119D746() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D9E8FC707D59914D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D9F079E62DEE5B29() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DA17CE4F29748536() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DA40B9EFD7F61185() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DA6B274FEBC2666A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DAD01535C87A51FC() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DB4511D448510EC4() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DB8EF1FFFC66269C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DBB508FA1B9DA8F7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DC59C9B870B729A2() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DC669ED6CBF6751C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DCB8A2849A41C991() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DD8F9916D7F03AF7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DDC33F2F4E480C2A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_DE0B420BDE8B22D7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E0C0BC29898FE370() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E0CD893E46FB55BA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E25530164B7F659F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E3682F43FDF76C58() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E38177E1C78A80FA() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E3CA74CFF965DF0A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E45BB191B49B2ED9() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E465B9D6B60E6D7D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E4D82876C296C38A() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E4DDB5350FA5B538() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E54BFF6FB72BC7BE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E592A93203020BBB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E5A44AF6D7D48AFD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E639A97CF9FF1430() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E6AC0179E48A8927() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E751596682775D83() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E788B1E52EF82702() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E94F17613F5C9D31() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E9590113128D55E0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E9E0B0DD12560B16() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EAF5C8ECE64C7B05() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EB98BF5C42D4A7EB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EBABC4AAC43A468C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EBF00085F082CC8B() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ECB659EE058D06AF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ECF096AB751487AE() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EE5A271701DB33C0() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EF64CB6A1625248E() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EF6C8A357C7ED863() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F00FE94F7E699994() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F1A51DBA30329038() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F216E766A90FDC12() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F2A10584ABE5D82C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F2D99D395E5421A3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F38001E528BA1371() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F39EC9C8FA7687B3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F3AFFFDCD632775C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F3B8DFF33748BFD3() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F5E47F9550F7A147() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F6E93714D1A939CF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F6FD19AD48E4EF09() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F744EBFC620F7CBF() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F76E4525ACBACC7F() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F7957A48882F42CB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F7A80B07809BA838() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F8571C6CC5B6B59D() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F9787CFA873836FB() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FA789F6D34D383F8() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FABA574083AC1E6C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FC04FDBBAE368FB7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FD2DAFBF2E40EEE7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FD55EE6D35F950AD() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FE55EE32098D0D58() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FE79841022E1DA1C() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FFF4A3E279FB44A7() {
    LOG_ERROR(Lib_NpCommon, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommonCompat", 1, "libSceNpCommon", 1, 1, sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommonCompat", 1, "libSceNpCommon", 1, 1,
                 sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommonCompat", 1, "libSceNpCommon", 1, 1,
                 sceNpCmpOnlineId);
    LIB_FUNCTION("0gdlCVNNHCI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorExConvertAllocator);
    LIB_FUNCTION("Zh23aSLeeZo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpAllocatorExFree);
    LIB_FUNCTION("a2qdVU8RWb4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorExMalloc);
    LIB_FUNCTION("kKF3w-XkCWA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorExRealloc);
    LIB_FUNCTION("Cmd4+m7V00c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorExStrdup);
    LIB_FUNCTION("EziLjfyTnKI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorExStrndup);
    LIB_FUNCTION("BztTl7QeYqE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpAllocatorFree);
    LIB_FUNCTION("mzlILsFx0cU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpAllocatorMalloc);
    LIB_FUNCTION("VWcTu8wKwlQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorRealloc);
    LIB_FUNCTION("c8-4aC9opYE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpAllocatorStrdup);
    LIB_FUNCTION("vqA9bl6WsF0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _sceNpAllocatorStrndup);
    LIB_FUNCTION("z5kwfM5InpI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpFree);
    LIB_FUNCTION("p1vvpKGRXe4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpHeapFree);
    LIB_FUNCTION("kwW5qddf+Lo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpHeapMalloc);
    LIB_FUNCTION("wsfyvM+VbUk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpHeapRealloc);
    LIB_FUNCTION("atWcfgasESY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpHeapStrdup);
    LIB_FUNCTION("RzLv+HR5E2A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpHeapStrndup);
    LIB_FUNCTION("w2+qV1RJgcI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpMalloc);
    LIB_FUNCTION("UmzxltBpiiY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _sceNpRealloc);
    LIB_FUNCTION("LJvHO3uCNm4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable10IsCanceledEv);
    LIB_FUNCTION("fd+grYAEph0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable10LockCancelEPKciS3_);
    LIB_FUNCTION("IwDQAbQxvD0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable11CheckCancelEPKciS3_);
    LIB_FUNCTION("-zbpF68OGDs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable12UnlockCancelEPKciS3_);
    LIB_FUNCTION("bBLapYYwyr0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable13SetCancelableEb);
    LIB_FUNCTION("j4gLOIpHgNk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable14SetupSubCancelEPS1_PKciS4_);
    LIB_FUNCTION("vmt3ZOlQu3o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable16CleanupSubCancelEPS1_);
    LIB_FUNCTION("Y7f+qBjKxdo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable4InitEv);
    LIB_FUNCTION("Jhbrpz0YhHU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable6CancelEij);
    LIB_FUNCTION("v2yJZLY0w1U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10Cancelable7DestroyEv);
    LIB_FUNCTION("vqekW3s-eFg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelableC2Ev);
    LIB_FUNCTION("kdOC-2AE06w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelableD0Ev);
    LIB_FUNCTION("upzdrzOYkS0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelableD1Ev);
    LIB_FUNCTION("vZXDqs2x7t0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelableD2Ev);
    LIB_FUNCTION("nleHqndSeQ0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLock3EndEPKciS3_);
    LIB_FUNCTION("lJ2Efd9PUKI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLock5BeginEPNS0_6HandleEPKciS5_);
    LIB_FUNCTION("Vq9LKkPXkIQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLockC1Ev);
    LIB_FUNCTION("MecB8wAHCfE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLockC2Ev);
    LIB_FUNCTION("K7FjXiy2z+A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLockD1Ev);
    LIB_FUNCTION("1iHBAKrdE90", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10CancelLockD2Ev);
    LIB_FUNCTION("aoas3bJANfY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue10ClearAbortEt);
    LIB_FUNCTION("QlP4t2SGZ4I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue10TryDequeueEPvm);
    LIB_FUNCTION("xu9qWN0YYC4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue4ctorEv);
    LIB_FUNCTION("N1gnYosdK7Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue4dtorEv);
    LIB_FUNCTION("b20e017Ei94", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue4InitEPKcmm);
    LIB_FUNCTION("slmKkuIoC28", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue5AbortEt);
    LIB_FUNCTION("suxln7PooIo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue7DequeueEPvmj);
    LIB_FUNCTION("qvpEuKumIGM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue7DestroyEv);
    LIB_FUNCTION("AV5jHo8O3+E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueue7EnqueueEPKvmj);
    LIB_FUNCTION("esiO4He2WTU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueueC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("E4uoqSdo8ek", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueueD0Ev);
    LIB_FUNCTION("lQXgvDXBGtA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueueD1Ev);
    LIB_FUNCTION("8kUkQPQP7bA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10EventQueueD2Ev);
    LIB_FUNCTION("YHNEgBCSL2o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber5ClearEv);
    LIB_FUNCTION("UgmqDr1BCLw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber6SetNumEi);
    LIB_FUNCTION("PccynQ5NdVQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber6SetNumEj);
    LIB_FUNCTION("MY0CSk24EcY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber6SetNumEl);
    LIB_FUNCTION("qbW7qOvVafI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber6SetNumEm);
    LIB_FUNCTION("VyCn9EVJGlU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonNumber6SetNumEPKc);
    LIB_FUNCTION("-WgnISXjJ7A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonObject16DeleteFieldValueEPKc);
    LIB_FUNCTION("DiHxx2k5zfM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonObject5ClearEv);
    LIB_FUNCTION("AGadQiCfKDY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParser4InitEPK7JsonDefPNS1_12EventHandlerE);
    LIB_FUNCTION("CDzSgHA6hWg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParser5ParseEPKcm);
    LIB_FUNCTION("ZJbPQt+FTnY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParserC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("u+A16O-TAHk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParserD0Ev);
    LIB_FUNCTION("qJb7IXDg9xk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParserD1Ev);
    LIB_FUNCTION("AvvE5A5A6ZA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonParserD2Ev);
    LIB_FUNCTION("kXE1imLw7yo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonString5ClearEv);
    LIB_FUNCTION("SN4IgvT26To", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10JsonString6SetStrEPKc);
    LIB_FUNCTION("EyhtbPFMWNA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFile4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("AZTMWob-mog", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFile4SyncEv);
    LIB_FUNCTION("dl6+SFHLke0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFile5CloseEv);
    LIB_FUNCTION("r2O0f9X-mqs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFile5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("1DtavqenQjg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFile8TruncateEl);
    LIB_FUNCTION("ev77AviWYu8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFileC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("6Vst7HqJMXU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFileD0Ev);
    LIB_FUNCTION("ZUf92uPkRuA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFileD1Ev);
    LIB_FUNCTION("lGjyfcI++PY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np10MemoryFileD2Ev);
    LIB_FUNCTION(
        "ezJnmv7hkAg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
        _ZN3sce2np12HttpTemplate19SetAuthInfoCallbackEPFii15SceHttpAuthTypePKcPcS5_iPPhPmPiPvESA_);
    LIB_FUNCTION("iOTsJTR6Y9U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplate4InitEiPKcib);
    LIB_FUNCTION("73qbxKjBH0o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplate7DestroyEv);
    LIB_FUNCTION("Vj7HiXK-tTg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplateC1Ev);
    LIB_FUNCTION("hw-UPUK9T+w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplateC2Ev);
    LIB_FUNCTION("cXYOwTVAuMs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplateD0Ev);
    LIB_FUNCTION("Bm74HLvoNY4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplateD1Ev);
    LIB_FUNCTION("h6XPsGpHAtc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12HttpTemplateD2Ev);
    LIB_FUNCTION("jr0OcEeQJ8o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamBufferixEi);
    LIB_FUNCTION("rCRh3V03bPs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader4ReadEPNS0_6HandleEPNS0_9StreamCtxEPvmPm);
    LIB_FUNCTION("2SKuIvr9sYU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader7ReadAllEPNS0_6HandleEPNS0_9StreamCtxEPvmPm);
    LIB_FUNCTION("f1ncwa-JXlA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader7ReadAllEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("z8qO7hql4Fs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader8ReadDataEPNS0_6HandleEPNS0_9StreamCtxEPvmPm);
    LIB_FUNCTION("oNqSobbGC80", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader8ReadDataEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("MSMPXUL5AuM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader8SkipDataEPNS0_6HandleElPl);
    LIB_FUNCTION("fJB07vDf7no", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamReader8SkipDataEPNS0_6HandleEPNS0_9StreamCtxElPl);
    LIB_FUNCTION("etMUeqIhN+w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamWriter15WriteFilledDataEPNS0_6HandleEcl);
    LIB_FUNCTION("SP2010+gtqw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamWriter15WriteFilledDataEPNS0_6HandleEPNS0_9StreamCtxEcl);
    LIB_FUNCTION("Z1MRG-L+V0o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamWriter5WriteEPNS0_6HandleEPNS0_9StreamCtxEPKvmPm);
    LIB_FUNCTION("vHaV+tsSVu4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamWriter9WriteDataEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("u9s1aUWSZB0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12StreamWriter9WriteDataEPNS0_6HandleEPNS0_9StreamCtxEPKvmPm);
    LIB_FUNCTION("gimH2zdBANg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThread10ThreadMainEv);
    LIB_FUNCTION("YKz2oBW3ZkM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThreadC1EPNS0_9WorkQueueE);
    LIB_FUNCTION("L9Ty-fG1IM4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThreadC2EPNS0_9WorkQueueE);
    LIB_FUNCTION("f5L6ax7EWHk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThreadD0Ev);
    LIB_FUNCTION("PvGTq9AGFfk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThreadD1Ev);
    LIB_FUNCTION("+qB+WcQlMio", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np12WorkerThreadD2Ev);
    LIB_FUNCTION("4nCyBD9jBus", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParser5ParseEPKcm);
    LIB_FUNCTION("sgh9D+MBBKA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParser9GetResultEPPNS0_10JsonObjectE);
    LIB_FUNCTION("lZWmdDoBDmI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParser9GetResultEPPNS0_9JsonValueE);
    LIB_FUNCTION("yPmQcnrgR2Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParserC2EP16SceNpAllocatorExPK7JsonDef);
    LIB_FUNCTION("p5hRe1k4Wlg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParserD0Ev);
    LIB_FUNCTION("iFOXfoXRHFQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParserD1Ev);
    LIB_FUNCTION("xS-Hjw1psYs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13JsonDocParserD2Ev);
    LIB_FUNCTION("X0vEo7cZamA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecret5ClearEv);
    LIB_FUNCTION("IjOpzNzl57o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC1EPKvm);
    LIB_FUNCTION("bC4+qi0mqJE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC1ERK16SceNpTitleSecret);
    LIB_FUNCTION("fYr7Ahl-vNA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC1ERKS1_);
    LIB_FUNCTION("08AQ2wYpzpk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC1Ev);
    LIB_FUNCTION("Ft-VezxSErk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC2EPKvm);
    LIB_FUNCTION("9QN7g5mQgCU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC2ERK16SceNpTitleSecret);
    LIB_FUNCTION("JHG9CTmkdQw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC2ERKS1_);
    LIB_FUNCTION("K1+uzxxReX0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretC2Ev);
    LIB_FUNCTION("dJRIc7d5iqU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretD0Ev);
    LIB_FUNCTION("XBzzdzT3qyg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretD1Ev);
    LIB_FUNCTION("QDlnJL6stA0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13NpTitleSecretD2Ev);
    LIB_FUNCTION("RPv5L-o5qRQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory4ctorEv);
    LIB_FUNCTION("NfhXX6LFmj8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory4dtorEv);
    LIB_FUNCTION("BkuxOAPlMMw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory4InitEm);
    LIB_FUNCTION("do0t--lEKMM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory6ExpandEm);
    LIB_FUNCTION("zdRXyt-65kA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory6IsInitEv);
    LIB_FUNCTION("Za00SEoNA2A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemory7DestroyEv);
    LIB_FUNCTION("lGIw3qfqI60", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemoryC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("70qFzq4z3UI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemoryD0Ev);
    LIB_FUNCTION("C1TJsMv9wb8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemoryD1Ev);
    LIB_FUNCTION("EaxLv8TfsrM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np13RingBufMemoryD2Ev);
    LIB_FUNCTION("j6CorpmdjRk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContext4InitEPKcimm);
    LIB_FUNCTION("oLpLfV2Ov9A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContext4InitEPKNS1_5ParamE);
    LIB_FUNCTION("C282U0P6Nwg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContext7DestroyEv);
    LIB_FUNCTION("dV+zK-Ce-2E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContextC1Ev);
    LIB_FUNCTION("j4IAvbKKTzw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContextC2Ev);
    LIB_FUNCTION("WR4mjQeqz6s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContextD0Ev);
    LIB_FUNCTION("S+a+rgnGX8A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContextD1Ev);
    LIB_FUNCTION("wY9g+hVxLTM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14CalloutContextD2Ev);
    LIB_FUNCTION("PYBehFWVd60", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilder12BuildBufSizeEv);
    LIB_FUNCTION("cLdoHqi5Ezg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilder16EscapeJsonStringEPKcPcmPm);
    LIB_FUNCTION("V5xX2eroaWY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilder23EscapeJsonStringBufSizeEPKc);
    LIB_FUNCTION("irex3q-O6po", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilder5BuildEPcmPm);
    LIB_FUNCTION("ikFI73f3hP4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilderC1ERKNS0_9JsonValueE);
    LIB_FUNCTION("dhJGQPKLmn0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilderC2ERKNS0_9JsonValueE);
    LIB_FUNCTION("wDLaq7IgfIc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilderD0Ev);
    LIB_FUNCTION("Kfv9jPxf7qA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilderD1Ev);
    LIB_FUNCTION("MH0LyghLJEE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np14JsonDocBuilderD2Ev);
    LIB_FUNCTION("pCIB7QX5e1g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScope3EndEiPKciS3_);
    LIB_FUNCTION("Etvu03IpTEc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScope5BeginEPNS0_6HandleEPKciS5_);
    LIB_FUNCTION("pp88xnRgJrM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScopeC2Ev);
    LIB_FUNCTION("E8yuDNYbzl0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScopeD0Ev);
    LIB_FUNCTION("km5-rjNjSFk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScopeD1Ev);
    LIB_FUNCTION("xpLjHhJBhpo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np15CancelableScopeD2Ev);
    LIB_FUNCTION("LCk8T5b1h+4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np16StreamReadBufferC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("ZufKqNXItD0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np16StreamReadBufferD1Ev);
    LIB_FUNCTION("bH7ljyLOsBw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np16StreamReadBufferD2Ev);
    LIB_FUNCTION("et05S+nkWG8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPool13InvalidateAllEv);
    LIB_FUNCTION("Vzob5RCgfnY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPool4InitEi);
    LIB_FUNCTION("iBdEFRdfpgg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPool7DestroyEv);
    LIB_FUNCTION("PznfSvchYJ8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPoolC1EP16SceNpAllocatorEx);
    LIB_FUNCTION("-2TYwZ4ERbM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPoolC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("5HWP63cOH+w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPoolD0Ev);
    LIB_FUNCTION("kTfkKhcdW5Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPoolD1Ev);
    LIB_FUNCTION("3MVW8+eWnjs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18HttpConnectionPoolD2Ev);
    LIB_FUNCTION("ELa6nMcCO9w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReader4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("UHj0GDTA2CU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReaderC1EPKvm);
    LIB_FUNCTION("WXRruhGp9dI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReaderC2EPKvm);
    LIB_FUNCTION("gA0CaCjJpg0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReaderD0Ev);
    LIB_FUNCTION("oULMh4JVC4o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReaderD1Ev);
    LIB_FUNCTION("rNJ1+3KoZP4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamReaderD2Ev);
    LIB_FUNCTION("VxKQGrudnzk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriter5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("Lkdm2yqZN1c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriterC1EPvm);
    LIB_FUNCTION("abQ7xd3yVXM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriterC2EPvm);
    LIB_FUNCTION("TXJnPiKuTf8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriterD0Ev);
    LIB_FUNCTION("3VdCUl+DkNw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriterD1Ev);
    LIB_FUNCTION("YmOVGwSJmzk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np18MemoryStreamWriterD2Ev);
    LIB_FUNCTION("INZSjlRcuyQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReader4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("3Ku9r8b6gCg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReader5CloseEv);
    LIB_FUNCTION("l6s7aomzWGA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReaderC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("i28bR54-QFQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReaderD0Ev);
    LIB_FUNCTION("Tgr66MThOxA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReaderD1Ev);
    LIB_FUNCTION("PHWvRXbOnYs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np20BufferedStreamReaderD2Ev);
    LIB_FUNCTION("7dyKpPHU+Yk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient10DisconnectEv);
    LIB_FUNCTION("prj9aMR74bA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient11IsConnectedEv);
    LIB_FUNCTION("r7UpNm1Po9s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient16invokeSyncMethodEjPKvmPvPmm);
    LIB_FUNCTION("+EQNga+wsPc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient4ctorEv);
    LIB_FUNCTION("2h59YqPcrdM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient4dtorEv);
    LIB_FUNCTION("iRH-NE2evR4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient4InitEPKNS2_6ConfigE);
    LIB_FUNCTION("CGKtxL26XqI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient7ConnectEPKvm);
    LIB_FUNCTION("+xvhXA8Ci4E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClient7DestroyEv);
    LIB_FUNCTION("6aKYLBS8Di8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClientC1Ev);
    LIB_FUNCTION("dqjlsaUX0sc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClientC2Ev);
    LIB_FUNCTION("3LuoWoXJ1WI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClientD0Ev);
    LIB_FUNCTION("DRbjyNom-BE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClientD1Ev);
    LIB_FUNCTION("J1lpiTKAEuk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc10IpmiClientD2Ev);
    LIB_FUNCTION(
        "aQzxfON3l2Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
        _ZN3sce2np3ipc13ServiceClientC1EPNS1_17ServiceIpmiClientEPKNS1_17ServiceClientInfoE);
    LIB_FUNCTION(
        "Mx6wrcdGC2w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
        _ZN3sce2np3ipc13ServiceClientC2EPNS1_17ServiceIpmiClientEPKNS1_17ServiceClientInfoE);
    LIB_FUNCTION("uvYTUK5xYG8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient10DisconnectEv);
    LIB_FUNCTION("fFGPlE0oNhw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient10EndRequestEii);
    LIB_FUNCTION("F2xYmg5DiR4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient11findServiceEi);
    LIB_FUNCTION("G4FYQtsjOX0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient11InitServiceEi);
    LIB_FUNCTION("0rqwC4+sgzU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient11TermServiceEi);
    LIB_FUNCTION("oCx3mVNvqzU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient11WaitRequestEiij);
    LIB_FUNCTION("tQOrMf4KtIo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient12AbortRequestEii);
    LIB_FUNCTION("9aiQo-uRPJY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient12BeginRequestEii);
    LIB_FUNCTION("H35UsHYlhB4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient13CreateRequestEPiiPKvm);
    LIB_FUNCTION("cMj7li0eXgw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient13DeleteRequestEii);
    LIB_FUNCTION("+ZC8QYB-BA8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient13PollEventFlagEijmjPm);
    LIB_FUNCTION("4GZ9O-OrfzE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient13WaitEventFlagEijmjPmj);
    LIB_FUNCTION("q+uCQLffwQE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient14PollEventQueueEiPvm);
    LIB_FUNCTION("bH08FzR5rFU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient15CancelEventFlagEijm);
    LIB_FUNCTION("stUzNgtFmtY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient15RegisterServiceEPKNS1_17ServiceClientInfoE);
    LIB_FUNCTION("fqAS9GQTmOU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient16RegisterServicesEPKNS1_17ServiceClientInfoE);
    LIB_FUNCTION("BJCXJJCi0Zc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient17invokeInitServiceEi);
    LIB_FUNCTION("GuruEy9Q-Zk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient17invokeTermServiceEi);
    LIB_FUNCTION("k9jCtANC+QM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient17UnregisterServiceEi);
    LIB_FUNCTION("8TpAxZoLLRw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient18EndRequestForAsyncEii);
    LIB_FUNCTION("1ONFW86TETY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient19WaitRequestForAsyncEiij);
    LIB_FUNCTION("nQm4o5iOye0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient20AbortRequestForAsyncEii);
    LIB_FUNCTION(
        "ktb6iOBLnd4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
        _ZN3sce2np3ipc17ServiceIpmiClient20BeginRequestForAsyncEiiPN4IPMI6Client12EventNotifeeE);
    LIB_FUNCTION("v5Z2LAKua28", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient21CreateRequestForAsyncEPiiPKvm);
    LIB_FUNCTION("7oJpAd+vJQA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient21DeleteRequestForAsyncEii);
    LIB_FUNCTION("KxlKRHLf9AY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient4ctorEv);
    LIB_FUNCTION("s+dG6iqG7j0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient4dtorEv);
    LIB_FUNCTION("qFdG8Ucfeqg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient4InitEPNS2_6ConfigE);
    LIB_FUNCTION("NTPZ5GZIA6U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient7ConnectEPKvm);
    LIB_FUNCTION("IGngArGbzHo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClient7DestroyEv);
    LIB_FUNCTION("FubuBXanVWk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClientC1Ev);
    LIB_FUNCTION("zARyDXgocuk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClientC2Ev);
    LIB_FUNCTION("PmsH4f3z8Yk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClientD0Ev);
    LIB_FUNCTION("90XdvAqFFn8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClientD1Ev);
    LIB_FUNCTION("agYDXAyL-K8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np3ipc17ServiceIpmiClientD2Ev);
    LIB_FUNCTION("n9pzAHeCCVU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond4ctorEv);
    LIB_FUNCTION("BtXPJQEg41Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond4dtorEv);
    LIB_FUNCTION("wWTqVcTnep8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond4InitEPKcPNS0_5MutexE);
    LIB_FUNCTION("SLPuaDLbeD4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond4WaitEj);
    LIB_FUNCTION("OQiPXR6gfj0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond6SignalEv);
    LIB_FUNCTION("I5uzTXxbziU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond7DestroyEv);
    LIB_FUNCTION("-hchsElmzXY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Cond9SignalAllEv);
    LIB_FUNCTION("3z5EPY-ph14", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4CondC1Ev);
    LIB_FUNCTION("6nW8WXQYRgM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4CondC2Ev);
    LIB_FUNCTION("AKiHGWhC2KU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4CondD0Ev);
    LIB_FUNCTION("yX9ISVXv+0M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4CondD1Ev);
    LIB_FUNCTION("6RQRpTn+-cc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4CondD2Ev);
    LIB_FUNCTION("6r6ssbPbKc4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Path11BuildAppendEPcmcPKcm);
    LIB_FUNCTION("vfBKsg+lKWc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Path12AddDelimiterEPcmc);
    LIB_FUNCTION("BqFx1VLEMPk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Path5ClearEv);
    LIB_FUNCTION("AcG6blobOQE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Path6SetStrEPKcm);
    LIB_FUNCTION("0fwoTW7gqfM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4PathD0Ev);
    LIB_FUNCTION("-3UvpBs-26g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4PathD1Ev);
    LIB_FUNCTION("1nF0eXrBZYM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np4PathD2Ev);
    LIB_FUNCTION("KhoD7EapiYI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time10AddMinutesEl);
    LIB_FUNCTION("PgiCaoqRKKc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time10AddSecondsEl);
    LIB_FUNCTION("vINvzJOaqws", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time12GetUserClockEPS1_);
    LIB_FUNCTION("dLNhHwYyt4c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time15AddMicroSecondsEl);
    LIB_FUNCTION("WZqwoPoMzFA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time15GetNetworkClockEPS1_);
    LIB_FUNCTION("fimORKx4RDg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time20GetDebugNetworkClockEPS1_);
    LIB_FUNCTION("++qSDotsHuE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time7AddDaysEl);
    LIB_FUNCTION("Zc+a6k6i7gY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4Time8AddHoursEl);
    LIB_FUNCTION("Fgm7cz6AX4k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4TimeplERK10SceRtcTick);
    LIB_FUNCTION("F9khEfgTmsE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np4TimeplERKS1_);
    LIB_FUNCTION("I1kBZV6keO4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex4ctorEv);
    LIB_FUNCTION("mo+gaebiE+M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex4dtorEv);
    LIB_FUNCTION("aTNOl9EB4V4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex4InitEPKcj);
    LIB_FUNCTION("VM+CXTW4F-s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex4LockEv);
    LIB_FUNCTION("eYgHIWx0Hco", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex6UnlockEv);
    LIB_FUNCTION("RgGW4f0ox1g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex7DestroyEv);
    LIB_FUNCTION("TJNrs69haak", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5Mutex7TryLockEv);
    LIB_FUNCTION("O1AvlQU33pI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np5MutexC1Ev);
    LIB_FUNCTION("2beu2bHw6qo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np5MutexC2Ev);
    LIB_FUNCTION("omf1GoUEJCA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np5MutexD0Ev);
    LIB_FUNCTION("9zi9FTPol74", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np5MutexD1Ev);
    LIB_FUNCTION("CI7ciM21NXs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np5MutexD2Ev);
    LIB_FUNCTION("uuyEiBHghY4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np5NpEnv8GetNpEnvEPS1_);
    LIB_FUNCTION("-c9QK+CpQLg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Handle10CancelImplEi);
    LIB_FUNCTION("ifqJb-V1QZw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Handle4InitEv);
    LIB_FUNCTION("1atFu71dFAU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Handle7DestroyEv);
    LIB_FUNCTION("KUJtztDMJYY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6HandleC1Ev);
    LIB_FUNCTION("OhpofCxYOJc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6HandleC2Ev);
    LIB_FUNCTION("ZOHgNNSZq4Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6HandleD0Ev);
    LIB_FUNCTION("YWt5S4-cg9c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6HandleD1Ev);
    LIB_FUNCTION("dt0A2cWjwLs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6HandleD2Ev);
    LIB_FUNCTION("1x0jThSUr4w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdaEPv);
    LIB_FUNCTION("4il4PZAZOnQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdaEPvR14SceNpAllocator);
    LIB_FUNCTION("q2USyzLF4kI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdaEPvR16SceNpAllocatorEx);
    LIB_FUNCTION("CnDHI7sU+l0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdlEPv);
    LIB_FUNCTION("05KEwpDf4Ls", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdlEPvR14SceNpAllocator);
    LIB_FUNCTION("iwDNdnEGyhI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectdlEPvR16SceNpAllocatorEx);
    LIB_FUNCTION("V75N47uYdQc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectnaEmR14SceNpAllocator);
    LIB_FUNCTION("bKMVqRcCQ1U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectnaEmR16SceNpAllocatorEx);
    LIB_FUNCTION("0syNkhJANVw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectnwEmR14SceNpAllocator);
    LIB_FUNCTION("orRb69nSo64", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6ObjectnwEmR16SceNpAllocatorEx);
    LIB_FUNCTION("Ehkz-BkTPwI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread12DoThreadMainEv);
    LIB_FUNCTION("3CJl5ewd7-0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread4ctorEv);
    LIB_FUNCTION("-3gV5N2u-sc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread4dtorEv);
    LIB_FUNCTION("EqX45DhWUpo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread4InitEPKcimm);
    LIB_FUNCTION("OoK0Ah0l1ko", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread4InitEPKNS1_5ParamE);
    LIB_FUNCTION("ne77q1GOlF8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread4JoinEPi);
    LIB_FUNCTION("VNKdE2Dgp0Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread5StartEv);
    LIB_FUNCTION("sPti0OkVM8c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread7DestroyEv);
    LIB_FUNCTION("uphWwLZAuXA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread9EntryFuncEPv);
    LIB_FUNCTION("gnwCmkY-V70", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread9GetResultEv);
    LIB_FUNCTION("qy4V8O+snLU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np6Thread9IsRunningEv);
    LIB_FUNCTION("0f3ylOQJwqE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6ThreadC2Ev);
    LIB_FUNCTION("MEYMyfJxWXg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6ThreadD0Ev);
    LIB_FUNCTION("0Q5aKjYErBA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6ThreadD1Ev);
    LIB_FUNCTION("6750DaF5Pas", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, _ZN3sce2np6ThreadD2Ev);
    LIB_FUNCTION("xxOTJpEyoj4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout10IsTimedoutEv);
    LIB_FUNCTION("Zw3QlKu49eM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout11CalloutFuncEPv);
    LIB_FUNCTION("14PDhhMEBKY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout4StopEv);
    LIB_FUNCTION("TDuC6To9HJ8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout5StartEjPNS1_7HandlerE);
    LIB_FUNCTION("r0PYNWZLZS8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout5StartEmPNS1_7HandlerE);
    LIB_FUNCTION("3ErXia+y89M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7Callout9IsStartedEv);
    LIB_FUNCTION("XEXFdmQj5oI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7CalloutC1EPNS0_14CalloutContextE);
    LIB_FUNCTION("Bpay3NjseSU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7CalloutC2EPNS0_14CalloutContextE);
    LIB_FUNCTION("Fx2UwoQVVmo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7CalloutD0Ev);
    LIB_FUNCTION("kUitiIVR43g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7CalloutD1Ev);
    LIB_FUNCTION("ebomQLbpptw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7CalloutD2Ev);
    LIB_FUNCTION("YtzL-Rso9bk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUri5BuildEPKS1_PcmPmj);
    LIB_FUNCTION("Xp92SsA5atA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUri5ParseEPS1_PKc);
    LIB_FUNCTION("LL9z5QvmwaA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUriC1EP16SceNpAllocatorEx);
    LIB_FUNCTION("q4G7qxTJWps", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUriC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("w+C8QXqZKSw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUriD0Ev);
    LIB_FUNCTION("wSCKvDDBPy4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUriD1Ev);
    LIB_FUNCTION("D-dT+vERWmU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7HttpUriD2Ev);
    LIB_FUNCTION("oaSKGgwTWG0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf14CheckinForReadEm);
    LIB_FUNCTION("78yvwepeL7U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf15CheckinForWriteEm);
    LIB_FUNCTION("d8NGGmSEFfU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf15CheckoutForReadEPm);
    LIB_FUNCTION("E2QFpAcDPq4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf16CheckoutForWriteEPm);
    LIB_FUNCTION("1P-MUvbtyTM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf4ctorEv);
    LIB_FUNCTION("rvz8xYxhMW0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf4dtorEv);
    LIB_FUNCTION("IL3Wk7QuRhA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf4InitEPvm);
    LIB_FUNCTION("kDaQLJv89bs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf4PeekEmPvm);
    LIB_FUNCTION("Mg-IhL6SWfg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf4ReadEPvm);
    LIB_FUNCTION("IZOGdJ+LFFU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf5ClearEv);
    LIB_FUNCTION("8Y5OOBb0B5Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf5WriteEPKvm);
    LIB_FUNCTION("u-TlLaJUJEA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBuf7DestroyEv);
    LIB_FUNCTION("L5BnZpuQImk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBufC1Ev);
    LIB_FUNCTION("e2a1ZA+lJC4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBufC2Ev);
    LIB_FUNCTION("hfJ1gGLgvq8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBufD0Ev);
    LIB_FUNCTION("7w+LeZ5ymys", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBufD1Ev);
    LIB_FUNCTION("9+NmoosRoBA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np7RingBufD2Ev);
    LIB_FUNCTION("d+xJZ63-wrc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFile4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("jcPO4bt5i3o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFile5CloseEv);
    LIB_FUNCTION("RXdPqxVnrvo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFileC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("T2w3ndcG-+Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFileD0Ev);
    LIB_FUNCTION("6fomUWNk6Xc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFileD1Ev);
    LIB_FUNCTION("WAat5MtCKpc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8HttpFileD2Ev);
    LIB_FUNCTION("uDyILPgHF9Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonBool5ClearEv);
    LIB_FUNCTION("FdpYFbq5C3Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonBool7SetBoolEb);
    LIB_FUNCTION("mFZezLIogNI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonFile5CloseEv);
    LIB_FUNCTION("hqPavTyQlNg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonFileD0Ev);
    LIB_FUNCTION("wzqAM7IYGzU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonFileD1Ev);
    LIB_FUNCTION("QFYVZvAJNC8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonFileD2Ev);
    LIB_FUNCTION("88GKkivBFhI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8JsonNull5ClearEv);
    LIB_FUNCTION("WcLP8wPB9X4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommId5BuildERKS1_Pcm);
    LIB_FUNCTION("LnjjzlJ+L5c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommId5ClearEv);
    LIB_FUNCTION("1TjLUwirok0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommId5ParseEPS1_PKc);
    LIB_FUNCTION("UrJocI5M8GY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommId5ParseEPS1_PKcm);
    LIB_FUNCTION("To1XvNOzjo0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC1ERK20SceNpCommunicationId);
    LIB_FUNCTION("N6SkkX1GkFU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC1ERKS1_);
    LIB_FUNCTION("AQyiYChNI0c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC1Ev);
    LIB_FUNCTION("WywlusFissg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC2ERK20SceNpCommunicationId);
    LIB_FUNCTION("rB0oqLSjH6g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC2ERKS1_);
    LIB_FUNCTION("BBtBjx9-bMI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdC2Ev);
    LIB_FUNCTION("XeCZTzqIk2k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdD0Ev);
    LIB_FUNCTION("EPJbX73AVeU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdD1Ev);
    LIB_FUNCTION("hP18CDS6eBU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8NpCommIdD2Ev);
    LIB_FUNCTION("5WuiSZkU3mg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8Selector4InitEPKc);
    LIB_FUNCTION("2HkOOhiWK3M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8SelectorD0Ev);
    LIB_FUNCTION("asZdig1mPlA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8SelectorD1Ev);
    LIB_FUNCTION("PA9VYFAVKIE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8SelectorD2Ev);
    LIB_FUNCTION("2YbS+GhInZQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem10SetPendingEv);
    LIB_FUNCTION("XUCjhejJvPc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem10SetRunningEv);
    LIB_FUNCTION("-91vFSqiuKw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem11SetFinishedEi);
    LIB_FUNCTION("zepqHjfGe0M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem14FinishCallbackEv);
    LIB_FUNCTION("3rGzxcMK-Mg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem15RemoveFromQueueEv);
    LIB_FUNCTION("Oq5aepLkEWg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem6CancelEi);
    LIB_FUNCTION("gnh2cpEgSS8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItem9BindQueueEPNS0_9WorkQueueEi);
    LIB_FUNCTION("HldN461O2Dw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItemC2EPKc);
    LIB_FUNCTION("Y-I66cSNp+A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItemD0Ev);
    LIB_FUNCTION("dnwItoXLoy4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItemD1Ev);
    LIB_FUNCTION("ga4OW9MGahU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np8WorkItemD2Ev);
    LIB_FUNCTION("8i-vOVRVt5w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag3SetEm);
    LIB_FUNCTION("vhbvgH7wWiE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag4ctorEv);
    LIB_FUNCTION("5nM4Yy92Qwg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag4dtorEv);
    LIB_FUNCTION("5Wy+JxpCBxg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag4OpenEPKc);
    LIB_FUNCTION("37Rd2JS+FCM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag4PollEmjPm);
    LIB_FUNCTION("1s+c3SG0WYc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag4WaitEmjPmj);
    LIB_FUNCTION("03UlDLFsTfw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag5ClearEm);
    LIB_FUNCTION("wJ-k9+UShJg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag6CancelEm);
    LIB_FUNCTION("amFi-Av19hU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag6CreateEPKcj);
    LIB_FUNCTION("QlaBcxSFPZI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlag7DestroyEv);
    LIB_FUNCTION("cMOgkE2M2e8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlagC1Ev);
    LIB_FUNCTION("Uv1IQpTWecw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlagC2Ev);
    LIB_FUNCTION("uHOOEbuzjEQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlagD0Ev);
    LIB_FUNCTION("WWW4bvT-rSw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlagD1Ev);
    LIB_FUNCTION("RpWWfCEs9xA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9EventFlagD2Ev);
    LIB_FUNCTION("jDDvll2aQpQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans10SetTimeoutEPKNS1_12TimeoutParamE);
    LIB_FUNCTION("+hKyaJJCE+0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans11SendRequestEPNS0_6HandleEPKvm);
    LIB_FUNCTION("EhLaOnhdcXo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans12RecvResponseEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("fV+Q5a6p+zQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans12SkipResponseEPNS0_6HandleE);
    LIB_FUNCTION("Qfsmqs-bHeY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans16AddRequestHeaderEPKcS3_);
    LIB_FUNCTION("6bYsRATI3tQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans16SetRequestHeaderEPKcS3_);
    LIB_FUNCTION("WoFp77mNyw0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans21GetResponseStatusCodeEPNS0_6HandleEPi);
    LIB_FUNCTION("RJOlguLEy-E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans21SetRequestContentTypeEPKc);
    LIB_FUNCTION("ws3x3yjUyeE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans23SetRequestContentLengthEm);
    LIB_FUNCTION("YW09CP0Vrtw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans24GetResponseContentLengthEPNS0_6HandleEPbPm);
    LIB_FUNCTION("JEYp0T1VC58", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans4InitERKNS0_12HttpTemplateEPNS0_18HttpConnectionPoolEiPKcm);
    LIB_FUNCTION(
        "O+FeLkOM7w0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
        _ZN3sce2np9HttpTrans4InitERKNS0_12HttpTemplateEPNS0_18HttpConnectionPoolEiPKcS8_tS8_m);
    LIB_FUNCTION("aWo+7jvpllY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("cocNRQpq+NA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("2e9GLlHTKA4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTrans7DestroyEv);
    LIB_FUNCTION("sqNxD6H5ZOQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTransC1EP16SceNpAllocatorEx);
    LIB_FUNCTION("HEeXBdgvJI4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTransC2EP16SceNpAllocatorEx);
    LIB_FUNCTION("Pe9fHKX7krE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTransD0Ev);
    LIB_FUNCTION("ls8yIODZmzc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTransD1Ev);
    LIB_FUNCTION("GSVe-aaTiEg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9HttpTransD2Ev);
    LIB_FUNCTION("4cIJxNKQK5g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonArray12AddItemArrayEPPS1_);
    LIB_FUNCTION("cWsZswBMjqg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonArray5ClearEv);
    LIB_FUNCTION("aCZjveAsynw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValue12GetItemValueEi);
    LIB_FUNCTION("aIV+HI6llz4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValue13GetFieldValueEiPPKc);
    LIB_FUNCTION("BDie4qEtKuA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValue13GetFieldValueEPKc);
    LIB_FUNCTION("LotC9rVP3Lo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValueD0Ev);
    LIB_FUNCTION("hBuLbn3mGBw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValueD1Ev);
    LIB_FUNCTION("FfSNfBmn+K8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9JsonValueD2Ev);
    LIB_FUNCTION("PsP6LYRZ7Dc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("Flyyg6hzUOM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile4SeekEliPl);
    LIB_FUNCTION("YtvLEI7uZRI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile4SyncEv);
    LIB_FUNCTION("9q+h2q5YprU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile5CloseEv);
    LIB_FUNCTION("0xL7AwgxphE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("haDbtVOmaao", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile6RemoveEPKc);
    LIB_FUNCTION("Sgo7wy9okFI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFile8TruncateEl);
    LIB_FUNCTION("QWlZu1JZOww", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFileC1Ev);
    LIB_FUNCTION("HP4jsVYqBKg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFileC2Ev);
    LIB_FUNCTION("-n0CR0QxhnY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFileD0Ev);
    LIB_FUNCTION("3eoh4hjcYag", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFileD1Ev);
    LIB_FUNCTION("s-C88O6Y8iU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9LocalFileD2Ev);
    LIB_FUNCTION("euE6Yo5hkrY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleId5BuildERKS1_Pcm);
    LIB_FUNCTION("a76a3D9Adts", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleId5ClearEv);
    LIB_FUNCTION("4O8lYvForpk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleId5ParseEPS1_PKc);
    LIB_FUNCTION("-swgMjedLUQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleId5ParseEPS1_PKcm);
    LIB_FUNCTION("Fcvdbqpwpnw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC1ERK12SceNpTitleId);
    LIB_FUNCTION("wd+YWDKMTQE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC1ERKS1_);
    LIB_FUNCTION("-Ja2aT6A3fg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC1Ev);
    LIB_FUNCTION("9n60S+t4Cxs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC2ERK12SceNpTitleId);
    LIB_FUNCTION("IefAhNUAivM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC2ERKS1_);
    LIB_FUNCTION("OL7DU1kkm+4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdC2Ev);
    LIB_FUNCTION("rFcQRK+GMcQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdD0Ev);
    LIB_FUNCTION("TGJ5bE+Fb1s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdD1Ev);
    LIB_FUNCTION("XKVRBLdw+7I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9NpTitleIdD2Ev);
    LIB_FUNCTION("zurkNUps5o8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObject6AddRefEv);
    LIB_FUNCTION("5tYi1l9CXD0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObject7ReleaseEv);
    LIB_FUNCTION("brUrttJp6MM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObjectC1Ev);
    LIB_FUNCTION("JRtw5pROOiM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObjectC2Ev);
    LIB_FUNCTION("8DrClRz7Z2U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObjectD0Ev);
    LIB_FUNCTION("lPQzOhwPjuw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObjectD1Ev);
    LIB_FUNCTION("417JucZaE3g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9RefObjectD2Ev);
    LIB_FUNCTION("EFffsPLsOio", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9Semaphore4OpenEPKc);
    LIB_FUNCTION("hQLw6eE4O44", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9Semaphore4WaitEj);
    LIB_FUNCTION("wcOCedFKan4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9Semaphore6CreateEiiPKc);
    LIB_FUNCTION("b7qnGORh+H4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9Semaphore6SignalEv);
    LIB_FUNCTION("Es-CwSVnalY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9Semaphore7DestroyEv);
    LIB_FUNCTION("Tuth2BRl4x0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9SemaphoreC1Ev);
    LIB_FUNCTION("8k1rNqvczTc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9SemaphoreC2Ev);
    LIB_FUNCTION("S6luQz76AQ4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9SemaphoreD0Ev);
    LIB_FUNCTION("nW9XeX3eokI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9SemaphoreD1Ev);
    LIB_FUNCTION("OukNoRur97E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9SemaphoreD2Ev);
    LIB_FUNCTION("F2umEBpQFHc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue11GetItemByIdEi);
    LIB_FUNCTION("wM4q1JMisvA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue15GetFinishedItemENS0_14WorkItemStatusE);
    LIB_FUNCTION("UYAD7sUQcYU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue16WorkItemFinishedEPNS0_8WorkItemEi);
    LIB_FUNCTION("-9cU3y6rXVM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue17ProcFinishedItemsENS0_14WorkItemStatusE);
    LIB_FUNCTION("ovc4ZvD0YjY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue18RemoveFinishedItemEPNS0_8WorkItemE);
    LIB_FUNCTION("vPju3W13byw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue18WaitForPendingItemEPPNS0_8WorkItemEPb);
    LIB_FUNCTION("XMIv42L5bEA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue4ctorEv);
    LIB_FUNCTION("wESN-qrVhOU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue4dtorEv);
    LIB_FUNCTION("+dGO+GS2ZXQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue4InitEPKcimm);
    LIB_FUNCTION("U0YoWwgg8aI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue4InitEPKNS0_6Thread5ParamE);
    LIB_FUNCTION("4DE+nnCVRPA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue4StopEv);
    LIB_FUNCTION("VnQolo6vTr4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue5StartEv);
    LIB_FUNCTION("laqZEULcfgw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue6CancelEii);
    LIB_FUNCTION("CznMfhTIvVY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue6IsInitEv);
    LIB_FUNCTION("NeopmYshD0U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue7DestroyEv);
    LIB_FUNCTION("KQSxXJBepQ4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue7EnqueueEiPNS0_8WorkItemE);
    LIB_FUNCTION("zmOmSLnqlBQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue9CancelAllEi);
    LIB_FUNCTION("eTy3L1azX4E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueue9IsRunningEv);
    LIB_FUNCTION("X6NVkdpRnog", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueueC1Ev);
    LIB_FUNCTION("p+bd65J177I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueueC2Ev);
    LIB_FUNCTION("uyNO0GnFhPw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueueD0Ev);
    LIB_FUNCTION("1QFKnDJxk3A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueueD1Ev);
    LIB_FUNCTION("AIDhc3KCK7w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2np9WorkQueueD2Ev);
    LIB_FUNCTION("XLpPRMl5jro", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("6jHOZ6fItFU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERK12SceNpTitleIdRKNS0_9NpTitleIdE);
    LIB_FUNCTION("i+xzwYeeEtk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERK16SceNpTitleSecretRKNS0_13NpTitleSecretE);
    LIB_FUNCTION("ZWZ9KqoIvQY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERK20SceNpCommunicationIdRKNS0_8NpCommIdE);
    LIB_FUNCTION("Vsj50ZwNUFM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_13NpTitleSecretERK16SceNpTitleSecret);
    LIB_FUNCTION("WM5DPO-LryU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_13NpTitleSecretES3_);
    LIB_FUNCTION("ps246w9eXI8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("UVLmT9lzRYA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_4TimeES3_);
    LIB_FUNCTION("WaNQzws1ATU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_8NpCommIdERK20SceNpCommunicationId);
    LIB_FUNCTION("E-mYAG-aa1A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_8NpCommIdES3_);
    LIB_FUNCTION("FmDmhB16wwE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_9NpTitleIdERK12SceNpTitleId);
    LIB_FUNCTION("niXN2N4o3yY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npeqERKNS0_9NpTitleIdES3_);
    LIB_FUNCTION("gKruhA35EXQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgeERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("1mnghWFX0wQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgeERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("svAQxJ3yow4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgeERKNS0_4TimeES3_);
    LIB_FUNCTION("oVZ6spoeeN0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgtERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("snloJp6qQCc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgtERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("EFES6UR65oU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npgtERKNS0_4TimeES3_);
    LIB_FUNCTION("UIrMxV07mL0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npleERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("cAeFZE72SXU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npleERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("ttA9TcO06uA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npleERKNS0_4TimeES3_);
    LIB_FUNCTION("rVtImV4rxSA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npltERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("nVB1Nsjwpj0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npltERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("d0zSLZMER34", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npltERKNS0_4TimeES3_);
    LIB_FUNCTION("MVY+jtY-WiQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERK10SceRtcTickRKNS0_4TimeE);
    LIB_FUNCTION("tDs31ASQGV8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERK12SceNpTitleIdRKNS0_9NpTitleIdE);
    LIB_FUNCTION("OwsjgCQyZUI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERK16SceNpTitleSecretRKNS0_13NpTitleSecretE);
    LIB_FUNCTION("O5QkjyiPM4c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERK20SceNpCommunicationIdRKNS0_8NpCommIdE);
    LIB_FUNCTION("7b5y1XSa+KQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_13NpTitleSecretERK16SceNpTitleSecret);
    LIB_FUNCTION("zbliTwZKRyU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_13NpTitleSecretES3_);
    LIB_FUNCTION("yXMjXN--3rY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_4TimeERK10SceRtcTick);
    LIB_FUNCTION("cnoM7EjlLe4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_4TimeES3_);
    LIB_FUNCTION("SM7OEf11LCA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_8NpCommIdERK20SceNpCommunicationId);
    LIB_FUNCTION("QQCqBHk79sI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_8NpCommIdES3_);
    LIB_FUNCTION("ONgEITYl9mA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_9NpTitleIdERK12SceNpTitleId);
    LIB_FUNCTION("9pp9-dwqIHM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZN3sce2npneERKNS0_9NpTitleIdES3_);
    LIB_FUNCTION("KyDWNwpREH4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10Cancelable6IsInitEv);
    LIB_FUNCTION("VI8AHrfLdqY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10EventQueue6IsInitEv);
    LIB_FUNCTION("jxPY-0x8e-M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10EventQueue7IsEmptyEv);
    LIB_FUNCTION("COxqqhvLSyM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber5CloneEP16SceNpAllocatorEx);
    LIB_FUNCTION("m+dAaZ5pyO4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber6GetNumEPcm);
    LIB_FUNCTION("Sk8AdNQUDm8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber6GetNumEPi);
    LIB_FUNCTION("nHgo2VpnCB8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber6GetNumEPj);
    LIB_FUNCTION("Agsyrf4L8uA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber6GetNumEPl);
    LIB_FUNCTION("P2cGbJ5nD1w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np10JsonNumber6GetNumEPm);
    LIB_FUNCTION("EcboqmwkrMY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np9JsonArray5CloneEP16SceNpAllocatorEx);
    LIB_FUNCTION("JcAsZlyr3Mo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np9JsonValue12GetItemValueEi);
    LIB_FUNCTION("XZTZqqSVGlY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np9NpTitleId7IsEmptyEv);
    LIB_FUNCTION("sreH33xjV0A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZNK3sce2np9Semaphore6IsInitEv);
    LIB_FUNCTION("QwO4sr6XzSY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np10MemoryFile5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("ojBk-UJxzWw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np10MemoryFileD0Ev);
    LIB_FUNCTION("8S1mWU-N9kM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np10MemoryFileD1Ev);
    LIB_FUNCTION("eRlqlofFKYg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9HttpTrans5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("zWIFe+d77PU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9HttpTransD0Ev);
    LIB_FUNCTION("GG1Y+vBUkdU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9HttpTransD1Ev);
    LIB_FUNCTION("+3ySpB1buMs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9LocalFile5WriteEPNS0_6HandleEPKvmPm);
    LIB_FUNCTION("hSnLhjGefsU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9LocalFileD0Ev);
    LIB_FUNCTION("q3s6++iIzjE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn16_N3sce2np9LocalFileD1Ev);
    LIB_FUNCTION("E6GYo9uzjds", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np10MemoryFile4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("7bzUdBtIQhE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np10MemoryFileD0Ev);
    LIB_FUNCTION("lNs-oTKpG9s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np10MemoryFileD1Ev);
    LIB_FUNCTION("xDrWJARfCbk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np6Handle10CancelImplEi);
    LIB_FUNCTION("YqMS-iAjFY8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np6HandleD0Ev);
    LIB_FUNCTION("lUsG1QfgVN4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np6HandleD1Ev);
    LIB_FUNCTION("G+v692ul7MA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9HttpTrans4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("sGhCzaJf+jQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9HttpTransD0Ev);
    LIB_FUNCTION("PUqCtFwnNvA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9HttpTransD1Ev);
    LIB_FUNCTION("NtsHoOq2ao4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9LocalFile4ReadEPNS0_6HandleEPvmPm);
    LIB_FUNCTION("Gh35wbyg4U8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9LocalFileD0Ev);
    LIB_FUNCTION("kD3l0P19Wzg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZThn8_N3sce2np9LocalFileD1Ev);
    LIB_FUNCTION("IvTsS4VJq1w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np10JsonNumberE);
    LIB_FUNCTION("aLGD1kOLQXE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np10JsonObjectE);
    LIB_FUNCTION("1At86OClqtY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np10JsonStringE);
    LIB_FUNCTION("jsHe99x6l0w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np8JsonBoolE);
    LIB_FUNCTION("A742Lh-FnVE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np8JsonNullE);
    LIB_FUNCTION("FfXZGW1TMvo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np8SelectorE);
    LIB_FUNCTION("0qrLVqNUn2Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np9JsonArrayE);
    LIB_FUNCTION("S8TLtKfZCfc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 _ZTVN3sce2np9JsonValueE);
    LIB_FUNCTION("MWPOkqzYss0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpAllocateKernelMemoryNoAlignment);
    LIB_FUNCTION("gMlY6eewr-c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpAllocateKernelMemoryWithAlignment);
    LIB_FUNCTION("jGF+MaB4b-M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpArchInit);
    LIB_FUNCTION("UskWpVWxSvg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpArchTerm);
    LIB_FUNCTION("+9+kKMY9YIw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpAtomicCas32);
    LIB_FUNCTION("Yohe0MMDfj0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpAtomicDec32);
    LIB_FUNCTION("pfJgSA4jO3M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpAtomicInc32);
    LIB_FUNCTION("l67qBmMmKP4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpBase64Decoder);
    LIB_FUNCTION("pu39pU8UgCo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpBase64Encoder);
    LIB_FUNCTION("a5IfPlpchXI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpBase64GetDecodeSize);
    LIB_FUNCTION("moGcgMNTHvQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpBase64UrlDecoder);
    LIB_FUNCTION("IeNj+OcWgU8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpBase64UrlEncoder);
    LIB_FUNCTION("7BjZKcN+oZ4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpBase64UrlGetDecodeSize);
    LIB_FUNCTION("9+m5nRdJ-wQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCalloutInitCtx);
    LIB_FUNCTION("fClnlkZmA6k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpCalloutStartOnCtx);
    LIB_FUNCTION("lpr66Gby8dQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpCalloutStartOnCtx64);
    LIB_FUNCTION("in19gH7G040", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCalloutStopOnCtx);
    LIB_FUNCTION("AqJ4xkWsV+I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCalloutTermCtx);
    LIB_FUNCTION("kb2thTuS8t8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCancelEventFlag);
    LIB_FUNCTION("9pLoHoPMxeg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpClearEventFlag);
    LIB_FUNCTION("+nmn+Z0nWDo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCloseEventFlag);
    LIB_FUNCTION("8hPzfjZzV88", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCloseSema);
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCmpOnlineId);
    LIB_FUNCTION("1a+iY5YUJcI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondDestroy);
    LIB_FUNCTION("q2tsVO3lM4A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondInit);
    LIB_FUNCTION("uMJFOA62mVU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondSignal);
    LIB_FUNCTION("bsjWg59A7aE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondSignalAll);
    LIB_FUNCTION("bAHIOyNnx5Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondSignalTo);
    LIB_FUNCTION("ss2xO9IJxKQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondTimedwait);
    LIB_FUNCTION("fZShld2PQ7w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCondWait);
    LIB_FUNCTION("6jFWpAfqAcc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCreateEventFlag);
    LIB_FUNCTION("LHZtCT2W1Pw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCreateSema);
    LIB_FUNCTION("fhJ5uKzcn0w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpCreateThread);
    LIB_FUNCTION("90pmGqDK4BI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpDbgAssignDebugId);
    LIB_FUNCTION("Etq15-l9yko", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpDbgDumpBinary);
    LIB_FUNCTION("ZaKa5x61hGA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpDbgDumpText);
    LIB_FUNCTION("sjnIeFCuTD0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpDeleteEventFlag);
    LIB_FUNCTION("xPrF2nGPBXQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpDeleteSema);
    LIB_FUNCTION("OQTweRLgFr8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpEventGetCurrentNetworkTick);
    LIB_FUNCTION("vjwlDmsGtME", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpFreeKernelMemory);
    LIB_FUNCTION("QmDEFikd3VA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpGetNavSdkVersion);
    LIB_FUNCTION("sXVQUIGmk2U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpGetPlatformType);
    LIB_FUNCTION("Z3mnqcGmf8E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpGetProcessId);
    LIB_FUNCTION("pJlGhXEt5CU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpGetRandom);
    LIB_FUNCTION("Pglk7zFj0DI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpGetSdkVersion);
    LIB_FUNCTION("ljqnF0hmLjo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGetSdkVersionUInt);
    LIB_FUNCTION("PVVsRmMkO1g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGetSystemClockUsec);
    LIB_FUNCTION("-gN6uE+zWng", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGlobalHeapGetAllocator);
    LIB_FUNCTION("VUHUasztbUY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGlobalHeapGetAllocatorEx);
    LIB_FUNCTION("P4YpPziLBd4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGlobalHeapGetAllocatorExPtr);
    LIB_FUNCTION("DI5n4aOdxmk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpGlobalHeapGetAllocatorPtr);
    LIB_FUNCTION("wVdn78HKc30", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHeapDestroy);
    LIB_FUNCTION("lvek8w7yqyE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHeapGetAllocator);
    LIB_FUNCTION("2jdHoPpS+W0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHeapGetStat);
    LIB_FUNCTION("B+yGIX1+BTI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHeapInit);
    LIB_FUNCTION("evz0-93ucJc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHeapShowStat);
    LIB_FUNCTION("Hvpr+otU4bo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpHexToInt);
    LIB_FUNCTION("5y0wMPQkaeU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpInt32ToStr);
    LIB_FUNCTION("HoPC33siDD4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpInt64ToStr);
    LIB_FUNCTION("G6qytFoBJ-w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpIntGetPlatformType);
    LIB_FUNCTION("fY4XQoA20i8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpIntIsOnlineIdString);
    LIB_FUNCTION("hkeX9iuCwlI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpIntIsValidOnlineId);
    LIB_FUNCTION("X6emt+LbSEI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpIntSetPlatformType);
    LIB_FUNCTION("TWPY1x1Atys", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpIntToHex);
    LIB_FUNCTION("kgDwlmy78k0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpIpc2ClientInit);
    LIB_FUNCTION("CI2p6Viee9w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpIpc2ClientTerm);
    LIB_FUNCTION("EjMsfO3GCIA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJoinThread);
    LIB_FUNCTION("vJGDnNh4I0g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParse);
    LIB_FUNCTION("RgfCYkjW7As", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParseBuf);
    LIB_FUNCTION("SnAdybtBK3o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParseBufInit);
    LIB_FUNCTION("p5ZkSMRR7AU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParseEx);
    LIB_FUNCTION("nhgjiwPUIzI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParseExInit);
    LIB_FUNCTION("teVnFAL6GNY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpJsonParseInit);
    LIB_FUNCTION("zNb6IxegrCE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondDestroy);
    LIB_FUNCTION("++eqYdzB8Go", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondInit);
    LIB_FUNCTION("Xkn6VoN-wuQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondSignal);
    LIB_FUNCTION("FJ4DCt8VzVE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondSignalAll);
    LIB_FUNCTION("Bwi+EP8VQ+g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondSignalTo);
    LIB_FUNCTION("ExeLuE3EQCQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwCondWait);
    LIB_FUNCTION("4zxevggtYrQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwMutexDestroy);
    LIB_FUNCTION("1CiXI-MyEKs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwMutexInit);
    LIB_FUNCTION("18j+qk6dRwk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwMutexLock);
    LIB_FUNCTION("hp0kVgu5Fxw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwMutexTryLock);
    LIB_FUNCTION("CQG2oyx1-nM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpLwMutexUnlock);
    LIB_FUNCTION("dfXSH2Tsjkw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpMemoryHeapDestroy);
    LIB_FUNCTION("FaMNvjMA6to", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpMemoryHeapGetAllocator);
    LIB_FUNCTION("xHAiSVEEjSI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpMemoryHeapGetAllocatorEx);
    LIB_FUNCTION("kZizwrFvWZY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMemoryHeapInit);
    LIB_FUNCTION("lQ11BpMM4LU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMutexDestroy);
    LIB_FUNCTION("uEwag-0YZPc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMutexInit);
    LIB_FUNCTION("r9Bet+s6fKc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMutexLock);
    LIB_FUNCTION("DuslmoqQ+nk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMutexTryLock);
    LIB_FUNCTION("oZyb9ktuCpA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpMutexUnlock);
    LIB_FUNCTION("5DkyduAF2rs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpOpenEventFlag);
    LIB_FUNCTION("-blITIdtUd0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpOpenSema);
    LIB_FUNCTION("ZoXUrTiwKNw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpPanic);
    LIB_FUNCTION("9YmBJ8KF9eI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpPollEventFlag);
    LIB_FUNCTION("xmF0yIF4iXc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpPollSema);
    LIB_FUNCTION("VMjIo2Z-aW0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpRtcConvertToPosixTime);
    LIB_FUNCTION("W0YWLVDndx0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpRtcFormatRFC3339);
    LIB_FUNCTION("LtkeQwMIEWY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpRtcParseRFC3339);
    LIB_FUNCTION("0lZHbA-HRD0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpServerErrorJsonGetErrorCode);
    LIB_FUNCTION("cRabutqUG7c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpServerErrorJsonMultiGetErrorCode);
    LIB_FUNCTION("WSQxnAVLKgw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpServerErrorJsonParse);
    LIB_FUNCTION("UbStlMKTBeU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpServerErrorJsonParseInit);
    LIB_FUNCTION("hbe+DdooIi4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpServerErrorJsonParseMultiInit);
    LIB_FUNCTION("29ftOGIrUCo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpSetEventFlag);
    LIB_FUNCTION("m9JzZSoDVFY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpSetPlatformType);
    LIB_FUNCTION("-W28+9p1CKI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpSignalSema);
    LIB_FUNCTION("i5TP5NLmkoQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrBuildHex);
    LIB_FUNCTION("ivnnssCwjGI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrcpyToBuf);
    LIB_FUNCTION("PHrpHMSU8Cs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrncpyToBuf);
    LIB_FUNCTION("h1SWCcBdImo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrnParseHex);
    LIB_FUNCTION("DUHzVPNlugg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrParseHex);
    LIB_FUNCTION("fElyBSn-l24", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrToInt32);
    LIB_FUNCTION("CwqYdG4TrjA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrToInt64);
    LIB_FUNCTION("uj86YxCYid0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrToUInt32);
    LIB_FUNCTION("Ted2YU9lv94", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpStrToUInt64);
    LIB_FUNCTION("yvaNTRiKXmo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpThreadGetId);
    LIB_FUNCTION("rRN89jBArEM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUInt32ToStr);
    LIB_FUNCTION("QjNUYQbGoHA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUInt64ToStr);
    LIB_FUNCTION("Gh74vNl06sg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUserGetUserIdList);
    LIB_FUNCTION("N3tAHlBnowE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilBuildTitleId);
    LIB_FUNCTION("4mEAk-UKVNw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilCanonicalizeNpIdForPs4);
    LIB_FUNCTION("N3FB4r8JoRE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilCanonicalizeNpIdForPsp2);
    LIB_FUNCTION("xPRHNaD3kTc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilCmpAccountId);
    LIB_FUNCTION("owm52JoZ8uc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetDateSetAuto);
    LIB_FUNCTION("1Gfhi+tZ9IE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetDbgCommerce);
    LIB_FUNCTION("kBON3bAtfGs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilGetEnv);
    LIB_FUNCTION("MUj0IV6XFGs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetFakeDisplayNameMode);
    LIB_FUNCTION("O86rgZ2azfg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetFakeRateLimit);
    LIB_FUNCTION("FrxliFYAO8Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetIgnoreNpTitleId);
    LIB_FUNCTION("GRvK1ZE+FEQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilGetNpDebug);
    LIB_FUNCTION("OFiFmfsADas", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetNpLanguageCode);
    LIB_FUNCTION("X9CqyP164Hc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetNpLanguageCode2);
    LIB_FUNCTION("Fxux7Ob+Ynk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetNpLanguageCode2Str);
    LIB_FUNCTION("RfiA17kV+xs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetNpLanguageCodeStr);
    LIB_FUNCTION("OA8f3KF9JsM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetNpTestPatch);
    LIB_FUNCTION("KCk4OGu8+sc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilGetNthChar);
    LIB_FUNCTION("fB5hE65pzbU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetShareTitleCheck);
    LIB_FUNCTION("SXUNKr9Zkv0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetSystemLanguage);
    LIB_FUNCTION("AjzLvR0g5Zs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilGetTrcNotify);
    LIB_FUNCTION("pmHBFJyju9E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetWebApi2FakeRateLimit);
    LIB_FUNCTION("ZRxKp9vjcNc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetWebApi2FakeRateLimitTarget);
    LIB_FUNCTION("4CqfNm3pisU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilGetWebTraceSetting);
    LIB_FUNCTION("ajoqGz0D9Dw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilHttpUrlEncode);
    LIB_FUNCTION("458yjI+OECI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilJidToNpId);
    LIB_FUNCTION("EftEB4kmkSg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilJsonEscape);
    LIB_FUNCTION("vj04qzp7uKY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilJsonGetOneChar);
    LIB_FUNCTION("4YJ5gYtRAAE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilJsonUnescape);
    LIB_FUNCTION("KyB1IAY2BiU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilNpIdToJid);
    LIB_FUNCTION("c+ssxRf1Si0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilNumChars);
    LIB_FUNCTION("oz2SlXNAnuI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilParseJid);
    LIB_FUNCTION("EfnfZtjjyR0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilParseTitleId);
    LIB_FUNCTION("okX7IjW0QsI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilSerializeJid);
    LIB_FUNCTION("5bBPLZV49kY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilXmlEscape);
    LIB_FUNCTION("Ls4eWDrbNmg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1,
                 sceNpUtilXmlGetOneChar);
    LIB_FUNCTION("+0rj9KhmYb0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpUtilXmlUnescape);
    LIB_FUNCTION("ZbdPHUm7jOY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpWaitEventFlag);
    LIB_FUNCTION("6adrFGe2cpU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpWaitSema);
    LIB_FUNCTION("fEcrs9UPPyo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpXmlParse);
    LIB_FUNCTION("MCLGkfBmw4c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, sceNpXmlParseInit);
    LIB_FUNCTION("AP1XjC3ZZt8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_00FD578C2DD966DF);
    LIB_FUNCTION("ATGi6oBon0w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0131A2EA80689F4C);
    LIB_FUNCTION("AUQ8VIY73SA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_01443C54863BDD20);
    LIB_FUNCTION("AbxVvcXAra0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_01BC55BDC5C0ADAD);
    LIB_FUNCTION("AdHs9XUPQOg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_01D1ECF5750F40E8);
    LIB_FUNCTION("AgpHmnT1+6w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_020A479A74F5FBAC);
    LIB_FUNCTION("Akr14dlHKrU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_024AF5E1D9472AB5);
    LIB_FUNCTION("AnxdSIcTprM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_027C5D488713A6B3);
    LIB_FUNCTION("Av6dlMaFg1U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_02FE9D94C6858355);
    LIB_FUNCTION("BB808ccNFcE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_041F34F1C70D15C1);
    LIB_FUNCTION("BTCx0nYRQkg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0530B1D276114248);
    LIB_FUNCTION("Bl2qFOnHOtk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_065DAA14E9C73AD9);
    LIB_FUNCTION("Bq-05dBCvD4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_06AFF4E5D042BC3E);
    LIB_FUNCTION("Bu42kpn3OZc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_06EE369299F73997);
    LIB_FUNCTION("B8ktn412thc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_07C92D9F8D76B617);
    LIB_FUNCTION("B+kRdJjx5L8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_07E9117498F1E4BF);
    LIB_FUNCTION("CPPgrzZk8nU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_08F3E0AF3664F275);
    LIB_FUNCTION("Cpk3wB7yE3U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0A9937C01EF21375);
    LIB_FUNCTION("CsvmrMujh20", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0ACBE6ACCBA3876D);
    LIB_FUNCTION("CuB9M1RRDOY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0AE07D3354510CE6);
    LIB_FUNCTION("Cuw8NCrme3w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0AEC3C342AE67B7C);
    LIB_FUNCTION("CzGEIMEefCM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0B318420C11E7C23);
    LIB_FUNCTION("C7bDewPzXYk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0BB6C37B03F35D89);
    LIB_FUNCTION("C76Kms3ZD98", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0BBE8A9ACDD90FDF);
    LIB_FUNCTION("DHtikF4iTpw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0C7B62905E224E9C);
    LIB_FUNCTION("DTWRMRckGvk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0D35913117241AF9);
    LIB_FUNCTION("DV7pXO7Yeac", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0D5EE95CEED879A7);
    LIB_FUNCTION("DW+ySyerHaI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0D6FB24B27AB1DA2);
    LIB_FUNCTION("DegDLVNKxBw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0DE8032D534AC41C);
    LIB_FUNCTION("DfTMqdyp50I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0DF4CCA9DCA9E742);
    LIB_FUNCTION("DnRJsdPZjAE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0E7449B1D3D98C01);
    LIB_FUNCTION("DncJS3dQyzc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0E77094B7750CB37);
    LIB_FUNCTION("Dsqzl7bVBgM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0ECAB397B6D50603);
    LIB_FUNCTION("Dx3h0eraKUg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0F1DE1D1EADA2948);
    LIB_FUNCTION("D4r++h0mvxo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_0F8AFEFA1D26BF1A);
    LIB_FUNCTION("EYgXEFYqa60", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_11881710562A6BAD);
    LIB_FUNCTION("Ea-Yi70McNs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_11AFD88BBD0C70DB);
    LIB_FUNCTION("EecEowpLiHc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_11E704A30A4B8877);
    LIB_FUNCTION("ElAUhCRS+Us", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_125014842452F94B);
    LIB_FUNCTION("Em8AceEcrEY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_126F0071E11CAC46);
    LIB_FUNCTION("EpJtzzWZSwE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_12926DCF35994B01);
    LIB_FUNCTION("Esx6v78xYY8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_12CC7ABFBF31618F);
    LIB_FUNCTION("E8TlH0RZKqI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_13C4E51F44592AA2);
    LIB_FUNCTION("FTMOfFYzglQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_15330E7C56338254);
    LIB_FUNCTION("FWazWMq-JhI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1566B358CABF2612);
    LIB_FUNCTION("FiWBjyaPRe8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1625818F268F45EF);
    LIB_FUNCTION("FtMrQNKKmsI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_16D32B40D28A9AC2);
    LIB_FUNCTION("GD9Eg729Jc0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_183F4483BDBD25CD);
    LIB_FUNCTION("GIfp6Vr2Lz0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1887E9E95AF62F3D);
    LIB_FUNCTION("GKPOlf2JPTo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_18A3CE95FD893D3A);
    LIB_FUNCTION("GLNmXkhU5+k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_18B3665E4854E7E9);
    LIB_FUNCTION("GSOwA5SK9H4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1923B003948AF47E);
    LIB_FUNCTION("GbUz2kxZpTI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_19B533DA4C59A532);
    LIB_FUNCTION("G7OZdy22jgg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1BB399772DB68E08);
    LIB_FUNCTION("HArGEtOilxs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1C0AC612D3A2971B);
    LIB_FUNCTION("HFWZt3mZCkM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1C5599B779990A43);
    LIB_FUNCTION("HMuylrBDF74", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1CCBB296B04317BE);
    LIB_FUNCTION("HNBFVC+5MAI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1CD045542FB93002);
    LIB_FUNCTION("HezspnOrd7c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1DECECA673AB77B7);
    LIB_FUNCTION("HgPgJOJsGn8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1E03E024E26C1A7F);
    LIB_FUNCTION("HxAXMrsNfiE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1F101732BB0D7E21);
    LIB_FUNCTION("H00VPsPdR7s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1F4D153EC3DD47BB);
    LIB_FUNCTION("H3xH9j+vDL4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1F7C47F63FAF0CBE);
    LIB_FUNCTION("H74u5owPMbY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_1FBE2EE68C0F31B6);
    LIB_FUNCTION("IDjBYokUuck", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2038C1628914B9C9);
    LIB_FUNCTION("ID-LVv24anQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_203FCB56FDB86A74);
    LIB_FUNCTION("IFacEHxssIw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_20569C107C6CB08C);
    LIB_FUNCTION("IKstc07eVfA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_20AB2D734EDE55F0);
    LIB_FUNCTION("IrEoEYD7Cl4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_22B1281180FB0A5E);
    LIB_FUNCTION("IvGq2makSa4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_22F1AADA66A449AE);
    LIB_FUNCTION("I4shXv-fPTA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_238B215EFFDF3D30);
    LIB_FUNCTION("JOjsUdFJ+hU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_24E8EC51D149FA15);
    LIB_FUNCTION("JXKOeKOWLAI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_25728E78A3962C02);
    LIB_FUNCTION("JeZJocaJHAU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_25E649A1C6891C05);
    LIB_FUNCTION("JkuKOLV3cF0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_264B8A38B577705D);
    LIB_FUNCTION("Jm7QjcHIKg4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_266ED08DC1C82A0E);
    LIB_FUNCTION("J7tN5iq1i60", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_27BB4DE62AB58BAD);
    LIB_FUNCTION("KDqpahluouo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_283AA96A196EA2EA);
    LIB_FUNCTION("KFMVo5CoWpQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_285315A390A85A94);
    LIB_FUNCTION("KQSdux7zGU4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_29049DBB1EF3194E);
    LIB_FUNCTION("Kfe6nDcyy0c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_29F7BA9C3732CB47);
    LIB_FUNCTION("KnMt8zGsyzc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2A732DF331ACCB37);
    LIB_FUNCTION("KqAWYOx1tvs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2AA01660EC75B6FB);
    LIB_FUNCTION("KzfLzpQcFoE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2B37CBCE941C1681);
    LIB_FUNCTION("LKo7ZNBUTlU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2CAA3B64D0544E55);
    LIB_FUNCTION("LM15YX7BCnU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2CCD79617EC10A75);
    LIB_FUNCTION("LNi2lxasBmc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2CD8B69716AC0667);
    LIB_FUNCTION("LXT3wP+bXpw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2D74F7C0FF9B5E9C);
    LIB_FUNCTION("LcpagIBUTpU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2DCA5A8080544E95);
    LIB_FUNCTION("LmnydDznzlc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2E69F2743CE7CE57);
    LIB_FUNCTION("Lq8fO6-wUn0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_2EAF1F3BAFF0527D);
    LIB_FUNCTION("MUk+VbtOj2Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_31493E55BB4E8F66);
    LIB_FUNCTION("MX7crQD7X14", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_317EDCAD00FB5F5E);
    LIB_FUNCTION("MeAc+ooYzaI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_31E01CFA8A18CDA2);
    LIB_FUNCTION("Mq-XgqBhtSY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_32AFD782A061B526);
    LIB_FUNCTION("MrXN6wk7gYk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_32B5CDEB093B8189);
    LIB_FUNCTION("NBVRUlE8k64", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_34155152513C93AE);
    LIB_FUNCTION("NOTv-472yf4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_34E4EFFF8EF6C9FE);
    LIB_FUNCTION("NXL6DVxUVjs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3572FA0D5C54563B);
    LIB_FUNCTION("NnxHmyZODbk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_367C479B264E0DB9);
    LIB_FUNCTION("NohPvJZLKcw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_36884FBC964B29CC);
    LIB_FUNCTION("OGAIG7dVmUk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3860081BB7559949);
    LIB_FUNCTION("OTFPfmdKsTI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_39314F7E674AB132);
    LIB_FUNCTION("OgLngPzFVqU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3A02E780FCC556A5);
    LIB_FUNCTION("Ohe4hbpISbY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3A17B885BA4849B6);
    LIB_FUNCTION("OjjqyupeI6Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3A38EACAEA5E23A4);
    LIB_FUNCTION("OzSl4H8NvB8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3B34A5E07F0DBC1F);
    LIB_FUNCTION("O06P-AD8fqQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3B4E8FFC00FC7EA4);
    LIB_FUNCTION("O6sY-aI1EHo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3BAB18FDA235107A);
    LIB_FUNCTION("O9+ZlqCjPxE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3BDF9996A0A33F11);
    LIB_FUNCTION("PBlS8aRcw3o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3C1952F1A45CC37A);
    LIB_FUNCTION("PKN5Bs2wXzs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3CA37906CDB05F3B);
    LIB_FUNCTION("PNspCKzuOm8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3CDB2908ACEE3A6F);
    LIB_FUNCTION("PT7RZfK9zTM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3D3ED165F2BDCD33);
    LIB_FUNCTION("PaTX0Vdfzc4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3DA4D7D1575FCDCE);
    LIB_FUNCTION("Pd+2Es0Lx2k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3DDFB612CD0BC769);
    LIB_FUNCTION("PgQV4Wfercc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3E0415E167DEADC7);
    LIB_FUNCTION("Pn6fDxWBweY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3E7E9F0F1581C1E6);
    LIB_FUNCTION("PtOJ24KA7WU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3ED389DB8280ED65);
    LIB_FUNCTION("Pwx-bAw1SH0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3F0C7F6C0C35487D);
    LIB_FUNCTION("P9pyADie8NI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3FDA7200389EF0D2);
    LIB_FUNCTION("P-PCWLpRblg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_3FF3C258BA516E58);
    LIB_FUNCTION("QClFP2KKPF0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4029453F628A3C5D);
    LIB_FUNCTION("QFgm3bSuU44", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_405826DDB4AE538E);
    LIB_FUNCTION("QFqSZ1nyWGU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_405A926759F25865);
    LIB_FUNCTION("QGYI-e566Io", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_406608FDEE7AE88A);
    LIB_FUNCTION("QN2lVYwX3c8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_40DDA5558C17DDCF);
    LIB_FUNCTION("QZ0S5S-2BmQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_419D12E52FF60664);
    LIB_FUNCTION("QpblOUdL538", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4296E539474BE77F);
    LIB_FUNCTION("QvQfxWPMNlQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_42F41FC563CC3654);
    LIB_FUNCTION("Q8zIb0yTAmo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_43CCC86F4C93026A);
    LIB_FUNCTION("RAn2C9q8ZeE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4409F60BDABC65E1);
    LIB_FUNCTION("RWPHCuxnU4I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4563C70AEC675382);
    LIB_FUNCTION("ReZjcCGb0F4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_45E66370219BD05E);
    LIB_FUNCTION("RmpU8HJ4VpY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_466A54F072785696);
    LIB_FUNCTION("Rs0lNpdvIJo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_46CD2536976F209A);
    LIB_FUNCTION("SGNxe9L90Vc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4863717BD2FDD157);
    LIB_FUNCTION("SQLr0ZomMUk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4902EBD19A263149);
    LIB_FUNCTION("SQT3-o2D9Aw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4904F7FE8D83F40C);
    LIB_FUNCTION("Sl4T94Sr-Oc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4A5E13F784ABFCE7);
    LIB_FUNCTION("S2XusTXBJ4E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4B65EEB135C12781);
    LIB_FUNCTION("TBnUmXjaheI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4C19D49978DA85E2);
    LIB_FUNCTION("TeXWIP9m8TY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4DE5D620FF66F136);
    LIB_FUNCTION("ThcMErV6j54", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4E170C12B57A8F9E);
    LIB_FUNCTION("Ti8-pAXDJgw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4E2F3FA405C3260C);
    LIB_FUNCTION("Tqk1BXdRO00", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4EA9350577513B4D);
    LIB_FUNCTION("T3jrb8S18h8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_4F78EB6FC4B5F21F);
    LIB_FUNCTION("UDSL5DMRF7c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_50348BE4331117B7);
    LIB_FUNCTION("UIx+jN0oHKo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_508C7E8CDD281CAA);
    LIB_FUNCTION("UhwdLAKPWn4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_521C1D2C028F5A7E);
    LIB_FUNCTION("Ui-ySjXmcpE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_522FF24A35E67291);
    LIB_FUNCTION("VHD+kMJc3Uw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5470FE90C25CDD4C);
    LIB_FUNCTION("VX8mD5pKzRg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_557F260F9A4ACD18);
    LIB_FUNCTION("VYb5cgnzkes", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5586F97209F391EB);
    LIB_FUNCTION("VbLJt62pXDw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_55B2C9B7ADA95C3C);
    LIB_FUNCTION("VbSIo6VAuTY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_55B488A3A540B936);
    LIB_FUNCTION("VkLf6Cr0MUM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5642DFE82AF43143);
    LIB_FUNCTION("V04EbylK4Yc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_574E046F294AE187);
    LIB_FUNCTION("V4km6-iqbL8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_578926EBF8AA6CBF);
    LIB_FUNCTION("WF2l-GUIlrw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_585DA5FC650896BC);
    LIB_FUNCTION("WNbrJzSewnY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_58D6EB27349EC276);
    LIB_FUNCTION("WQa3MXlJhy0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5906B7317949872D);
    LIB_FUNCTION("WRC1YUM1vnA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5910B5614335BE70);
    LIB_FUNCTION("WT19qJEfCMk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_593D7DA8911F08C9);
    LIB_FUNCTION("WXV-5qk7DVM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_59757FE6A93B0D53);
    LIB_FUNCTION("WY5g+GKxFB4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_598E60F862B1141E);
    LIB_FUNCTION("WkU1FmZoDa8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5A45351666680DAF);
    LIB_FUNCTION("Wqvp6nAuan8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5AABE9EA702E6A7F);
    LIB_FUNCTION("WupK5HI1W4A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5AEA4AE472355B80);
    LIB_FUNCTION("WyDlPN5Zh0E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5B20E53CDE598741);
    LIB_FUNCTION("W0gLWfrpR+A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5B480B59FAE947E0);
    LIB_FUNCTION("W17sI2kKub0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5B5EEC23690AB9BD);
    LIB_FUNCTION("XArFsK8+2uA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5C0AC5B0AF3EDAE0);
    LIB_FUNCTION("XS6Zm+oHYtQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5D2E999BEA0762D4);
    LIB_FUNCTION("XVW7-UURDhY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5D55BBFD45110E16);
    LIB_FUNCTION("Xe4VQD0rtf0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_5DEE15403D2BB5FD);
    LIB_FUNCTION("YCDHCMp0sTA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6020C708CA74B130);
    LIB_FUNCTION("YG4UFVA8NNI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_606E1415503C34D2);
    LIB_FUNCTION("YSFA6O6aaT4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_612140E8EE9A693E);
    LIB_FUNCTION("YfE-VR2vYd8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_61F13F551DAF61DF);
    LIB_FUNCTION("YgbTkTF1Iyg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6206D39131752328);
    LIB_FUNCTION("Yh1FQ+8DRN4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_621D4543EF0344DE);
    LIB_FUNCTION("YlmpqOVtAnM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6259A9A8E56D0273);
    LIB_FUNCTION("Yl+ccBY0b04", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_625F9C7016346F4E);
    LIB_FUNCTION("Yu+N90bNjEo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_62EF8DF746CD8C4A);
    LIB_FUNCTION("Y20qmf0eays", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_636D2A99FD1E6B2B);
    LIB_FUNCTION("aAE+32b+dCU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_68013EDF66FE7425);
    LIB_FUNCTION("aXH3Bn3WOdE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6971F7067DD639D1);
    LIB_FUNCTION("aYlq2zq0ELI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_69896ADB3AB410B2);
    LIB_FUNCTION("ahOJqm5WE4c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6A1389AA6E561387);
    LIB_FUNCTION("alVg2J8Ssuc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6A5560D89F12B2E7);
    LIB_FUNCTION("ar+Zz4VKvPE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6ABF99CF854ABCF1);
    LIB_FUNCTION("a0-dxlANjcs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6B4FDDC6500D8DCB);
    LIB_FUNCTION("bKEdW0nRkoo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6CA11D5B49D1928A);
    LIB_FUNCTION("bWwPth5tBxU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6D6C0FB61E6D0715);
    LIB_FUNCTION("bXUHRf4TSPU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6D750745FE1348F5);
    LIB_FUNCTION("bhrz+dCZFL4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6E1AF3F9D09914BE);
    LIB_FUNCTION("blPtTAiypSE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6E53ED4C08B2A521);
    LIB_FUNCTION("bvQ6yh7WuWg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6EF43ACA1ED6B968);
    LIB_FUNCTION("b2+gnz4bamA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_6F6FA09F3E1B6A60);
    LIB_FUNCTION("cDXDQMcZWQE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7035C340C7195901);
    LIB_FUNCTION("cDjiHLXPZBs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7038E21CB5CF641B);
    LIB_FUNCTION("cGNF3NpbpE0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_706345DCDA5BA44D);
    LIB_FUNCTION("cSBxTr8Qvx8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7120714EBF10BF1F);
    LIB_FUNCTION("cT0oqRvIA90", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_713D28A91BC803DD);
    LIB_FUNCTION("cVO9dqU6oBI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7153BD76A53AA012);
    LIB_FUNCTION("cVxiXMcEG2s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_715C625CC7041B6B);
    LIB_FUNCTION("ceRnvbGHEdA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_71E467BDB18711D0);
    LIB_FUNCTION("cg0XllwfTj8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_720D17965C1F4E3F);
    LIB_FUNCTION("c0OAybz2W5o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_734380C9BCF65B9A);
    LIB_FUNCTION("c-TAjM1LvM8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_73F4C08CCD4BBCCF);
    LIB_FUNCTION("dEAxAbeynUY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_74403101B7B29D46);
    LIB_FUNCTION("dSWwgazWb-Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7525B081ACD66FF4);
    LIB_FUNCTION("db9Ed8E6Bco", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_75BF4477C13A05CA);
    LIB_FUNCTION("dgl5P1mHxvc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7609793F5987C6F7);
    LIB_FUNCTION("dhbtAbBHaao", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7616ED01B04769AA);
    LIB_FUNCTION("dk+HPZGhJNg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_764F873D91A124D8);
    LIB_FUNCTION("dwbx4SMFlWU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7706F1E123059565);
    LIB_FUNCTION("d-LQfrbYBuY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_77F2D07EB6D806E6);
    LIB_FUNCTION("ecNwTNzVnlc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_79C3704CDCD59E57);
    LIB_FUNCTION("edoLuiE1FUU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_79DA0BBA21351545);
    LIB_FUNCTION("efokR7Xz8MQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_79FA2447B5F3F0C4);
    LIB_FUNCTION("ek1vZf9hlaU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7A4D6F65FF6195A5);
    LIB_FUNCTION("ezGVzRFN7Oc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7B3195CD114DECE7);
    LIB_FUNCTION("ezI48jAa020", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7B3238F2301AD36D);
    LIB_FUNCTION("fHf8cHUKMmY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7C77FC70750A3266);
    LIB_FUNCTION("fSOp3EWdbRg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7D23A9DC459D6D18);
    LIB_FUNCTION("fVmIx0jQoF8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7D5988C748D0A05F);
    LIB_FUNCTION("fZWXFHqZ9PQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7D9597147A99F4F4);
    LIB_FUNCTION("filT9Afdg0Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7E2953F407DD8346);
    LIB_FUNCTION("fuNOUJlwmzI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_7EE34E5099709B32);
    LIB_FUNCTION("gEcOVRHVygA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_80470E5511D5CA00);
    LIB_FUNCTION("gHF5cBwI8Gk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_807179701C08F069);
    LIB_FUNCTION("gJboH-ryTkY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8096E81FFAF24E46);
    LIB_FUNCTION("gLdk9PG4cEI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_80B764F4F1B87042);
    LIB_FUNCTION("gL9pFDitAIs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_80BF691438AD008B);
    LIB_FUNCTION("gM9s-JYBJEI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_80CF6CFC96012442);
    LIB_FUNCTION("gOp3L4wFGf0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_80EA772F8C0519FD);
    LIB_FUNCTION("gdCv0AhNMno", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_81D0AFD0084D327A);
    LIB_FUNCTION("gh64pyF2-Wc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_821EB8A72176FD67);
    LIB_FUNCTION("gtL6tUEnJz8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_82D2FAB54127273F);
    LIB_FUNCTION("g2rmacQqWek", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_836AE669C42A59E9);
    LIB_FUNCTION("hVmiW-7DUYw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8559A25BFEC3518C);
    LIB_FUNCTION("hcH2bHZ6SdI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_85C1F66C767A49D2);
    LIB_FUNCTION("hontE4P4e6c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8689ED1383F87BA7);
    LIB_FUNCTION("h5bNnlNV06Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8796CD9E5355D3A6);
    LIB_FUNCTION("h9N+tt3BnZk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_87D37EB6DDC19D99);
    LIB_FUNCTION("iAqkj3D4T90", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_880AA48F70F84FDD);
    LIB_FUNCTION("iXsHViCTZls", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_897B07562093665B);
    LIB_FUNCTION("isr1XxY2gIc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8ACAF55F16368087);
    LIB_FUNCTION("iuilWJsw1OA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8AE8A5589B30D4E0);
    LIB_FUNCTION("iumXkJgxszE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8AE997909831B331);
    LIB_FUNCTION("iy1kC+DQ+5k", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8B2D640BE0D0FB99);
    LIB_FUNCTION("iz2atGaNrss", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8B3D9AB4668DAECB);
    LIB_FUNCTION("i176qqzgtGw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8B5EFAAAACE0B46C);
    LIB_FUNCTION("jCeUP0CpiNs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8C27943F40A988DB);
    LIB_FUNCTION("jFQJbHX18tA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8C54096C75F5F2D0);
    LIB_FUNCTION("jXZjoKUWiBQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8D7663A0A5168814);
    LIB_FUNCTION("jmGPUJmU+tc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8E618F509994FAD7);
    LIB_FUNCTION("jxnmzAZOK5g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8F19E6CC064E2B98);
    LIB_FUNCTION("j2qK6u6SL-U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_8F6A8AEAEE922FF5);
    LIB_FUNCTION("kBDhrY67+8o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9010E1AD8EBBFBCA);
    LIB_FUNCTION("kKlVoOcAGuk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_90A955A0E7001AE9);
    LIB_FUNCTION("kPnWBn-uzAU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_90F9D6067FEECC05);
    LIB_FUNCTION("k0jz0ZVGodo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9348F3D19546A1DA);
    LIB_FUNCTION("k9PAEdsZOIo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_93D3C011DB19388A);
    LIB_FUNCTION("lW56T9n4kQM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_956E7A4FD9F89103);
    LIB_FUNCTION("lfaZ4ELD5A8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_95F699E042C3E40F);
    LIB_FUNCTION("lod7OaoOhzU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_96877B39AA0E8735);
    LIB_FUNCTION("ls4HxJ7SNOo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_96CE07C49ED234EA);
    LIB_FUNCTION("l2uxeCNbVoE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_976BB178235B5681);
    LIB_FUNCTION("l4wLJeWIxNY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_978C0B25E588C4D6);
    LIB_FUNCTION("mLomEr7yONY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_98BA2612BEF238D6);
    LIB_FUNCTION("mVvdSTGvkTc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_995BDD4931AF9137);
    LIB_FUNCTION("mWbjmpJrclA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9966E39A926B7250);
    LIB_FUNCTION("mcIwbxiWNGQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_99C2306F18963464);
    LIB_FUNCTION("mcksYTt3a6c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_99C92C613B776BA7);
    LIB_FUNCTION("mk5Lk4zIrTk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9A4E4B938CC8AD39);
    LIB_FUNCTION("myP3tLf3IIE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9B23F7B4B7F72081);
    LIB_FUNCTION("nA6u6ucFqNs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9C0EAEEAE705A8DB);
    LIB_FUNCTION("nUesWVRd6eg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_9D47AC59545DE9E8);
    LIB_FUNCTION("oTBS2LGyrPo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A13052D8B1B2ACFA);
    LIB_FUNCTION("oapD46ePb2I", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A1AA43E3A78F6F62);
    LIB_FUNCTION("oeSM31Rknck", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A1E48CDF54649DC9);
    LIB_FUNCTION("oufe5bCvXRQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A2E7DEE5B0AF5D14);
    LIB_FUNCTION("ovXH-Z-xE-U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A2F5C7FD9FF113F5);
    LIB_FUNCTION("o2KW4iadRrw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A36296E2269D46BC);
    LIB_FUNCTION("o+4qe58NiK8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A3EE2A7B9F0D88AF);
    LIB_FUNCTION("pEcfn34L+oI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A4471F9F7E0BFA82);
    LIB_FUNCTION("pEm7pSHqNOE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A449BBA521EA34E1);
    LIB_FUNCTION("pI5mbDNOcmw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A48E666C334E726C);
    LIB_FUNCTION("pJt0SbTd5pw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A49B7449B4DDE69C);
    LIB_FUNCTION("pXSEURJcnqQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A5748451125C9EA4);
    LIB_FUNCTION("ppCijWSMwXY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A690A28D648CC176);
    LIB_FUNCTION("pqht4bHLsdk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A6A86DE1B1CBB1D9);
    LIB_FUNCTION("qPK7e4FXQKE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A8F2BB7B815740A1);
    LIB_FUNCTION("qT9kwGpvc5c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_A93F64C06A6F7397);
    LIB_FUNCTION("qzWSX8l9aqM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AB35925FC97D6AA3);
    LIB_FUNCTION("rAFKosmR+ik", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AC014AA2C991FA29);
    LIB_FUNCTION("rAbhCQFASus", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AC06E10901404AEB);
    LIB_FUNCTION("rHXGiBNSNQU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AC75C68813523505);
    LIB_FUNCTION("rUQbxJcILD4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AD441BC497082C3E);
    LIB_FUNCTION("rU8l8CHTVMM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AD4F25F021D354C3);
    LIB_FUNCTION("rfoEqFVBpP4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_ADFA04A85541A4FE);
    LIB_FUNCTION("rpYQprUheiM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AE9610A6B5217A23);
    LIB_FUNCTION("ryAZI4JvClg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AF201923826F0A58);
    LIB_FUNCTION("r8AhtDico-o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_AFC021B4389CA3FA);
    LIB_FUNCTION("sBXpmaM3PY8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B015E999A3373D8F);
    LIB_FUNCTION("sDhLhhB-xlI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B0384B86107FC652);
    LIB_FUNCTION("sMYwZTsxZWM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B0C630653B316563);
    LIB_FUNCTION("sQDczYjVxz0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B100DCCD88D5C73D);
    LIB_FUNCTION("sRo-6l5NnqQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B11A3FEA5E4D9EA4);
    LIB_FUNCTION("suf43BmcC5M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B2E7F8DC199C0B93);
    LIB_FUNCTION("s6thopb23cg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B3AB61A296F6DDC8);
    LIB_FUNCTION("s-MvauYZ7II", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B3F32F6AE619EC82);
    LIB_FUNCTION("tCJ6shO-jPU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B4227AB213BF8CF5);
    LIB_FUNCTION("tGUr9CtgQ2A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B4652BF42B604360);
    LIB_FUNCTION("tTbB8Tv+l8s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B536C1F13BFE97CB);
    LIB_FUNCTION("tkXMJkGEvIk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B645CC264184BC89);
    LIB_FUNCTION("tn4XsVgsb70", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B67E17B1582C6FBD);
    LIB_FUNCTION("ttBHxddpWk0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B6D047C5D7695A4D);
    LIB_FUNCTION("t17Y4epi78c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B75ED8E1EA62EFC7);
    LIB_FUNCTION("t6mpRNvX4QA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B7A9A944DBD7E100);
    LIB_FUNCTION("t8TnW+lPMfM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B7C4E75BE94F31F3);
    LIB_FUNCTION("uIix+SxGQSE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B888B1F92C464121);
    LIB_FUNCTION("uN7CJWSqBXs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B8DEC22564AA057B);
    LIB_FUNCTION("ubrdHLu65Pg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_B9BADD1CBBBAE4F8);
    LIB_FUNCTION("uqn3FpyF5Z8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BAA9F7169C85E59F);
    LIB_FUNCTION("uu5cOJCNYts", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BAEE5C38908D62DB);
    LIB_FUNCTION("vMhV6yUYP4Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BCC855EB25183F84);
    LIB_FUNCTION("vQH2NwKcc2Q", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BD01F637029C7364);
    LIB_FUNCTION("vdKfWscHflM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BDD29F5AC7077E53);
    LIB_FUNCTION("vtg90z7K1Q0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BED83DD33ECAD50D);
    LIB_FUNCTION("vufV0Jir9yg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_BEE7D5D098ABF728);
    LIB_FUNCTION("wNsVzPWa5iw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C0DB15CCF59AE62C);
    LIB_FUNCTION("wcIp-uD9YPo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C1C229FEE0FD60FA);
    LIB_FUNCTION("wii5rWgpjpg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C228B9AD68298E98);
    LIB_FUNCTION("wphSXO9vsoM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C298525CEF6FB283);
    LIB_FUNCTION("w1Dwk1H21rU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C350F09351F6D6B5);
    LIB_FUNCTION("w3QugPpYAxk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C3742E80FA580319);
    LIB_FUNCTION("w8mFPV1NRdQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C3C9853D5D4D45D4);
    LIB_FUNCTION("w-Xa1Pufw0A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C3F5DAD4FB9FC340);
    LIB_FUNCTION("xF+w5MzprtY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C45FB0E4CCE9AED6);
    LIB_FUNCTION("xJecuUi348c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C4979CB948B7E3C7);
    LIB_FUNCTION("xJsluhbPC4w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C49B25BA16CF0B8C);
    LIB_FUNCTION("xVE0XZYxIB4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C551345D9631201E);
    LIB_FUNCTION("xXopRCE2gpg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C57A294421368298);
    LIB_FUNCTION("xdyRytch1ig", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C5DC91CAD721D628);
    LIB_FUNCTION("xt7O5YkTU1c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C6DECEE589135357);
    LIB_FUNCTION("yB+LINZ6x40", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C81F8B20D67AC78D);
    LIB_FUNCTION("yCD6VvrIe+o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C820FA56FAC87BEA);
    LIB_FUNCTION("yHjqkRTF5JA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C878EA9114C5E490);
    LIB_FUNCTION("yKgT6-9HdQk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C8A813EBFF477509);
    LIB_FUNCTION("yWamY9WjVII", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C966A663D5A35482);
    LIB_FUNCTION("yXxMZ-02dNM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C97C4C67FD3674D3);
    LIB_FUNCTION("yZBVDxWEiwc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_C990550F15848B07);
    LIB_FUNCTION("yllzeo7Bu74", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CA59737A8EC1BBBE);
    LIB_FUNCTION("ysX96PgNe2U", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CAC5FDE8F80D7B65);
    LIB_FUNCTION("yxNbMNBjm4M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CB135B30D0639B83);
    LIB_FUNCTION("y4oaqmH2TDo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CB8A1AAA61F64C3A);
    LIB_FUNCTION("y55nRnJYB1c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CB9E674672580757);
    LIB_FUNCTION("zCudJerqqx0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CC2B9D25EAEAAB1D);
    LIB_FUNCTION("zRslK77fW1M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CD1B252BBEDF5B53);
    LIB_FUNCTION("zwA76Qy+Gic", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CF003BE90CBE1A27);
    LIB_FUNCTION("zwCONIhKweI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_CF008E34884AC1E2);
    LIB_FUNCTION("0Lj0s6NoerI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D0B8F4B3A3687AB2);
    LIB_FUNCTION("0O4ZuOkfYPU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D0EE19B8E91F60F5);
    LIB_FUNCTION("0SuSlL0OD1Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D12B9294BD0E0F56);
    LIB_FUNCTION("0cyGJtj6Mos", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D1CC8626D8FA328B);
    LIB_FUNCTION("0vorueuLY6w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D2FA2BB9EB8B63AC);
    LIB_FUNCTION("0yGXiAz5POs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D32197880CF93CEB);
    LIB_FUNCTION("0yb1wmzIG44", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D326F5C26CC81B8E);
    LIB_FUNCTION("1PoGuVoyG3o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D4FA06B95A321B7A);
    LIB_FUNCTION("1So3qQHgSyE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D52A37A901E04B21);
    LIB_FUNCTION("1VBN-DmatAA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D5504DFC399AB400);
    LIB_FUNCTION("1WEFyyf49dw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D56105CB27F8F5DC);
    LIB_FUNCTION("1WirGSNeyxk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D568AB19235ECB19);
    LIB_FUNCTION("1t979mOf5hE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D6DF7BF6639FE611);
    LIB_FUNCTION("2GCKkDEZ10Y", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D8608A903119D746);
    LIB_FUNCTION("2ej8cH1ZkU0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D9E8FC707D59914D);
    LIB_FUNCTION("2fB55i3uWyk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_D9F079E62DEE5B29);
    LIB_FUNCTION("2hfOTyl0hTY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DA17CE4F29748536);
    LIB_FUNCTION("2kC579f2EYU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DA40B9EFD7F61185);
    LIB_FUNCTION("2msnT+vCZmo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DA6B274FEBC2666A);
    LIB_FUNCTION("2tAVNch6Ufw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DAD01535C87A51FC);
    LIB_FUNCTION("20UR1EhRDsQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DB4511D448510EC4);
    LIB_FUNCTION("247x--xmJpw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DB8EF1FFFC66269C);
    LIB_FUNCTION("27UI+hudqPc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DBB508FA1B9DA8F7);
    LIB_FUNCTION("3FnJuHC3KaI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DC59C9B870B729A2);
    LIB_FUNCTION("3Gae1sv2dRw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DC669ED6CBF6751C);
    LIB_FUNCTION("3LiihJpByZE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DCB8A2849A41C991);
    LIB_FUNCTION("3Y+ZFtfwOvc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DD8F9916D7F03AF7);
    LIB_FUNCTION("3cM-L05IDCo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DDC33F2F4E480C2A);
    LIB_FUNCTION("3gtCC96LItc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_DE0B420BDE8B22D7);
    LIB_FUNCTION("4MC8KYmP43A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E0C0BC29898FE370);
    LIB_FUNCTION("4M2JPkb7Vbo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E0CD893E46FB55BA);
    LIB_FUNCTION("4lUwFkt-ZZ8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E25530164B7F659F);
    LIB_FUNCTION("42gvQ-33bFg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E3682F43FDF76C58);
    LIB_FUNCTION("44F34ceKgPo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E38177E1C78A80FA);
    LIB_FUNCTION("48p0z-ll3wo", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E3CA74CFF965DF0A);
    LIB_FUNCTION("5FuxkbSbLtk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E45BB191B49B2ED9);
    LIB_FUNCTION("5GW51rYObX0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E465B9D6B60E6D7D);
    LIB_FUNCTION("5NgodsKWw4o", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E4D82876C296C38A);
    LIB_FUNCTION("5N21NQ+ltTg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E4DDB5350FA5B538);
    LIB_FUNCTION("5Uv-b7crx74", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E54BFF6FB72BC7BE);
    LIB_FUNCTION("5ZKpMgMCC7s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E592A93203020BBB);
    LIB_FUNCTION("5aRK9tfUiv0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E5A44AF6D7D48AFD);
    LIB_FUNCTION("5jmpfPn-FDA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E639A97CF9FF1430);
    LIB_FUNCTION("5qwBeeSKiSc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E6AC0179E48A8927);
    LIB_FUNCTION("51FZZoJ3XYM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E751596682775D83);
    LIB_FUNCTION("54ix5S74JwI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E788B1E52EF82702);
    LIB_FUNCTION("6U8XYT9cnTE", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E94F17613F5C9D31);
    LIB_FUNCTION("6VkBExKNVeA", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E9590113128D55E0);
    LIB_FUNCTION("6eCw3RJWCxY", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_E9E0B0DD12560B16);
    LIB_FUNCTION("6vXI7OZMewU", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EAF5C8ECE64C7B05);
    LIB_FUNCTION("65i-XELUp+s", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EB98BF5C42D4A7EB);
    LIB_FUNCTION("66vEqsQ6Row", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EBABC4AAC43A468C);
    LIB_FUNCTION("6-AAhfCCzIs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EBF00085F082CC8B);
    LIB_FUNCTION("7LZZ7gWNBq8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_ECB659EE058D06AF);
    LIB_FUNCTION("7PCWq3UUh64", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_ECF096AB751487AE);
    LIB_FUNCTION("7lonFwHbM8A", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EE5A271701DB33C0);
    LIB_FUNCTION("72TLahYlJI4", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EF64CB6A1625248E);
    LIB_FUNCTION("72yKNXx+2GM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_EF6C8A357C7ED863);
    LIB_FUNCTION("8A-pT35pmZQ", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F00FE94F7E699994);
    LIB_FUNCTION("8aUdujAykDg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F1A51DBA30329038);
    LIB_FUNCTION("8hbnZqkP3BI", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F216E766A90FDC12);
    LIB_FUNCTION("8qEFhKvl2Cw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F2A10584ABE5D82C);
    LIB_FUNCTION("8tmdOV5UIaM", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F2D99D395E5421A3);
    LIB_FUNCTION("84AB5Si6E3E", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F38001E528BA1371);
    LIB_FUNCTION("857JyPp2h7M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F39EC9C8FA7687B3);
    LIB_FUNCTION("86--3NYyd1w", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F3AFFFDCD632775C);
    LIB_FUNCTION("87jf8zdIv9M", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F3B8DFF33748BFD3);
    LIB_FUNCTION("9eR-lVD3oUc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F5E47F9550F7A147);
    LIB_FUNCTION("9uk3FNGpOc8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F6E93714D1A939CF);
    LIB_FUNCTION("9v0ZrUjk7wk", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F6FD19AD48E4EF09);
    LIB_FUNCTION("90Tr-GIPfL8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F744EBFC620F7CBF);
    LIB_FUNCTION("925FJay6zH8", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F76E4525ACBACC7F);
    LIB_FUNCTION("95V6SIgvQss", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F7957A48882F42CB);
    LIB_FUNCTION("96gLB4CbqDg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F7A80B07809BA838);
    LIB_FUNCTION("+FccbMW2tZ0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F8571C6CC5B6B59D);
    LIB_FUNCTION("+Xh8+oc4Nvs", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_F9787CFA873836FB);
    LIB_FUNCTION("+nifbTTTg-g", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FA789F6D34D383F8);
    LIB_FUNCTION("+rpXQIOsHmw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FABA574083AC1E6C);
    LIB_FUNCTION("-AT9u642j7c", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FC04FDBBAE368FB7);
    LIB_FUNCTION("-S2vvy5A7uc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FD2DAFBF2E40EEE7);
    LIB_FUNCTION("-VXubTX5UK0", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FD55EE6D35F950AD);
    LIB_FUNCTION("-lXuMgmNDVg", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FE55EE32098D0D58);
    LIB_FUNCTION("-nmEECLh2hw", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FE79841022E1DA1C);
    LIB_FUNCTION("--Sj4nn7RKc", "libSceNpCommon", 1, "libSceNpCommon", 1, 1, Func_FFF4A3E279FB44A7);
};

} // namespace Libraries::NpCommon