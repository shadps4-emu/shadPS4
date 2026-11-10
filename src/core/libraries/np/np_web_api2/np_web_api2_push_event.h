// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <string>
#include <vector>

#include "common/logging/log.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"

namespace Libraries::Np::NpWebApi2 {

class UserContext;

class PushEventHandle {
public:
    PushEventHandle(s32 new_id) : id(new_id) {}

    s32 GetId() {
        return id;
    }

    void SetTimeout(u32 new_timeout) {
        timeout = new_timeout;
    }

    void AddUser() {
        user_count++;
    }

    void RemoveUser() {
        user_count--;
    }

    void Abort() {
        aborted = true;
    }

    bool IsAborted() {
        return aborted;
    }

    void MarkForDeletion() {
        deleting = true;
    }

    bool IsDeleted() {
        return deleting;
    }

    void SetEndTime() {
        if (timeout != 0) {
            end_time = Libraries::Kernel::sceKernelGetProcessTime() + timeout;
        }
    }

    void ClearEndTime() {
        end_time = 0;
    }

    u64 GetEndTime() {
        return end_time;
    }

    bool IsTimedout() {
        return timed_out;
    }

    void CheckTimeout(u64 time) {
        if (!timed_out && end_time != 0 && end_time < time) {
            timed_out = true;
            Abort();
        }
    }

    bool IsBusy() {
        return user_count > 1;
    }

private:
    s32 id{};
    u32 timeout{};
    s32 user_count{};
    bool aborted{};
    bool deleting{};
    bool timed_out{};
    u64 end_time{};
};

class PushEventFilter {
public:
    struct Entry {
        std::string data_type;
        std::vector<std::string> extd_data_keys;
    };

    PushEventFilter(s32 new_id, const char* service_name, OrbisNpServiceLabel service_label,
                    bool is_internal)
        : id(new_id), np_service_name(service_name ? service_name : ""),
          np_service_label(service_label), internal(is_internal) {}

    s32 Initialize(PushEventHandle* handle,
                   const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                   u64 filter_param_num);

    s32 GetId() {
        return id;
    }

    const std::string& GetServiceName() const {
        return np_service_name;
    }

    OrbisNpServiceLabel GetServiceLabel() const {
        return np_service_label;
    }

    const std::vector<Entry>& GetEntries() const {
        return entries;
    }

    const Entry* FindEntry(const std::string& data_type) const {
        for (const Entry& entry : entries) {
            if (entry.data_type == data_type) {
                return &entry;
            }
        }
        return nullptr;
    }

    bool MatchesService(const std::string& event_service_name) const {
        return np_service_name.empty() || event_service_name.empty() ||
               np_service_name == event_service_name;
    }

private:
    s32 id{};
    std::string np_service_name{};
    OrbisNpServiceLabel np_service_label{};
    bool internal{};
    std::vector<Entry> entries{};
};

struct PushEventCallback {
    s32 id;
    s32 filter_id;
    OrbisNpWebApi2PushEventCallback cb_func;
    void* user_arg;
    bool is_busy;
};

class PushEventPushContext {
public:
    PushEventPushContext(u32 new_timeout, UserContext* user_ctx)
        : timeout(new_timeout), parent_user_ctx(user_ctx) {}

    void Initialize();

    OrbisNpWebApi2PushEventPushContextId* GetId() {
        return &id;
    }

    // Returns the actual numerical id currently stored in id.
    s64 GetFakeId() {
        s64 fake_id{};
        std::memcpy(&fake_id, &id, sizeof(s64));
        return fake_id;
    }

    void Start() {
        started = true;
    }

    void SetState(s32 new_state) {
        state = new_state;
    }

    bool CallbackRunning() {
        return state == 2;
    }

    bool IsStarted() const {
        return started;
    }

    std::string GetUuid() const {
        return std::string(id.uuid, strnlen(id.uuid, sizeof(id.uuid)));
    }

private:
    s32 state{};
    u32 timeout{};
    UserContext* parent_user_ctx{};
    u64 end_time{};
    u64 start_time{};
    OrbisNpWebApi2PushEventPushContextId id{};
    bool started{};
};

struct PushEventPushContextCallback {
    s32 id;
    s32 filter_id;
    OrbisNpWebApi2PushEventPushContextCallback cb_func;
    void* user_arg;
    bool is_busy;
};

}; // namespace Libraries::Np::NpWebApi2