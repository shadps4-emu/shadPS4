// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <string_view>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_web_api2/np_web_api2_push_event.h"

namespace Libraries::Np::NpWebApi2 {

s64 g_current_push_event_push_context_id{};

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
        OrbisNpWebApi2PushEventFilterParameter new_param = filter_param[i];
        if (new_param.extd_data_key != nullptr && new_param.extd_data_key_num != 0) {
            auto& data_keys = this->data_key_copy_storage.emplace_back(
                new_param.extd_data_key, new_param.extd_data_key + new_param.extd_data_key_num);
            new_param.extd_data_key = data_keys.data();
        } else {
            new_param.extd_data_key = nullptr;
            new_param.extd_data_key_num = 0;
        }
        this->filter_params.emplace_back(new_param);
    }

    return ORBIS_OK;
}

const OrbisNpWebApi2PushEventFilterParameter* PushEventFilter::GetMatchingParameter(
    const std::string& service_name, const std::string& data_type) {
    if (service_name != this->np_service_name) {
        return nullptr;
    }
    for (const auto& param : this->filter_params) {
        if (data_type ==
            std::string_view(param.data_type.val,
                             strnlen(param.data_type.val, sizeof(param.data_type.val)))) {
            return &param;
        }
    }
    return nullptr;
}

void PushEventPushContext::Initialize() {
    // In the real library, there's a whole process of registering this with NpManager's Push2 API.
    // For now, we'll just create the uuid for this push context.

    // As for the ID itself, for now we'll cheat a bit.
    // Real library does a mostly random string, but it's easier to just use an actual index.
    ++g_current_push_event_push_context_id;
    if (g_current_push_event_push_context_id < 0) {
        g_current_push_event_push_context_id = 1;
    }
    s64 new_id = g_current_push_event_push_context_id;
    std::memset(&this->id, 0, sizeof(this->id));
    std::memcpy(&this->id, &new_id, sizeof(s64));
}

}; // namespace Libraries::Np::NpWebApi2