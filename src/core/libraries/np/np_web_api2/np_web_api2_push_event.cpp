// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <mutex>
#include <random>

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
        Entry entry{};
        const OrbisNpWebApi2PushEventDataType& data_type = filter_param[i].data_type;
        entry.data_type.assign(data_type.val, strnlen(data_type.val, sizeof(data_type.val)));
        if (filter_param[i].extd_data_key != nullptr) {
            for (u64 j = 0; j < filter_param[i].extd_data_key_num; j++) {
                const OrbisNpWebApi2PushEventExtdDataKey& key = filter_param[i].extd_data_key[j];
                entry.extd_data_keys.emplace_back(key.val, strnlen(key.val, sizeof(key.val)));
            }
        }
        this->entries.emplace_back(std::move(entry));
    }

    return ORBIS_OK;
}

void PushEventPushContext::Initialize() {
    // Titles send this string to the server (e.g. in a player session's pushContexts), so it must
    // be a printable UUID. The first 8 characters are a counter, which is what the maps key on.
    static std::mutex id_mutex;
    static std::mt19937_64 rng{std::random_device{}()};
    u64 counter{};
    u64 random_a{};
    u64 random_b{};
    {
        std::scoped_lock lk{id_mutex};
        ++g_current_push_event_push_context_id;
        if (g_current_push_event_push_context_id < 0 ||
            g_current_push_event_push_context_id > 0xffffffffll) {
            g_current_push_event_push_context_id = 1;
        }
        counter = static_cast<u64>(g_current_push_event_push_context_id);
        random_a = rng();
        random_b = rng();
    }

    char uuid[37]{};
    std::snprintf(uuid, sizeof(uuid), "%08x-%04x-4%03x-%x%03x-%012llx", static_cast<u32>(counter),
                  static_cast<u32>(random_a & 0xffff), static_cast<u32>((random_a >> 16) & 0xfff),
                  static_cast<u32>(8 + ((random_a >> 28) & 3)),
                  static_cast<u32>((random_a >> 32) & 0xfff),
                  static_cast<unsigned long long>(random_b & 0xffffffffffffull));
    std::memset(&this->id, 0, sizeof(this->id));
    std::memcpy(this->id.uuid, uuid, 36);
}

}; // namespace Libraries::Np::NpWebApi2