// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"

namespace Libraries::Np::NpWebApi2 {

// Forward declarations to prevent compile issues
class PushEventHandle;
class PushEventFilter;
class PushEventCallback;
class PushEventPushContext;
class PushEventPushContextCallback;

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
        timed_out = end_time != 0 && time > end_time;
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

class PushEventFilter {};

class PushEventCallback {};

class PushEventPushContext {};

class PushEventPushContextCallback {};

}; // namespace Libraries::Np::NpWebApi2