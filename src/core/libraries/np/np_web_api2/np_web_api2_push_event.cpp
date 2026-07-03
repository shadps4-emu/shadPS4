// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_web_api2/np_web_api2_push_event.h"

namespace Libraries::Np::NpWebApi2 {
s32 PushEventFilter::Initialize(PushEventHandle* handle,
                                const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                                u64 filter_param_num) {
    if (handle->IsDeleted()) {
        LOG_ERROR(Lib_NpWebApi2, "handle {:#x} is invalid", handle->GetId());
        return ORBIS_NP_WEBAPI2_ERROR_HANDLE_NOT_FOUND;
    }
    if (handle->IsTimedout()) {
        LOG_ERROR(Lib_NpWebApi2, "handle {:#x} has timed out", handle->GetId());
        return ORBIS_NP_WEBAPI2_ERROR_TIMEOUT;
    }
    if (handle->IsAborted()) {
        LOG_ERROR(Lib_NpWebApi2, "handle {:#x} was aborted", handle->GetId());
        return ORBIS_NP_WEBAPI2_ERROR_ABORTED;
    }

    for (u64 i = 0; i < filter_param_num; i++) {
        OrbisNpWebApi2PushEventFilterParameter new_param = OrbisNpWebApi2PushEventFilterParameter();
        memcpy(&new_param, &filter_param[i], sizeof(OrbisNpWebApi2PushEventFilterParameter));
        this->filter_params.push_back(new_param);
    }

    return ORBIS_OK;
}
}; // namespace Libraries::Np::NpWebApi2