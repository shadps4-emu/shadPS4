// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_common_error.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpCommon {

s32 PS4_SYSV_ABI sceNpCmpNpId(OrbisNpId* np_id1, OrbisNpId* np_id2) {
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

s32 PS4_SYSV_ABI sceNpCmpNpIdInOrder(OrbisNpId* np_id1, OrbisNpId* np_id2, u32* out_result) {
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

s32 PS4_SYSV_ABI sceNpCmpOnlineId(OrbisNpOnlineId* online_id1, OrbisNpOnlineId* online_id2) {
    if (online_id1 == nullptr || online_id2 == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (std::strncmp(online_id1->data, online_id2->data, ORBIS_NP_ONLINEID_MAX_LENGTH) != 0) {
        return ORBIS_NP_UTIL_ERROR_NOT_MATCH;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommonCompat", 1, "libSceNpCommon", sceNpCmpOnlineId);
    LIB_FUNCTION("i8UmXTSq7N4", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpNpId);
    LIB_FUNCTION("TcwEFnakiSc", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpNpIdInOrder);
    LIB_FUNCTION("dj+O5aD2a0Q", "libSceNpCommon", 1, "libSceNpCommon", sceNpCmpOnlineId);
};

} // namespace Libraries::Np::NpCommon